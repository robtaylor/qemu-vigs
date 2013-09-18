/*
 * Generated by gen-yagl-calls.py, do not modify!
 */
#include "yagl_egl_calls.h"
#include "yagl_host_egl_calls.h"
#include "yagl_transport_egl.h"
#include "yagl_thread.h"
#include "yagl_process.h"
#include "yagl_log.h"

/*
 * eglGetError dispatcher. id = 1
 */
static bool yagl_func_eglGetError(struct yagl_transport *t)
{
    EGLint *retval;
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT0(eglGetError);
    *retval = yagl_host_eglGetError();
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLint, *retval);

    return true;
}

/*
 * eglGetDisplay dispatcher. id = 2
 */
static bool yagl_func_eglGetDisplay(struct yagl_transport *t)
{
    uint32_t display_id;
    yagl_host_handle *retval;
    display_id = yagl_transport_get_out_uint32_t(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT1(eglGetDisplay, uint32_t, display_id);
    *retval = yagl_host_eglGetDisplay(display_id);
    YAGL_LOG_FUNC_EXIT_SPLIT(yagl_host_handle, *retval);

    return true;
}

/*
 * eglInitialize dispatcher. id = 3
 */
static bool yagl_func_eglInitialize(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    EGLint *major;
    EGLint *minor;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    yagl_transport_get_in_arg(t, (void**)&major);
    yagl_transport_get_in_arg(t, (void**)&minor);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT3(eglInitialize, yagl_host_handle, void*, void*, dpy, major, minor);
    *retval = yagl_host_eglInitialize(dpy, major, minor);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglTerminate dispatcher. id = 4
 */
static bool yagl_func_eglTerminate(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT1(eglTerminate, yagl_host_handle, dpy);
    *retval = yagl_host_eglTerminate(dpy);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglGetConfigs dispatcher. id = 5
 */
static bool yagl_func_eglGetConfigs(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle *configs;
    int32_t configs_maxcount;
    int32_t *configs_count;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    if (!yagl_transport_get_in_array(t, sizeof(yagl_host_handle), (void**)&configs, &configs_maxcount, &configs_count)) {
        return false;
    }
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT2(eglGetConfigs, yagl_host_handle, void*, dpy, configs);
    *configs_count = 0;
    *retval = yagl_host_eglGetConfigs(dpy, configs, configs_maxcount, configs_count);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglChooseConfig dispatcher. id = 6
 */
static bool yagl_func_eglChooseConfig(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    const EGLint *attrib_list;
    int32_t attrib_list_count;
    yagl_host_handle *configs;
    int32_t configs_maxcount;
    int32_t *configs_count;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    if (!yagl_transport_get_out_array(t, sizeof(EGLint), (const void**)&attrib_list, &attrib_list_count)) {
        return false;
    }
    if (!yagl_transport_get_in_array(t, sizeof(yagl_host_handle), (void**)&configs, &configs_maxcount, &configs_count)) {
        return false;
    }
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT3(eglChooseConfig, yagl_host_handle, void*, void*, dpy, attrib_list, configs);
    *configs_count = 0;
    *retval = yagl_host_eglChooseConfig(dpy, attrib_list, attrib_list_count, configs, configs_maxcount, configs_count);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglGetConfigAttrib dispatcher. id = 7
 */
static bool yagl_func_eglGetConfigAttrib(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle config;
    EGLint attribute;
    EGLint *value;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    config = yagl_transport_get_out_yagl_host_handle(t);
    attribute = yagl_transport_get_out_EGLint(t);
    yagl_transport_get_in_arg(t, (void**)&value);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT4(eglGetConfigAttrib, yagl_host_handle, yagl_host_handle, EGLint, void*, dpy, config, attribute, value);
    *retval = yagl_host_eglGetConfigAttrib(dpy, config, attribute, value);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglDestroySurface dispatcher. id = 8
 */
static bool yagl_func_eglDestroySurface(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle surface;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    surface = yagl_transport_get_out_yagl_host_handle(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT2(eglDestroySurface, yagl_host_handle, yagl_host_handle, dpy, surface);
    *retval = yagl_host_eglDestroySurface(dpy, surface);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglQuerySurface dispatcher. id = 9
 */
static bool yagl_func_eglQuerySurface(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle surface;
    EGLint attribute;
    EGLint *value;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    surface = yagl_transport_get_out_yagl_host_handle(t);
    attribute = yagl_transport_get_out_EGLint(t);
    yagl_transport_get_in_arg(t, (void**)&value);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT4(eglQuerySurface, yagl_host_handle, yagl_host_handle, EGLint, void*, dpy, surface, attribute, value);
    *retval = yagl_host_eglQuerySurface(dpy, surface, attribute, value);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglBindAPI dispatcher. id = 10
 */
static bool yagl_func_eglBindAPI(struct yagl_transport *t)
{
    EGLenum api;
    EGLBoolean *retval;
    api = yagl_transport_get_out_EGLenum(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT1(eglBindAPI, EGLenum, api);
    *retval = yagl_host_eglBindAPI(api);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglWaitClient dispatcher. id = 11
 */
static bool yagl_func_eglWaitClient(struct yagl_transport *t)
{
    EGLBoolean *retval;
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT0(eglWaitClient);
    *retval = yagl_host_eglWaitClient();
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglReleaseThread dispatcher. id = 12
 */
static bool yagl_func_eglReleaseThread(struct yagl_transport *t)
{
    EGLBoolean *retval;
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT0(eglReleaseThread);
    *retval = yagl_host_eglReleaseThread();
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglSurfaceAttrib dispatcher. id = 13
 */
static bool yagl_func_eglSurfaceAttrib(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle surface;
    EGLint attribute;
    EGLint value;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    surface = yagl_transport_get_out_yagl_host_handle(t);
    attribute = yagl_transport_get_out_EGLint(t);
    value = yagl_transport_get_out_EGLint(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT4(eglSurfaceAttrib, yagl_host_handle, yagl_host_handle, EGLint, EGLint, dpy, surface, attribute, value);
    *retval = yagl_host_eglSurfaceAttrib(dpy, surface, attribute, value);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglBindTexImage dispatcher. id = 14
 */
static bool yagl_func_eglBindTexImage(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle surface;
    EGLint buffer;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    surface = yagl_transport_get_out_yagl_host_handle(t);
    buffer = yagl_transport_get_out_EGLint(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT3(eglBindTexImage, yagl_host_handle, yagl_host_handle, EGLint, dpy, surface, buffer);
    *retval = yagl_host_eglBindTexImage(dpy, surface, buffer);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglReleaseTexImage dispatcher. id = 15
 */
static bool yagl_func_eglReleaseTexImage(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle surface;
    EGLint buffer;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    surface = yagl_transport_get_out_yagl_host_handle(t);
    buffer = yagl_transport_get_out_EGLint(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT3(eglReleaseTexImage, yagl_host_handle, yagl_host_handle, EGLint, dpy, surface, buffer);
    *retval = yagl_host_eglReleaseTexImage(dpy, surface, buffer);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglCreateContext dispatcher. id = 16
 */
static bool yagl_func_eglCreateContext(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle config;
    yagl_host_handle share_context;
    const EGLint *attrib_list;
    int32_t attrib_list_count;
    yagl_host_handle *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    config = yagl_transport_get_out_yagl_host_handle(t);
    share_context = yagl_transport_get_out_yagl_host_handle(t);
    if (!yagl_transport_get_out_array(t, sizeof(EGLint), (const void**)&attrib_list, &attrib_list_count)) {
        return false;
    }
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT4(eglCreateContext, yagl_host_handle, yagl_host_handle, yagl_host_handle, void*, dpy, config, share_context, attrib_list);
    *retval = yagl_host_eglCreateContext(dpy, config, share_context, attrib_list, attrib_list_count);
    YAGL_LOG_FUNC_EXIT_SPLIT(yagl_host_handle, *retval);

    return true;
}

/*
 * eglDestroyContext dispatcher. id = 17
 */
static bool yagl_func_eglDestroyContext(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle ctx;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    ctx = yagl_transport_get_out_yagl_host_handle(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT2(eglDestroyContext, yagl_host_handle, yagl_host_handle, dpy, ctx);
    *retval = yagl_host_eglDestroyContext(dpy, ctx);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglMakeCurrent dispatcher. id = 18
 */
static bool yagl_func_eglMakeCurrent(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle draw;
    yagl_host_handle read;
    yagl_host_handle ctx;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    draw = yagl_transport_get_out_yagl_host_handle(t);
    read = yagl_transport_get_out_yagl_host_handle(t);
    ctx = yagl_transport_get_out_yagl_host_handle(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT4(eglMakeCurrent, yagl_host_handle, yagl_host_handle, yagl_host_handle, yagl_host_handle, dpy, draw, read, ctx);
    *retval = yagl_host_eglMakeCurrent(dpy, draw, read, ctx);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglQueryContext dispatcher. id = 19
 */
static bool yagl_func_eglQueryContext(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle ctx;
    EGLint attribute;
    EGLint *value;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    ctx = yagl_transport_get_out_yagl_host_handle(t);
    attribute = yagl_transport_get_out_EGLint(t);
    yagl_transport_get_in_arg(t, (void**)&value);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT4(eglQueryContext, yagl_host_handle, yagl_host_handle, EGLint, void*, dpy, ctx, attribute, value);
    *retval = yagl_host_eglQueryContext(dpy, ctx, attribute, value);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglSwapBuffers dispatcher. id = 20
 */
static bool yagl_func_eglSwapBuffers(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle surface;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    surface = yagl_transport_get_out_yagl_host_handle(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT2(eglSwapBuffers, yagl_host_handle, yagl_host_handle, dpy, surface);
    *retval = yagl_host_eglSwapBuffers(dpy, surface);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglCopyBuffers dispatcher. id = 21
 */
static bool yagl_func_eglCopyBuffers(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle surface;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    surface = yagl_transport_get_out_yagl_host_handle(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT2(eglCopyBuffers, yagl_host_handle, yagl_host_handle, dpy, surface);
    *retval = yagl_host_eglCopyBuffers(dpy, surface);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglCreateImageKHR dispatcher. id = 22
 */
static bool yagl_func_eglCreateImageKHR(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle ctx;
    EGLenum target;
    yagl_winsys_id buffer;
    const EGLint *attrib_list;
    int32_t attrib_list_count;
    yagl_host_handle *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    ctx = yagl_transport_get_out_yagl_host_handle(t);
    target = yagl_transport_get_out_EGLenum(t);
    buffer = yagl_transport_get_out_yagl_winsys_id(t);
    if (!yagl_transport_get_out_array(t, sizeof(EGLint), (const void**)&attrib_list, &attrib_list_count)) {
        return false;
    }
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT5(eglCreateImageKHR, yagl_host_handle, yagl_host_handle, EGLenum, yagl_winsys_id, void*, dpy, ctx, target, buffer, attrib_list);
    *retval = yagl_host_eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list, attrib_list_count);
    YAGL_LOG_FUNC_EXIT_SPLIT(yagl_host_handle, *retval);

    return true;
}

/*
 * eglDestroyImageKHR dispatcher. id = 23
 */
static bool yagl_func_eglDestroyImageKHR(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle image;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    image = yagl_transport_get_out_yagl_host_handle(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT2(eglDestroyImageKHR, yagl_host_handle, yagl_host_handle, dpy, image);
    *retval = yagl_host_eglDestroyImageKHR(dpy, image);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglCreateWindowSurfaceOffscreenYAGL dispatcher. id = 24
 */
static bool yagl_func_eglCreateWindowSurfaceOffscreenYAGL(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle config;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    target_ulong pixels;
    const EGLint *attrib_list;
    int32_t attrib_list_count;
    yagl_host_handle *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    config = yagl_transport_get_out_yagl_host_handle(t);
    width = yagl_transport_get_out_uint32_t(t);
    height = yagl_transport_get_out_uint32_t(t);
    bpp = yagl_transport_get_out_uint32_t(t);
    pixels = yagl_transport_get_out_va(t);
    if (!yagl_transport_get_out_array(t, sizeof(EGLint), (const void**)&attrib_list, &attrib_list_count)) {
        return false;
    }
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT7(eglCreateWindowSurfaceOffscreenYAGL, yagl_host_handle, yagl_host_handle, uint32_t, uint32_t, uint32_t, target_ulong, void*, dpy, config, width, height, bpp, pixels, attrib_list);
    *retval = yagl_host_eglCreateWindowSurfaceOffscreenYAGL(dpy, config, width, height, bpp, pixels, attrib_list, attrib_list_count);
    YAGL_LOG_FUNC_EXIT_SPLIT(yagl_host_handle, *retval);

    return true;
}

/*
 * eglCreatePbufferSurfaceOffscreenYAGL dispatcher. id = 25
 */
static bool yagl_func_eglCreatePbufferSurfaceOffscreenYAGL(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle config;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    target_ulong pixels;
    const EGLint *attrib_list;
    int32_t attrib_list_count;
    yagl_host_handle *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    config = yagl_transport_get_out_yagl_host_handle(t);
    width = yagl_transport_get_out_uint32_t(t);
    height = yagl_transport_get_out_uint32_t(t);
    bpp = yagl_transport_get_out_uint32_t(t);
    pixels = yagl_transport_get_out_va(t);
    if (!yagl_transport_get_out_array(t, sizeof(EGLint), (const void**)&attrib_list, &attrib_list_count)) {
        return false;
    }
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT7(eglCreatePbufferSurfaceOffscreenYAGL, yagl_host_handle, yagl_host_handle, uint32_t, uint32_t, uint32_t, target_ulong, void*, dpy, config, width, height, bpp, pixels, attrib_list);
    *retval = yagl_host_eglCreatePbufferSurfaceOffscreenYAGL(dpy, config, width, height, bpp, pixels, attrib_list, attrib_list_count);
    YAGL_LOG_FUNC_EXIT_SPLIT(yagl_host_handle, *retval);

    return true;
}

/*
 * eglCreatePixmapSurfaceOffscreenYAGL dispatcher. id = 26
 */
static bool yagl_func_eglCreatePixmapSurfaceOffscreenYAGL(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle config;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    target_ulong pixels;
    const EGLint *attrib_list;
    int32_t attrib_list_count;
    yagl_host_handle *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    config = yagl_transport_get_out_yagl_host_handle(t);
    width = yagl_transport_get_out_uint32_t(t);
    height = yagl_transport_get_out_uint32_t(t);
    bpp = yagl_transport_get_out_uint32_t(t);
    pixels = yagl_transport_get_out_va(t);
    if (!yagl_transport_get_out_array(t, sizeof(EGLint), (const void**)&attrib_list, &attrib_list_count)) {
        return false;
    }
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT7(eglCreatePixmapSurfaceOffscreenYAGL, yagl_host_handle, yagl_host_handle, uint32_t, uint32_t, uint32_t, target_ulong, void*, dpy, config, width, height, bpp, pixels, attrib_list);
    *retval = yagl_host_eglCreatePixmapSurfaceOffscreenYAGL(dpy, config, width, height, bpp, pixels, attrib_list, attrib_list_count);
    YAGL_LOG_FUNC_EXIT_SPLIT(yagl_host_handle, *retval);

    return true;
}

/*
 * eglResizeOffscreenSurfaceYAGL dispatcher. id = 27
 */
static bool yagl_func_eglResizeOffscreenSurfaceYAGL(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle surface;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    target_ulong pixels;
    EGLBoolean *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    surface = yagl_transport_get_out_yagl_host_handle(t);
    width = yagl_transport_get_out_uint32_t(t);
    height = yagl_transport_get_out_uint32_t(t);
    bpp = yagl_transport_get_out_uint32_t(t);
    pixels = yagl_transport_get_out_va(t);
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT6(eglResizeOffscreenSurfaceYAGL, yagl_host_handle, yagl_host_handle, uint32_t, uint32_t, uint32_t, target_ulong, dpy, surface, width, height, bpp, pixels);
    *retval = yagl_host_eglResizeOffscreenSurfaceYAGL(dpy, surface, width, height, bpp, pixels);
    YAGL_LOG_FUNC_EXIT_SPLIT(EGLBoolean, *retval);

    return true;
}

/*
 * eglUpdateOffscreenImageYAGL dispatcher. id = 28
 */
static bool yagl_func_eglUpdateOffscreenImageYAGL(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle image;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    const void *pixels;
    int32_t pixels_count;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    image = yagl_transport_get_out_yagl_host_handle(t);
    width = yagl_transport_get_out_uint32_t(t);
    height = yagl_transport_get_out_uint32_t(t);
    bpp = yagl_transport_get_out_uint32_t(t);
    if (!yagl_transport_get_out_array(t, 1, (const void**)&pixels, &pixels_count)) {
        return false;
    }
    YAGL_LOG_FUNC_ENTER_SPLIT6(eglUpdateOffscreenImageYAGL, yagl_host_handle, yagl_host_handle, uint32_t, uint32_t, uint32_t, void*, dpy, image, width, height, bpp, pixels);
    (void)yagl_host_eglUpdateOffscreenImageYAGL(dpy, image, width, height, bpp, pixels, pixels_count);
    YAGL_LOG_FUNC_EXIT(NULL);

    return true;
}

/*
 * eglCreateWindowSurfaceOnscreenYAGL dispatcher. id = 29
 */
static bool yagl_func_eglCreateWindowSurfaceOnscreenYAGL(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle config;
    yagl_winsys_id win;
    const EGLint *attrib_list;
    int32_t attrib_list_count;
    yagl_host_handle *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    config = yagl_transport_get_out_yagl_host_handle(t);
    win = yagl_transport_get_out_yagl_winsys_id(t);
    if (!yagl_transport_get_out_array(t, sizeof(EGLint), (const void**)&attrib_list, &attrib_list_count)) {
        return false;
    }
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT4(eglCreateWindowSurfaceOnscreenYAGL, yagl_host_handle, yagl_host_handle, yagl_winsys_id, void*, dpy, config, win, attrib_list);
    *retval = yagl_host_eglCreateWindowSurfaceOnscreenYAGL(dpy, config, win, attrib_list, attrib_list_count);
    YAGL_LOG_FUNC_EXIT_SPLIT(yagl_host_handle, *retval);

    return true;
}

/*
 * eglCreatePbufferSurfaceOnscreenYAGL dispatcher. id = 30
 */
static bool yagl_func_eglCreatePbufferSurfaceOnscreenYAGL(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle config;
    yagl_winsys_id buffer;
    const EGLint *attrib_list;
    int32_t attrib_list_count;
    yagl_host_handle *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    config = yagl_transport_get_out_yagl_host_handle(t);
    buffer = yagl_transport_get_out_yagl_winsys_id(t);
    if (!yagl_transport_get_out_array(t, sizeof(EGLint), (const void**)&attrib_list, &attrib_list_count)) {
        return false;
    }
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT4(eglCreatePbufferSurfaceOnscreenYAGL, yagl_host_handle, yagl_host_handle, yagl_winsys_id, void*, dpy, config, buffer, attrib_list);
    *retval = yagl_host_eglCreatePbufferSurfaceOnscreenYAGL(dpy, config, buffer, attrib_list, attrib_list_count);
    YAGL_LOG_FUNC_EXIT_SPLIT(yagl_host_handle, *retval);

    return true;
}

/*
 * eglCreatePixmapSurfaceOnscreenYAGL dispatcher. id = 31
 */
static bool yagl_func_eglCreatePixmapSurfaceOnscreenYAGL(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle config;
    yagl_winsys_id pixmap;
    const EGLint *attrib_list;
    int32_t attrib_list_count;
    yagl_host_handle *retval;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    config = yagl_transport_get_out_yagl_host_handle(t);
    pixmap = yagl_transport_get_out_yagl_winsys_id(t);
    if (!yagl_transport_get_out_array(t, sizeof(EGLint), (const void**)&attrib_list, &attrib_list_count)) {
        return false;
    }
    yagl_transport_get_in_arg(t, (void**)&retval);
    YAGL_LOG_FUNC_ENTER_SPLIT4(eglCreatePixmapSurfaceOnscreenYAGL, yagl_host_handle, yagl_host_handle, yagl_winsys_id, void*, dpy, config, pixmap, attrib_list);
    *retval = yagl_host_eglCreatePixmapSurfaceOnscreenYAGL(dpy, config, pixmap, attrib_list, attrib_list_count);
    YAGL_LOG_FUNC_EXIT_SPLIT(yagl_host_handle, *retval);

    return true;
}

/*
 * eglInvalidateOnscreenSurfaceYAGL dispatcher. id = 32
 */
static bool yagl_func_eglInvalidateOnscreenSurfaceYAGL(struct yagl_transport *t)
{
    yagl_host_handle dpy;
    yagl_host_handle surface;
    yagl_winsys_id buffer;
    dpy = yagl_transport_get_out_yagl_host_handle(t);
    surface = yagl_transport_get_out_yagl_host_handle(t);
    buffer = yagl_transport_get_out_yagl_winsys_id(t);
    YAGL_LOG_FUNC_ENTER_SPLIT3(eglInvalidateOnscreenSurfaceYAGL, yagl_host_handle, yagl_host_handle, yagl_winsys_id, dpy, surface, buffer);
    (void)yagl_host_eglInvalidateOnscreenSurfaceYAGL(dpy, surface, buffer);
    YAGL_LOG_FUNC_EXIT(NULL);

    return true;
}

const uint32_t yagl_egl_api_num_funcs = 32;

yagl_api_func yagl_egl_api_funcs[] = {
    &yagl_func_eglGetError,
    &yagl_func_eglGetDisplay,
    &yagl_func_eglInitialize,
    &yagl_func_eglTerminate,
    &yagl_func_eglGetConfigs,
    &yagl_func_eglChooseConfig,
    &yagl_func_eglGetConfigAttrib,
    &yagl_func_eglDestroySurface,
    &yagl_func_eglQuerySurface,
    &yagl_func_eglBindAPI,
    &yagl_func_eglWaitClient,
    &yagl_func_eglReleaseThread,
    &yagl_func_eglSurfaceAttrib,
    &yagl_func_eglBindTexImage,
    &yagl_func_eglReleaseTexImage,
    &yagl_func_eglCreateContext,
    &yagl_func_eglDestroyContext,
    &yagl_func_eglMakeCurrent,
    &yagl_func_eglQueryContext,
    &yagl_func_eglSwapBuffers,
    &yagl_func_eglCopyBuffers,
    &yagl_func_eglCreateImageKHR,
    &yagl_func_eglDestroyImageKHR,
    &yagl_func_eglCreateWindowSurfaceOffscreenYAGL,
    &yagl_func_eglCreatePbufferSurfaceOffscreenYAGL,
    &yagl_func_eglCreatePixmapSurfaceOffscreenYAGL,
    &yagl_func_eglResizeOffscreenSurfaceYAGL,
    &yagl_func_eglUpdateOffscreenImageYAGL,
    &yagl_func_eglCreateWindowSurfaceOnscreenYAGL,
    &yagl_func_eglCreatePbufferSurfaceOnscreenYAGL,
    &yagl_func_eglCreatePixmapSurfaceOnscreenYAGL,
    &yagl_func_eglInvalidateOnscreenSurfaceYAGL,
};
