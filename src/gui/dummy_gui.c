#include "dummy_gui.h"
#include "../data/stack.h"
#include "../decode/decode.h"

void run_dummy_gui(struct DStack *stack) {
  while (1) {
    void *f = dstack_pop_block(stack);
    if (!f)
      continue;
    release_dframe((struct DFrame *)f);
  }
}