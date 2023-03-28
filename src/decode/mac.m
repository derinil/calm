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
#include <strings.h>

struct MacDecoder {
  struct Decoder decoder;
  VTDecompressionSessionRef decompression_session;
  CMVideoFormatDescriptionRef format_description;
};

static void raw_decompressed_frame_callback(
    void *decompressionOutputRefCon, void *sourceFrameRefCon, OSStatus status,
    VTDecodeInfoFlags infoFlags, CVImageBufferRef imageBuffer,
    CMTime presentationTimeStamp, CMTime presentationDuration) {
  printf("received decompressed frame\n");
}

CMSampleBufferRef create_sample_buffer(CMFormatDescriptionRef fmt_desc,
                                       void *buffer, int size) {
  OSStatus status;
  CMBlockBufferRef block_buf;
  CMSampleBufferRef sample_buf;

  block_buf = NULL;
  sample_buf = NULL;

  status = CMBlockBufferCreateWithMemoryBlock(
      kCFAllocatorDefault, // structureAllocator
      buffer,              // memoryBlock
      size,                // blockLength
      kCFAllocatorNull,    // blockAllocator
      NULL,                // customBlockSource
      0,                   // offsetToData
      size,                // dataLength
      0,                   // flags
      &block_buf);

  if (!status) {
    status = CMSampleBufferCreate(kCFAllocatorDefault, // allocator
                                  block_buf,           // dataBuffer
                                  TRUE,                // dataReady
                                  0,                   // makeDataReadyCallback
                                  0,                   // makeDataReadyRefcon
                                  fmt_desc,            // formatDescription
                                  1,                   // numSamples
                                  0,                   // numSampleTimingEntries
                                  NULL,                // sampleTimingArray
                                  0,                   // numSampleSizeEntries
                                  NULL,                // sampleSizeArray
                                  &sample_buf);
  }

  if (block_buf)
    CFRelease(block_buf);

  return sample_buf;
}

void decode_frame(struct Decoder *decoder, struct CFrame *frame) {
  int err;
  OSStatus status;
  CMSampleBufferRef sample_buf;
  struct MacDecoder *this = (struct MacDecoder *)decoder;

  if (frame->is_keyframe) {
    err = start_decoder(decoder, frame);
    if (err)
      printf("start_decoder failed with %d\n", err);
  }

  // sample_buf = create_sample_buffer(
  //     videotoolbox->cm_fmt_desc, vtctx->bitstream, vtctx->bitstream_size);

  // if (!sample_buf)
  //   return -1;

  // status = VTDecompressionSessionDecodeFrame(this->decompression_session,
  // NULL,
  //                                            0, NULL, 0);
  // if (status)
  //   printf("decodeframe failed with %d\n", status);
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
  // with new pps
  if (this->decompression_session)
    return 0;

  // TODO: make this variable?
  const int nalu_header_length = 4;

  for (size_t i = 0; i < frame->parameter_sets_count; i++) {
    // Trim the nalu starting code/header
    frame->parameter_sets[i] = frame->parameter_sets[i] + nalu_header_length;
    for (size_t x = 0; x < frame->parameter_sets_lengths[i]; x++) {
      printf("%02x", frame->parameter_sets[i][x]);
    }
    printf("\n");
  }

  CMFormatDescriptionRef format_description = NULL;
  // TODO: Figure out NALUnitHeaderLength, 2 seems to be working
  // Size, in bytes, of the NALUnitLength field in an AVC video sample or AVC
  // parameter set sample. Pass 1, 2, or 4.
  OSStatus status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
      NULL, frame->parameter_sets_count,
      // FIXME: ugly hack
      (const unsigned char *const *)frame->parameter_sets,
      frame->parameter_sets_lengths, nalu_header_length, &format_description);
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
  printf("got dimensions %d %d\n", dimensions.height, dimensions.width);
#endif

  SInt32 destinationPixelType = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;

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
