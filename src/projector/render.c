#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "clock.h"
#include "tinycthread.h"
#include "debug.h"
#include "ogl-loader.h"
#include "render.h"
#include "render-obs.h"

static render_layer *render;
static render_output *output = NULL;

static int transfer_window_initialized;
static GLFWwindow *transfer_window;

void renders_get_output(render_output **out) {
   (*out) = output;
}

void initialize_renders() {
    render_obs_initialize();
}

void shutdown_renders() {
    transfer_window_initialized = 0;

    render_obs_shutdown();

    free(output);
    free(render);

    glfwDestroyWindow(transfer_window);
}

void renders_init() {
    GLuint renderedTexture;
    glGenTextures(1, &renderedTexture);

    render->rendered_texture = renderedTexture;

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, renderedTexture);

    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 600, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);

    tex_set_default_params();

    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint FramebufferName = 0;
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    render->framebuffer_name = FramebufferName;
    output->rendered_texture = renderedTexture;

    render_obs_create_assets();
}

void renders_update_assets()
{
    if (!transfer_window_initialized) {
        return;
    }

    render_obs_update_assets();
}

void renders_cycle() {
    if (!transfer_window_initialized) {
        return;
    }

    int width = render->size.render_width;
    int height = render->size.render_height;

    if (width == 0 || height == 0) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, render->rendered_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
    tex_set_default_params();
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, render->framebuffer_name);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render->rendered_texture, 0);

    glViewport(0, 0, width, height);

    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, width, 0.0, height, 0.0, 1.0);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    render_obs_render(render);
    glDisable(GL_TEXTURE_2D);
    
    glPopMatrix();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renders_flush_buffers() {
    if (!transfer_window_initialized) {
        return;
    }

    render_obs_flush_buffers();
}

void renders_terminate() {
    render_obs_deallocate_assets();

    output->rendered_texture = 0;

    glDeleteFramebuffers(1, &render->framebuffer_name);
    glDeleteTextures(1, &render->rendered_texture);
}

void renders_push_frame(void *data, int width, int line_size, int height) {
    glfwMakeContextCurrent(transfer_window);
    render_obs_push_frame(data, width, line_size, height);
    glfwMakeContextCurrent(NULL);

    render->size.render_width = width;
    render->size.render_height = height;
    output->size.render_width = width;
    output->size.render_height = height;
}

void activate_renders(GLFWwindow *shared_context) {
    render = calloc(1, sizeof(render_layer));
    output = (render_output*) calloc(1, sizeof(render_output));

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 0);
    transfer_window = glfwCreateWindow(800, 600, "Projector Stream Window", NULL, shared_context);

    glfwMakeContextCurrent(transfer_window);
    
    #ifdef _GLEW_ENABLED_
    glewInit();
    #endif
    
    render_obs_create_buffers();

    transfer_window_initialized = 1;

    glfwMakeContextCurrent(NULL);

    log_debug("renders activated\n");
}
