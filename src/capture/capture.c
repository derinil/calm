#include "capture.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_cframe_hash(struct CFrame *frame);

// Quick hash function
// http://www.partow.net/programming/hashfunctions/index.html
static unsigned int DJBHash(char *str, unsigned int length) {
  unsigned int hash = 5381;
  unsigned int i = 0;

  for (i = 0; i < length; ++str, ++i) {
    hash = ((hash << 5) + hash) + (*str);
  }

  return hash;
}

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
  // Length for nalu_h_len
  buf_len += sizeof(frame->nalu_h_len);
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
  memset(buf, 0, sizeof(*buf));

  buf_off = 0;

  // Subtract the sizeof buf_len from buf_len so that buf_len is actually the
  // length of the frame
  write_uint64(buf + buf_off, buf_len - sizeof(buf_len));
  buf_off += 8;

  write_uint64(buf + buf_off, frame->nalu_h_len);
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

  frame->frame_length = read_uint64(buffer + off);
  off += 8;

  frame->frame = malloc(sizeof(*frame->frame) * frame->frame_length);
  // TODO: dont need this
  // memset(frame->frame, 0, sizeof(*frame->frame));
  memcpy(frame->frame, buffer + off, frame->frame_length);
  off += frame->frame_length;

  frame->parameter_sets_count = read_uint64(buffer + off);
  off += 8;

  frame->is_keyframe = frame->parameter_sets_count > 0;

  frame->parameter_sets_lengths = malloc(
      sizeof(*frame->parameter_sets_lengths) * frame->parameter_sets_count);
  memset(frame->parameter_sets_lengths, 0,
         sizeof(*frame->parameter_sets_lengths));
  frame->parameter_sets =
      malloc(sizeof(*frame->parameter_sets) * frame->parameter_sets_count);
  memset(frame->parameter_sets, 0, sizeof(*frame->parameter_sets));

  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    frame->parameter_sets_lengths[i] = read_uint64(buffer + off);
    off += 8;

    frame->parameter_sets[i] = malloc(sizeof(*frame->parameter_sets[i]) *
                                      frame->parameter_sets_lengths[i]);
    memcpy(frame->parameter_sets[i], buffer + off,
           frame->parameter_sets_lengths[i]);
    off += frame->parameter_sets_lengths[i];
  }

  assert(off == length);

  return frame;
}

void print_cframe_hash(struct CFrame *frame) {
  printf("frame length %llu\n", frame->frame_length);
  // for (uint64_t i = 0; i < frame->frame_length; i++)
  //   printf("%x", frame->frame[i]);
  // printf("\n");
  printf("got hash %u\n", DJBHash((char *)frame->frame, frame->frame_length));
  printf("ps count %llu\n", frame->parameter_sets_count);
  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    printf("ps length %llu\n", frame->parameter_sets_lengths[i]);
    for (uint64_t x = 0; x < frame->parameter_sets_lengths[i]; x++)
      printf("%x", frame->parameter_sets[i][x]);
    printf("\n");
  }
}

struct SerializedBuffer *condense_cframe(struct CFrame *frame) {
  uint8_t *buf;
  uint64_t total_len = 0;
  const uint8_t start_code[4] = {0, 0, 0, 1};

  // 4 byte start codes between each nalu

  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    total_len += 4;
    total_len += frame->parameter_sets_lengths[i];
  }

  total_len += 4;
  total_len += frame->frame_length;

  buf = calloc(total_len, sizeof(*buf));
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    memcpy(buf, start_code, 4);
    memcpy(buf, frame->parameter_sets[i], frame->parameter_sets_lengths[i]);
  }

  memcpy(buf, start_code, 4);
  memcpy(buf, frame->frame, frame->frame_length);

  struct SerializedBuffer *serbuf = calloc(1,sizeof(*serbuf));
  serbuf->buffer = buf;
  serbuf->length = total_len;
  
  return serbuf;
}