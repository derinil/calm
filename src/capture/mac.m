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

struct MacCFrame {
  struct CFrame frame;
  NSMutableData **datas;
  size_t datas_len;
  uint8_t *block_buf_data;
  CMBlockBufferRef block_buffer;
};

void release_cframe(struct CFrame **frame_ptr) {
  struct CFrame *frame = *frame_ptr;
  atomic_fetch_sub(&frame->refcount, 1);
  if (frame->refcount > 0)
    return;
  NSMutableData *data;
  struct MacCFrame *this = (struct MacCFrame *)frame;
  for (size_t x = 0; x < this->datas_len; x++) {
    data = this->datas[x];
    [data release];
  }
  CFRelease(this->block_buffer);
  free(frame->parameter_sets);
  free(frame->parameter_sets_lengths);
  free(this);
  *frame_ptr = NULL;
}

struct MacCFrame *init_mac_cframe() {
  struct MacCFrame *frame = calloc(1, sizeof(*frame));
  if (!frame)
    return NULL;
  frame->frame.refcount = ATOMIC_VAR_INIT(0);
  return frame;
}

void compressed_frame_callback(void *output_callback_ref_con,
                               void *source_frame_ref_con, OSStatus status,
                               VTEncodeInfoFlags info_flags,
                               CMSampleBufferRef sample_buffer) {
  NSMutableData *ps_data;
  CMBlockBufferRef block_buffer;
  struct MacCFrame *frame = init_mac_cframe();
  if (!frame)
    return;

  if (status == kVTEncodeInfo_FrameDropped || !sample_buffer)
    return;

  CompressedFrameHandler compressed_frame_handler =
      (CompressedFrameHandler)output_callback_ref_con;

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

// TODO: Allocate better
// 10kb seems to be around average
#define AVG_PS_LEN 1024
#define AVG_DAT_LEN 10 * 1024

  // Write the SPS and PPS NAL units to the elementary stream before every
  // I-Frame
  if (is_key) {
    CMFormatDescriptionRef description =
        CMSampleBufferGetFormatDescription(sample_buffer);

    // Find out how many parameter sets there are
    size_t ps_len;
    CMVideoFormatDescriptionGetH264ParameterSetAtIndex(description, 0, NULL,
                                                       NULL, &ps_len, NULL);

    frame->frame.parameter_sets =
        malloc(ps_len * sizeof(*(frame->frame.parameter_sets)));
    frame->frame.parameter_sets_lengths =
        malloc(ps_len * sizeof(*(frame->frame.parameter_sets_lengths)));
    frame->datas = malloc(ps_len * sizeof(*(frame->datas)));
    frame->frame.parameter_sets_count = ps_len;

    // Write each parameter set to the elementary stream
    for (size_t i = 0; i < ps_len; i++) {
      ps_data = [NSMutableData dataWithCapacity:AVG_PS_LEN];
      const uint8_t *psp;
      size_t psp_len;
      int nalu_h_len;
      // TODO: use the points to frame->frame.parameter_sets[i] instead of psp
      CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
          description, i, &psp, &psp_len, NULL, &nalu_h_len);

      // TODO: we can use memcpy and malloc over parameter_sets instead of this
      // [ps_data appendBytes:startCode length:startCodeLength];
      [ps_data appendBytes:psp length:psp_len];

      frame->frame.nalu_h_len = nalu_h_len;
      frame->datas[i] = ps_data;
      frame->frame.parameter_sets[i] = ps_data.mutableBytes;
      frame->frame.parameter_sets_lengths[i] = ps_data.length;
    }

    frame->frame.is_keyframe = 1;
  }

  block_buffer = CMSampleBufferGetDataBuffer(sample_buffer);

  // TODO: multiple nalu units in blockbuffer
  // https://stackoverflow.com/questions/28396622/extracting-h264-from-cmblockbuffer

  // Get a pointer to the raw AVCC NAL unit data in the sample buffer
  uint8_t *buffer = NULL;
  size_t block_buffer_length;
  CMBlockBufferGetDataPointer(block_buffer, 0, NULL, &block_buffer_length,
                              (char **)&buffer);

  uint64_t buf_off = 0;
  static const uint64_t avcc_h_len = 4;

  while (buf_off < block_buffer_length - avcc_h_len) {
    // Read the NAL unit length
    uint32_t nalu_len = 0;
    memcpy(&nalu_len, buffer + buf_off, avcc_h_len);
    // Convert the length value from Big-endian to Little-endian
    nalu_len = CFSwapInt32BigToHost(nalu_len);

    frame->frame.nalus =
        realloc(frame->frame.nalus,
                (frame->frame.nalus_count + 1) * sizeof(*frame->frame.nalus));
    frame->frame.nalus_lengths = realloc(
        frame->frame.nalus_lengths,
        (frame->frame.nalus_count + 1) * sizeof(*frame->frame.nalus_lengths));
    frame->frame.nalus[frame->frame.nalus_count] =
        buffer + buf_off + avcc_h_len;
    frame->frame.nalus_lengths[frame->frame.nalus_count] = nalu_len;

    // Move to the next NAL unit in the block buffer
    frame->frame.nalus_count++;
    buf_off += avcc_h_len + nalu_len;
  }

  CFRetain(block_buffer);
  frame->block_buffer = block_buffer;

  compressed_frame_handler(&frame->frame);
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
