#include "capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

union ULLSplitter {
  uint64_t ull;
  uint8_t bs[8];
};

static void write_uint64(uint8_t *buf, uint64_t u) {
  union ULLSplitter split;
  split.ull = u;
  memcpy(buf, split.bs, 8);
}

uint64_t read_uint64(uint8_t *buf) {
  union ULLSplitter split;
  memcpy(split.bs, buf, 8);
  return split.ull;
}

struct SerializedBuffer *serialize_cframe(struct CFrame *frame) {
  uint8_t *buf;
  uint64_t buf_len = 0;
  uint64_t buf_off = 0;
  struct SerializedBuffer *serbuf;

  serbuf = malloc(sizeof(*serbuf));
  if (!serbuf)
    return NULL;
  memset(serbuf, 0, sizeof(*serbuf));

  buf_len = 0;
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

  buf_off = 0;

  write_uint64(buf + buf_off, buf_len);
  buf_off += 8;

  write_uint64(buf + buf_off, frame->frame_length);
  buf_off += 8;

  memcpy(buf + buf_off, frame->frame, frame->frame_length);
  buf_off += frame->frame_length;

  write_uint64(buf + buf_off, frame->parameter_sets_count);
  buf_off += 8;

  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    write_uint64(buf + buf_off, frame->parameter_sets_lengths[i]);
    buf_off += 8;
    memcpy(buf + buf_off, frame->parameter_sets[i],
           frame->parameter_sets_lengths[i]);
    buf_off += frame->parameter_sets_lengths[i];
  }

  // TODO: Sanity check
  if (buf_off != buf_len) {
    printf("offset and length mismatch\n");
  }

  serbuf->buffer = buf;
  serbuf->length = buf_len;

  return serbuf;
}

void release_serbuf_cframe(struct SerializedBuffer *buffer) {
  free(buffer->buffer);
  free(buffer);
}

struct CFrame *unmarshal_cframe(uint8_t *buffer, uint64_t length) {
  uint64_t off = 0;
  struct CFrame *frame = malloc(sizeof(*frame));

  off = 0;

  frame->frame_length = read_uint64(buffer + off);
  off += 8;

  frame->frame = malloc(sizeof(*frame->frame) * frame->frame_length);
  memcpy(frame->frame, buffer + off, frame->frame_length);
  off += frame->frame_length;

  frame->parameter_sets_count = read_uint64(buffer + off);
  off += 8;

  frame->parameter_sets_lengths = malloc(
      sizeof(*frame->parameter_sets_lengths) * frame->parameter_sets_count);
  frame->parameter_sets =
      malloc(sizeof(*frame->parameter_sets) * frame->parameter_sets_count);

  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    frame->parameter_sets_lengths[i] = read_uint64(buffer + off);
    off += 8;

    frame->parameter_sets[i] = malloc(sizeof(*frame->parameter_sets[i]) *
                                      frame->parameter_sets_lengths[i]);
    memcpy(frame->parameter_sets[i], buffer + off,
           frame->parameter_sets_lengths[i]);
    off += frame->parameter_sets_lengths[i];
  }

  // TODO: Sanity check
  if (off != length) {
    printf("offset and length mismatch\n");
  }

  return frame;
}
