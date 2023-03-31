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
    GLenum err = 0;
    GLuint texture;
    int width, height;
    size_t img_width, img_height;
    struct DFrame *dframe = NULL, *new_dframe = NULL;
    static ImVec4 clearColor;
    clearColor.x = 0.45f;
    clearColor.y = 0.55f;
    clearColor.z = 0.60f;
    clearColor.w = 1.00f;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    err = glGetError();
    if (err)
        printf("params err %d\n", err);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        err = 0;

        new_dframe = (struct DFrame *)dstack_pop(stack);
        if (new_dframe)
        {
            if (dframe)
                release_dframe(dframe);
            dframe = new_dframe;
            err = glGetError();
            if (err)
                printf("gl 1 err %d\n", err);
            glBindTexture(GL_TEXTURE_2D, texture);
            err = glGetError();
            if (err)
                printf("gl 2 err %d\n", err);
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
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dframe->width, dframe->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, dframe->data);
            err = glGetError();
            if (err)
                printf("gl 3 err %d\n", err);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();

        width = dframe->width;
        height = dframe->height;

        {
            igBegin("video", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
            igSetWindowPos_Vec2((ImVec2){}, ImGuiCond_Always);
            ImGuiViewport *viewport = igGetMainViewport();
            ImDrawList *drawlist = igGetBackgroundDrawList_ViewportPtr(viewport);
            igSetWindowSize_Vec2(viewport->Size, ImGuiCond_Always);
            igImage((void *)(intptr_t)texture, (ImVec2){width, height}, (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){0, 0, 0, 0});
            igEnd();
        }

        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        igRender();
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

        err = glGetError();
        if (err)
            printf("bind err %d\n", err);

        err = glGetError();
        if (err)
            printf("end err %d\n", err);

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
