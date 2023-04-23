#include "capture.h"
#include "../util/util.h"
#include "binn.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void release_serialized_cframe(struct SerializedCFrame *serf) {
  binn_free(serf->obj);
}

struct SerializedCFrame serialize_cframe(struct CFrame *frame) {
  uint8_t *buf;
  uint64_t buf_len = 0;
  static const uint64_t packet_type = 1;
  uint64_t buf_off = 0;
  struct SerializedCFrame *serbuf;
  binn *obj = binn_object();
  binn *list;
  binn *subobj;

  binn_object_set_uint64(obj, "nalu_h_length", frame->nalu_h_len);

  binn_object_set_uint64(obj, "ps_count", frame->parameter_sets_count);
  if (frame->parameter_sets_count > 0) {
    list = binn_list();
    for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
      subobj = binn_object();
      binn_object_set_uint64(subobj, "ps_length",
                             frame->parameter_sets_lengths[i]);
      binn_object_set_str(subobj, "ps_buffer",
                          (char *)frame->parameter_sets[i]);
      binn_list_add_object(list, subobj);
    }
    binn_object_set_list(obj, "pss", list);
  }

  binn_object_set_uint64(obj, "nalus_count", frame->nalus_count);
  if (frame->nalus_count > 0) {
    list = binn_list();
    for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
      subobj = binn_object();
      binn_object_set_uint64(subobj, "nalu_length", frame->nalus_lengths[i]);
      binn_object_set_str(subobj, "nalu_buffer", (char *)frame->nalus[i]);
      binn_list_add_object(list, subobj);
    }
    binn_object_set_list(obj, "nalus", list);
  }

  return (struct SerializedCFrame){
      .obj = obj,
      .buffer = binn_ptr(obj),
      .length = binn_size(obj),
  };
}

struct CFrame *unmarshal_cframe(uint8_t *buffer, uint64_t length) {
  binn *obj;
  binn *list;
  binn *subobj;
  struct CFrame *frame = calloc(1, sizeof(*frame));

  obj = binn_open(buffer);

  frame->nalu_h_len = binn_object_uint64(obj, "nalu_h_length");

  frame->parameter_sets_count = binn_object_uint64(obj, "ps_count");

  if (frame->parameter_sets_count > 0) {
    frame->is_keyframe = 1;
    frame->parameter_sets_lengths = calloc(
        frame->parameter_sets_count, sizeof(*frame->parameter_sets_lengths));
    frame->parameter_sets =
        calloc(frame->parameter_sets_count, sizeof(*frame->parameter_sets));
    list = binn_object_list(obj, "pss");
    for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
      subobj = binn_list_object(list, i);
      frame->parameter_sets_lengths[i] =
          binn_object_uint64(subobj, "ps_length");
      frame->parameter_sets[i] =
          (uint8_t *)binn_object_str(subobj, "ps_buffer");
    }
  }

  frame->nalus_count = binn_object_uint64(obj, "nalus_count");

  if (frame->nalus_count > 0) {
    frame->nalus_lengths =
        calloc(frame->nalus_count, sizeof(*frame->nalus_lengths));
    frame->nalus = calloc(frame->nalus_count, sizeof(*frame->nalus));
    list = binn_object_list(obj, "nalus");
    for (uint64_t i = 0; i < frame->nalus_count; i++) {
      subobj = binn_list_object(list, i);
      frame->nalus_lengths[i] = binn_object_uint64(subobj, "nalu_length");
      frame->nalus[i] = (uint8_t *)binn_object_str(subobj, "nalu_buffer");
    }
  }

  binn_free(obj);

  return frame;
}
