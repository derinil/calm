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

  char *base;
  size_t length;

  CMBlockBufferRef block_buffer = CMSampleBufferGetDataBuffer(sample_buffer);
  OSStatus get_data_status =
      CMBlockBufferGetDataPointer(block_buffer, 0, NULL, &length, &base);
  if (get_data_status)
    goto fail;

  printf("got base, len %lu, null %s\n", length, (!base ? "true" : "false"));

  compressed_frame_handler(base, length);

fail:
  CFRelease(sample_buffer);
}

void raw_frame_callback(VTCompressionSessionRef compression_session,
                        CGDisplayStreamFrameStatus status,
                        uint64_t display_time, IOSurfaceRef surface,
                        CGDisplayStreamUpdateRef update) {
  if (status == kCGDisplayStreamFrameStatusStopped) {
    stop = 1;
    return;
  }

  CFRetain(surface);
  IOSurfaceIncrementUseCount(surface);

  // TODO: Find a way to do this without allocating.
  CVImageBufferRef image_buffer = NULL;
  CVPixelBufferCreateWithIOSurface(NULL, surface, NULL, &image_buffer);

  VTCompressionSessionEncodeFrame(compression_session, image_buffer,
                                  CMTimeMake(display_time, 1), kCMTimeInvalid,
                                  NULL, NULL, NULL);

  CVPixelBufferRelease(image_buffer);
  IOSurfaceDecrementUseCount(surface);
  CFRelease(surface);

  printf("received frame %lld\n", display_time);
}

int setup(struct Capturer **target,
          CompressedFrameHandler compressed_frame_handler) {
  struct MacCapturer *this = malloc(sizeof(*this));
  if (!this)
    return 1;

  *target = &this->capturer;

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
  };

  CFTypeRef compression_values[] = {
      kCFBooleanTrue,
      kVTProfileLevel_H264_Main_AutoLevel,
      kVTH264EntropyMode_CABAC,
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
    return status;

  CFRetain(compression_session);
  this->compression_session = compression_session;

  dispatch_queue_attr_t concurr_dispatch = NULL;
  dispatch_queue_t queue =
      dispatch_queue_create("display_stream_dispatch", concurr_dispatch);

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
  };
  CFTypeRef values[] = {
    // Supposedly the default value for kCGDisplayStreamMinimumFrameTime
    // is 0 which should not throttle but this does not seem to be the default
    // behaviour.
    @(0),
  };

  CFDictionaryRef opts =
      CFDictionaryCreate(NULL, keys, values, 1, &kCFTypeDictionaryKeyCallBacks,
                         &kCFTypeDictionaryValueCallBacks);

  CGDisplayStreamRef stream = CGDisplayStreamCreateWithDispatchQueue(
      this->display_id, this->capturer.width, this->capturer.height,
      kCMPixelFormat_32BGRA, opts, queue, frame_callback);

  CFRelease(opts);

  CFRetain(stream);
  this->stream = stream;

  return 0;
}

int start_capture(struct Capturer *capturer) {
  struct MacCapturer *this = (struct MacCapturer *)capturer;
  CGError err = CGDisplayStreamStart(this->stream);
  return err;
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
