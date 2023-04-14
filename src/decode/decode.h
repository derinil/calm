#ifndef DECODE_H_
#define DECODE_H_

#include "../capture/capture.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct DFrame {
  size_t width;
  size_t height;
  size_t data_length;
  size_t bytes_per_row;
  // 32 bit bgra
  uint8_t *data;
};

typedef void (*DecompressedFrameHandler)(struct DFrame *frame);

struct Decoder {
  // Height of the display in pixels.
  size_t height;
  // Width of the display in pixels.
  size_t width;
  // Initialized with keyframe
  int is_initialized;
  // Decoded frame handler
  DecompressedFrameHandler decompressed_frame_handler;
};

// Sets up the backend for the capturer.
// This will allocate a capturer to be used throughout the lifetime of a
// program.
struct Decoder *setup_decoder(DecompressedFrameHandler handler);
int start_decoder(struct Decoder *decoder, struct CFrame *frame);
void decode_frame(struct Decoder *decoder, struct CFrame *frame);
int stop_decoder(struct Decoder *decoder);
void release_dframe(struct DFrame *frame);

#endif
