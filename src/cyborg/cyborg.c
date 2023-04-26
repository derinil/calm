#include "control.h"
#include "keycode.h"
#include "keypress.h"
#include "math.h"
#include "mouse.h"
#include "types.h"

void inject_control(struct Control *ctrl) {
  switch (ctrl->source) {
  case Mouse:
    updateScreenMetrics();
    if (ctrl->type == Move) {
#if 0
      MMSignedPoint sp = MMSignedPointMake(ctrl->pos_x, ctrl->pos_y);
      moveMouse(sp);
#else
      MMPoint point = getMousePos();
      // TODO: at some point use doubles to set position
      int32_t x = point.x + rint(ctrl->pos_x_delta);
      int32_t y = point.y + rint(ctrl->pos_y_delta);
      MMSignedPoint sp = MMSignedPointMake(x, y);
      moveMouse(sp);
#endif
    } else {
      toggleMouse(ctrl->type == 1 ? 1 : 0, ctrl->value);
    }
    break;
  case Keyboard:
    toggleKeyCode(ctrl->value, ctrl->type == 1 ? 1 : 0, 0);
    break;
  default:
    break;
  }
}
