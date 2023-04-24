#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_

#include <stdlib.h>
#include <string.h>

// TODO: could probably write these with macros

union ULLSplitter {
  uint64_t ull;
  uint8_t bs[8];
};

void write_uint64(uint8_t *buf, uint64_t u);
uint64_t read_uint64(uint8_t *buf);

union USplitter {
  uint32_t ull;
  uint8_t bs[4];
};

void write_uint32(uint8_t *buf, uint32_t u);
uint32_t read_uint32(uint8_t *buf);

// create_packet_id allocates a buffer of 8 bytes and puts the buffer length in
// the first 4 and packet type in the next 4
uint8_t *create_packet_id(uint32_t length, uint32_t packet_type);
void read_packet_id(uint8_t *buffer, uint32_t *length, uint32_t *packet_type);

uint8_t *combine_two_str(uint8_t *one, uint32_t one_len, uint8_t *two,
                         uint32_t two_len);

#endif
