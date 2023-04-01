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

GLFWwindow *setup_window()
{
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return NULL;

#ifdef __APPLE__
  const char *glsl_version = "#version 330";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  const char *glsl_version = "#version 330";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

  GLFWwindow *window = glfwCreateWindow(640, 480, "Calm", NULL, NULL);
  if (!window)
    return NULL;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  igCreateContext(NULL);
  ImGuiIO *io = igGetIO();
  io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  igStyleColorsLight(NULL);

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return NULL;

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);

  // glMatrixMode(GL_PROJECTION); // switch to projection matrix
  // glLoadIdentity();            // reset projection matrix
  // glOrtho(0, width, 0, height, 0, 1);
  // // gluOrtho2D(0, width, 0, height);  // set 2D orthographic projection to match viewport dimensions
  // glMatrixMode(GL_MODELVIEW);       // switch back to modelview matrix
  // glLoadIdentity();                 // reset modelview matrix
  // glClearColor(1.0, 1.0, 1.0, 0.0); // set clear color to white

  return window;
}

void destroy_window(GLFWwindow *window)
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(NULL);
  glfwDestroyWindow(window);
  glfwTerminate();
}
