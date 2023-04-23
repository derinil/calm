#include "capture.h"
#include "../util/util.h"
#include "mpack-writer.h"
#include "mpack.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void release_serialized_cframe(struct SerializedCFrame *serf) {
  free(serf->buffer);
  free(serf);
}
struct SerializedCFrame *serialize_cframe(struct CFrame *frame) {
  uint8_t *buf;
  uint64_t buf_len = 0;
  static const uint64_t packet_type = 1;
  uint64_t buf_off = 0;
  struct SerializedCFrame *serbuf;

  char *data;
  size_t size;
  mpack_writer_t writer;
  mpack_writer_init_growable(&writer, &data, &size);

  mpack_build_map(&writer);

  mpack_write_u64(&writer, frame->nalu_h_len);
  mpack_write_u64(&writer, frame->parameter_sets_count);
  for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
    mpack_write_u64(&writer, frame->parameter_sets_lengths[i]);
    mpack_write_bin(&writer, (const char *)frame->parameter_sets[i],
                    frame->parameter_sets_lengths[i]);
  }
  mpack_write_u64(&writer, frame->nalus_count);
  for (uint64_t i = 0; i < frame->nalus_count; i++) {
    mpack_write_u64(&writer, frame->nalus_lengths[i]);
    mpack_write_bin(&writer, (const char *)frame->nalus[i],
                    frame->nalus_lengths[i]);
  }

  mpack_complete_map(&writer);
  mpack_writer_destroy(&writer);

  struct SerializedCFrame *ser = calloc(1, sizeof(*ser));
  ser->buffer = (uint8_t *)data;
  ser->length = size;

#if 1
  mpack_reader_t reader;
  mpack_reader_init_data(&reader, data, size);
  mpack_reader
  return mpack_ok == mpack_reader_destroy(&reader);
#endif

  return ser;
}

struct CFrame *unmarshal_cframe(uint8_t *buffer, uint64_t length) {
  return NULL;
}
