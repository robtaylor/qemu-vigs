#ifndef _QEMU_YAGL_EGL_OFFSCREEN_IMAGE_H
#define _QEMU_YAGL_EGL_OFFSCREEN_IMAGE_H

#include "yagl_eglb_image.h"
#include "qemu-thread.h"

struct yagl_egl_offscreen_display;

struct yagl_egl_offscreen_image
{
    struct yagl_eglb_image base;

    /*
     * To serialize 'update_offscreen'.
     */
    QemuMutex update_mtx;
};

struct yagl_egl_offscreen_image
    *yagl_egl_offscreen_image_create(struct yagl_egl_offscreen_display *dpy,
                                     yagl_winsys_id buffer);

#endif
