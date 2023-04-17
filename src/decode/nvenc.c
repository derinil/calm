#include "decode.h"
#include <stdlib.h>
#include "cuviddec.h"

struct Decoder *setup_decoder(DecompressedFrameHandler handler) { return NULL; }

int start_decoder(struct Decoder *decoder, struct CFrame *frame) { return 0; }

void decode_frame(struct Decoder *decoder, struct CFrame *frame) {}

int stop_decoder(struct Decoder *decoder) { return 0; }

void release_dframe(struct DFrame *frame) { free(frame); }
