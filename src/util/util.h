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

#endif