#include "window.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include "cimgui_impl.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"

void draw(GLFWwindow *window)
{
    static ImVec4 clearColor;
    clearColor.x = 0.45f;
    clearColor.y = 0.55f;
    clearColor.z = 0.60f;
    clearColor.w = 1.00f;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

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

        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        igRender();
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        glfwSwapBuffers(window);
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int handle_client_gui()
{
    GLFWwindow *window;

    int err = setup_window(&window);
    if (err)
        return err;

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // NOTE: can use glfwWaitEvents(); to update gui only on input event
    draw(window);
    destroy_window(window);

    return 0;
}
