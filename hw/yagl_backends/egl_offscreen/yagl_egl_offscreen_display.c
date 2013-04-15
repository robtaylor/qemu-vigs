#include "yagl_egl_offscreen_display.h"
#include "yagl_egl_offscreen_context.h"
#include "yagl_egl_offscreen_surface.h"
#include "yagl_egl_offscreen_image.h"
#include "yagl_egl_offscreen_ts.h"
#include "yagl_egl_offscreen.h"
#include "yagl_egl_native_config.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"

YAGL_DECLARE_TLS(struct yagl_egl_offscreen_ts*, egl_offscreen_ts);

static struct yagl_egl_native_config
    *yagl_egl_offscreen_display_config_enum(struct yagl_eglb_display *dpy,
                                            int *num_configs)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)dpy->backend;
    struct yagl_egl_native_config *native_configs;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_display_config_enum,
                        "dpy = %p", dpy);

    native_configs =
        egl_offscreen->driver->config_enum(egl_offscreen->driver,
                                           egl_offscreen_dpy->native_dpy,
                                           num_configs);

    YAGL_LOG_FUNC_EXIT(NULL);

    return native_configs;
}

static void yagl_egl_offscreen_display_config_cleanup(struct yagl_eglb_display *dpy,
    const struct yagl_egl_native_config *cfg)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_display_config_cleanup,
                        "dpy = %p, cfg = %d",
                        dpy,
                        cfg->config_id);

    egl_offscreen->driver->config_cleanup(egl_offscreen->driver,
                                          egl_offscreen_dpy->native_dpy,
                                          cfg);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_eglb_context
    *yagl_egl_offscreen_display_create_context(struct yagl_eglb_display *dpy,
                                               const struct yagl_egl_native_config *cfg,
                                               struct yagl_client_context *client_ctx,
                                               struct yagl_eglb_context *share_context)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_context *ctx =
        yagl_egl_offscreen_context_create(egl_offscreen_dpy,
                                          cfg,
                                          client_ctx,
                                          (struct yagl_egl_offscreen_context*)share_context);

    return ctx ? &ctx->base : NULL;
}

static struct yagl_eglb_surface
    *yagl_egl_offscreen_display_create_surface(struct yagl_eglb_display *dpy,
                                               const struct yagl_egl_native_config *cfg,
                                               EGLenum type,
                                               const void *attribs,
                                               uint32_t width,
                                               uint32_t height,
                                               uint32_t bpp,
                                               target_ulong pixels)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_surface *sfc =
        yagl_egl_offscreen_surface_create(egl_offscreen_dpy,
                                          cfg,
                                          type,
                                          attribs,
                                          width,
                                          height,
                                          bpp,
                                          pixels);

    return sfc ? &sfc->base : NULL;
}

static struct yagl_eglb_image
    *yagl_egl_offscreen_display_create_image(struct yagl_eglb_display *dpy)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen_image *image =
        yagl_egl_offscreen_image_create(egl_offscreen_dpy);

    return image ? &image->base : NULL;
}

static void yagl_egl_offscreen_display_destroy(struct yagl_eglb_display *dpy)
{
    struct yagl_egl_offscreen_display *egl_offscreen_dpy =
        (struct yagl_egl_offscreen_display*)dpy;
    struct yagl_egl_offscreen *egl_offscreen =
        (struct yagl_egl_offscreen*)dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_display_destroy,
                        "dpy = %p", dpy);

    egl_offscreen->driver->display_close(egl_offscreen->driver,
                                         egl_offscreen_dpy->native_dpy);

    yagl_eglb_display_cleanup(dpy);

    g_free(egl_offscreen_dpy);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_offscreen_display
    *yagl_egl_offscreen_display_create(struct yagl_egl_offscreen *egl_offscreen)
{
    struct yagl_egl_offscreen_display *dpy;
    EGLNativeDisplayType native_dpy;

    YAGL_LOG_FUNC_ENTER(yagl_egl_offscreen_display_create, NULL);

    native_dpy = egl_offscreen->driver->display_open(egl_offscreen->driver);

    if (!native_dpy) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    dpy = g_malloc0(sizeof(*dpy));

    yagl_eglb_display_init(&dpy->base, &egl_offscreen->base);

    dpy->base.config_enum = &yagl_egl_offscreen_display_config_enum;
    dpy->base.config_cleanup = &yagl_egl_offscreen_display_config_cleanup;
    dpy->base.create_context = &yagl_egl_offscreen_display_create_context;
    dpy->base.create_offscreen_surface = &yagl_egl_offscreen_display_create_surface;
    dpy->base.create_image = &yagl_egl_offscreen_display_create_image;
    dpy->base.destroy = &yagl_egl_offscreen_display_destroy;

    dpy->native_dpy = native_dpy;

    YAGL_LOG_FUNC_EXIT("%p", dpy);

    return dpy;
}
