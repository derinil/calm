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

void drawImage(GLuint file,
    float x,
    float y,
    float w,
    float h,
    float angle)
{
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glPushMatrix();
    glTranslatef(x, y, 0.0);
    glRotatef(angle, 0.0, 0.0, 1.0);

    glBindTexture(GL_TEXTURE_2D, file);
    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(x, y, 0.0f);
    glTexCoord2f(0.0, 2.0); glVertex3f(x, y + h, 0.0f);
    glTexCoord2f(2.0, 2.0); glVertex3f(x + w, y + h, 0.0f);
    glTexCoord2f(2.0, 0.0); glVertex3f(x + w, y, 0.0f);
    glEnd();

    glPopMatrix();
}

GLuint texture;
void render()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const double w = glutGet( GLUT_WINDOW_WIDTH );
    const double h = glutGet( GLUT_WINDOW_HEIGHT );
    gluPerspective(45.0, w / h, 0.1, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef( 0, 0, -15 );

    drawImage(texture, 0.0f, 0.0f, 4.0f, 4.0f, 0.0);

    glutSwapBuffers();
}


static void draw(GLFWwindow *window, struct DStack *stack)
{
    GLuint framebuffer;
    GLuint texture;
    int new_frame = 0;
    size_t img_width, img_height;
    struct DFrame *dframe;
    static ImVec4 clearColor;
    clearColor.x = 0.45f;
    clearColor.y = 0.55f;
    clearColor.z = 0.60f;
    clearColor.w = 1.00f;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        dframe = (struct DFrame *)dstack_pop(stack);
        if (dframe)
        {
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, 4, dframe->width, dframe->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, dframe->data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            new_frame = 1;
            img_width = dframe->width;
            img_height = dframe->height;
            release_dframe(dframe);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();

        {
            igBegin("Main", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
            igSetWindowPos_Vec2((ImVec2){}, ImGuiCond_Always);
            ImGuiViewport *viewport = igGetMainViewport();
            ImDrawList *drawlist = igGetBackgroundDrawList_ViewportPtr(viewport);
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

            if (new_frame)
            {
                ImVec2 img_size;
                img_size.x = img_width;
                img_size.y = img_height;
                // igImage((void *)(intptr_t)texture, img_size, (ImVec2){0}, (ImVec2){1, 1}, (ImVec4){0}, (ImVec4){0});
                ImDrawList_AddImage(drawlist, (void *)(intptr_t)texture, (ImVec2){0}, img_size, (ImVec2){0}, (ImVec2){0}, 0);
            }

            igEnd();
        }

        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
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
