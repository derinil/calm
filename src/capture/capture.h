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

// We conditionally compile the implementations based on the platform.

// NAL unit
struct CompressedFrame {
  uint8_t start_code[4];
  uint8_t sps[128];
  uint8_t psp[128];
};

// TODO: use uint8_t over char
typedef void (*CompressedFrameHandler)(char *data, size_t length);

struct Capturer
{
  // Height of the display in pixels.
  size_t height;
  // Width of the display in pixels.
  size_t width;
  // Should the frames be compressed
  int should_compress;
  // Compressed frame handler
  CompressedFrameHandler compressed_frame_handler;
};

// Sets up the backend for the capturer.
// This will allocate a capturer to be used throughout the lifetime of a
// program.
int setup(struct Capturer **target, CompressedFrameHandler handler);
// start_capture starts the screen capturer. frames will be h264 compressed.
// this does not block.
int start_capture(struct Capturer *capturer);
int stop_capture(struct Capturer *capturer);
// release_compressed_frame releases/frees a compressed frame.
// should be called after the frame is processed in the handler.
int release_compressed_frame(struct CompressedFrame *frame);

#endif
