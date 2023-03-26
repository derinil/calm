#ifndef CAPTURE_H_
#define CAPTURE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// TODO: remove
#include <stdio.h>

// TODO: use uint8_t over char
typedef void (*DecodedFrameHandler)(char *data, size_t length);

struct Decoder {
  // Height of the display in pixels.
  size_t height;
  // Width of the display in pixels.
  size_t width;
  // Decoded frame handler
  DecodedFrameHandler decoded_frame_handler;
};

// Sets up the backend for the capturer.
// This will allocate a capturer to be used throughout the lifetime of a
// program.
struct Decoder *setup_decoder(DecodedFrameHandler handler);
int decode_frame(struct Decoder *decoder);
int stop_decode(struct Decoder *decoder);
// release_compressed_frame releases/frees a compressed frame.
// should be called after the frame is processed in the handler.
int release_compressed_frame(struct CompressedFrame *frame);

#endif
