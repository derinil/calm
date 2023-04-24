#include "common.h"
#include "capture/capture.h"
#include "cyborg/control.h"

void void_release_cframe(void *vcf) {
  struct CFrame *frame = vcf;
  release_cframe(&frame);
}

void void_release_dframe(void *vdf) { release_dframe((struct DFrame **)&vdf); }

void void_release_control(void *ctrl) {
  ctrl_release_control((struct Control **)&ctrl);
}
