#include "control.h"
#include "keycode.h"
#include "keypress.h"
#include "mouse.h"
#include "types.h"

void inject_control(struct Control *ctrl) {
  switch (ctrl->source) {
  case Mouse:
    updateScreenMetrics();
    if (ctrl->type == Move) {
#if 1
      MMSignedPoint sp = MMSignedPointMake(ctrl->pos_x, ctrl->pos_y);
      moveMouse(sp);
#else
      MMPoint point = getMousePos();
      point.x += ctrl->pos_x_delta;
      point.y += ctrl->pos_y_delta;
      MMSignedPoint sp = MMSignedPointMake(point.x, point.y);
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
