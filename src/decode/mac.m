#include "../capture/capture.h"
#include "decode.h"
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

struct MacDecoder {
  struct Decoder decoder;
  VTDecompressionSessionRef decompression_session;
};

void raw_decompressed_frame_callback(void *decompressionOutputRefCon,
                                     void *sourceFrameRefCon, OSStatus status,
                                     VTDecodeInfoFlags infoFlags,
                                     CVImageBufferRef imageBuffer,
                                     CMTime presentationTimeStamp,
                                     CMTime presentationDuration) {
  printf("received decompressed frame\n");
}

void decode_frame(struct Decoder *decoder, struct CFrame *frame) {
  int err;
  struct MacDecoder *this = (struct MacDecoder *)decoder;

  if (frame->is_keyframe) {
    err = start_decoder(decoder, frame);
    if (err)
      printf("start_decoder failed with %d\n", err);
  }
}

struct Decoder *setup_decoder(DecodedFrameHandler decoded_frame_handler) {
  struct MacDecoder *this = malloc(sizeof(*this));
  if (!this)
    return NULL;
  this->decoder.decoded_frame_handler = decoded_frame_handler;
  return &this->decoder;
}

int start_decoder(struct Decoder *decoder, struct CFrame *frame) {
  struct MacDecoder *this = (struct MacDecoder *)decoder;

  // Already initialized
  // TODO: maybe simply remove this check so we can create new session
  // with new pps
  if (this->decompression_session)
    return 0;

  for (size_t i = 0; i < frame->parameter_sets_count; i++) {
    for (size_t x = 0; x < frame->parameter_sets_lengths[i]; x++) {
      printf("%02x", frame->parameter_sets[i][x]);
    }
    printf("\n");
  }

  CMFormatDescriptionRef format_description = NULL;
  // TODO: NALUnitHeaderLength
  // Size, in bytes, of the NALUnitLength field in an AVC video sample or AVC
  // parameter set sample. Pass 1, 2, or 4.
  OSStatus status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
      NULL, frame->parameter_sets_count,
      // FIXME: ugly hack
      (const unsigned char *const *)frame->parameter_sets,
      frame->parameter_sets_lengths, 4, &format_description);
  if (status) {
    printf("lol status %d\n", status);
    exit(1);
    return status;
  }

  CMVideoDimensions dimensions =
      CMVideoFormatDescriptionGetDimensions(format_description);

  printf("got dimensions %d %d\n", dimensions.height, dimensions.width);

  SInt32 destinationPixelType = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;

  CFTypeRef pixel_keys[] = {
      kCVPixelBufferPixelFormatTypeKey,
      kCVPixelBufferWidthKey,
      kCVPixelBufferHeightKey,
      kCVPixelBufferOpenGLCompatibilityKey,
  };

  CFTypeRef pixel_values[] = {
      &destinationPixelType,
      &dimensions.width,
      &dimensions.height,
      kCFBooleanTrue,
  };

  CFDictionaryRef pixel_opts = CFDictionaryCreate(
      NULL, pixel_keys, pixel_values, 4, &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);

  CFTypeRef decompression_keys[] = {
      kVTDecompressionPropertyKey_RealTime,
  };

  CFTypeRef decompression_values[] = {
      kCFBooleanTrue,
  };

  CFDictionaryRef decompression_opts = CFDictionaryCreate(
      NULL, decompression_keys, decompression_values, 1,
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

  const VTDecompressionOutputCallbackRecord callback = {
      raw_decompressed_frame_callback, NULL};

  VTDecompressionSessionRef decompression_session;
  status = VTDecompressionSessionCreate(kCFAllocatorDefault, format_description,
                                        decompression_opts, pixel_opts,
                                        &callback, &decompression_session);
  if (status)
    return status;

  CFRetain(decompression_session);
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
