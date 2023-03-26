#include "capture.h"
#include <AVFoundation/AVFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreServices/CoreServices.h>
#include <CoreVideo/CVBuffer.h>
#include <CoreVideo/CoreVideo.h>
#include <Foundation/Foundation.h>
#include <IOSurface/IOSurfaceAPI.h>
#include <VideoToolbox/VideoToolbox.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

/*
NOTE: turns out CGDisplayStream with CFRunLoop throttles
up to 50ms per frame (20fps) when there is no activity on the screen.
as soon as the mouse starts moving, or display starts updating
frame time drops to ~8ms.
*/

// TODO: Surely we can make this better
volatile int stop = 0;

struct MacCapturer {
  struct Capturer capturer;
  CGDirectDisplayID display_id;
  int is_retina;
  CGDisplayStreamRef stream;
  VTCompressionSessionRef compression_session;
};

void compressed_frame_callback(void *output_callback_ref_con,
                               void *source_frame_ref_con, OSStatus status,
                               VTEncodeInfoFlags info_flags,
                               CMSampleBufferRef sample_buffer) {
  if (status == kVTEncodeInfo_FrameDropped || !sample_buffer)
    return;

  CFRetain(sample_buffer);

  CompressedFrameHandler compressed_frame_handler =
      (CompressedFrameHandler)output_callback_ref_con;

  // TODO: Taken from
  // https://stackoverflow.com/questions/28396622/extracting-h264-from-cmblockbuffer

  // Find out if the sample buffer contains an I-Frame.
  // If so we will write the SPS and PPS NAL units to the elementary stream.
  int is_key = 0;
  CFArrayRef attachmentsArray =
      CMSampleBufferGetSampleAttachmentsArray(sample_buffer, 0);
  if (CFArrayGetCount(attachmentsArray)) {
    CFBooleanRef notSync;
    CFDictionaryRef dict = CFArrayGetValueAtIndex(attachmentsArray, 0);
    BOOL keyExists = CFDictionaryGetValueIfPresent(
        dict, kCMSampleAttachmentKey_NotSync, (const void **)&notSync);
    // An I-Frame is a sync frame
    is_key = !keyExists || !CFBooleanGetValue(notSync);
  }

  // This is the start code that we will write to
  // the elementary stream before every NAL unit
  static const size_t startCodeLength = 4;
  static const uint8_t startCode[] = {0x00, 0x00, 0x00, 0x01};

// TODO: Allocate better
// 10kb seems to be around average
#define AVG_DAT_LEN 10 * 1024
  NSMutableData *elementaryStream =
      [NSMutableData dataWithCapacity:AVG_DAT_LEN];

  // Write the SPS and PPS NAL units to the elementary stream before every
  // I-Frame
  if (is_key) {
    CMFormatDescriptionRef description =
        CMSampleBufferGetFormatDescription(sample_buffer);

    // Find out how many parameter sets there are
    size_t numberOfParameterSets;
    CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
        description, 0, NULL, NULL, &numberOfParameterSets, NULL);

    // Write each parameter set to the elementary stream
    for (size_t i = 0; i < numberOfParameterSets; i++) {
      const uint8_t *parameterSetPointer;
      size_t parameterSetLength;
      CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
          description, i, &parameterSetPointer, &parameterSetLength, NULL,
          NULL);

      // TODO: move these into a structure
      [elementaryStream appendBytes:startCode length:startCodeLength];
      [elementaryStream appendBytes:parameterSetPointer
                             length:parameterSetLength];
    }
  }

  // Get a pointer to the raw AVCC NAL unit data in the sample buffer
  size_t blockBufferLength;
  uint8_t *bufferDataPointer = NULL;
  CMBlockBufferGetDataPointer(CMSampleBufferGetDataBuffer(sample_buffer), 0,
                              NULL, &blockBufferLength,
                              (char **)&bufferDataPointer);

  // Loop through all the NAL units in the block buffer
  // and write them to the elementary stream with
  // start codes instead of AVCC length headers
  size_t bufferOffset = 0;
  static const int AVCCHeaderLength = 4;
  while (bufferOffset < blockBufferLength - AVCCHeaderLength) {
    // Read the NAL unit length
    uint32_t nalu_len;
    memcpy(&nalu_len, bufferDataPointer + bufferOffset, AVCCHeaderLength);
    // Convert the length value from Big-endian to Little-endian
    nalu_len = CFSwapInt32BigToHost(nalu_len);
    // Write start code to the elementary stream
    [elementaryStream appendBytes:startCode length:startCodeLength];
    // Write the NAL unit without the AVCC length header to the elementary
    // stream
    [elementaryStream
        appendBytes:bufferDataPointer + bufferOffset + AVCCHeaderLength
             length:nalu_len];
    // Move to the next NAL unit in the block buffer
    bufferOffset += AVCCHeaderLength + nalu_len;
  }

  uint8_t *bytes = elementaryStream.mutableBytes;
  size_t length = elementaryStream.length;

#define DUMP_DEBUG 0
#if DUMP_DEBUG
  FILE *f = fopen("dump.h264", "a+");

  for (size_t x = 0; x < elementaryStream.length; x++) {
    fprintf(f, "%c", bytes[x]);
  }

  fflush(f);
  fclose(f);
#endif

  struct CFrame *frame = malloc(sizeof(*frame));
  if (!frame)
    goto end;
  memset(frame, 0, sizeof(*frame));
  frame->data = bytes;
  frame->length = length;
  compressed_frame_handler(frame);

end:
  CFRelease(sample_buffer);
}

void raw_frame_callback(VTCompressionSessionRef compression_session,
                        CGDisplayStreamFrameStatus status,
                        uint64_t display_time, IOSurfaceRef surface,
                        CGDisplayStreamUpdateRef update) {
  static uint64_t prev_time = 0;

  // https://developer.apple.com/documentation/coregraphics/cgdisplaystreamframestatus
  // we only want to create a new frame when the display actually updates. or do
  // we?
  switch (status) {
  case kCGDisplayStreamFrameStatusStopped:
    stop = 1;
    return;
  case kCGDisplayStreamFrameStatusFrameComplete:
    break;
  default:
    return;
  }

  CMTime timestamp = CMClockMakeHostTimeFromSystemUnits(display_time);
  float curr = CMTimeGetSeconds(timestamp) * 1000;

  CFRetain(surface);
  IOSurfaceIncrementUseCount(surface);

  CVImageBufferRef image_buffer = NULL;
  CVPixelBufferCreateWithIOSurface(NULL, surface, NULL, &image_buffer);

  VTCompressionSessionEncodeFrame(compression_session, image_buffer, timestamp,
                                  kCMTimeInvalid, NULL, NULL, NULL);

  CVPixelBufferRelease(image_buffer);
  IOSurfaceDecrementUseCount(surface);
  CFRelease(surface);

  printf("received frame with difference %.0f\n", curr - prev_time);
  prev_time = curr;
}

struct Capturer *
setup_capturer(CompressedFrameHandler compressed_frame_handler) {
  struct MacCapturer *this = malloc(sizeof(*this));
  if (!this)
    return NULL;
  memset(this, 0, sizeof(*this));

  CGDirectDisplayID main = CGMainDisplayID();

  // These actually return the pixel dimensions.
  CGDisplayModeRef displayMode = CGDisplayCopyDisplayMode(main);
  size_t width = CGDisplayModeGetPixelWidth(displayMode);
  size_t height = CGDisplayModeGetPixelHeight(displayMode);
  CGDisplayModeRelease(displayMode);

  // This returns points.
  size_t wide = CGDisplayPixelsWide(main);

  this->is_retina = (width / wide == 2);
  this->display_id = main;
  this->capturer.should_compress = 1;
  this->capturer.height = height;
  this->capturer.width = width;
  this->capturer.compressed_frame_handler = compressed_frame_handler;

  // TODO: Better configuration of the compressor.
  CFTypeRef compression_keys[] = {
      kVTCompressionPropertyKey_RealTime,
      kVTCompressionPropertyKey_ProfileLevel,
      kVTCompressionPropertyKey_H264EntropyMode,
      kVTCompressionPropertyKey_ExpectedFrameRate,
      kVTCompressionPropertyKey_HDRMetadataInsertionMode,
      kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder,
      kVTCompressionPropertyKey_AlphaChannelMode,
      kVTCompressionPropertyKey_PrioritizeEncodingSpeedOverQuality,
      kVTCompressionPropertyKey_EnableLTR,
      kVTCompressionPropertyKey_PreserveDynamicHDRMetadata,
      kVTVideoEncoderSpecification_EnableLowLatencyRateControl,
  };

  CFTypeRef compression_values[] = {
    kCFBooleanTrue,
    kVTProfileLevel_H264_Baseline_AutoLevel,
    kVTH264EntropyMode_CABAC,
    @120,
    kVTHDRMetadataInsertionMode_Auto,
    kCFBooleanTrue,
    kVTAlphaChannelMode_StraightAlpha,
    kCFBooleanTrue,
    kCFBooleanTrue,
    kCFBooleanTrue,
    kCFBooleanTrue,
  };

  CFDictionaryRef compression_opts = CFDictionaryCreate(
      NULL, compression_keys, compression_values, 3,
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

  VTCompressionSessionRef compression_session;
  OSStatus status = VTCompressionSessionCreate(
      kCFAllocatorDefault, this->capturer.width, this->capturer.height,
      kCMVideoCodecType_H264, NULL, compression_opts, NULL,
      compressed_frame_callback, compressed_frame_handler,
      &compression_session);
  if (status)
    return NULL;

  CFRetain(compression_session);
  this->compression_session = compression_session;

  CFRelease(compression_opts);

  dispatch_queue_t queue = dispatch_queue_create("display_stream_dispatch",
                                                 DISPATCH_QUEUE_CONCURRENT);

  CGDisplayStreamFrameAvailableHandler frame_callback =
      ^(CGDisplayStreamFrameStatus status, uint64_t display_time,
        IOSurfaceRef surface, CGDisplayStreamUpdateRef update) {
        raw_frame_callback(compression_session, status, display_time, surface,
                           update);
      };

  // TODO: allow configuration of kCGDisplayStreamQueueDepth
  // and kCGDisplayStreamMinimumFrameTime
  CFTypeRef keys[] = {
      kCGDisplayStreamMinimumFrameTime,
      kCGDisplayStreamQueueDepth,
  };
  CFTypeRef values[] = {
    // Supposedly the default value for kCGDisplayStreamMinimumFrameTime
    // is 0 which should not throttle but this does not seem to be the default
    // behaviour.
    @0,
    @5,
  };

  CFDictionaryRef opts =
      CFDictionaryCreate(NULL, keys, values, 2, &kCFTypeDictionaryKeyCallBacks,
                         &kCFTypeDictionaryValueCallBacks);

  CGDisplayStreamRef stream = CGDisplayStreamCreateWithDispatchQueue(
      this->display_id, this->capturer.width, this->capturer.height, '420v',
      opts, queue, frame_callback);

  CFRelease(opts);

  CFRetain(stream);
  this->stream = stream;

  return &this->capturer;
}

int start_capture(struct Capturer *capturer) {
  struct MacCapturer *this = (struct MacCapturer *)capturer;
  OSStatus status =
      VTCompressionSessionPrepareToEncodeFrames(this->compression_session);
  if (status)
    return status;
  return CGDisplayStreamStart(this->stream);
}

int stop_capture(struct Capturer *capturer) {
  struct MacCapturer *this = (struct MacCapturer *)capturer;
  CGError err = CGDisplayStreamStop(this->stream);
  while (!stop)
    ;
  CFRelease(this->stream);
  CFRelease(this->compression_session);
  return err;
}
