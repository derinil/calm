#ifndef COMMON_H_
#define COMMON_H_

#include "capture/capture.h"
#include "decode/decode.h"

struct ThreadArgs {
  void *args;
  int ret;
};

// Helper functions shared between server and client
void void_release_cframe(void *vcf);
void void_release_dframe(void *vdf);

#endif