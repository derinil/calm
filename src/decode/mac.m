#include "../capture/capture.h"
#include "decode.h"
#define COREVIDEO_SILENCE_GL_DEPRECATION
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
#include <string.h>

/*
https://developer.apple.com/forums/thread/18986
https://developer.apple.com/forums/thread/725744
https://stackoverflow.com/questions/29050062/i-want-to-release-the-cvpixelbufferref-in-swift
*/

struct MacDecoder {
  struct Decoder decoder;
  VTDecompressionSessionRef decompression_session;
  CMVideoFormatDescriptionRef format_description;
};

struct MacDFrame {
  struct DFrame frame;
  CVImageBufferRef imageBuffer;
};

void release_dframe(struct DFrame *frame) {
  struct MacDFrame *mcf = (struct MacDFrame *)frame;
  // printf("freeing data\n");
  free(mcf->frame.data);
  // printf("freed data\n");
  free(mcf);
  // printf("freed frame\n");
}

void raw_decompressed_frame_callback(void *decompressionOutputRefCon,
                                     void *sourceFrameRefCon, OSStatus status,
                                     VTDecodeInfoFlags infoFlags,
                                     CVImageBufferRef imageBuffer,
                                     CMTime presentationTimeStamp,
                                     CMTime presentationDuration) {
  struct MacDecoder *this = (struct MacDecoder *)sourceFrameRefCon;
  if (!this)
    return;

  struct MacDFrame *frame = malloc(sizeof(*frame));
  if (!frame)
    return;
  memset(frame, 0, sizeof(*frame));

  if (!imageBuffer || infoFlags & kVTDecodeInfo_FrameDropped) {
    printf("frame dropped or failed, status: %d\n", status);
    return;
  }

  CVPixelBufferRetain(imageBuffer);
  CVPixelBufferLockBaseAddress(imageBuffer, 0);

  void *baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
  size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
  size_t width = CVPixelBufferGetWidth(imageBuffer);
  size_t height = CVPixelBufferGetHeight(imageBuffer);
  size_t dataSize = CVPixelBufferGetDataSize(imageBuffer);

  frame->frame.bytes_per_row = bytesPerRow;
  frame->frame.width = width;
  frame->frame.height = height;

  size_t len = width * height * (bytesPerRow / width);
  uint8_t *buf = malloc(len * sizeof(*buf));
  memcpy(buf, (uint8_t *)baseAddress, len);

  CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
  CVPixelBufferRelease(imageBuffer);

  frame->frame.data = buf;
  frame->frame.data_length = len;

  this->decoder.decompressed_frame_handler(&frame->frame);
}

CMSampleBufferRef
create_sample_buffer(CMFormatDescriptionRef format_description,
                     struct CFrame *frame) {
  OSStatus status;
  CMBlockBufferRef block_buf = NULL;
  CMSampleBufferRef sample_buf = NULL;

  status = CMBlockBufferCreateWithMemoryBlock(
      NULL, frame->frame, frame->frame_length, NULL, NULL, 0,
      frame->frame_length, 0, &block_buf);
  if (status)
    goto end;

  status = CMSampleBufferCreate(NULL, block_buf, TRUE, NULL, NULL,
                                format_description, 1, 0, NULL, 0, NULL,
                                &sample_buf);
  if (status)
    goto end;

  if (block_buf)
    CFRelease(block_buf);
  CFRetain(sample_buf);
  return sample_buf;
end:
  if (block_buf)
    CFRelease(block_buf);
  if (sample_buf)
    CFRelease(sample_buf);
  return NULL;
}

void decode_frame(struct Decoder *decoder, struct CFrame *frame) {
  int err;
  OSStatus status;
  CMSampleBufferRef sample_buffer;
  struct MacDecoder *this = (struct MacDecoder *)decoder;

  if (frame->is_keyframe) {
    err = start_decoder(decoder, frame);
    if (err) {
      printf("start_decoder failed with %d\n", err);
      return;
    }
  }

  sample_buffer = create_sample_buffer(this->format_description, frame);
  if (!sample_buffer || sample_buffer == NULL) {
    printf("create_sample_buffer failed\n");
    return;
  }

  VTDecodeFrameFlags decode_flags =
      kVTDecodeFrame_EnableAsynchronousDecompression;
#if 0
  status = VTDecompressionSessionDecodeFrame(
      this->decompression_session, sample_buffer, decode_flags, this, NULL);
  if (status)
    printf("decodeframe failed with %d\n", status);
#else
  VTDecompressionSessionDecodeFrame(this->decompression_session, sample_buffer,
                                    decode_flags, this, NULL);
#endif
  CMSampleBufferInvalidate(sample_buffer);
  CFRelease(sample_buffer);
}

struct Decoder *
setup_decoder(DecompressedFrameHandler decompressed_frame_handler) {
  struct MacDecoder *this = malloc(sizeof(*this));
  if (!this)
    return NULL;
  memset(this, 0, sizeof(*this));
  this->decoder.decompressed_frame_handler = decompressed_frame_handler;
  return &this->decoder;
}

int start_decoder(struct Decoder *decoder, struct CFrame *frame) {
  struct MacDecoder *this = (struct MacDecoder *)decoder;

  // Already initialized
  // TODO: maybe simply remove this check so we can create new session
  // with new pps, CFRelease format and session
  if (this->decompression_session) {
    return 0;
  }

  // TODO: NALUnitHeaderLength: 4 works for vt compressed frames

  CMFormatDescriptionRef format_description = NULL;
  OSStatus status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
      NULL, frame->parameter_sets_count,
      (const unsigned char *const *)frame->parameter_sets,
      frame->parameter_sets_lengths, 4, &format_description);
  if (status) {
#if 0
    printf("failed to create format %d\n", status);
    exit(1);
#endif
    return status;
  }

  CMVideoDimensions dimensions =
      CMVideoFormatDescriptionGetDimensions(format_description);

#if 0
  printf("got dimensions %d %d\n", dimensions.width, dimensions.height);
#endif

  // TODO: whats the best pixel format for opengl
  // https://developer.apple.com/documentation/corevideo/1563591-pixel_format_identifiers/kcvpixelformattype_32argb?language=objc
  // SInt32 destinationPixelType =
  // kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
  SInt32 destinationPixelType = kCVPixelFormatType_32BGRA;
  // SInt32 destinationPixelType = kCVPixelFormatType_24RGB;

  CFMutableDictionaryRef pixel_opts =
      CFDictionaryCreateMutable(NULL, 4, &kCFTypeDictionaryKeyCallBacks,
                                &kCFTypeDictionaryValueCallBacks);

  CFDictionarySetValue(
      pixel_opts, kCVPixelBufferPixelFormatTypeKey,
      CFNumberCreate(NULL, kCFNumberSInt32Type, &destinationPixelType));
  CFDictionarySetValue(
      pixel_opts, kCVPixelBufferWidthKey,
      CFNumberCreate(NULL, kCFNumberSInt32Type, &dimensions.width));
  CFDictionarySetValue(
      pixel_opts, kCVPixelBufferHeightKey,
      CFNumberCreate(NULL, kCFNumberSInt32Type, &dimensions.height));

  CFDictionarySetValue(pixel_opts, kCVPixelBufferExtendedPixelsLeftKey, @0);
  CFDictionarySetValue(pixel_opts, kCVPixelBufferExtendedPixelsRightKey, @0);
  CFDictionarySetValue(pixel_opts, kCVPixelBufferExtendedPixelsTopKey, @0);
  CFDictionarySetValue(pixel_opts, kCVPixelBufferExtendedPixelsBottomKey, @0);
  CFDictionarySetValue(pixel_opts, kCVPixelBufferPlaneAlignmentKey, @0);
  CFDictionarySetValue(pixel_opts, kCVPixelBufferOpenGLCompatibilityKey, @YES);
  CFDictionarySetValue(
      pixel_opts, kCVPixelBufferIOSurfaceOpenGLTextureCompatibilityKey, @YES);

  CFTypeRef decompression_keys[] = {
      kVTDecompressionPropertyKey_RealTime,
  };

  CFTypeRef decompression_values[] = {
      kCFBooleanTrue,
  };

  CFDictionaryRef decompression_opts = CFDictionaryCreate(
      NULL, decompression_keys, decompression_values, 1, NULL, NULL);

  const VTDecompressionOutputCallbackRecord callback = {
      raw_decompressed_frame_callback, NULL};

  VTDecompressionSessionRef decompression_session;
  status = VTDecompressionSessionCreate(kCFAllocatorDefault, format_description,
                                        decompression_opts, pixel_opts,
                                        &callback, &decompression_session);
  if (status)
    return status;

  CFRelease(decompression_opts);
  CFRelease(pixel_opts);

  CFRetain(decompression_session);
  CFRetain(format_description);
  this->format_description = format_description;
  this->decompression_session = decompression_session;

  return 0;
}

int stop_decoder(struct Decoder *decoder) {
  struct MacDecoder *this = (struct MacDecoder *)decoder;
  VTDecompressionSessionFinishDelayedFrames(this->decompression_session);
  VTDecompressionSessionWaitForAsynchronousFrames(this->decompression_session);
  VTDecompressionSessionInvalidate(this->decompression_session);
  CFRelease(this->decompression_session);
  CFRelease(this->format_description);
  return 0;
}
