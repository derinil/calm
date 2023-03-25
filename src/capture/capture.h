#ifndef CAPTURE_H_
#define CAPTURE_H_

#include <stdint.h>
#include <stddef.h>

// We conditionally compile the implementations based on the platform.

struct CFrame
{
    uint8_t *data;
    size_t length;
};

// TODO: use uint8_t over char
typedef void (*CompressedFrameHandler)(struct CFrame *frame);

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
struct Capturer *setup_capturer(CompressedFrameHandler handler);
// start_capture starts the screen capturer. frames will be h264 compressed.
// this does not block.
int start_capture(struct Capturer *capturer);
int stop_capture(struct Capturer *capturer);
// release_compressed_frame releases/frees a compressed frame.
// should be called after the frame is processed in the handler.
int release_compressed_frame(struct CFrame *frame);

#endif
