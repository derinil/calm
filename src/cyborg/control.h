#ifndef CONTROL_H_
#define CONTROL_H_

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
  int value;
  int pos_x;
  int pos_y;
};

#endif