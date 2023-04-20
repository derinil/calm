#ifndef CONTROL_H_
#define CONTROL_H_

#include <stddef.h>

enum ControlType {
  Mouse = 0,
  Keyboard = 1,
};

struct Control {
  enum ControlType type;
  int value;
};

#endif