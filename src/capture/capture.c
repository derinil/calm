#include "capture.h"
#include "../crestial/crestial.h"
#include "../util/util.h"
#include "mpack-writer.h"
#include "mpack.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct UnmarshaledCFrame {
  struct CFrame frame;
  uint8_t *buffer;
};

void release_serialized_cframe(struct SerializedCFrame *serf) {
  free(serf->buffer);
  free(serf);
}

struct SerializedCFrame *serialize_cframe(struct CFrame *frame) {
  struct SerializedCFrame *ser;

  uint8_t *data;
  uint32_t size;
  struct CrestialWriter *writer = crestial_writer_init(&data, &size, 500);

  crestial_write_u64(writer, frame->nalu_h_len);
  crestial_write_u64(writer, frame->parameter_sets_count);
  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    crestial_write_u64(writer, frame->parameter_sets_lengths[i]);
    crestial_write_str(writer, frame->parameter_sets[i],
                       frame->parameter_sets_lengths[i]);
  }
  crestial_write_u64(writer, frame->nalus_count);
  for (uint64_t i = 0; i < frame->nalus_count; i++) {
    crestial_write_u64(writer, frame->nalus_lengths[i]);
    crestial_write_str(writer, frame->nalus[i], frame->nalus_lengths[i]);
  }

  crestial_writer_finalize(writer);

  ser = calloc(1, sizeof(*ser));
  ser->buffer = (uint8_t *)data;
  ser->length = size;

  return ser;
}

struct CFrame *unmarshal_cframe(uint8_t *buffer, uint64_t length) {
  struct CrestialReader *reader = crestial_reader_init(buffer, length);
  struct UnmarshaledCFrame *ucf = calloc(1, sizeof(*ucf));
  struct CFrame *frame = &ucf->frame;

  frame->nalu_h_len = crestial_read_u64(reader);

  frame->parameter_sets_count = crestial_read_u64(reader);
  frame->is_keyframe = frame->parameter_sets_count > 0;
  frame->parameter_sets_lengths = calloc(
      frame->parameter_sets_count, sizeof(*frame->parameter_sets_lengths));
  frame->parameter_sets =
      calloc(frame->parameter_sets_count, sizeof(*frame->parameter_sets));
  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    frame->parameter_sets_lengths[i] = crestial_read_u64(reader);
    frame->parameter_sets[i] =
        crestial_read_str(reader, frame->parameter_sets_lengths[i]);
  }

  frame->nalus_count = crestial_read_u64(reader);
  frame->nalus_lengths =
      calloc(frame->nalus_count, sizeof(*frame->nalus_lengths));
  frame->nalus = calloc(frame->nalus_count, sizeof(*frame->nalus));
  for (uint64_t i = 0; i < frame->nalus_count; i++) {
    frame->nalus_lengths[i] = crestial_read_u64(reader);
    frame->nalus[i] = crestial_read_str(reader, frame->nalus_lengths[i]);
  }

  return frame;
}

void release_unmarshaled_cframe(struct CFrame *frame) {
  struct UnmarshaledCFrame *uf = (struct UnmarshaledCFrame *)frame;
  free(frame->parameter_sets_lengths);
  free(frame->parameter_sets);
  free(frame->nalus_lengths);
  free(frame->nalus);
  free(uf->buffer);
  free(uf);
}
