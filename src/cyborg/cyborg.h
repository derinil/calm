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
// clicks is the number of clicks up to 3. anything less than 2 will be a normal
// click.
void cy_click_mouse(enum Button button, int clicks);
void cy_press_mouse(enum Button button);
void cy_release_mouse(enum Button button);

void cy_scroll_vertical(size_t offset);
void cy_scroll_horizontal(size_t offset);

struct Point cy_get_mouse_position();
void cy_set_mouse_position(size_t x, size_t y);

void cy_press_key(int key);
void cy_release_key(int key);

#if defined(__WIN32) || (__WIN64)

#elif defined(__APPLE__)

#include <CoreGraphics/CoreGraphics.h>

/*
This is most wonderful news: https://developer.apple.com/forums/thread/103992
"We are changing the default system permissions for using CGEventTaps and
CGEvent posting to remove this capability without explicit user permission.

I believe the intended behavior is that the first time that your application
attempts to post an event, you should see an alert stating that application
"Foo" is trying to control the computer using Accessibility feature, and
suggesting that you can grant permission to do this in System Prefs, Security &
Privacy pref pane. In that pref pane, go to the Privacy tab, choose
Accessibility from the left list, and you should see your app show up, initially
without a checkbox. If you enter an admin password and check the checkbox, your
app should be able to post events again."
(its not)
There is no prompt, no nothing, you need to go and enable the app in System
Settings > Privacy & Security > Accessibility
*/

void cy_click_mouse(enum Button button, int clicks) {
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
    mouse_type_down = kCGEventOtherMouseDown;
    mouse_type_up = kCGEventOtherMouseUp;
    mouse_button = kCGMouseButtonCenter;
    break;
  case ButtonRight:
    mouse_type_down = kCGEventRightMouseDown;
    mouse_type_up = kCGEventRightMouseUp;
    mouse_button = kCGMouseButtonRight;
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
                              clicks ? clicks : 1);
  CGEventPost(kCGHIDEventTap, event);
  CGEventSetType(event, mouse_type_up);
  CGEventPost(kCGHIDEventTap, event);

  /*
  According to this, we might want to use multiple downs and ups but it appears
  to work fine for now.
  https://stackoverflow.com/questions/1483657/performing-a-double-click-using-cgeventcreatemouseevent
  */

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
  CGMouseButton mouse_button;

  switch (button) {
  case ButtonLeft:
    mouse_type_down = kCGEventLeftMouseDown;
    mouse_button = kCGMouseButtonLeft;
    break;
  case ButtonMiddle:
    mouse_type_down = kCGEventOtherMouseDown;
    mouse_button = kCGMouseButtonCenter;
    break;
  case ButtonRight:
    mouse_type_down = kCGEventRightMouseDown;
    mouse_button = kCGMouseButtonRight;
    break;
  default:
    mouse_type_down = kCGEventOtherMouseDown;
    mouse_button = button;
    break;
  }

  event =
      CGEventCreateMouseEvent(source, mouse_type_down, position, mouse_button);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
  CFRelease(source);
}

void cy_release_mouse(enum Button button) {
  CGEventSourceRef source =
      CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
  CGEventRef event = CGEventCreate(source);
  CGPoint position = CGEventGetLocation(event);
  CFRelease(event);
  event = NULL;

  CGEventType mouse_type_up;
  CGMouseButton mouse_button;

  switch (button) {
  case ButtonLeft:
    mouse_button = kCGMouseButtonLeft;
    break;
  case ButtonMiddle:
    mouse_button = kCGMouseButtonCenter;
    break;
  case ButtonRight:
    mouse_button = kCGMouseButtonRight;
    break;
  default:
    mouse_button = button;
    break;
  }

  event =
      CGEventCreateMouseEvent(source, mouse_type_up, position, mouse_button);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
  CFRelease(source);
}

#elif defined(__WAYLAND)

#elif defined(__X11)

#endif

#endif