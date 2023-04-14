#ifndef CAPTURE_H_
#define CAPTURE_H_

#include <stddef.h>
#include <stdint.h>

// We conditionally compile the implementations based on the platform.

struct CFrame {
  int is_keyframe;
  uint8_t **parameter_sets;
  uint64_t parameter_sets_count;
  uint64_t *parameter_sets_lengths;
  uint8_t *frame;
  uint64_t frame_length;
};

// TODO: use uint8_t over char
typedef void (*CompressedFrameHandler)(struct CFrame *frame);

struct Capturer {
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
struct Capturer *setup_capturer(CompressedFrameHandler handler);
// start_capture starts the screen capturer. frames will be h264 compressed.
// this does not block.
int start_capture(struct Capturer *capturer);
int stop_capture(struct Capturer *capturer);
void release_cframe(struct CFrame *frame);

#endif
