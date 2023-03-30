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

// https://stackoverflow.com/questions/30191911/is-it-possible-to-draw-yuv422-and-yuv420-texture-using-opengl
char *yuv_to_rgba_shader =
    "uniform sampler2DRect Ytex;\n"
    "uniform sampler2DRect Utex,Vtex;\n"
    "void main(void) {\n"
    "  float nx,ny,r,g,b,y,u,v;\n"
    "  vec4 txl,ux,vx;"
    "  nx=gl_TexCoord[0].x;\n"
    "  ny=576.0-gl_TexCoord[0].y;\n"
    "  y=texture2DRect(Ytex,vec2(nx,ny)).r;\n"
    "  u=texture2DRect(Utex,vec2(nx/2.0,ny/2.0)).r;\n"
    "  v=texture2DRect(Vtex,vec2(nx/2.0,ny/2.0)).r;\n"

    "  y=1.1643*(y-0.0625);\n"
    "  u=u-0.5;\n"
    "  v=v-0.5;\n"

    "  r=y+1.5958*v;\n"
    "  g=y-0.39173*u-0.81290*v;\n"
    "  b=y+2.017*u;\n"

    "  gl_FragColor=vec4(r,g,b,1.0);\n"
    "}\n";

static void draw(GLFWwindow *window, struct DStack *stack)
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

        // if (dstack_ready(stack))
        // {
        //     struct CFrame *frame = (struct CFrame *)dstack_pop(stack);
        // }

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
#if 1
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
