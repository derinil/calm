#ifndef DECODE_H_
#define DECODE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../capture/capture.h"

// TODO: use uint8_t over char
typedef void (*DecodedFrameHandler)(struct CFrame *frame);

struct Decoder {
  // Height of the display in pixels.
  size_t height;
  // Width of the display in pixels.
  size_t width;
  // Latest keyframe we have
  struct CFrame *curr_keyframe;
  // Initialized with keyframe
  int is_initialized;
  // Decoded frame handler
  DecodedFrameHandler decoded_frame_handler;
};

// Sets up the backend for the capturer.
// This will allocate a capturer to be used throughout the lifetime of a
// program.
struct Decoder *setup_decoder(DecodedFrameHandler handler);
int start_decoder(struct Decoder *decoder, struct CFrame *frame);
void decode_frame(struct Decoder *decoder, struct CFrame *frame);
int stop_decoder(struct Decoder *decoder);

#endif
