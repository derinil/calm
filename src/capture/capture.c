#include "capture.h"
#include "../util/util.h"
#include "binn.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void release_serialized_cframe(struct SerializedCFrame *serf) {
  binn_free(serf->obj);
  free(serf);
}

struct SerializedCFrame *serialize_cframe(struct CFrame *frame) {
  uint8_t *buf;
  uint64_t buf_len = 0;
  static const uint64_t packet_type = 1;
  uint64_t buf_off = 0;
  struct SerializedCFrame *serbuf;
  binn *obj = binn_object();
  binn *lone, *ltwo;

  binn_object_set_uint64(obj, "nalu_h_length", frame->nalu_h_len);

  binn_object_set_uint64(obj, "ps_count", frame->parameter_sets_count);
  if (frame->parameter_sets_count > 0) {
    lone = binn_list();
    ltwo = binn_list();
    for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
      printf("adding to lists\n");
      binn_list_add_uint64(lone, frame->parameter_sets_lengths[i]);
      binn_list_add_str(ltwo, (char *)frame->parameter_sets[i]);
    }
    int good = binn_object_set_list(obj, "ps_lengths", lone);
    if (!good)
      printf("failed to set list\n");
    // binn_object_set_list(obj, "ps_buffers", ltwo);
    binn_free(lone);
    binn_free(ltwo);
  }

  binn_object_set_uint64(obj, "nalus_count", frame->nalus_count);
  if (frame->nalus_count > 0) {
    lone = binn_list();
    ltwo = binn_list();
    for (uint64_t i = 0; i < frame->nalus_count; i++) {
      binn_list_add_uint64(lone, frame->nalus_lengths[i]);
      binn_list_add_str(ltwo, (char *)frame->nalus[i]);
    }
    // binn_object_set_list(obj, "nalu_lengths", lone);
    // binn_object_set_list(obj, "nalu_buffers", ltwo);
    binn_free(lone);
    binn_free(ltwo);
  }

  struct SerializedCFrame *ser = calloc(1, sizeof(*ser));
  ser->obj = obj;
  ser->buffer = binn_ptr(obj);
  ser->length = binn_size(obj);

#if 1
  char *nb = malloc(ser->length * sizeof(*nb));
  memcpy(nb, ser->buffer, ser->length);
  printf("yoo \n");
  binn *no = binn_open(nb);
  printf("yoo dude\n");
  uint64_t psc = binn_object_uint64(no, "ps_count");
  printf("got count %llu\n", psc);
  binn *nl = binn_object_list(no, "ps_lengths");
  printf("cc p list count %d\n", binn_count(nl));
#endif

  return ser;
}

struct CFrame *unmarshal_cframe(uint8_t *buffer, uint64_t length) {
  binn *obj = NULL;
  binn *list = NULL;
  binn *subobj = NULL;
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
    printf("hoho\n");
    if (list == NULL)
      printf("null list\n");
    else
      printf("not null\n");
    int count = binn_count(list);
    printf("p list count %d\n", count);

    for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
      // binn is 1-indexed for whatever reason
      subobj = binn_list_object(list, i + 1);
      frame->parameter_sets_lengths[i] =
          binn_object_uint64(subobj, "ps_length");
      frame->parameter_sets[i] =
          (uint8_t *)binn_object_str(subobj, "ps_buffer");
    }
    list = NULL;
  }

  frame->nalus_count = binn_object_uint64(obj, "nalus_count");

  if (frame->nalus_count > 0) {
    frame->nalus_lengths =
        calloc(frame->nalus_count, sizeof(*frame->nalus_lengths));
    frame->nalus = calloc(frame->nalus_count, sizeof(*frame->nalus));
    list = binn_object_list(obj, "nalus");
    int count = binn_count(list);
    printf("n list count %d\n", count);
    for (uint64_t i = 0; i < frame->nalus_count; i++) {
      // binn is 1-indexed for whatever reason
      subobj = binn_list_object(list, i + 1);
      frame->nalus_lengths[i] = binn_object_uint64(subobj, "nalu_length");
      frame->nalus[i] = (uint8_t *)binn_object_str(subobj, "nalu_buffer");
    }
  }

  binn_free(obj);

  return frame;
}
