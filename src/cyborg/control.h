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
  uint32_t pos_x;
  uint32_t pos_y;
  int32_t pos_x_delta;
  int32_t pos_y_delta;
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
