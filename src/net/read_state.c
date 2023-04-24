#include "read_state.h"
#include "../util/util.h"
#include <stddef.h>
#include <stdio.h>

void read_state_alloc_buffer(struct ReadState *read_state, uint8_t **out_buffer,
                             size_t *out_length) {
  switch (read_state->state) {
  case AllocatePacketID:
    read_state->packet_id_buffer =
        calloc(8, sizeof(*read_state->packet_id_buffer));
    *out_buffer = read_state->packet_id_buffer;
    *out_length = 8;
    return;
  case FillPacketID:
    *out_buffer = read_state->packet_id_buffer + read_state->current_offset;
    *out_length = 8 - read_state->current_offset;
    break;

  case AllocateBuffer:
    read_state->buffer =
        calloc(read_state->buffer_len, sizeof(*read_state->buffer));
    *out_buffer = read_state->buffer;
    *out_length = read_state->buffer_len;
    break;
  case FillBuffer:
    *out_buffer = read_state->buffer + read_state->current_offset;
    *out_length = read_state->buffer_len - read_state->current_offset;
    break;

  default:
    /* UNREACHABLE */
    break;
  }
}

int read_state_handle_buffer(struct ReadState *state, uint8_t *buffer,
                             uint32_t nread) {
  state->current_offset += nread;

  printf("offs %d\n", state->current_offset);

  switch (state->state) {
  case AllocatePacketID:
    if (state->current_offset != 8) {
      state->state = FillPacketID;
      break;
    }
    /* FALLTHROUGH IF BUFFER IS FULL */
  case FillPacketID:
    if (state->current_offset != 8) {
      break;
    }
    read_packet_id(state->packet_id_buffer, &state->buffer_len,
                   &state->packet_type);
    free(state->packet_id_buffer);
    printf("got packet id %d\n", state->buffer_len);
    state->state = AllocateBuffer;
    state->current_offset = 0;
    break;

  case AllocateBuffer:
    if (state->current_offset != state->buffer_len) {
      state->state = FillBuffer;
      break;
    }
    /* FALLTHROUGH IF BUFFER IS FULL */
  case FillBuffer:
    if (state->current_offset != state->buffer_len) {
      break;
    }
    return 1;
  default:
    break;
  }
  return 0;
}
