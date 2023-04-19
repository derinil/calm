#ifndef ROBOT_H_
#define ROBOT_H_

/*
Windows and X11
https://stackoverflow.com/questions/7492529/how-to-simulate-a-mouse-movement
https://stackoverflow.com/questions/62921040/windows-c-how-to-simulate-mouse-movement-in-fps-games
https://github.com/Zpes/mouse-input-injection
https://doxygen.reactos.org/d0/d55/win32ss_2user_2ntuser_2input_8c_source.html
https://github.com/Robot/robot/
https://developer.apple.com/forums/thread/685618
*/

#include <stddef.h>

struct RobotPoint {
  size_t x;
  size_t y;
};

enum RobotMouseState {
  Pressed,
};

struct RobotMouse {
  struct RobotPoint position;
  enum RobotMouseState state;
  size_t double_click_interval;
  size_t vertical_scroll_offset;
  size_t horizontal_scroll_offset;
};

enum Button {
  ButtonLeft = 0,
  ButtonMid = 1,
  ButtonMiddle = 1,
  ButtonRight = 2,
  ButtonX1 = 3,
  ButtonX2 = 4,
};

void robot_click_mouse(int button);
void robot_doubleclick_mouse(int button);
void robot_press_mouse(int button);
void robot_release_mouse(int button);

void robot_scroll_vertical(size_t offset);
void robot_scroll_horizontal(size_t offset);

struct Point robot_get_mouse_position();
void robot_set_mouse_position(size_t x, size_t y);

#if defined(__WIN32) || (__WIN64)

#elif defined(__APPLE__)

// #include <ApplicationServices/ApplicationServices.h>
#include <CoreGraphics/CoreGraphics.h>

void robot_click_mouse(int button) {

  // Ignore extra buttons
  if (button == ButtonX1 || button == ButtonX2)
    return;

  // Create an HID hardware event source
  CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

  // Retrieve the current mouse position
  CGEventRef pos = CGEventCreate(src);
  CGPoint p = CGEventGetLocation(pos);
  CFRelease(pos);

  // Create a press event
  CGEventRef evt = nullptr;
  switch (button) {
  case ButtonLeft:
    evt = CGEventCreateMouseEvent(src, kCGEventLeftMouseDown, p,
                                  kCGMouseButtonLeft);
    break;
  case ButtonMid:
    evt = CGEventCreateMouseEvent(src, kCGEventOtherMouseDown, p,
                                  kCGMouseButtonCenter);
    break;
  case ButtonRight:
    evt = CGEventCreateMouseEvent(src, kCGEventRightMouseDown, p,
                                  kCGMouseButtonRight);
    break;
  default:
    break;
  }

  // Detect double click version
  if (!elapsed.HasExpired(500)) {
    // TODO: Use system double click
    // interval instead. See [NSEvent
    // doubleClickInterval] function.

    CGEventSetIntegerValueField(evt, kCGMouseEventClickState, 2);
  } else {
    CGEventSetIntegerValueField(evt, kCGMouseEventClickState, 1);
  }

  CGEventPost(kCGHIDEventTap, evt);
  CFRelease(evt);
  CFRelease(src);
}

#elif defined(__WAYLAND)

#elif defined(__X11)

#endif

#endif