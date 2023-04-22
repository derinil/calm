#include "util.h"

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
  uint64_t id = (uint64_t)length << 32 | packet_type;
  uint8_t *buf = calloc(8, sizeof(*buf));
  write_uint64(buf, id);
  return buf;
}
