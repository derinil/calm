#include "cuviddec.h"
#include "decode.h"
#include "nvcuvid.h"
#include <stdlib.h>

struct NvencDecoder {
  struct Decoder decoder;
};

struct Decoder *setup_decoder(DecompressedFrameHandler handler) {
  struct NvencDecoder *decoder = calloc(1, sizeof(*decoder));
  return &decoder->decoder;
}

int start_decoder(struct Decoder *decoder, struct CFrame *frame) { return 0; }

void decode_frame(struct Decoder *decoder, struct CFrame *frame) {}

int stop_decoder(struct Decoder *decoder) { return 0; }

void release_dframe(struct DFrame **frame_ptr) {
  struct DFrame *frame = *frame_ptr;
  free(frame->data);
  free(frame);
  *frame_ptr = NULL;
}
