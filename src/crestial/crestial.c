#include "crestial.h"
#include <stdlib.h>
#include <string.h>

struct CrestialWriter *crestial_writer_init(uint8_t **dest, uint32_t *size,
                                            uint32_t estimate_size) {
  if (!dest || !size)
    return NULL;
  struct CrestialWriter *writer = calloc(1, sizeof(*writer));
  writer->dest = dest;
  if (estimate_size) {
    *writer->dest = calloc(estimate_size, sizeof(**writer->dest));
    writer->alloced = estimate_size;
    writer->free = writer->alloced;
  }
  writer->size = size;
  writer->curr_write = *writer->dest;
  return writer;
}

void crestial_expand_to_fit(struct CrestialWriter *w, uint32_t u) {
  // we could allocate more here to save a bit more realloc calls
  // but im gonna leave it as it is for now
  uint32_t to_alloc = 0;
  if (w->free < u) {
    to_alloc = u - w->free;
  }
  *w->dest = realloc(*w->dest, w->alloced + to_alloc);
  w->alloced += to_alloc;
  w->free += to_alloc;
  w->curr_write = *w->dest + w->wrote;
}

static inline void crestial_wrote(struct CrestialWriter *w, uint32_t u) {
  w->wrote += u;
  w->free -= u;
  w->curr_write += u;
}

void crestial_write_u32(struct CrestialWriter *w, uint32_t u) {
  static const int len = 4;
  if (w->free < len)
    crestial_expand_to_fit(w, len);
  memcpy(w->curr_write, (char *)&u, len);
  crestial_wrote(w, len);
}

void crestial_write_u64(struct CrestialWriter *w, uint64_t u) {
  static const int len = 8;
  if (w->free < len)
    crestial_expand_to_fit(w, len);
  memcpy(w->curr_write, (char *)&u, len);
  crestial_wrote(w, len);
}

// crestial_write_str memcpy's the char array into the buffer
void crestial_write_str(struct CrestialWriter *w, uint8_t *s, uint32_t length) {
  if (w->free < length)
    crestial_expand_to_fit(w, length);
  memcpy(w->curr_write, s, length);
  crestial_wrote(w, length);
}

// crestial_finalize sets the size variable that was passed in init and frees
// the writer
void crestial_writer_finalize(struct CrestialWriter *w) {
  *w->size = w->wrote;
  if (w->alloced > w->wrote) {
    *w->dest = realloc(*w->dest, w->wrote);
  }
  free(w);
}

struct CrestialReader *crestial_reader_init(uint8_t *src, uint32_t size) {
  if (!src || !size)
    return NULL;
  struct CrestialReader *reader = calloc(1, sizeof(*reader));
  reader->src = src;
  reader->size = size;
  return reader;
}

static inline void crestial_read(struct CrestialReader *r, uint32_t u) {
  r->src += u;
  r->read += u;
}

uint32_t crestial_reader_u32(struct CrestialReader *r) {
  static const int len = 4;
  uint32_t u;
  memcpy((uint8_t *)&u, r->src, len);
  crestial_read(r, len);
  return u;
}

uint64_t crestial_read_u64(struct CrestialReader *r) {
  static const int len = 8;
  uint64_t u = 0;
  memcpy(&u, r->src, len);
  crestial_read(r, len);
  return u;
}

void crestial_read_str(struct CrestialReader *r, uint8_t *dest, uint32_t length) {
  memcpy(dest, r->src, length);
  crestial_read(r, length);
}

void crestial_reader_finalize(struct CrestialReader *r) { free(r); }
