#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include "cimgui_impl.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"

#include "window.h"

int setup_window(GLFWwindow **target)
{
  if (!glfwInit())
    return 1;

  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

  GLFWwindow *window = glfwCreateWindow(640, 480, "Calm", NULL, NULL);
  if (!window)
    return 2;

  *target = window;

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return 3;

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);

  // enable vsync
  glfwSwapInterval(1);

  igCreateContext(NULL);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  // TODO: is this too low?
  ImGui_ImplOpenGL3_Init("#version 330");
  igStyleColorsLight(NULL);
  // ImFontAtlas_AddFontDefault(io.Fonts, NULL);

  return 0;
}

void destroy_window(GLFWwindow *window)
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(NULL);
  glfwDestroyWindow(window);
  glfwTerminate();
}
