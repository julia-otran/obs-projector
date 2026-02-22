#include <stdlib.h>

#include <obs.h>

#include "tinycthread.h"
#include "debug.h"
#include "loop.h"
#include "ogl-loader.h"
#include "render.h"
#include "clock.h"

static int run = 0;
static int waiting = 0;

static thrd_t thread_id;
static mtx_t thread_mutex;
static cnd_t thread_cond;

static projection_config *config;
static int pending_config_reload;

static int poolRunning;

void poolUIEvents() {
    glfwPollEvents();
    poolRunning = 0;
}

int loop(void *_) {
    time_measure* tm0 = create_measure("Renders Update Assets");
    time_measure* tm1 = create_measure("Renders Cycle");
    time_measure* tm2 = create_measure("Monitors Cycle");
    time_measure* tm3 = create_measure("Monitors Flip");

    render_output *output;

    renders_get_output(&output);
    monitors_load_renders(output);

    log_debug("Monitors ready to connect to renders.\n");

    monitors_start(config);

    log_debug("Monitors started.\n");

    pending_config_reload = 0;

    monitors_set_share_context();
    renders_init();

    log_debug("Main loop initalized.\n");

    while (run) {
        mtx_lock(&thread_mutex);

        if (pending_config_reload) {
            monitors_config_hot_reload(config);
            pending_config_reload = 0;
        }

        if (waiting) {
            cnd_signal(&thread_cond);
        }

        mtx_unlock(&thread_mutex);

        begin_measure(tm0);
        renders_update_assets();
        end_measure(tm0);

        begin_measure(tm1);
        renders_cycle();
        end_measure(tm1);

        begin_measure(tm2);
        monitors_cycle();
        end_measure(tm2);

        begin_measure(tm3);
        monitors_flip();
        end_measure(tm3);

        monitors_set_share_context();
        renders_flush_buffers();

        if (window_should_close()) {
            run = 0;
        }

        register_monitor_frame();

#ifndef __APPLE__
        glfwPollEvents();
#endif

#ifdef __APPLE__
        if (poolRunning == 0) {
            poolRunning = 1;

            auto task = [](void *) {
                poolUIEvents();
            };

            obs_queue_task(OBS_TASK_UI, task, nullptr, false);
        }
#endif
    }

    monitors_stop();
    renders_terminate();
    monitors_terminate();

    return 0;
}

void main_loop_schedule_config_reload(projection_config *in_config) {
    if (run) {
        mtx_lock(&thread_mutex);
        waiting = 1;
    }

    config = in_config;
    pending_config_reload = 1;

    if (run) {
        cnd_wait(&thread_cond, &thread_mutex);
        waiting = 0;
        mtx_unlock(&thread_mutex);
    }
}

void main_loop_start() {
    mtx_init(&thread_mutex, 0);
    cnd_init(&thread_cond);

    run = 1;

    thrd_create(&thread_id, loop, NULL);

    mtx_lock(&thread_mutex);
    waiting = 1;
    cnd_wait(&thread_cond, &thread_mutex);
    waiting = 0;
    mtx_unlock(&thread_mutex);

}

void main_loop_terminate() {
    run = 0;

    thrd_join(thread_id, NULL);
    cnd_destroy(&thread_cond);
    mtx_destroy(&thread_mutex);

    config = NULL;
}
