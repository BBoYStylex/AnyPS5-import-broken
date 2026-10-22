#include <cstddef>
#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <SDL_egl.h>
#include <SDL_opengles2_khrplatform.h>
#include <SDL_opengles2_gl2platform.h>
#include <SDL_opengles2_gl2.h>

extern "C" {

void APS5_VABI glGetIntegerv_nid_postfix(GLenum pname, GLint *data) {
    (void)pname;
    (void)data;
    NotImplemented_nid_no_patch(__func__);
}

GLenum APS5_VABI glCheckFramebufferStatus_nid_postfix(GLenum target) {
    (void)target;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

GLuint APS5_VABI glCreateShader_nid_postfix(GLenum type) {
    (void)type;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

EGLBoolean APS5_VABI eglSurfaceAttrib_nid_postfix(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value) {
    (void)dpy;
    (void)surface;
    (void)attribute;
    (void)value;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

EGLContext APS5_VABI eglCreateContext_nid_postfix(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list) {
    (void)dpy;
    (void)config;
    (void)share_context;
    (void)attrib_list;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

EGLDisplay APS5_VABI eglGetCurrentDisplay_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glDrawArrays_nid_postfix(GLenum mode, GLint first, GLsizei count) {
    (void)mode;
    (void)first;
    (void)count;
    NotImplemented_nid_no_patch(__func__);
}

__eglMustCastToProperFunctionPointerType APS5_VABI eglGetProcAddress_nid_postfix(const char *procname) {
    (void)procname;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glStencilMaskSeparate_nid_postfix(GLenum face, GLuint mask) {
    (void)face;
    (void)mask;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glTexParameterf_nid_postfix(GLenum target, GLenum pname, GLfloat param) {
    (void)target;
    (void)pname;
    (void)param;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglSwapInterval_nid_postfix(EGLDisplay dpy, EGLint interval) {
    (void)dpy;
    (void)interval;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

GLboolean APS5_VABI glIsProgram_nid_postfix(GLuint program) {
    (void)program;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glShaderSource_nid_postfix(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {
    (void)shader;
    (void)count;
    (void)string;
    (void)length;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glRenderbufferStorage_nid_postfix(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    (void)target;
    (void)internalformat;
    (void)width;
    (void)height;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glFramebufferTexture2D_nid_postfix(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    (void)target;
    (void)attachment;
    (void)textarget;
    (void)texture;
    (void)level;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glBlendFunc_nid_postfix(GLenum sfactor, GLenum dfactor) {
    (void)sfactor;
    (void)dfactor;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glTexSubImage2D_nid_postfix(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    (void)target;
    (void)level;
    (void)xoffset;
    (void)yoffset;
    (void)width;
    (void)height;
    (void)format;
    (void)type;
    (void)pixels;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDeleteFramebuffers_nid_postfix(GLsizei n, const GLuint *framebuffers) {
    (void)n;
    (void)framebuffers;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glBlendEquationSeparate_nid_postfix(GLenum modeRGB, GLenum modeAlpha) {
    (void)modeRGB;
    (void)modeAlpha;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glFrontFace_nid_postfix(GLenum mode) {
    (void)mode;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGenBuffers_nid_postfix(GLsizei n, GLuint *buffers) {
    (void)n;
    (void)buffers;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglSwapBuffers_nid_postfix(EGLDisplay dpy, EGLSurface surface) {
    (void)dpy;
    (void)surface;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glTexImage2D_nid_postfix(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
    (void)target;
    (void)level;
    (void)internalformat;
    (void)width;
    (void)height;
    (void)border;
    (void)format;
    (void)type;
    (void)pixels;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglGetConfigAttrib_nid_postfix(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value) {
    (void)dpy;
    (void)config;
    (void)attribute;
    (void)value;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

EGLBoolean APS5_VABI eglQueryContext_nid_postfix(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value) {
    (void)dpy;
    (void)ctx;
    (void)attribute;
    (void)value;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glGenFramebuffers_nid_postfix(GLsizei n, GLuint *framebuffers) {
    (void)n;
    (void)framebuffers;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glTexParameteriv_nid_postfix(GLenum target, GLenum pname, const GLint *params) {
    (void)target;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glStencilFuncSeparate_nid_postfix(GLenum face, GLenum func, GLint ref, GLuint mask) {
    (void)face;
    (void)func;
    (void)ref;
    (void)mask;
    NotImplemented_nid_no_patch(__func__);
}

EGLContext APS5_VABI eglGetCurrentContext_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

EGLBoolean APS5_VABI eglDestroySurface_nid_postfix(EGLDisplay dpy, EGLSurface surface) {
    (void)dpy;
    (void)surface;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glReleaseShaderCompiler_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glBufferSubData_nid_postfix(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    (void)target;
    (void)offset;
    (void)size;
    (void)data;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glFinish_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetUniformiv_nid_postfix(GLuint program, GLint location, GLint *params) {
    (void)program;
    (void)location;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDepthRangef_nid_postfix(GLfloat n, GLfloat f) {
    (void)n;
    (void)f;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetAttachedShaders_nid_postfix(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) {
    (void)program;
    (void)maxCount;
    (void)count;
    (void)shaders;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDisableVertexAttribArray_nid_postfix(GLuint index) {
    (void)index;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDeleteRenderbuffers_nid_postfix(GLsizei n, const GLuint *renderbuffers) {
    (void)n;
    (void)renderbuffers;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglTerminate_nid_postfix(EGLDisplay dpy) {
    (void)dpy;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

GLboolean APS5_VABI glIsEnabled_nid_postfix(GLenum cap) {
    (void)cap;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

EGLBoolean APS5_VABI eglMakeCurrent_nid_postfix(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
    (void)dpy;
    (void)draw;
    (void)read;
    (void)ctx;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glGetShaderPrecisionFormat_nid_postfix(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) {
    (void)shadertype;
    (void)precisiontype;
    (void)range;
    (void)precision;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glUniform1fv_nid_postfix(GLint location, GLsizei count, const GLfloat *value) {
    (void)location;
    (void)count;
    (void)value;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glUniform1iv_nid_postfix(GLint location, GLsizei count, const GLint *value) {
    (void)location;
    (void)count;
    (void)value;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglBindTexImage_nid_postfix(EGLDisplay dpy, EGLSurface surface, EGLint buffer) {
    (void)dpy;
    (void)surface;
    (void)buffer;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glBlendFuncSeparate_nid_postfix(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    (void)sfactorRGB;
    (void)dfactorRGB;
    (void)sfactorAlpha;
    (void)dfactorAlpha;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDepthFunc_nid_postfix(GLenum func) {
    (void)func;
    NotImplemented_nid_no_patch(__func__);
}

EGLSurface APS5_VABI eglCreatePbufferSurface_nid_postfix(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list) {
    (void)dpy;
    (void)config;
    (void)attrib_list;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glPixelStorei_nid_postfix(GLenum pname, GLint param) {
    (void)pname;
    (void)param;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetFloatv_nid_postfix(GLenum pname, GLfloat *data) {
    (void)pname;
    (void)data;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glScissor_nid_postfix(GLint x, GLint y, GLsizei width, GLsizei height) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    NotImplemented_nid_no_patch(__func__);
}

EGLenum APS5_VABI eglQueryAPI_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glBlendEquation_nid_postfix(GLenum mode) {
    (void)mode;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glBindTexture_nid_postfix(GLenum target, GLuint texture) {
    (void)target;
    (void)texture;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglInitialize_nid_postfix(EGLDisplay dpy, EGLint *major, EGLint *minor) {
    (void)dpy;
    (void)major;
    (void)minor;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glClear_nid_postfix(GLbitfield mask) {
    (void)mask;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glCompressedTexImage2D_nid_postfix(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
    (void)target;
    (void)level;
    (void)internalformat;
    (void)width;
    (void)height;
    (void)border;
    (void)imageSize;
    (void)data;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glTexParameteri_nid_postfix(GLenum target, GLenum pname, GLint param) {
    (void)target;
    (void)pname;
    (void)param;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glSampleCoverage_nid_postfix(GLfloat value, GLboolean invert) {
    (void)value;
    (void)invert;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glPolygonOffset_nid_postfix(GLfloat factor, GLfloat units) {
    (void)factor;
    (void)units;
    NotImplemented_nid_no_patch(__func__);
}

EGLSurface APS5_VABI eglGetCurrentSurface_nid_postfix(EGLint readdraw) {
    (void)readdraw;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glCopyTexSubImage2D_nid_postfix(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    (void)target;
    (void)level;
    (void)xoffset;
    (void)yoffset;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glLinkProgram_nid_postfix(GLuint program) {
    (void)program;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGenTextures_nid_postfix(GLsizei n, GLuint *textures) {
    (void)n;
    (void)textures;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glUniform1f_nid_postfix(GLint location, GLfloat v0) {
    (void)location;
    (void)v0;
    NotImplemented_nid_no_patch(__func__);
}

EGLSurface APS5_VABI eglCreateWindowSurface_nid_postfix(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list) {
    (void)dpy;
    (void)config;
    (void)win;
    (void)attrib_list;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glClearColor_nid_postfix(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
    NotImplemented_nid_no_patch(__func__);
}

EGLint APS5_VABI eglGetError_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

EGLBoolean APS5_VABI eglWaitClient_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glClearStencil_nid_postfix(GLint s) {
    (void)s;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDetachShader_nid_postfix(GLuint program, GLuint shader) {
    (void)program;
    (void)shader;
    NotImplemented_nid_no_patch(__func__);
}

EGLSurface APS5_VABI eglCreatePbufferFromClientBuffer_nid_postfix(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list) {
    (void)dpy;
    (void)buftype;
    (void)buffer;
    (void)config;
    (void)attrib_list;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

GLboolean APS5_VABI glIsFramebuffer_nid_postfix(GLuint framebuffer) {
    (void)framebuffer;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glHint_nid_postfix(GLenum target, GLenum mode) {
    (void)target;
    (void)mode;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglQuerySurface_nid_postfix(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value) {
    (void)dpy;
    (void)surface;
    (void)attribute;
    (void)value;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glEnable_nid_postfix(GLenum cap) {
    (void)cap;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetBufferParameteriv_nid_postfix(GLenum target, GLenum pname, GLint *params) {
    (void)target;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglReleaseTexImage_nid_postfix(EGLDisplay dpy, EGLSurface surface, EGLint buffer) {
    (void)dpy;
    (void)surface;
    (void)buffer;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

GLint APS5_VABI glGetUniformLocation_nid_postfix(GLuint program, const GLchar *name) {
    (void)program;
    (void)name;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glFlush_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetTexParameteriv_nid_postfix(GLenum target, GLenum pname, GLint *params) {
    (void)target;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glColorMask_nid_postfix(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glCompressedTexSubImage2D_nid_postfix(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
    (void)target;
    (void)level;
    (void)xoffset;
    (void)yoffset;
    (void)width;
    (void)height;
    (void)format;
    (void)imageSize;
    (void)data;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDrawElements_nid_postfix(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    (void)mode;
    (void)count;
    (void)type;
    (void)indices;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glFramebufferRenderbuffer_nid_postfix(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    (void)target;
    (void)attachment;
    (void)renderbuffertarget;
    (void)renderbuffer;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetShaderInfoLog_nid_postfix(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    (void)shader;
    (void)bufSize;
    (void)length;
    (void)infoLog;
    NotImplemented_nid_no_patch(__func__);
}

EGLDisplay APS5_VABI eglGetDisplay_nid_postfix(EGLNativeDisplayType display_id) {
    (void)display_id;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glGenRenderbuffers_nid_postfix(GLsizei n, GLuint *renderbuffers) {
    (void)n;
    (void)renderbuffers;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDepthMask_nid_postfix(GLboolean flag) {
    (void)flag;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glCullFace_nid_postfix(GLenum mode) {
    (void)mode;
    NotImplemented_nid_no_patch(__func__);
}

GLint APS5_VABI glGetAttribLocation_nid_postfix(GLuint program, const GLchar *name) {
    (void)program;
    (void)name;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glTexParameterfv_nid_postfix(GLenum target, GLenum pname, const GLfloat *params) {
    (void)target;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glBlendColor_nid_postfix(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
    NotImplemented_nid_no_patch(__func__);
}

GLboolean APS5_VABI glIsRenderbuffer_nid_postfix(GLuint renderbuffer) {
    (void)renderbuffer;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

EGLBoolean APS5_VABI eglChooseConfig_nid_postfix(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
    (void)dpy;
    (void)attrib_list;
    (void)configs;
    (void)config_size;
    (void)num_config;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glGetVertexAttribPointerv_nid_postfix(GLuint index, GLenum pname, void **pointer) {
    (void)index;
    (void)pname;
    (void)pointer;
    NotImplemented_nid_no_patch(__func__);
}

GLboolean APS5_VABI glIsShader_nid_postfix(GLuint shader) {
    (void)shader;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glGetVertexAttribiv_nid_postfix(GLuint index, GLenum pname, GLint *params) {
    (void)index;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglWaitNative_nid_postfix(EGLint engine) {
    (void)engine;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glBufferData_nid_postfix(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    (void)target;
    (void)size;
    (void)data;
    (void)usage;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glUniform1i_nid_postfix(GLint location, GLint v0) {
    (void)location;
    (void)v0;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glStencilFunc_nid_postfix(GLenum func, GLint ref, GLuint mask) {
    (void)func;
    (void)ref;
    (void)mask;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glStencilMask_nid_postfix(GLuint mask) {
    (void)mask;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetRenderbufferParameteriv_nid_postfix(GLenum target, GLenum pname, GLint *params) {
    (void)target;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetActiveAttrib_nid_postfix(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    (void)program;
    (void)index;
    (void)bufSize;
    (void)length;
    (void)size;
    (void)type;
    (void)name;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glBindRenderbuffer_nid_postfix(GLenum target, GLuint renderbuffer) {
    (void)target;
    (void)renderbuffer;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glBindAttribLocation_nid_postfix(GLuint program, GLuint index, const GLchar *name) {
    (void)program;
    (void)index;
    (void)name;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetProgramiv_nid_postfix(GLuint program, GLenum pname, GLint *params) {
    (void)program;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglReleaseThread_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glGetTexParameterfv_nid_postfix(GLenum target, GLenum pname, GLfloat *params) {
    (void)target;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDeleteBuffers_nid_postfix(GLsizei n, const GLuint *buffers) {
    (void)n;
    (void)buffers;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glStencilOpSeparate_nid_postfix(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    (void)face;
    (void)sfail;
    (void)dpfail;
    (void)dppass;
    NotImplemented_nid_no_patch(__func__);
}

const char * APS5_VABI eglQueryString_nid_postfix(EGLDisplay dpy, EGLint name) {
    (void)dpy;
    (void)name;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glLineWidth_nid_postfix(GLfloat width) {
    (void)width;
    NotImplemented_nid_no_patch(__func__);
}

GLboolean APS5_VABI glIsBuffer_nid_postfix(GLuint buffer) {
    (void)buffer;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glBindBuffer_nid_postfix(GLenum target, GLuint buffer) {
    (void)target;
    (void)buffer;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGenerateMipmap_nid_postfix(GLenum target) {
    (void)target;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDeleteProgram_nid_postfix(GLuint program) {
    (void)program;
    NotImplemented_nid_no_patch(__func__);
}

GLboolean APS5_VABI glIsTexture_nid_postfix(GLuint texture) {
    (void)texture;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glDisable_nid_postfix(GLenum cap) {
    (void)cap;
    NotImplemented_nid_no_patch(__func__);
}

GLuint APS5_VABI glCreateProgram_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glGetUniformfv_nid_postfix(GLuint program, GLint location, GLfloat *params) {
    (void)program;
    (void)location;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDeleteShader_nid_postfix(GLuint shader) {
    (void)shader;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glClearDepthf_nid_postfix(GLfloat d) {
    (void)d;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glEnableVertexAttribArray_nid_postfix(GLuint index) {
    (void)index;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetShaderiv_nid_postfix(GLuint shader, GLenum pname, GLint *params) {
    (void)shader;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetFramebufferAttachmentParameteriv_nid_postfix(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    (void)target;
    (void)attachment;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glStencilOp_nid_postfix(GLenum fail, GLenum zfail, GLenum zpass) {
    (void)fail;
    (void)zfail;
    (void)zpass;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glReadPixels_nid_postfix(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)format;
    (void)type;
    (void)pixels;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetActiveUniform_nid_postfix(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    (void)program;
    (void)index;
    (void)bufSize;
    (void)length;
    (void)size;
    (void)type;
    (void)name;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glDeleteTextures_nid_postfix(GLsizei n, const GLuint *textures) {
    (void)n;
    (void)textures;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetBooleanv_nid_postfix(GLenum pname, GLboolean *data) {
    (void)pname;
    (void)data;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glAttachShader_nid_postfix(GLuint program, GLuint shader) {
    (void)program;
    (void)shader;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetProgramInfoLog_nid_postfix(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    (void)program;
    (void)bufSize;
    (void)length;
    (void)infoLog;
    NotImplemented_nid_no_patch(__func__);
}

GLenum APS5_VABI glGetError_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glBindFramebuffer_nid_postfix(GLenum target, GLuint framebuffer) {
    (void)target;
    (void)framebuffer;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglDestroyContext_nid_postfix(EGLDisplay dpy, EGLContext ctx) {
    (void)dpy;
    (void)ctx;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

EGLBoolean APS5_VABI eglWaitGL_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

EGLBoolean APS5_VABI eglGetConfigs_nid_postfix(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
    (void)dpy;
    (void)configs;
    (void)config_size;
    (void)num_config;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

const GLubyte * APS5_VABI glGetString_nid_postfix(GLenum name) {
    (void)name;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glActiveTexture_nid_postfix(GLenum texture) {
    (void)texture;
    NotImplemented_nid_no_patch(__func__);
}

EGLBoolean APS5_VABI eglBindAPI_nid_postfix(EGLenum api) {
    (void)api;
    NotImplemented_nid_no_patch(__func__);
    return {};
}

void APS5_VABI glShaderBinary_nid_postfix(GLsizei count, const GLuint *shaders, GLenum binaryFormat, const void *binary, GLsizei length) {
    (void)count;
    (void)shaders;
    (void)binaryFormat;
    (void)binary;
    (void)length;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glGetVertexAttribfv_nid_postfix(GLuint index, GLenum pname, GLfloat *params) {
    (void)index;
    (void)pname;
    (void)params;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glCopyTexImage2D_nid_postfix(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    (void)target;
    (void)level;
    (void)internalformat;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)border;
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI glCompileShader_nid_postfix(GLuint shader) {
    (void)shader;
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceGLSlimVSHUnknown_J5IG3lL56FI_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("J5IG3lL56FI", sceGLSlimVSHUnknown_J5IG3lL56FI_nid_no_patch);

int APS5_VABI sceGLSlimVSHUnknown_LuP1plTAR7g_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("LuP1plTAR7g", sceGLSlimVSHUnknown_LuP1plTAR7g_nid_no_patch);

int APS5_VABI sceGLSlimVSHUnknown_hA8TWZkwPRU_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("hA8TWZkwPRU", sceGLSlimVSHUnknown_hA8TWZkwPRU_nid_no_patch);

int APS5_VABI sceGLSlimVSHUnknown_iIsF4NzYgD4_nid_no_patch() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
APS5_EXPORT("iIsF4NzYgD4", sceGLSlimVSHUnknown_iIsF4NzYgD4_nid_no_patch);

}
