#ifndef GUI_H_
#define GUI_H_

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

GLFWwindow *setup_window();
void destroy_window(GLFWwindow *window);

#endif
