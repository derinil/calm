#include "capture.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

union ULLSplitter {
  uint64_t ull;
  uint8_t bs[8];
};

void write_uint64(uint8_t *buf, uint64_t u) {
  union ULLSplitter split;
  split.ull = u;
  memcpy(buf, split.bs, 8);
}

uint64_t read_uint64(uint8_t *buf) {
  union ULLSplitter split;
  memcpy(split.bs, buf, 8);
  return split.ull;
}

void retain_cframe(struct CFrame *frame) {
  atomic_fetch_add(&frame->refcount, 1);
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
  // Length for nalu_h_len
  buf_len += sizeof(frame->nalu_h_len);

  // Ps
  buf_len += sizeof(frame->parameter_sets_count);
  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    // Size of ps length
    buf_len += sizeof(frame->parameter_sets_lengths[i]);
    // Ps length
    buf_len += frame->parameter_sets_lengths[i];
  }

  // Nalus
  buf_len += sizeof(frame->nalus_count);
  for (uint64_t i = 0; i < frame->nalus_count; i++) {
    // Size of nalu length
    buf_len += sizeof(frame->nalus_lengths[i]);
    // Nalu length
    buf_len += frame->nalus_lengths[i];
  }

  buf = malloc(sizeof(*buf) * buf_len);
  if (!buf)
    return NULL;
  memset(buf, 0, sizeof(*buf));

  buf_off = 0;

  // Subtract the sizeof buf_len from buf_len so that buf_len is actually the
  // length of the frame
  write_uint64(buf + buf_off, buf_len - sizeof(buf_len));
  buf_off += 8;

  write_uint64(buf + buf_off, frame->nalu_h_len);
  buf_off += 8;

  write_uint64(buf + buf_off, frame->parameter_sets_count);
  buf_off += 8;
  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    write_uint64(buf + buf_off, frame->parameter_sets_lengths[i]);
    buf_off += 8;
    memcpy(buf + buf_off, frame->parameter_sets[i],
           frame->parameter_sets_lengths[i]);
    buf_off += frame->parameter_sets_lengths[i];
  }

  write_uint64(buf + buf_off, frame->nalus_count);
  buf_off += 8;
  for (uint64_t i = 0; i < frame->nalus_count; i++) {
    write_uint64(buf + buf_off, frame->nalus_lengths[i]);
    buf_off += 8;
    memcpy(buf + buf_off, frame->nalus[i], frame->nalus_lengths[i]);
    buf_off += frame->nalus_lengths[i];
  }

  assert(buf_off == buf_len);

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
  memset(frame, 0, sizeof(*frame));

  off = 0;

  frame->nalu_h_len = read_uint64(buffer + off);
  off += 8;

  frame->parameter_sets_count = read_uint64(buffer + off);
  off += 8;

  frame->is_keyframe = frame->parameter_sets_count > 0;

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

  frame->nalus_count = read_uint64(buffer + off);
  off += 8;

  frame->nalus_lengths =
      malloc(sizeof(*frame->nalus_lengths) * frame->nalus_count);
  frame->nalus = malloc(sizeof(*frame->nalus) * frame->nalus_count);

  for (uint64_t i = 0; i < frame->nalus_count; i++) {
    frame->nalus_lengths[i] = read_uint64(buffer + off);
    off += 8;

    frame->nalus[i] =
        malloc(sizeof(*frame->nalus[i]) * frame->nalus_lengths[i]);
    memcpy(frame->nalus[i], buffer + off, frame->nalus_lengths[i]);
    off += frame->nalus_lengths[i];
  }

  assert(off == length);

  return frame;
}
