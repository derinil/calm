#include "mouse.h"
#include "types.h"

void click() {
  updateScreenMetrics();
  moveMouse((MMSignedPoint){.x = 100, .y = 100});
}