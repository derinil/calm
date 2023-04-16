#include "common.h"

void void_cframe_releaser(void *vcf) { release_cframe((struct CFrame *)vcf); }
void void_dframe_releaser(void *vdf) { release_dframe((struct DFrame *)vdf); }
