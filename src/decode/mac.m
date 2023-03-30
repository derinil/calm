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
#include <strings.h>

struct MacDecoder {
  struct Decoder decoder;
  VTDecompressionSessionRef decompression_session;
  CMVideoFormatDescriptionRef format_description;
};

void raw_decompressed_frame_callback(void *decompressionOutputRefCon,
                                     void *sourceFrameRefCon, OSStatus status,
                                     VTDecodeInfoFlags infoFlags,
                                     CVImageBufferRef imageBuffer,
                                     CMTime presentationTimeStamp,
                                     CMTime presentationDuration) {
  struct DFrame *frame = malloc(sizeof(*frame));
  if (!frame)
    return;

  if (!imageBuffer || infoFlags & kVTDecodeInfo_FrameDropped) {
    printf("frame dropped or failed, status: %d\n", status);
    return;
  }

  // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
  // GL_RGB, opengl attaches a 1 alpha, is that faster or is it faster to get the alpha from decompressor
  // Might be faster to pass less bytes to opengl

  // CGSize size = CVImageBufferGetEncodedSize(imageBuffer);
  // printf("size %f %f\n", size.height, size.width);

  // size = CVImageBufferGetDisplaySize(imageBuffer);
  // printf("size %f %f\n", size.height, size.width);

  CVPixelBufferLockBaseAddress(imageBuffer, 0);

  void *baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
  size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
  size_t width = CVPixelBufferGetWidth(imageBuffer);
  size_t height = CVPixelBufferGetHeight(imageBuffer);
  OSType pixelFormatType = CVPixelBufferGetPixelFormatType(imageBuffer);

  printf("bpr %lu pixels %d\n", bytesPerRow, pixelFormatType);

  CVPixelBufferUnlockBaseAddress(imageBuffer, 0);

  printf("received decompressed frame\n");
}

void release_dframe() {
  // TODO:
  CVOpenGLTextureRelease(NULL);
}

CMSampleBufferRef
create_sample_buffer(CMFormatDescriptionRef format_description,
                     struct CFrame *frame) {
  OSStatus status;
  // CMBlockBufferRef *block_bufs;
  CMBlockBufferRef block_buf;
  CMSampleBufferRef sample_buffer;

  const size_t nalu_header_length = 4;

  // block_bufs = malloc(frame->nalus_count * sizeof(*block_bufs));
  // if (!block_bufs)
  //   return NULL;

  // for (size_t x = 0; x < frame->nalus_count; x++) {
  //   status = CMBlockBufferCreateWithMemoryBlock(
  //       NULL, frame->nalus[x], frame->nalus_lengths[x], NULL, NULL, 0,
  //       frame->nalus_lengths[x], 0, &block_bufs[x]);
  //   if (status)
  //     return NULL;
  // }

  status = CMBlockBufferCreateWithMemoryBlock(
      NULL, frame->solid_frame, frame->solid_frame_length, NULL, NULL, 0,
      frame->solid_frame_length, 0, &block_buf);
  if (status)
    return NULL;

  status = CMSampleBufferCreate(NULL, block_buf, TRUE, NULL, NULL,
                                format_description, 1, 0, NULL, 0, NULL,
                                &sample_buffer);
  if (status)
    return NULL;

  // for (size_t i = 0; i < frame->frame_length; i++) {
  //   printf("%02x", frame->frame[i]);
  // }
  // printf("\n");
  //
  // nalu_len = CFSwapInt32BigToHost(nalu_len);

  if (block_buf)
    CFRelease(block_buf);

  CFRetain(sample_buffer);

  return sample_buffer;
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

  status = VTDecompressionSessionDecodeFrame(
      this->decompression_session, sample_buffer, decode_flags, NULL, NULL);
  if (status)
    printf("decodeframe failed with %d\n", status);

  CFRelease(sample_buffer);
  // sample_buffer = NULL;
}

struct Decoder *setup_decoder(DecodedFrameHandler decoded_frame_handler) {
  struct MacDecoder *this = malloc(sizeof(*this));
  if (!this)
    return NULL;
  memset(this, 0, sizeof(*this));
  this->decoder.decoded_frame_handler = decoded_frame_handler;
  return &this->decoder;
}

int start_decoder(struct Decoder *decoder, struct CFrame *frame) {
  struct MacDecoder *this = (struct MacDecoder *)decoder;

  // Already initialized
  // TODO: maybe simply remove this check so we can create new session
  // with new pps, CFRelease format and session
  if (this->decompression_session) {
    printf("exists");
    return 0;
  }

  // TODO: make this variable?
  const int nalu_header_length = 4;

  for (size_t i = 0; i < frame->parameter_sets_count; i++) {
    // Trim the nalu starting code/header
    // frame->parameter_sets[i] = frame->parameter_sets[i] + nalu_header_length;
#if 0
    for (size_t x = 0; x < frame->parameter_sets_lengths[i]; x++) {
      printf("%02x", frame->parameter_sets[i][x]);
    }
    printf("\n");
#endif
  }

  CMFormatDescriptionRef format_description = NULL;
  // TODO: Figure out NALUnitHeaderLength, 2 seems to be working
  // Size, in bytes, of the NALUnitLength field in an AVC video sample or AVC
  // parameter set sample. Pass 1, 2, or 4.
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

#if 1
  printf("got dimensions %d %d\n", dimensions.width, dimensions.height);
#endif

  // TODO: whats the best pixel format for opengl
  // https://developer.apple.com/documentation/corevideo/1563591-pixel_format_identifiers/kcvpixelformattype_32argb?language=objc
  // SInt32 destinationPixelType =
  // kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
  // SInt32 destinationPixelType = kCVPixelFormatType_32BGRA;
  SInt32 destinationPixelType = kCVPixelFormatType_24RGB;

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
  CFDictionarySetValue(pixel_opts, kCVPixelBufferOpenGLCompatibilityKey,
                       kCFBooleanTrue);

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
  return 0;
}
