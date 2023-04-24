#include "control.h"
#include "../util/util.h"
#include <assert.h>
#include <stdlib.h>

void ctrl_release_serializedcontrol(struct SerializedControl *ctrl) {}

void ctrl_release_control(struct Control **ctrl) {
  free(*ctrl);
  *ctrl = NULL;
}

struct SerializedControl ctrl_serialize_control(struct Control *ctrl) {
  return (struct SerializedControl){0};
  // binn *obj = binn_object();

  // binn_object_set_int32(obj, "source", ctrl->source);
  // binn_object_set_int32(obj, "type", ctrl->type);
  // binn_object_set_uint32(obj, "value", ctrl->value);
  // // TODO: add pos fields once figured out

  // return (struct SerializedControl){
  //     .obj = obj,
  //     .buffer = binn_ptr(obj),
  //     .length = binn_size(obj),
  // };
}

struct Control *ctrl_unmarshal_control(uint8_t *buffer, uint32_t length) {
  return NULL;
  // binn *obj;
  // struct Control *ctrl = calloc(1, sizeof(*ctrl));

  // obj = binn_open(buffer);

  // ctrl->source = binn_object_uint32(obj, "source");
  // ctrl->type = binn_object_uint32(obj, "type");
  // ctrl->value = binn_object_uint32(obj, "value");

  // binn_free(obj);

  // return ctrl;
}
