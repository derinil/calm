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

const char *vertex_shader_source = "#version 330 core\n"
                                   "layout (location = 0) in vec3 aPos;\n"
                                   "layout (location = 1) in vec3 aColor;\n"
                                   "layout (location = 2) in vec2 aTexCoord;\n"
                                   "out vec3 ourColor;\n"
                                   "out vec2 TexCoord;\n"
                                   "void main()\n"
                                   "{\n"
                                   "    gl_Position = vec4(aPos, 1.0);\n"
                                   "    ourColor = aColor;\n"
                                   //    Flip the y coordinate because opengl is dumb!!
                                   "    TexCoord = vec2(aTexCoord.x, 1-aTexCoord.y);\n"
                                   "}\0";

const char *fragment_shader_source = "#version 330 core\n"
                                     "out vec4 FragColor;\n"
                                     "in vec3 ourColor;\n"
                                     "in vec2 TexCoord;\n"
                                     "uniform sampler2D ourTexture;\n"
                                     "void main()\n"
                                     "{\n"
                                     "    FragColor = texture(ourTexture, TexCoord);\n"
                                     "}\0";

static void draw(GLFWwindow *window, struct DStack *stack)
{
    GLenum err = 0;
    GLuint texture;
    unsigned int vert_shad, frag_shad, shader_prog;
    int width = 0, height = 0;
    struct DFrame *dframe = NULL, *new_dframe = NULL;
    float vertices[] = {
        // positions          // colors           // texture coords
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // top right
        1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
        -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    vert_shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shad, 1, &vertex_shader_source, NULL);
    glCompileShader(vert_shad);
    unsigned int fragmentShader;
    frag_shad = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shad, 1, &fragment_shader_source, NULL);
    glCompileShader(frag_shad);
    shader_prog = glCreateProgram();
    glAttachShader(shader_prog, vert_shad);
    glAttachShader(shader_prog, frag_shad);
    glLinkProgram(shader_prog);
    glUseProgram(shader_prog);
    glDeleteShader(vert_shad);
    glDeleteShader(frag_shad);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        new_dframe = (struct DFrame *)dstack_pop(stack, 1);
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

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glUseProgram(shader_prog);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shader_prog);
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
    draw(window, stack);
    destroy_window(window);
    return 0;
}
