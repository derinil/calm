#include "read_state.h"
#include "../util/util.h"
#include <stddef.h>
#include <stdio.h>

void read_state_alloc_buffer(struct ReadState *read_state, uint8_t **out_buffer,
                             size_t *out_length) {
  switch (read_state->state) {
  case AllocateBufferLength:
    read_state->buf_len_buffer =
        malloc(sizeof(*read_state->buf_len_buffer) * 4);
    *out_buffer = read_state->buf_len_buffer;
    *out_length = 4;
    return;
  case FillBufferLength:
    *out_buffer = read_state->buf_len_buffer + read_state->current_offset;
    *out_length = 4 - read_state->current_offset;
    break;

  case AllocatePacketTypeLength:
    read_state->packet_type_buffer =
        malloc(sizeof(*read_state->packet_type_buffer) * 4);
    *out_buffer = read_state->packet_type_buffer;
    *out_length = 4;
    break;
  case FillPacketTypeLength:
    *out_buffer = read_state->packet_type_buffer + read_state->current_offset;
    *out_length = 4 - read_state->current_offset;
    break;

  case AllocateBuffer:
    read_state->buffer =
        malloc(read_state->buf_len * sizeof(*read_state->buffer));
    *out_buffer = read_state->buffer;
    *out_length = read_state->buf_len;
    break;
  case FillBuffer:
    *out_buffer = read_state->buffer + read_state->current_offset;
    *out_length = read_state->buf_len - read_state->current_offset;
    break;

  default:
    /* UNREACHABLE */
    break;
  }
}

int read_state_handle_buffer(struct ReadState *state, uint8_t *buffer,
                             uint32_t nread) {
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
    state->buf_len = read_uint32(state->buf_len_buffer);
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
    state->packet_type = read_uint32(state->packet_type_buffer);
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
  default:
    break;
  }
  return 0;
}
