#include "read_state.h"
#include "../util/util.h"
#include <stddef.h>

int read_state_handle_buffer(struct ReadState *state, uint8_t *buffer, uint32_t nread) {
  state->current_offset += nread;

  switch (state->state) {
  case AllocateBufferLength:
    if (state->current_offset != 4) {
      state->state = FillBufferLength;
      break;
    }
    /* FALLTHROUGH IF BUFFER IS FULL */
  case FillBufferLength:
    if (state->current_offset != 4) {
      break;
    }
    state->buf_len = read_uint64(state->buf_len_buffer);
    free(state->buf_len_buffer);
    state->state = AllocatePacketTypeLength;
    state->current_offset = 0;
    break;

  case AllocatePacketTypeLength:
    if (state->current_offset != 4) {
      state->state = FillPacketTypeLength;
      break;
    }
    /* FALLTHROUGH IF BUFFER IS FULL */
  case FillPacketTypeLength:
    if (state->current_offset != 4) {
      break;
    }
    state->buf_len = read_uint32(state->packet_type_buffer);
    free(state->packet_type_buffer);
    state->state = AllocateBuffer;
    state->current_offset = 0;
    break;

  case AllocateBuffer:
    if ((uint64_t)state->current_offset != state->buf_len) {
      state->state = FillBuffer;
      break;
    }
    /* FALLTHROUGH IF BUFFER IS FULL */
  case FillBuffer:
    if (state->current_offset != state->buf_len) {
      break;
    }
    return 1;
    break;
  default:
    break;
  }
  return 0;
}