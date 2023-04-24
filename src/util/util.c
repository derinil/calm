#include "util.h"
#include <string.h>

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

void write_uint32(uint8_t *buf, uint32_t u) {
  union USplitter split;
  split.ull = u;
  memcpy(buf, split.bs, 4);
}

uint32_t read_uint32(uint8_t *buf) {
  union USplitter split;
  memcpy(split.bs, buf, 4);
  return split.ull;
}

uint8_t *create_packet_id(uint32_t length, uint32_t packet_type) {
  uint8_t *buf = calloc(8, sizeof(*buf));
  write_uint32(buf, length);
  write_uint32(buf + 4, packet_type);
  return buf;
}

void read_packet_id(uint8_t *buffer, uint32_t *length, uint32_t *packet_type) {
  *length = read_uint32(buffer);
  *packet_type = read_uint32(buffer + 4);
}

uint8_t *combine_two_str(uint8_t *one, uint32_t one_len, uint8_t *two,
                         uint32_t two_len) {
  uint8_t *out = malloc((one_len + two_len) * sizeof(*out));
  memcpy(out, one, one_len);
  memcpy(out + one_len, two, two_len);
  return out;
}
