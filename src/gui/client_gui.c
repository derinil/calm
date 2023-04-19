#include "../capture/capture.h"
#include "../data/stack.h"
#include "window.h"
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

int handle_client_gui(struct DStack *stack) {
  GLFWwindow *window;
  window = setup_window();
  if (!window)
    return 1;
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  // Hide the mouse on the client side
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  setup_video_renderer();
  draw(window, stack);
  destroy_window(window);
  destroy_video_renderer();
  return 0;
}
