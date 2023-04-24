#ifndef CRESTIAL_H_
#define CRESTIAL_H_

#include <stdint.h>

struct CrestialWriter {
  uint8_t **dest;
  uint8_t *curr_write;
  uint32_t *size;
  uint32_t alloced;
  uint32_t wrote;
  uint32_t free;
};

// crestial_init initializes a buffer with the estimated size.
// dest and size cannot be null while estimate can be zero.
struct CrestialWriter *crestial_writer_init(uint8_t **dest, uint32_t *size,
                                            uint32_t estimate_size);
// crestial_write_u64 writes the 4 bytes that make up the u64
void crestial_write_u32(struct CrestialWriter *w, uint32_t u);
// crestial_write_u64 writes the 8 bytes that make up the u64
void crestial_write_u64(struct CrestialWriter *w, uint64_t u);
// crestial_write_str memcpy's the char array into the buffer
void crestial_write_str(struct CrestialWriter *w, uint8_t *s, uint32_t length);
// crestial_finalize sets the size variable that was passed in init and frees
// the writer
// the final buffer will not be null terminated
void crestial_writer_finalize(struct CrestialWriter *w);

struct CrestialReader {
  uint8_t *src;
  uint32_t size;
  uint32_t read;
};

// crestial_reader_init initializes a reader.
// reader will not make any length checks so there might easily be a buffer
// overflow if the user tries to read too much.
struct CrestialReader *crestial_reader_init(uint8_t *src, uint32_t size);
// crestial_reader_u32 reads 4 bytes that make up a u32
uint32_t crestial_read_u32(struct CrestialReader *r);
// crestial_read_u64 reads 8 bytes that make up a u64
uint64_t crestial_read_u64(struct CrestialReader *r);
// crestial_read_str returns the pointer from the strings start
// user should store a length right before the string.
uint8_t *crestial_read_str(struct CrestialReader *r, uint32_t length);
// crestial_reader_finalize frees the reader
void crestial_reader_finalize(struct CrestialReader *r);

#endif
