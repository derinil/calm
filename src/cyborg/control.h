#ifndef CONTROL_H_
#define CONTROL_H_

#include <stddef.h>
#include <stdint.h>

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
  double pos_x;
  double pos_y;
  double pos_x_delta;
  double pos_y_delta;
};

struct SerializedControl {
  struct Control *ctrl;
  uint8_t *buffer;
  size_t length;
};

void ctrl_release_control(struct Control **ctrl);
struct SerializedControl *ctrl_serialize_control(struct Control *ctrl);
struct Control *ctrl_unmarshal_control(uint8_t *buffer, uint32_t length);
void ctrl_release_serializedcontrol(struct SerializedControl *ctrl);

void inject_control(struct Control *ctrl);

#endif
