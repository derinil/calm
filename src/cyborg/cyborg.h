#ifndef CYBORG_H_
#define CYBORG_H_

/*
https://stackoverflow.com/questions/7492529/how-to-simulate-a-mouse-movement
https://stackoverflow.com/questions/62921040/windows-c-how-to-simulate-mouse-movement-in-fps-games
https://github.com/Zpes/mouse-input-injection
https://doxygen.reactos.org/d0/d55/win32ss_2user_2ntuser_2input_8c_source.html
https://github.com/Cyborg/robot/
https://developer.apple.com/forums/thread/685618
https://stackoverflow.com/questions/51406276/simulating-mouse-and-keyboard-input-on-wayland-and-x11
https://stackoverflow.com/questions/1483657/performing-a-double-click-using-cgeventcreatemouseevent
*/

#include <CoreFoundation/CoreFoundation.h>
#include <stddef.h>

struct CyborgPoint {
  size_t x;
  size_t y;
};

enum CyborgMouseState {
  Pressed,
};

struct CyborgMouse {
  struct CyborgPoint position;
  enum CyborgMouseState state;
  size_t double_click_interval;
  size_t vertical_scroll_offset;
  size_t horizontal_scroll_offset;
};

// If you need to click extra buttons, simply pass a int thats > 2
enum Button {
  ButtonLeft = 0,
  ButtonMiddle = 1,
  ButtonRight = 2,
};

// cy_click_mouse simulates a mouse click at the current mouse position.
// set double_click > 0 to make it a double click.
void cy_click_mouse(enum Button button, int double_click);
void cy_press_mouse(int button);
void cy_release_mouse(int button);

void cy_scroll_vertical(size_t offset);
void cy_scroll_horizontal(size_t offset);

struct Point cy_get_mouse_position();
void cy_set_mouse_position(size_t x, size_t y);

#if defined(__WIN32) || (__WIN64)

#elif defined(__APPLE__)

#include <CoreGraphics/CoreGraphics.h>

void cy_click_mouse(enum Button button, int double_click) {
  CGEventSourceRef source =
      CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
  CGEventRef event = CGEventCreate(source);
  CGPoint position = CGEventGetLocation(event);
  CFRelease(event);
  event = NULL;

  CGEventType mouse_type_down;
  CGEventType mouse_type_up;
  CGMouseButton mouse_button;

  switch (button) {
  case ButtonLeft:
    mouse_type_down = kCGEventLeftMouseDown;
    mouse_type_up = kCGEventLeftMouseUp;
    mouse_button = kCGMouseButtonLeft;
    break;
  case ButtonMiddle:
    mouse_type_down = kCGEventLeftMouseDown;
    mouse_type_up = kCGEventLeftMouseUp;
    mouse_button = kCGMouseButtonLeft;
    break;
  case ButtonRight:
    mouse_type_down = kCGEventLeftMouseDown;
    mouse_type_up = kCGEventLeftMouseUp;
    mouse_button = kCGMouseButtonLeft;
    break;
  default:
    mouse_type_down = kCGEventOtherMouseDown;
    mouse_type_up = kCGEventOtherMouseUp;
    // Additional buttons are specified in USB order using the integers 3 to 31.
    mouse_button = button;
    break;
  }

  event =
      CGEventCreateMouseEvent(source, mouse_type_down, position, mouse_button);
  CGEventSetIntegerValueField(event, kCGMouseEventClickState,
                              double_click ? 2 : 1);

  CGEventPost(kCGHIDEventTap, event);


  CFRelease(event);
  CFRelease(source);
}

void cy_press_mouse(enum Button button) {
  CGEventSourceRef source =
      CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
  CGEventRef event = CGEventCreate(source);
  CGPoint position = CGEventGetLocation(event);
  CFRelease(event);
  event = NULL;

  CGEventType mouse_type_down;
  CGEventType mouse_type_up;
  CGMouseButton mouse_button;

  switch (button) {
  case ButtonLeft:
    mouse_type_down = kCGEventLeftMouseDown;
    mouse_type_up = kCGEventLeftMouseUp;
    mouse_button = kCGMouseButtonLeft;
    break;
  case ButtonMiddle:
    mouse_type_down = kCGEventLeftMouseDown;
    mouse_type_up = kCGEventLeftMouseUp;
    mouse_button = kCGMouseButtonLeft;
    break;
  case ButtonRight:
    mouse_type_down = kCGEventLeftMouseDown;
    mouse_type_up = kCGEventLeftMouseUp;
    mouse_button = kCGMouseButtonLeft;
    break;
  default:
    mouse_type_down = kCGEventOtherMouseDown;
    mouse_type_up = kCGEventOtherMouseUp;
    // Additional buttons are specified in USB order using the integers 3 to 31.
    mouse_button = button;
    break;
  }

  event =
      CGEventCreateMouseEvent(source, mouse_type_down, position, mouse_button);
  CGEventSetIntegerValueField(event, kCGMouseEventClickState,
                              double_click ? 2 : 1);

  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
  CFRelease(source);
}

#elif defined(__WAYLAND)

#elif defined(__X11)

#endif

#endif