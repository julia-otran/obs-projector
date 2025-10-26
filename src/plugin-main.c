/*
OBS Projector
Copyright (C) 2025 Julia Otranto Aulicino

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#define CONFIG_FILE "projection-config.json"

#include <obs-module.h>
#include <plugin-support.h>
#include <stdlib.h>

#include "debug.h"
#include "config-structs.h"
#include "config.h"
#include "config-debug.h"
#include "monitor.h"
#include "loop.h"
#include "render-obs.h"
#include "ogl-loader.h"

static int initialized = 0;
static int configured = 0;
static projection_config *config;
static render_output_size output_size;
static obs_output_t *output;

typedef struct {
    obs_output_t *output;
} context_info;

void glfwIntErrorCallback(GLint _, const GLchar *error_string) {
    log_debug("Catch GLFW error: %s\n", error_string);
}

void glfwIntMonitorCallback(GLFWmonitor* monitor, int event) {
    if (config) {
        monitors_reload();
        monitors_adjust_windows(config);
    }
}

void internal_lib_render_shutdown() {
    log_debug("Shutting down main loop...\n");
    main_loop_terminate();

    log_debug("Shutting down renders...\n");
    shutdown_renders();

    log_debug("Shutting down monitors...\n");
    monitors_destroy_windows();
}

void internal_lib_render_load_default_config() {
    config_bounds default_monitor;

    monitors_get_default_projection_bounds(&default_monitor);
    prepare_default_config(&default_monitor);
}

void internal_lib_render_startup() {
    log_debug("Will load config:\n");
    print_projection_config(config);

    log_debug("Initialize renders...\n");
    initialize_renders();

    log_debug("Creating windows...\n");
    monitors_create_windows(config);

    log_debug("Initializing async transfer windows...\n");
    activate_renders(monitors_get_shared_window(), &output_size);

    log_debug("Starting main loop...\n")
    main_loop_schedule_config_reload(config);
    main_loop_start();
}

void internal_lib_render_load_config() {
    configured = 0;

    log_debug("Reloading monitors");
    monitors_reload();

    log_debug("Creating default config");
    internal_lib_render_load_default_config();

    projection_config *new_config;

    log_debug("Loading screen configs");
    new_config = load_config(CONFIG_FILE);

    if (new_config == NULL) {
        generate_config(CONFIG_FILE);
        new_config = load_config(CONFIG_FILE);
    }

    if (new_config == NULL) {
        new_config = config_get_default();
    }

    if (config) {
        if (config_change_requires_restart(new_config, config)) {
            internal_lib_render_shutdown();   
            free_projection_config(config);
        } else {
            log_debug("New config was loaded! hot reloading...\n");
            projection_config* old_config = config;

            config = new_config;

            main_loop_schedule_config_reload(config);
            free_projection_config(old_config);

            return;
        }
    }

    log_debug("Staring engine...\n");

    config = new_config;

    internal_lib_render_startup();

    configured = 1;
}

const char* my_output_name(void* type_data) {
	return "Projector Output";
}

void* my_output_create(obs_data_t *settings, obs_output_t *output) {
    if (!glfwInit()) {
        return (void*)0;
    }

    config = NULL;

    glfwSetErrorCallback(glfwIntErrorCallback);
    glfwSetMonitorCallback(glfwIntMonitorCallback);

    initialized = 1;

    context_info *info = bzalloc(sizeof(context_info));
    info->output = output;

    obs_log(LOG_INFO, "Output created");

	return (void*)info;

}

void my_output_update(void *data, obs_data_t *settings) {

}

void my_output_destroy(void *data) {
    glfwTerminate();
}

bool my_output_start(void *data) {
    context_info *info = (context_info*) data;

    output_size.render_width = obs_output_get_width(info->output);
    output_size.render_height = obs_output_get_height(info->output);

    internal_lib_render_load_config();
    return true;
}

void my_output_stop(void *data, uint64_t ts) {
    internal_lib_render_shutdown();
    configured = 0;
    config = NULL;
}

void my_output_data(void *data, struct video_data *frame) {
    render_obs_push_frame(frame->data, output_size.render_width, frame->linesize[0], output_size.render_height);
}

struct obs_output_info my_output = {
        .id                   = "projector",
        .flags                = OBS_OUTPUT_VIDEO,
        .get_name             = my_output_name,
        .create               = my_output_create,
		.update               = my_output_update,
        .destroy              = my_output_destroy,
        .start                = my_output_start,
        .stop                 = my_output_stop,
        .raw_video            = my_output_data
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
	obs_register_output(&my_output);
    output = obs_output_create("projector", "Projector Output", NULL, NULL);
    obs_output_set_media(output, obs_get_video(), NULL);
    obs_output_start(output);

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
    obs_output_stop(output);
    obs_output_release(output);

	obs_log(LOG_INFO, "plugin unloaded");
}
