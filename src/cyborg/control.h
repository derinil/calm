#ifndef CONTROL_H_
#define CONTROL_H_

#include "../capture/capture.h"
#include <stddef.h>

enum ControlSource {
  Mouse = 0,
  Keyboard = 1,
};

enum ControlType {
  Press = 0,
  Release = 1,
  Move = 2,
};

struct Control {
  enum ControlSource source;
  enum ControlType type;
  uint32_t value;
  uint32_t pos_x;
  uint32_t pos_y;
};

void ctrl_release_control(struct Control **ctrl);
struct SerializedBuffer *ctrl_serialize_control(struct Control *ctrl);

#endif