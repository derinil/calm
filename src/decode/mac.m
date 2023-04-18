#include "../capture/capture.h"
#include "decode.h"
#include <CoreFoundation/CoreFoundation.h>
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

struct MacDecodeContext {
  struct MacDecoder *decoder;
  uint8_t *condensed;
};

union USplitter {
  uint32_t ull;
  uint8_t bs[4];
};

void write_uint32(uint8_t *buf, uint32_t u) {
  union USplitter split;
  split.ull = u;
  memcpy(buf, split.bs, 4);
}

uint32_t read_uint32(uint8_t *buf) {
  union USplitter split;
  memcpy(split.bs, buf, 4);
  return split.ull;
}

uint8_t *condense_nalus(struct CFrame *frame, uint64_t *len) {
  uint8_t *buf = NULL;
  uint32_t swapped = 0;
  uint64_t total_len = 0;
  static const int avcc_h_len = 4;

  for (uint64_t i = 0; i < frame->nalus_count; i++)
    total_len += avcc_h_len + frame->nalus_lengths[i];

  buf = malloc(total_len * sizeof(*buf));
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < frame->nalus_count; i++) {
    swapped = frame->nalus_lengths[i] < 1000000
                  ? CFSwapInt32HostToBig(frame->nalus_lengths[i])
                  : frame->nalus_lengths[i];
    write_uint32(buf, swapped);
    memcpy(buf + 4, frame->nalus[i], frame->nalus_lengths[i]);
  }

  *len = total_len;
  return buf;
}

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
  struct MacDecodeContext *ctx = (struct MacDecodeContext *)sourceFrameRefCon;
  struct MacDecoder *decoder = (struct MacDecoder *)ctx->decoder;
  if (!decoder)
    return;

  free(ctx->condensed);
  free(ctx);

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

  decoder->decoder.decompressed_frame_handler(&frame->frame);
}

CMSampleBufferRef
create_sample_buffer(CMFormatDescriptionRef format_description,
                     struct CFrame *frame, uint8_t **condensed_ptr) {
  OSStatus status;
  CMBlockBufferRef block_buf = NULL;
  CMSampleBufferRef sample_buf = NULL;

  uint64_t condensed_len;
  uint8_t *condensed = condense_nalus(frame, &condensed_len);
  if (!condensed)
    return NULL;
  *condensed_ptr = condensed;

  status =
      CMBlockBufferCreateWithMemoryBlock(NULL, condensed, condensed_len, NULL,
                                         NULL, 0, condensed_len, 0, &block_buf);
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

void decode_frame(struct Decoder *subdec, struct CFrame *frame) {
  int err;
  OSStatus status;
  uint8_t *condensed_ptr;
  CMSampleBufferRef sample_buffer;
  struct MacDecoder *decoder = (struct MacDecoder *)subdec;
  struct MacDecodeContext *ctx = malloc(sizeof(*ctx));

  if (frame->is_keyframe) {
    err = start_decoder(subdec, frame);
    if (err) {
      printf("start_decoder failed with %d\n", err);
      return;
    }
  }

  sample_buffer =
      create_sample_buffer(decoder->format_description, frame, &condensed_ptr);
  if (!sample_buffer || sample_buffer == NULL) {
    printf("create_sample_buffer failed\n");
    return;
  }

  VTDecodeFrameFlags decode_flags =
      kVTDecodeFrame_EnableAsynchronousDecompression;

  ctx->decoder = decoder;
  ctx->condensed = condensed_ptr;

  VTDecompressionSessionDecodeFrame(decoder->decompression_session,
                                    sample_buffer, decode_flags, ctx, NULL);
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
      (size_t *)frame->parameter_sets_lengths, frame->nalu_h_len,
      &format_description);
  if (status) {
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
