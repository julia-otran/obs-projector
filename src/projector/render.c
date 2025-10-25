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

static int transfer_window_thread_run;
static int transfer_window_initialized;
static GLFWwindow *transfer_window;
static thrd_t transfer_window_thread;

void get_render_output(render_output *out) {
   out = output;
}

void initialize_renders() {
    render_obs_initialize();
}

void shutdown_renders() {
    transfer_window_thread_run = 0;
    thrd_join(transfer_window_thread, NULL);

    render_obs_shutdown();

    free(output);
    free(render);

    glfwDestroyWindow(transfer_window);
}

void render_init(render_layer *render) {
    int width = render->size.render_width;
    int height = render->size.render_width;

    GLuint renderedTexture;
    glGenTextures(1, &renderedTexture);

    render->rendered_texture = renderedTexture;

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, renderedTexture);

    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);

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

void render_cycle(render_layer *render) {
    if (!transfer_window_initialized) {
        return;
    }

    int width = render->size.render_width;
    int height = render->size.render_height;

    glBindFramebuffer(GL_FRAMEBUFFER, render->framebuffer_name);

    glViewport(0, 0, width, height);

    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, width, 0.0, height, 0.0, 1.0);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    render_obs_render(render);

    glPopMatrix();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void render_terminate(render_layer *render) {
    render_obs_deallocate_assets();

    output->rendered_texture = 0;

    glDeleteFramebuffers(1, &render->framebuffer_name);
    glDeleteTextures(1, &render->rendered_texture);
}

void renders_init() {
    render_init(render);
}

void renders_update_assets()
{
    if (!transfer_window_initialized) {
        return;
    }

    render_obs_update_assets();
}

void renders_cycle() {
    glEnable(GL_TEXTURE_2D);
    render_cycle(render);
    glDisable(GL_TEXTURE_2D);
}

void renders_flush_buffers() {
    if (!transfer_window_initialized) {
        return;
    }

    render_obs_flush_buffers();
}

void renders_terminate() {
    render_terminate(render);
}

int transfer_window_loop(void *_) {
    struct timespec ts_prev, ts_curr;
    struct timespec sleep_time = {
        .tv_sec = 0,
        .tv_nsec = 5 * 1000 * 1000
    };

    glfwMakeContextCurrent(transfer_window);
    
#ifdef _GLEW_ENABLED_
    glewInit();
#endif
    
    render_obs_create_buffers();

    transfer_window_initialized = 1;

    glClearColor(0, 0, 0, 1.0);

    get_time(&ts_prev);

    while (transfer_window_thread_run) {
        glClear(GL_COLOR_BUFFER_BIT);

        render_obs_update_buffers();

        glFinish();
        
        get_time(&ts_curr);

        while (get_delta_time_ms(&ts_curr, &ts_prev) < 15)
        {
            thrd_sleep(&sleep_time, NULL);
            get_time(&ts_curr);
        }

        copy_time(&ts_prev, &ts_curr);

        register_stream_frame();
    }

    render_obs_deallocate_buffers();

    return 0;
}

void activate_renders(GLFWwindow *shared_context, render_output_size *out_size) {
    render = calloc(1, sizeof(render_layer));
    output = (render_output*) calloc(1, sizeof(render_output));

    output->size.render_width = out_size->render_width;
    output->size.render_height = out_size->render_height;

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 0);
    transfer_window = glfwCreateWindow(800, 600, "Projector Stream Window", NULL, shared_context);

    transfer_window_thread_run = 1;
    transfer_window_initialized = 0;
    thrd_create(&transfer_window_thread, transfer_window_loop, NULL);
    log_debug("renders activated\n");
}
