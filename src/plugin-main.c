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

// TODO: Adjust for Windows Platform
#define CONFIG_FILE "~/projection-config.json"

#include <obs-module.h>
#include <plugin-support.h>
#include <stdlib.h>

#include <uv.h>

#include "debug.h"
#include "config-structs.h"
#include "config.h"
#include "config-debug.h"
#include "monitor.h"
#include "loop.h"
#include "render.h"
#include "render-obs.h"
#include "ogl-loader.h"

static uv_loop_t *loop;

static int initialized = 0;
static int configured = 0;
static int terminated = 0;

static int last_width = 0;
static int last_height = 0;

static projection_config *config;
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
    void *test = NULL;
    log_debug("Shutting down main loop...");
    main_loop_terminate();

    (*test) = 1;
    log_debug("Shutting down renders...");
    shutdown_renders();

    log_debug("Shutting down monitors...");
    monitors_destroy_windows();
}

void internal_lib_render_load_default_config() {
    config_bounds default_monitor;

    monitors_get_default_projection_bounds(&default_monitor);
    prepare_default_config(&default_monitor);
}

void internal_lib_render_startup() {
    log_debug("Will load config:");
    print_projection_config(config);

    log_debug("Initialize renders...");
    initialize_renders();

    log_debug("Creating windows...");
    monitors_create_windows(config);

    log_debug("Initializing async transfer windows...");
    activate_renders(monitors_get_shared_window());

    log_debug("Starting main loop...")
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
            log_debug("New config was loaded! hot reloading...");
            projection_config* old_config = config;

            config = new_config;

            main_loop_schedule_config_reload(config);
            free_projection_config(old_config);

            return;
        }
    }

    log_debug("Staring engine...");

    config = new_config;

    internal_lib_render_startup();

    configured = 1;
}

const char* my_output_name(void* type_data) {
	return "Projector Output";
}

void* my_output_create(obs_data_t *settings, obs_output_t *output) {
    terminated = 0;

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
    log_debug("plugin will destroy");
    
    if (configured) {
        internal_lib_render_shutdown();
        configured = 0;
        config = NULL;
    }
    
    if (!terminated) {
        glfwTerminate();
        terminated = 1;
    }

    log_debug("output destroyed");
}

bool my_output_start(void *data) {
    context_info *info = (context_info*) data;

    internal_lib_render_load_config();

    obs_output_begin_data_capture(info->output, 0);
    return true;
}

void my_output_stop(void *data, uint64_t ts) {
    obs_log(LOG_INFO, "plugin will stop");

    context_info *info = (context_info*) data;

    if (configured) {
        obs_output_end_data_capture(info->output);

        internal_lib_render_shutdown();
        configured = 0;
        last_width = 0;
        last_height = 0;
        config = NULL;
    }

    log_debug("plugin stopped");
}

void my_output_data(void *data, struct video_data *frame) {
    if (!configured) {
        return;
    }

    context_info *info = (context_info*) data;

    int width = obs_output_get_width(info->output);
    int height = obs_output_get_height(info->output);

    uint8_t *video_data = frame->data[0];

    if (width && height && data && frame->linesize[0]) {
        renders_push_frame(video_data, width, frame->linesize[0], height);
        if (width != last_width || height != last_height) {
            last_width = width;
            last_height = height;
            main_loop_schedule_config_reload(config);
        }
    }
}

void handle_uv_fs_event(uv_fs_event_t *handle, const char *filename, int events, int status) {
    internal_lib_render_load_config();
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
    loop = uv_default_loop();
    uv_run(loop, UV_RUN_DEFAULT);

    obs_register_output(&my_output);

    output = obs_output_create("projector", "Projector Output", NULL, NULL);
    obs_output_set_media(output, obs_get_video(), NULL);

    int width = obs_output_get_width(output);
    int height = obs_output_get_height(output);

    const struct video_scale_info scale_info = {
        .format = VIDEO_FORMAT_BGRA,
        .width = width,
        .height = height,
        .range = VIDEO_RANGE_FULL,
        .colorspace = VIDEO_CS_SRGB,
    };
    
    obs_output_set_video_conversion(output, &scale_info);
    bool success = obs_output_start(output);

    if (!success) {
        const char *error = obs_output_get_last_error(output);
        obs_log(LOG_ERROR, "Failed to start output %s", error);
    } else {
        obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
    }

    uv_fs_event_t *fs_event_req = malloc(sizeof(uv_fs_event_t));
    uv_fs_event_init(loop, fs_event_req);
    uv_fs_event_start(fs_event_req, handle_uv_fs_event, CONFIG_FILE, 0);

	return success;
}

void obs_module_unload(void)
{
    obs_log(LOG_INFO, "plugin will unload");
    uv_loop_close(loop);

	obs_log(LOG_INFO, "plugin unloaded");
}
