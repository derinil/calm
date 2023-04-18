#ifndef CAPTURE_H_
#define CAPTURE_H_

#include <stddef.h>
#include <stdint.h>

// We conditionally compile the implementations based on the platform.

// TODO: combine ps and nalus all into one array
struct CFrame {
  int is_keyframe;
  uint64_t nalu_h_len;
  uint8_t **parameter_sets;
  uint64_t parameter_sets_count;
  uint64_t *parameter_sets_lengths;
  uint8_t **nalus;
  uint64_t nalus_count;
  uint64_t *nalus_lengths;
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

struct SerializedBuffer {
  uint8_t *buffer;
  uint64_t length;
};

struct SerializedBuffer *serialize_cframe(struct CFrame *frame);
void release_serbuf_cframe(struct SerializedBuffer *buffer);
uint64_t read_uint64(uint8_t *buf);
struct CFrame *unmarshal_cframe(uint8_t *buffer, uint64_t length);
uint8_t *condense_nalus(struct CFrame *frame, uint64_t *len);

#endif
