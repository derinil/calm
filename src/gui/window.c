#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include "cimgui_impl.h"

#include "window.h"

void glfw_error_callback(int error, const char *description)
{
  // TODO: better
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int setup_window(GLFWwindow **target)
{
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

#ifdef __APPLE__
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

  GLFWwindow *window = glfwCreateWindow(640, 480, "Calm", NULL, NULL);
  if (!window)
    return 2;

  *target = window;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  igCreateContext(NULL);
  ImGuiIO *io = igGetIO();
  io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  igStyleColorsLight(NULL);

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return 3;

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);

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
