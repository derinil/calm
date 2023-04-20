#ifndef COMMON_H_
#define COMMON_H_

#include "capture/capture.h"
#include "decode/decode.h"

// Helpers shared between server and client

struct ThreadArgs {
  void *args;
  int ret;
};

void void_release_cframe(void *vcf);
void void_release_dframe(void *vdf);
void void_release_control(void *ctrl);

#endif