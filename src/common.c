#include "common.h"
#include "cyborg/control.h"

void void_release_cframe(void *vcf) { release_cframe((struct CFrame **)&vcf); }
void void_release_dframe(void *vdf) { release_dframe((struct DFrame **)&vdf); }
void void_release_control(void *ctrl) {
  ctrl_release_control((struct Control **)&ctrl);
}
