#include "ogl-loader.h"
#include "render.h"

#ifndef _RENDER_OBS_H_
#define _RENDER_OBS_H_

void render_obs_initialize();

void render_obs_push_frame(void *buffer, int width, int line_size, int height);

void render_obs_create_buffers();
void render_obs_update_buffers();
void render_obs_flush_buffers();
void render_obs_deallocate_buffers();

void render_obs_create_assets();
void render_obs_update_assets();
void render_obs_deallocate_assets();

void render_obs_render(render_layer *layer);

void render_obs_shutdown();

#endif
