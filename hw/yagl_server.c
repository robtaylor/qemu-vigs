#include "yagl_server.h"
#include "yagl_api.h"
#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_marshal.h"
#include "yagl_version.h"
#include "yagl_log.h"
#include "yagl_egl_driver.h"
#include "yagl_egl_interface.h"
#include "yagl_apis/egl/yagl_egl_api.h"
#include "yagl_apis/gles2/yagl_gles2_api.h"
#include "yagl_drivers/gles2_ogl/yagl_gles2_ogl.h"
#include "yagl_backends/egl_offscreen/yagl_egl_offscreen.h"
#include <GL/gl.h>
#include "yagl_gles2_driver.h"

static struct yagl_thread_state
    *yagl_server_find_thread(struct yagl_server_state *ss,
                             yagl_pid target_pid,
                             yagl_tid target_tid)
{
    struct yagl_process_state *ps = NULL;

    QLIST_FOREACH(ps, &ss->processes, entry) {
        if (ps->id == target_pid) {
            return yagl_process_find_thread(ps, target_tid);
        }
    }

    return NULL;
}

#if defined(CONFIG_YAGL_EGL_GLX)
    struct yagl_server_state *yagl_server_state_create(Display *x_display)
#elif defined(CONFIG_YAGL_EGL_WGL)
    struct yagl_server_state *yagl_server_state_create(void)
#else
#error Unknown EGL driver
#endif
{
    int i;
    struct yagl_server_state *ss =
        g_malloc0(sizeof(struct yagl_server_state));
    struct yagl_egl_driver *egl_driver;
    struct yagl_egl_backend *egl_backend;
    struct yagl_gles2_driver *gles2_driver;

    QLIST_INIT(&ss->processes);

#if defined(CONFIG_YAGL_EGL_GLX)
    egl_driver = yagl_egl_driver_create(x_display);
#elif defined(CONFIG_YAGL_EGL_WGL)
    egl_driver = yagl_egl_driver_create();
#else
#error Unknown EGL driver
#endif

    if (!egl_driver) {
        goto fail;
    }

    egl_backend = yagl_egl_offscreen_create(egl_driver);

    if (!egl_backend) {
        egl_driver->destroy(egl_driver);
        goto fail;
    }

    ss->apis[yagl_api_id_egl - 1] = yagl_egl_api_create(egl_backend);

    if (!ss->apis[yagl_api_id_egl - 1]) {
        egl_backend->destroy(egl_backend);

        goto fail;
    }

    gles2_driver = yagl_gles2_ogl_create(egl_driver->dyn_lib);

    if (!gles2_driver) {
        goto fail;
    }

    ss->apis[yagl_api_id_gles2 - 1] = yagl_gles2_api_create(gles2_driver);

    if (!ss->apis[yagl_api_id_gles2 - 1]) {
        gles2_driver->destroy(gles2_driver);

        goto fail;
    }

    return ss;

fail:
    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ss->apis[i]) {
            ss->apis[i]->destroy(ss->apis[i]);
            ss->apis[i] = NULL;
        }
    }

    g_free(ss);

    return NULL;
}

void yagl_server_state_destroy(struct yagl_server_state *ss)
{
    int i;

    yagl_server_reset(ss);

    for (i = 0; i < YAGL_NUM_APIS; ++i) {
        if (ss->apis[i]) {
            ss->apis[i]->destroy(ss->apis[i]);
            ss->apis[i] = NULL;
        }
    }

    g_free(ss);
}

void yagl_server_reset(struct yagl_server_state *ss)
{
    struct yagl_process_state *ps, *next;

    QLIST_FOREACH_SAFE(ps, &ss->processes, entry, next) {
        QLIST_REMOVE(ps, entry);
        yagl_process_state_destroy(ps);
    }

    assert(QLIST_EMPTY(&ss->processes));
}

/*
 * init command out_buff is:
 *  (uint32_t) version
 * in_buff must be:
 *  (uint32_t) 1 - init ok, 0 - init error
 *  (yagl_render_type), in case of init ok
 */
bool yagl_server_dispatch_init(struct yagl_server_state *ss,
                               yagl_pid target_pid,
                               yagl_tid target_tid,
                               uint8_t *out_buff,
                               uint8_t *in_buff)
{
    uint32_t version = yagl_marshal_get_uint32(&out_buff);
    struct yagl_process_state *ps = NULL;
    struct yagl_thread_state *ts = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_server_dispatch_init, NULL);

    if (version != YAGL_VERSION) {
        YAGL_LOG_CRITICAL(
            "target-host version mismatch, target version is %u, host version is %u",
            version,
            YAGL_VERSION);

        yagl_marshal_put_uint32(&in_buff, 0);

        YAGL_LOG_FUNC_EXIT(NULL);

        return false;
    }

    QLIST_FOREACH(ps, &ss->processes, entry) {
        if (ps->id == target_pid) {
            /*
             * Process already exists.
             */

            ts = yagl_process_find_thread(ps, target_tid);

            if (ts) {
                YAGL_LOG_CRITICAL(
                    "thread %u already initialized in process %u",
                    target_tid, target_pid);

                yagl_marshal_put_uint32(&in_buff, 0);

                YAGL_LOG_FUNC_EXIT(NULL);

                return false;
            } else {
                ts = yagl_process_add_thread(ps, target_tid);

                if (!ts) {
                    YAGL_LOG_CRITICAL(
                        "cannot add thread %u to process %u",
                        target_tid, target_pid);

                    yagl_marshal_put_uint32(&in_buff, 0);

                    YAGL_LOG_FUNC_EXIT(NULL);

                    return false;
                } else {
                    yagl_marshal_put_uint32(&in_buff, 1);

                    YAGL_LOG_FUNC_EXIT(NULL);

                    return true;
                }
            }
        }
    }

    /*
     * No process found, create one.
     */

    ps = yagl_process_state_create(ss, target_pid);

    if (!ps) {
        YAGL_LOG_CRITICAL(
            "cannot create process %u",
            target_pid);

        yagl_marshal_put_uint32(&in_buff, 0);

        YAGL_LOG_FUNC_EXIT(NULL);

        return false;
    }

    ts = yagl_process_add_thread(ps, target_tid);

    if (!ts) {
        YAGL_LOG_CRITICAL(
            "cannot add thread %u to process %u",
            target_tid, target_pid);

        yagl_process_state_destroy(ps);

        yagl_marshal_put_uint32(&in_buff, 0);

        YAGL_LOG_FUNC_EXIT(NULL);

        return false;
    } else {
        QLIST_INSERT_HEAD(&ss->processes, ps, entry);

        yagl_marshal_put_uint32(&in_buff, 1);
        yagl_marshal_put_render_type(&in_buff, ps->egl_iface->render_type);

        YAGL_LOG_FUNC_EXIT(NULL);

        return true;
    }
}

void yagl_server_dispatch(struct yagl_server_state *ss,
                          yagl_pid target_pid,
                          yagl_tid target_tid,
                          uint8_t *out_buff,
                          uint8_t *in_buff)
{
    struct yagl_thread_state *ts;

    YAGL_LOG_FUNC_ENTER(yagl_server_dispatch, NULL);

    ts = yagl_server_find_thread(ss, target_pid, target_tid);

    if (ts) {
        yagl_thread_call(ts, out_buff, in_buff);
    } else {
        YAGL_LOG_CRITICAL(
            "process/thread %u/%u not found",
            target_pid, target_tid);

        yagl_marshal_put_call_result(&in_buff, yagl_call_result_fail);
    }

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_server_dispatch_exit(struct yagl_server_state *ss,
                               yagl_pid target_pid,
                               yagl_tid target_tid)
{
    struct yagl_thread_state *ts =
        yagl_server_find_thread(ss, target_pid, target_tid);
    struct yagl_process_state *ps = NULL;

    YAGL_LOG_FUNC_ENTER(yagl_server_dispatch_exit, NULL);

    if (!ts) {
        YAGL_LOG_CRITICAL(
            "process/thread %u/%u not found",
            target_pid, target_tid);

        YAGL_LOG_FUNC_EXIT(NULL);

        return;
    }

    ps = ts->ps;

    yagl_process_remove_thread(ps, target_tid);

    if (!yagl_process_has_threads(ps)) {
        QLIST_REMOVE(ps, entry);

        yagl_process_state_destroy(ps);
    }

    YAGL_LOG_FUNC_EXIT(NULL);
}
