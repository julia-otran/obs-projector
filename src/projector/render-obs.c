#include <string.h>
#include <stdlib.h>

#include "ogl-loader.h"
#include "debug.h"
#include "render-pixel-unpack-buffer.h"
#include "render-obs.h"

static render_pixel_unpack_buffer_instance* buffer_instance;
static GLuint texture_id;
static int dst_width, dst_height;

void render_obs_initialize() {
    dst_width = 0;
    dst_height = 0;
}

void render_obs_push_frame(void *data_in, int width, int line_size, int height) {
    render_pixel_unpack_buffer_node* buffer = render_pixel_unpack_buffer_dequeue_for_write(buffer_instance);

    if (buffer) {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer->gl_buffer);

        if (width > 0 && height > 0) {
            if (buffer->width != width || buffer->height != height || buffer->line_size != line_size) {
                glBufferData(GL_PIXEL_UNPACK_BUFFER, height * line_size, 0, GL_DYNAMIC_DRAW);
                buffer->width = width;
                buffer->height = height;
                buffer->line_size = line_size;
            }

            void* data = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            memcpy(data, data_in, height * line_size);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    render_pixel_unpack_buffer_enqueue_for_read(buffer_instance, buffer);
}

void render_obs_create_buffers() {
    render_pixel_unpack_buffer_create(&buffer_instance);
}

void render_obs_update_buffers() {

}

void render_obs_flush_buffers() {
    render_pixel_unpack_buffer_flush(buffer_instance);
}

void render_obs_deallocate_buffers() {
	render_pixel_unpack_buffer_deallocate(buffer_instance);
	buffer_instance = NULL;
}

void render_obs_create_assets() {
    glGenTextures(1, &texture_id);
}

void render_obs_update_assets() {
    render_pixel_unpack_buffer_node* buffer = render_pixel_unpack_buffer_dequeue_for_read(buffer_instance);

    if (buffer) {
        dst_width = buffer->width;
        dst_height = buffer->height;

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer->gl_buffer);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dst_width, dst_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
        tex_set_default_params();

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    render_pixel_unpack_buffer_enqueue_for_flush(buffer_instance, buffer);
}
void render_obs_deallocate_assets() {
    glDeleteTextures(1, &texture_id);
}

void render_obs_render(render_layer *layer) {
    if (!dst_width || !dst_height) {
        return;
    }

    double x, y, w, h;

    double w_scale = (dst_width / (double)dst_height);
    double h_scale = (dst_height / (double)dst_width);

    double w_sz = layer->size.render_height * w_scale;
    double h_sz = layer->size.render_width * h_scale;

    if ((w_sz > layer->size.render_width))
    {
        w = w_sz;
        h = layer->size.render_height;
    }
    else 
    {
        h = h_sz;
        w = layer->size.render_width;
    }

    x = (layer->size.render_width - w) / 2;
    y = (layer->size.render_height - h) / 2;

    glColor4f(1.0, 1.0, 1.0, 1.0);

    glBindTexture(GL_TEXTURE_2D, texture_id);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex2d(x, y);
    glTexCoord2f(0.0, 1.0); glVertex2d(x, y + h);
    glTexCoord2f(1.0, 1.0); glVertex2d(x + w, y + h);
    glTexCoord2f(1.0, 0.0); glVertex2d(x + w, y);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void render_obs_shutdown() {
}
