#include "capture.h"
#include <stdlib.h>
#include <string.h>

static void write_uint64(uint8_t *buf, uint64_t u) {
  for (int i = 0; i < 8; i++)
    buf[i] = u & (0xF0000000 >> i);
}

uint8_t *serialize_cframe(struct CFrame *frame) {
  uint8_t *buf;
  size_t buf_len = 0;

  // Length for buf_len itself
  buf_len += sizeof(buf_len);
  // Length for frame_length
  buf_len += sizeof(frame->frame_length);
  // Length for frame
  buf_len += frame->frame_length;
  buf_len += sizeof(frame->parameter_sets_count);
  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    // Size of ps length
    buf_len += sizeof(frame->parameter_sets_lengths[i]);
    // Ps length
    buf_len += frame->parameter_sets_lengths[i];
  }

  buf = malloc(sizeof(*buf) * buf_len);
  if (!buf)
    return NULL;

  write_uint64(buf, buf_len);
  write_uint64(buf, frame->frame_length);
  memcpy(buf, frame->frame, frame->frame_length);
  write_uint64(buf, frame->parameter_sets_count);
  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    write_uint64(buf, frame->parameter_sets_lengths[i]);
    memcpy(buf, frame->parameter_sets[i], frame->parameter_sets_lengths[i]);
  }

  return buf;
}
