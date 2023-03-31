#include "window.h"
#include "../data/stack.h"
#include "../capture/capture.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include "cimgui_impl.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "../decode/decode.h"

static void draw(GLFWwindow *window, struct DStack *stack)
{
    GLuint texture;
    int width = 0, height = 0;
    struct DFrame *dframe = NULL, *new_dframe = NULL;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        new_dframe = (struct DFrame *)dstack_pop(stack);
        if (new_dframe)
        {
            if (dframe)
                release_dframe(dframe);
            dframe = new_dframe;
            width = dframe->width;
            height = dframe->height;
#if 0
            FILE *f = fopen("test.ppm", "w+");
            fprintf(f, "P3\n%lu %lu\n255\n", dframe->width, dframe->height);
            for (size_t x = 0; x < dframe->data_length; x += 4)
            {
                fprintf(f, "%u %u %u\n", dframe->data[x + 2], dframe->data[x + 1], dframe->data[x]);
            }
            fflush(f);
            fclose(f);
            exit(0);
#endif
            glBindTexture(GL_TEXTURE_2D, texture);
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, dframe->data);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();

        {
            igBegin("video", NULL, ImGuiWindowFlags_NoSavedSettings);
            igSetWindowPos_Vec2((ImVec2){0}, ImGuiCond_Always);
            ImGuiViewport *viewport = igGetMainViewport();
            igSetWindowSize_Vec2(viewport->Size, ImGuiCond_Always);
            ImVec2 max;
            igGetWindowContentRegionMax(&max);
            igImage((void *)(intptr_t)texture, max, (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){0, 0, 0, 0});
            igEnd();
        }

        igRender();
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        glfwSwapBuffers(window);
    }
}

static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int handle_server_gui(struct DStack *stack)
{
#if 0
    return 0;
#endif
    GLFWwindow *window;
    window = setup_window();
    if (!window)
        return 1;
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // NOTE: can use glfwWaitEvents(); to update gui only on input event
    draw(window, stack);
    destroy_window(window);
    return 0;
}
