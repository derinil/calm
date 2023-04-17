#include "../capture/capture.h"
#include "../data/stack.h"
#include "video.h"
#include "window.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include "cimgui_impl.h"
#define GLFW_INCLUDE_NONE
#include "../decode/decode.h"
#include "GLFW/glfw3.h"
#include "glad/glad.h"

static void draw(GLFWwindow *window, struct DStack *stack) {
  GLenum err = 0;
  int width = 0, height = 0;
  int first_run = 1;
  struct DFrame *dframe = NULL;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    draw_video_frame(stack);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();
    {
      igBegin("Main", NULL,
              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
      ImGuiViewport *viewport = igGetMainViewport();

      ImVec2 size = viewport->Size;
      size.x /= 4;
      igSetWindowSize_Vec2(size, ImGuiCond_Always);

      static float f = 0.0f;
      static int counter = 0;

      igText("This is some useful text");

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

int handle_server_gui(struct DStack *stack) {
#if 0
    return 0;
#endif
  GLFWwindow *window;
  window = setup_window();
  if (!window)
    return 1;
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  setup_video_renderer();
  draw(window, stack);
  destroy_window(window);
  destroy_video_renderer();
  return 0;
}
