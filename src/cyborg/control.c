#include "control.h"
#include <assert.h>
#include <stdlib.h>
#include "../util/util.h"

void ctrl_release_control(struct Control **ctrl) {
  free(*ctrl);
  *ctrl = NULL;
}

struct SerializedBuffer *ctrl_serialize_control(struct Control *ctrl) {
  uint8_t *buf;
  uint32_t buf_len = 0;
  uint32_t packet_type = 2;
  uint64_t buf_off = 0;
  struct SerializedBuffer *serbuf;

  serbuf = calloc(1, sizeof(*serbuf));
  if (!serbuf)
    return NULL;

  buf_len = 0;
  buf_len += sizeof(buf_len);
  buf_len += sizeof(packet_type);
  buf_len += sizeof(ctrl->source);
  buf_len += sizeof(ctrl->type);
  buf_len += sizeof(ctrl->value);
  buf_len += sizeof(ctrl->pos_x);
  buf_len += sizeof(ctrl->pos_y);

  buf = calloc(buf_len, sizeof(*buf));
  if (!buf)
    return NULL;

  buf_off = 0;

  write_uint32(buf + buf_off, buf_len - sizeof(buf_len));
  buf_off += sizeof(buf_len);
  write_uint32(buf + buf_off, packet_type);
  buf_off += sizeof(packet_type);
  write_uint32(buf + buf_off, ctrl->source);
  buf_off += sizeof(ctrl->source);
  write_uint32(buf + buf_off, ctrl->type);
  buf_off += sizeof(ctrl->type);
  write_uint32(buf + buf_off, ctrl->value);
  buf_off += sizeof(ctrl->value);
  write_uint32(buf + buf_off, ctrl->pos_x);
  buf_off += sizeof(ctrl->pos_x);
  write_uint32(buf + buf_off, ctrl->pos_y);
  buf_off += sizeof(ctrl->pos_y);

  assert(buf_off == buf_len);

  serbuf->buffer = buf;
  serbuf->length = buf_len;

  return serbuf;
}

struct Control *ctrl_unmarshal_control(uint8_t *buffer, uint32_t length) {
  uint8_t *buf;
  uint32_t buf_len = 0;
  uint32_t packet_type = 2;
  uint64_t buf_off = 0;
  struct Control *ctrl = calloc(1, sizeof(*ctrl));

  buf_len = 0;
  buf_len += sizeof(buf_len);
  buf_len += sizeof(packet_type);
  buf_len += sizeof(ctrl->source);
  buf_len += sizeof(ctrl->type);
  buf_len += sizeof(ctrl->value);
  buf_len += sizeof(ctrl->pos_x);
  buf_len += sizeof(ctrl->pos_y);

  buf = calloc(buf_len, sizeof(*buf));
  if (!buf)
    return NULL;

  buf_off = 0;

  ctrl->source = read_uint32(buf + buf_off);
  buf_off += sizeof(ctrl->source);
  ctrl->type = read_uint32(buf + buf_off);
  buf_off += sizeof(ctrl->type);
  ctrl->value = read_uint32(buf + buf_off);
  buf_off += sizeof(ctrl->value);
  ctrl->pos_x = read_uint32(buf + buf_off);
  buf_off += sizeof(ctrl->pos_x);
  ctrl->pos_y = read_uint32(buf + buf_off);
  buf_off += sizeof(ctrl->pos_y);

  assert(buf_off == buf_len);

  return ctrl;
}