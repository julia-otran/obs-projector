#include "ogl-loader.h"
#include "config-structs.h"

#ifndef _RENDER_H_
#define _RENDER_H_

typedef struct {
    int render_width, render_height;
} render_output_size;

typedef struct {
    render_output_size size;
    GLuint rendered_texture;
    GLuint framebuffer_name;

    GLuint outline_vertex_array;
    GLuint outline_vertex_buffer;
    GLuint outline_uv_buffer;
} render_layer;

typedef struct {
    render_output_size size;
    GLuint rendered_texture;
} render_output;

void initialize_renders();
void activate_renders(GLFWwindow *shared_context, render_output_size *out_size);
void shutdown_renders();

void lock_renders();
void unlock_renders();

void renders_init();
void renders_update_assets();
void renders_cycle();
void renders_flush_buffers();
void renders_terminate();

void renders_get_output(render_output *out);

#endif