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

#ifdef IMGUI_HAS_IMSTR
#define igBegin igBegin_Str
#define igSliderFloat igSliderFloat_Str
#define igCheckbox igCheckbox_Str
#define igColorEdit3 igColorEdit3_Str
#define igButton igButton_Str
#endif

#include "gui.h"

GLFWwindow *window;

void draw()
{
  glfwPollEvents();

  static ImVec4 clearColor;
  clearColor.x = 0.45f;
  clearColor.y = 0.55f;
  clearColor.z = 0.60f;
  clearColor.w = 1.00f;

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  igNewFrame();

  {
    igBegin("Main", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
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

  glfwMakeContextCurrent(window);
  glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
  glClear(GL_COLOR_BUFFER_BIT);
  igRender();
  ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
  glfwSwapBuffers(window);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  glViewport(0, 0, width, height);
  draw();
}

int start_gui()
{
  if (!glfwInit())
    return 1;

  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
  window = glfwCreateWindow(640, 480, "Calm", NULL, NULL);
  if (!window)
    return 2;

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return 3;

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // enable vsync
  glfwSwapInterval(1);

  igCreateContext(NULL);

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  // TODO: is this too low?
  ImGui_ImplOpenGL3_Init("#version 120");

  igStyleColorsLight(NULL);
  // ImFontAtlas_AddFontDefault(io.Fonts, NULL);

  while (!glfwWindowShouldClose(window))
    draw();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(NULL);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
