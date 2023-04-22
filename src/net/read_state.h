#ifndef READ_STATE_H_
#define READ_STATE_H_

#include <stdint.h>
#include <stddef.h>

enum ReadStateState {
  AllocatePacketID,
  FillPacketID,
  AllocateBuffer,
  FillBuffer,
};

// NOTE: https://groups.google.com/g/libuv/c/fRNQV_QGgaA
struct ReadState {
  enum ReadStateState state;
  uint32_t buffer_len;
  uint32_t packet_type;
  // TODO: can make these single buffer
  uint8_t *packet_id_buffer;
  uint8_t *buffer;
  uint32_t current_offset;
};

// Allocations could never fail so its void
void read_state_alloc_buffer(struct ReadState *read_state, uint8_t **out_buffer, size_t *out_length);
// on_read returns 1 when the final buffer is completely filled.
int read_state_handle_buffer(struct ReadState *state, uint8_t *buffer, uint32_t nread);

#endif
