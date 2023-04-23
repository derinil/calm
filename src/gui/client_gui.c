#include "client_gui.h"
#include "../capture/capture.h"
#include "../cyborg/control.h"
#include "../data/stack.h"
#include "window.h"
#include <math.h>
#include <stdlib.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include "cimgui_impl.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "video.h"

static void draw(GLFWwindow *window, struct DStack *stack) {
  static ImVec4 clearColor;
  clearColor.x = 0.45f;
  clearColor.y = 0.55f;
  clearColor.z = 0.60f;
  clearColor.w = 1.00f;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    draw_video_frame(stack);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();
    {
      igBegin("Main", NULL,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoResize);
      igSetWindowPos_Vec2((ImVec2){}, ImGuiCond_Always);
      ImGuiViewport *viewport = igGetMainViewport();
      ImVec2 size = viewport->Size;
      size.x /= 4;
      igSetWindowSize_Vec2(size, ImGuiCond_Always);

      static float f = 0.0f;
      static int counter = 0;

      igText("This is some useful text");

      igSliderFloat("Float", &f, 0.0f, 1.0f, "%.3f", 0);
      igColorEdit3("clear color", (float *)&clearColor, 0);

      ImVec2 buttonSize;
      buttonSize.x = 0;
      buttonSize.y = 0;
      if (igButton("Button", buttonSize))
        counter++;
      igSameLine(0.0f, -1.0f);
      igText("counter = %d", counter);

      igText("Application average %.3f ms/frame (%.1f FPS)",
             1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
      igEnd();
    }
    igRender();
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
    glfwSwapBuffers(window);
  }
}

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
  glViewport(0, 0, width, height);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  struct DStack *ctrl_stack = glfwGetWindowUserPointer(window);
  struct Control *ctrl = calloc(1, sizeof(*ctrl));
  ctrl->source = Keyboard;
  ctrl->type = action == GLFW_PRESS ? Press : Release;
  ctrl->value = key;
  dstack_push(ctrl_stack, ctrl, 1);
  static const int modifiers[] = {
      GLFW_MOD_SHIFT, GLFW_MOD_CONTROL,   GLFW_MOD_ALT,
      GLFW_MOD_SUPER, GLFW_MOD_CAPS_LOCK, GLFW_MOD_NUM_LOCK,
  };
  // Hopefully no one is using these ints for keys
  static const int modifiers_keys[] = {
      1024, 1025, 1026, 1027, 1028, 1029,
  };
  // Send these every frame as there is no easy way of detecting their release
  // TODO: find an easy way, this is very inefficient
  for (int i = 0; i < 6; i++) {
    ctrl = calloc(1, sizeof(*ctrl));
    ctrl->source = Keyboard;
    ctrl->type = mods & modifiers[i] ? Press : Release;
    ctrl->value = modifiers_keys[i];
    dstack_push(ctrl_stack, ctrl, 1);
  }
}

static void cursor_position_callback(GLFWwindow *window, double xpos_d,
                                     double ypos_d) {
  printf("%f %f\n", xpos_d, ypos_d);
  // TODO: temporarily disabled until i figure out mouse pos
  // xpos - last_xpos can be negative so we need signed 32 bit int
  /*uint32_t xpos = (uint32_t)rint(xpos_d);
  uint32_t ypos = (uint32_t)rint(ypos_d);
  static uint32_t last_xpos;
  static uint32_t last_ypos;
  struct DStack *ctrl_stack = glfwGetWindowUserPointer(window);
  struct Control *ctrl = malloc(sizeof(*ctrl));
  ctrl->source = Mouse;
  ctrl->type = Move;
  ctrl->pos_x = xpos;
  ctrl->pos_y = ypos;
  ctrl->pos_x_delta = xpos - last_xpos;
  ctrl->pos_y_delta = ypos - last_ypos;
  dstack_push(ctrl_stack, ctrl, 1);
  last_xpos = xpos;
  last_ypos = ypos;*/
}

static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                  int mods) {
  struct DStack *ctrl_stack = glfwGetWindowUserPointer(window);
  struct Control *ctrl = calloc(1, sizeof(*ctrl));
  ctrl->source = Mouse;
  // Press release :DDDD
  ctrl->type = action == GLFW_PRESS ? Press : Release;
  // TODO: this has modifier keys as well, we need a cleaner of getting
  // modifiers
  ctrl->value = button;
  dstack_push(ctrl_stack, ctrl, 1);
}

int handle_client_gui(struct DStack *frame_stack, struct DStack *ctrl_stack) {
  GLFWwindow *window;
  window = setup_window();
  if (!window)
    return 1;
  glfwSetWindowUserPointer(window, ctrl_stack);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  glfwSetKeyCallback(window, key_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  setup_video_renderer();
  draw(window, frame_stack);
  destroy_window(window);
  destroy_video_renderer();
  return 0;
}
