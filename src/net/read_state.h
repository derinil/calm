#ifndef READ_STATE_H_
#define READ_STATE_H_

#include <stdint.h>

enum ReadStateState {
  AllocateBufferLength,
  FillBufferLength,
  AllocatePacketTypeLength,
  FillPacketTypeLength,
  AllocateBuffer,
  FillBuffer,
};

// NOTE: https://groups.google.com/g/libuv/c/fRNQV_QGgaA
struct ReadState {
  enum ReadStateState state;
  uint32_t buf_len;
  uint8_t *buf_len_buffer;
  uint32_t packet_type;
  uint8_t *packet_type_buffer;
  uint8_t *buffer;
  uint32_t current_offset;
};

#endif