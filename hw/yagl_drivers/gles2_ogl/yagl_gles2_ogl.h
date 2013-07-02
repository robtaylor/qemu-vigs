#ifndef _QEMU_YAGL_GLES2_OGL_H
#define _QEMU_YAGL_GLES2_OGL_H

#include "yagl_types.h"
#include "yagl_dyn_lib.h"

struct yagl_gles2_driver;

struct yagl_gles2_driver *yagl_gles2_ogl_create(struct yagl_dyn_lib *dyn_lib);

#endif
