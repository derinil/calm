#include "capture.h"
#include <stdlib.h>

// Temporary stubs

struct Capturer *setup_capturer(CompressedFrameHandler handler) {
  return NULL;
}

int start_capture(struct Capturer *capturer) {
  return 0;
}

int stop_capture(struct Capturer *capturer) {
  return 0;
}

void release_cframe(struct CFrame *frame) {
  free(frame);
}