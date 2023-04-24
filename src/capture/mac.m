#include "capture.h"
#include <AVFoundation/AVFoundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreServices/CoreServices.h>
#include <CoreVideo/CVBuffer.h>
#include <CoreVideo/CoreVideo.h>
#include <Foundation/Foundation.h>
#include <IOSurface/IOSurfaceAPI.h>
#include <VideoToolbox/VideoToolbox.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void release_cframe(struct CFrame **frame_ptr) {
  struct CFrame *frame = *frame_ptr;
  if (frame->nalus_count) {
    for (size_t x = 0; x < frame->nalus_count; x++) {
      free(frame->nalus[x]);
    }
    free(frame->nalus_lengths);
    free(frame->nalus);
  }
  if (frame->parameter_sets_count) {
    for (size_t x = 0; x < frame->parameter_sets_count; x++) {
      free(frame->parameter_sets[x]);
    }
    free(frame->parameter_sets_lengths);
    free(frame->parameter_sets);
  }
  free(frame);
  *frame_ptr = NULL;
}

void compressed_frame_callback(void *output_callback_ref_con,
                               void *source_frame_ref_con, OSStatus status,
                               VTEncodeInfoFlags info_flags,
                               CMSampleBufferRef sample_buffer) {
  struct CFrame *frame = NULL;
  CompressedFrameHandler compressed_frame_handler;

  if (status == kVTEncodeInfo_FrameDropped || !sample_buffer)
    return;

  frame = calloc(1, sizeof(*frame));
  if (!frame)
    return;

  compressed_frame_handler = (CompressedFrameHandler)output_callback_ref_con;

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

  // Write the SPS and PPS NAL units to the elementary stream before every
  // I-Frame
  if (is_key) {
    CMFormatDescriptionRef description =
        CMSampleBufferGetFormatDescription(sample_buffer);

    // Find out how many parameter sets there are
    size_t ps_len;
    CMVideoFormatDescriptionGetH264ParameterSetAtIndex(description, 0, NULL,
                                                       NULL, &ps_len, NULL);

    frame->parameter_sets = malloc(ps_len * sizeof(*(frame->parameter_sets)));
    frame->parameter_sets_lengths =
        malloc(ps_len * sizeof(*(frame->parameter_sets_lengths)));
    frame->parameter_sets_count = (uint64_t)ps_len;

    // Write each parameter set to the elementary stream
    for (size_t i = 0; i < ps_len; i++) {
      const uint8_t *psp;
      size_t psp_len;
      int nalu_h_len;
      CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
          description, i, &psp, &psp_len, NULL, &nalu_h_len);

      frame->nalu_h_len = nalu_h_len;
      frame->parameter_sets_lengths[i] = psp_len;
      frame->parameter_sets[i] = malloc(frame->parameter_sets_lengths[i] *
                                        sizeof(*frame->parameter_sets[i]));
      memcpy(frame->parameter_sets[i], psp, psp_len);
    }
  }

  // Get a pointer to the raw AVCC NAL unit data in the sample buffer
  uint8_t *buffer = NULL;
  size_t block_buffer_length;
  CMBlockBufferGetDataPointer(CMSampleBufferGetDataBuffer(sample_buffer), 0,
                              NULL, &block_buffer_length, (char **)&buffer);

  uint32_t buf_off = 0;
  // TODO: setting this to frame->nalu_h_len freezes the program
  static const uint32_t avcc_h_len = 4;

  while (buf_off < block_buffer_length - avcc_h_len) {
    // Read the NAL unit length
    uint32_t nalu_len = 0;
    memcpy(&nalu_len, buffer + buf_off, avcc_h_len);
    // Convert the length value from Big-endian to Little-endian
    nalu_len = CFSwapInt32BigToHost(nalu_len);

    frame->nalus =
        realloc(frame->nalus, (frame->nalus_count + 1) * sizeof(*frame->nalus));
    frame->nalus_lengths =
        realloc(frame->nalus_lengths,
                (frame->nalus_count + 1) * sizeof(*frame->nalus_lengths));
    frame->nalus[frame->nalus_count] =
        malloc(nalu_len * sizeof(*frame->nalus[frame->nalus_count]));
    memcpy(frame->nalus[frame->nalus_count], buffer + buf_off + avcc_h_len,
           nalu_len);
    frame->nalus_lengths[frame->nalus_count] = nalu_len;

    // Move to the next NAL unit in the block buffer
    frame->nalus_count++;
    buf_off += avcc_h_len + nalu_len;
  }

  compressed_frame_handler(frame);
}

void raw_frame_callback(VTCompressionSessionRef compression_session,
                        CGDisplayStreamFrameStatus status,
                        uint64_t display_time, IOSurfaceRef surface,
                        CGDisplayStreamUpdateRef update) {
#define TIME_DEBUG 0
#if TIME_DEBUG
  static uint64_t prev_time = 0;
#endif

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
#if TIME_DEBUG
  printf("received frame with difference %.0f\n", curr - prev_time);
  prev_time = curr;
#endif
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
      kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder,
      kVTCompressionPropertyKey_AlphaChannelMode,
      kVTCompressionPropertyKey_PrioritizeEncodingSpeedOverQuality,
      kVTCompressionPropertyKey_EnableLTR,
      kVTVideoEncoderSpecification_EnableLowLatencyRateControl,
  };

  // TODO: on the fly configuration of these
  // kVTH264EntropyMode_CAVLC and
  // kVTProfileLevel_H264_ConstrainedBaseline_AutoLevel

  CFTypeRef compression_values[] = {
    kCFBooleanTrue,
    kVTProfileLevel_H264_Baseline_AutoLevel,
    kVTH264EntropyMode_CABAC,
    @120,
    kCFBooleanTrue,
    kVTAlphaChannelMode_StraightAlpha,
    kCFBooleanTrue,
    kCFBooleanTrue,
    kCFBooleanTrue,
  };

  CFDictionaryRef compression_opts = CFDictionaryCreate(
      NULL, compression_keys, compression_values, 9,
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

  // TODO: use ScreenCaptureKit

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
      kCGDisplayStreamShowCursor,
  };
  CFTypeRef values[] = {
    // Supposedly the default value for kCGDisplayStreamMinimumFrameTime
    // is 0 which should not throttle but this does not seem to be the default
    // behaviour.
    @0,
    @5,
    @YES,
  };

  CFDictionaryRef opts =
      CFDictionaryCreate(NULL, keys, values, 3, &kCFTypeDictionaryKeyCallBacks,
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
