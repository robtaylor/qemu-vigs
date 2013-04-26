#ifndef _QEMU_VIGS_GL_BACKEND_H
#define _QEMU_VIGS_GL_BACKEND_H

#include "vigs_types.h"
#include "vigs_backend.h"
#include "vigs_vector.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include "winsys_gl.h"

struct vigs_gl_backend
{
    struct vigs_backend base;

    struct winsys_gl_info ws_info;

    bool (*has_current)(struct vigs_gl_backend */*gl_backend*/);

    bool (*make_current)(struct vigs_gl_backend */*gl_backend*/,
                         bool /*enable*/);

    /*
     * Mandatory GL functions and extensions.
     * @{
     */

    void (GLAPIENTRY *GenTextures)(GLsizei n, GLuint *textures);
    void (GLAPIENTRY *DeleteTextures)(GLsizei n, const GLuint *textures);
    void (GLAPIENTRY *BindTexture)(GLenum target, GLuint texture);
    void (GLAPIENTRY *Begin)(GLenum mode);
    void (GLAPIENTRY *End)(void);
    void (GLAPIENTRY *CullFace)(GLenum mode);
    void (GLAPIENTRY *TexParameterf)(GLenum target, GLenum pname, GLfloat param);
    void (GLAPIENTRY *TexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
    void (GLAPIENTRY *TexParameteri)(GLenum target, GLenum pname, GLint param);
    void (GLAPIENTRY *TexParameteriv)(GLenum target, GLenum pname, const GLint *params);
    void (GLAPIENTRY *TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (GLAPIENTRY *TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    void (GLAPIENTRY *TexEnvf)(GLenum target, GLenum pname, GLfloat param);
    void (GLAPIENTRY *TexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
    void (GLAPIENTRY *TexEnvi)(GLenum target, GLenum pname, GLint param);
    void (GLAPIENTRY *TexEnviv)(GLenum target, GLenum pname, const GLint *params);
    void (GLAPIENTRY *Clear)(GLbitfield mask);
    void (GLAPIENTRY *ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void (GLAPIENTRY *Disable)(GLenum cap);
    void (GLAPIENTRY *Enable)(GLenum cap);
    void (GLAPIENTRY *Finish)(void);
    void (GLAPIENTRY *Flush)(void);
    void (GLAPIENTRY *PixelStorei)(GLenum pname, GLint param);
    void (GLAPIENTRY *ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
    void (GLAPIENTRY *Viewport)(GLint x, GLint y, GLsizei width, GLsizei height);
    void (GLAPIENTRY *GenFramebuffers)(GLsizei n, GLuint *framebuffers);
    void (GLAPIENTRY *GenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
    void (GLAPIENTRY *DeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
    void (GLAPIENTRY *DeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
    void (GLAPIENTRY *BindFramebuffer)(GLenum target, GLuint framebuffer);
    void (GLAPIENTRY *BindRenderbuffer)(GLenum target, GLuint renderbuffer);
    void (GLAPIENTRY *RenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void (GLAPIENTRY *FramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void (GLAPIENTRY *FramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void (GLAPIENTRY *GetIntegerv)(GLenum pname, GLint *params);
    const GLubyte *(GLAPIENTRY *GetString)(GLenum name);
    void (GLAPIENTRY *LoadIdentity)(void);
    void (GLAPIENTRY *MatrixMode)(GLenum mode);
    void (GLAPIENTRY *Ortho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
    void (GLAPIENTRY *EnableClientState)(GLenum array);
    void (GLAPIENTRY *DisableClientState)(GLenum array);
    void (GLAPIENTRY *Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void (GLAPIENTRY *TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void (GLAPIENTRY *VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void (GLAPIENTRY *DrawArrays)(GLenum mode, GLint first, GLsizei count);
    void (GLAPIENTRY *Color4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
    void (GLAPIENTRY *RasterPos2f)(GLfloat x, GLfloat y);
    void (GLAPIENTRY *PixelZoom)(GLfloat xfactor, GLfloat yfactor);
    void (GLAPIENTRY *DrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *data);
    void (GLAPIENTRY *BlendFunc)(GLenum sfactor, GLenum dfactor);
    void (GLAPIENTRY *CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
    void (GLAPIENTRY *BlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

    /*
     * @}
     */

    /*
     * General purpose vectors.
     * @{
     */

    struct vigs_vector v1;
    struct vigs_vector v2;

    /*
     * @}
     */
};

bool vigs_gl_backend_init(struct vigs_gl_backend *gl_backend);

void vigs_gl_backend_cleanup(struct vigs_gl_backend *gl_backend);

#endif
