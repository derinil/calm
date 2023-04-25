#include "control.h"
#include "../crestial/crestial.h"
#include "../util/util.h"
#include <assert.h>
#include <stdlib.h>

void ctrl_release_control(struct Control **ctrl) {
  free(*ctrl);
  *ctrl = NULL;
}

void ctrl_release_serializedcontrol(struct SerializedControl *ctrl) {
  ctrl_release_control(&ctrl->ctrl);
  free(ctrl->buffer);
  free(ctrl);
}

struct SerializedControl *ctrl_serialize_control(struct Control *ctrl) {
  struct SerializedControl *ser;

  uint8_t *data;
  uint32_t size;
  struct CrestialWriter *writer = crestial_writer_init(&data, &size, 500);

  crestial_write_i32(writer, ctrl->source);
  crestial_write_i32(writer, ctrl->type);
  crestial_write_u32(writer, ctrl->value);
  if (ctrl->source == Mouse && ctrl->type == Move) {
    crestial_write_u32(writer, ctrl->pos_x);
    crestial_write_u32(writer, ctrl->pos_y);
    crestial_write_i32(writer, ctrl->pos_x_delta);
    crestial_write_i32(writer, ctrl->pos_y_delta);
  }

  crestial_writer_finalize(writer);

  ser = calloc(1, sizeof(*ser));
  ser->buffer = data;
  ser->length = size;
  ser->ctrl = ctrl;

  return ser;
}

struct Control *ctrl_unmarshal_control(uint8_t *buffer, uint32_t length) {
  struct Control *ctrl = calloc(1, sizeof(*ctrl));
  struct CrestialReader *reader = crestial_reader_init(buffer, length);

  ctrl->source = crestial_read_i32(reader);
  ctrl->type = crestial_read_i32(reader);
  if (ctrl->source == Mouse && ctrl->type == Move) {
    ctrl->pos_x = crestial_read_u32(reader);
    ctrl->pos_y = crestial_read_u32(reader);
    ctrl->pos_x_delta = crestial_read_i32(reader);
    ctrl->pos_y_delta = crestial_read_i32(reader);
  }

  crestial_reader_finalize(reader);
  return ctrl;
}
