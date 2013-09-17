#include "yagl_process.h"
#include "yagl_thread.h"
#include "yagl_server.h"
#include "yagl_log.h"
#include "yagl_stats.h"
#include "sysemu/kvm.h"

struct yagl_process_state
    *yagl_process_state_create(struct yagl_server_state *ss,
                               yagl_pid id)
{
    struct yagl_process_state *ps =
        g_malloc0(sizeof(struct yagl_process_state));

    ps->ss = ss;
    ps->id = id;
    QLIST_INIT(&ps->threads);

#ifdef CONFIG_KVM
    cpu_synchronize_state(current_cpu);
    memcpy(&ps->cr[0], &((CPUX86State*)current_cpu->env_ptr)->cr[0], sizeof(ps->cr));
#endif

    return ps;
}

void yagl_process_state_destroy(struct yagl_process_state *ps)
{
    struct yagl_thread_state *ts, *next;

    YAGL_LOG_FUNC_ENTER(yagl_process_state_destroy, NULL);

    QLIST_FOREACH_SAFE(ts, &ps->threads, entry, next) {
        bool is_last;
        QLIST_REMOVE(ts, entry);
        is_last = QLIST_EMPTY(&ps->threads);
        yagl_thread_state_destroy(ts, is_last);
    }

    assert(QLIST_EMPTY(&ps->threads));

    g_free(ps);

    yagl_stats_dump();

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_process_register_egl_interface(struct yagl_process_state *ps,
                                         struct yagl_egl_interface *egl_iface)
{
    YAGL_LOG_FUNC_ENTER(yagl_process_register_egl_interface, NULL);

    assert(egl_iface);
    assert(!ps->egl_iface);

    if (ps->egl_iface) {
        YAGL_LOG_CRITICAL("EGL interface is already registered");

        YAGL_LOG_FUNC_EXIT(NULL);

        return;
    }

    ps->egl_iface = egl_iface;

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_process_unregister_egl_interface(struct yagl_process_state *ps)
{
    YAGL_LOG_FUNC_ENTER(yagl_process_unregister_egl_interface, NULL);

    assert(ps->egl_iface);

    if (!ps->egl_iface) {
        YAGL_LOG_CRITICAL("EGL interface was not registered");

        YAGL_LOG_FUNC_EXIT(NULL);

        return;
    }

    ps->egl_iface = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_process_register_client_interface(struct yagl_process_state *ps,
                                            yagl_client_api client_api,
                                            struct yagl_client_interface *client_iface)
{
    YAGL_LOG_FUNC_ENTER(yagl_process_register_client_interface, NULL);

    assert(!ps->client_ifaces[client_api]);

    if (ps->client_ifaces[client_api]) {
        YAGL_LOG_CRITICAL("client interface %d is already registered",
                          client_api);

        YAGL_LOG_FUNC_EXIT(NULL);

        return;
    }

    ps->client_ifaces[client_api] = client_iface;

    YAGL_LOG_FUNC_EXIT(NULL);
}

void yagl_process_unregister_client_interface(struct yagl_process_state *ps,
                                              yagl_client_api client_api)
{
    YAGL_LOG_FUNC_ENTER(yagl_process_unregister_client_interface, NULL);

    assert(ps->client_ifaces[client_api]);

    if (!ps->client_ifaces[client_api]) {
        YAGL_LOG_CRITICAL("client interface %d was not registered",
                          client_api);

        YAGL_LOG_FUNC_EXIT(NULL);

        return;
    }

    ps->client_ifaces[client_api] = NULL;

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_thread_state*
    yagl_process_find_thread(struct yagl_process_state *ps,
                            yagl_tid id)
{
    struct yagl_thread_state *ts = NULL;

    QLIST_FOREACH(ts, &ps->threads, entry) {
        if (ts->id == id) {
            return ts;
        }
    }

    return NULL;
}

struct yagl_thread_state*
    yagl_process_add_thread(struct yagl_process_state *ps,
                            yagl_tid id)
{
    bool is_first = QLIST_EMPTY(&ps->threads);
    struct yagl_thread_state *ts =
        yagl_thread_state_create(ps, id, is_first);

    if (!ts) {
        return NULL;
    }

    QLIST_INSERT_HEAD(&ps->threads, ts, entry);

    return ts;
}

void yagl_process_remove_thread(struct yagl_process_state *ps,
                                yagl_tid id)
{
    struct yagl_thread_state *ts =
        yagl_process_find_thread(ps, id);

    if (ts) {
        bool is_last;

        QLIST_REMOVE(ts, entry);

        is_last = QLIST_EMPTY(&ps->threads);

        yagl_thread_state_destroy(ts, is_last);
    }
}

bool yagl_process_has_threads(struct yagl_process_state *ps)
{
    return !QLIST_EMPTY(&ps->threads);
}
