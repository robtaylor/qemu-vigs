#include "yagl_egl_onscreen_display.h"
#include <GL/gl.h>
#include "yagl_egl_onscreen_context.h"
#include "yagl_egl_onscreen_surface.h"
#include "yagl_egl_onscreen_image.h"
#include "yagl_egl_onscreen.h"
#include "yagl_egl_native_config.h"
#include "yagl_log.h"
#include "yagl_tls.h"
#include "yagl_process.h"
#include "yagl_thread.h"

static struct yagl_egl_native_config
    *yagl_egl_onscreen_display_config_enum(struct yagl_eglb_display *dpy,
                                            int *num_configs)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->backend;
    struct yagl_egl_native_config *native_configs;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_display_config_enum,
                        "dpy = %p", dpy);

    native_configs =
        egl_onscreen->egl_driver->config_enum(egl_onscreen->egl_driver,
                                              egl_onscreen_dpy->native_dpy,
                                              num_configs);

    YAGL_LOG_FUNC_EXIT(NULL);

    return native_configs;
}

static void yagl_egl_onscreen_display_config_cleanup(struct yagl_eglb_display *dpy,
    const struct yagl_egl_native_config *cfg)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_display_config_cleanup,
                        "dpy = %p, cfg = %d",
                        dpy,
                        cfg->config_id);

    egl_onscreen->egl_driver->config_cleanup(egl_onscreen->egl_driver,
                                             egl_onscreen_dpy->native_dpy,
                                             cfg);

    YAGL_LOG_FUNC_EXIT(NULL);
}

static struct yagl_eglb_context
    *yagl_egl_onscreen_display_create_context(struct yagl_eglb_display *dpy,
                                              const struct yagl_egl_native_config *cfg,
                                              struct yagl_client_context *client_ctx,
                                              struct yagl_eglb_context *share_context)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen_context *ctx =
        yagl_egl_onscreen_context_create(egl_onscreen_dpy,
                                         cfg,
                                         client_ctx,
                                         (struct yagl_egl_onscreen_context*)share_context);

    return ctx ? &ctx->base : NULL;
}

static struct yagl_eglb_surface
    *yagl_egl_onscreen_display_create_window_surface(struct yagl_eglb_display *dpy,
                                                     const struct yagl_egl_native_config *cfg,
                                                     const struct yagl_egl_window_attribs *attribs,
                                                     yagl_winsys_id id)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen_surface *sfc =
        yagl_egl_onscreen_surface_create_window(egl_onscreen_dpy,
                                                cfg,
                                                attribs,
                                                id);

    return sfc ? &sfc->base : NULL;
}

static struct yagl_eglb_surface
    *yagl_egl_onscreen_display_create_pixmap_surface(struct yagl_eglb_display *dpy,
                                                     const struct yagl_egl_native_config *cfg,
                                                     const struct yagl_egl_pixmap_attribs *attribs,
                                                     yagl_winsys_id id)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen_surface *sfc =
        yagl_egl_onscreen_surface_create_pixmap(egl_onscreen_dpy,
                                                cfg,
                                                attribs,
                                                id);

    return sfc ? &sfc->base : NULL;
}

static struct yagl_eglb_surface
    *yagl_egl_onscreen_display_create_pbuffer_surface(struct yagl_eglb_display *dpy,
                                                      const struct yagl_egl_native_config *cfg,
                                                      const struct yagl_egl_pbuffer_attribs *attribs,
                                                      uint32_t width,
                                                      uint32_t height)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen_surface *sfc =
        yagl_egl_onscreen_surface_create_pbuffer(egl_onscreen_dpy,
                                                 cfg,
                                                 attribs,
                                                 width,
                                                 height);

    return sfc ? &sfc->base : NULL;
}

static struct yagl_eglb_image
    *yagl_egl_onscreen_display_create_image(struct yagl_eglb_display *dpy,
                                            yagl_winsys_id buffer)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen_image *image =
        yagl_egl_onscreen_image_create(egl_onscreen_dpy, buffer);

    return image ? &image->base : NULL;
}

static void yagl_egl_onscreen_display_destroy(struct yagl_eglb_display *dpy)
{
    struct yagl_egl_onscreen_display *egl_onscreen_dpy =
        (struct yagl_egl_onscreen_display*)dpy;
    struct yagl_egl_onscreen *egl_onscreen =
        (struct yagl_egl_onscreen*)dpy->backend;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_display_destroy,
                        "dpy = %p", dpy);

    egl_onscreen->egl_driver->display_close(egl_onscreen->egl_driver,
                                            egl_onscreen_dpy->native_dpy);

    yagl_eglb_display_cleanup(dpy);

    g_free(egl_onscreen_dpy);

    YAGL_LOG_FUNC_EXIT(NULL);
}

struct yagl_egl_onscreen_display
    *yagl_egl_onscreen_display_create(struct yagl_egl_onscreen *egl_onscreen)
{
    struct yagl_egl_onscreen_display *dpy;
    EGLNativeDisplayType native_dpy;

    YAGL_LOG_FUNC_ENTER(yagl_egl_onscreen_display_create, NULL);

    native_dpy = egl_onscreen->egl_driver->display_open(egl_onscreen->egl_driver);

    if (!native_dpy) {
        YAGL_LOG_FUNC_EXIT(NULL);
        return NULL;
    }

    dpy = g_malloc0(sizeof(*dpy));

    yagl_eglb_display_init(&dpy->base, &egl_onscreen->base);

    dpy->base.config_enum = &yagl_egl_onscreen_display_config_enum;
    dpy->base.config_cleanup = &yagl_egl_onscreen_display_config_cleanup;
    dpy->base.create_context = &yagl_egl_onscreen_display_create_context;
    dpy->base.create_onscreen_window_surface = &yagl_egl_onscreen_display_create_window_surface;
    dpy->base.create_onscreen_pixmap_surface = &yagl_egl_onscreen_display_create_pixmap_surface;
    dpy->base.create_onscreen_pbuffer_surface = &yagl_egl_onscreen_display_create_pbuffer_surface;
    dpy->base.create_image = &yagl_egl_onscreen_display_create_image;
    dpy->base.destroy = &yagl_egl_onscreen_display_destroy;

    dpy->native_dpy = native_dpy;

    YAGL_LOG_FUNC_EXIT("%p", dpy);

    return dpy;
}
