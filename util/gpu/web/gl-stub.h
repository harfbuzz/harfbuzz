/* Minimal GL type and function stubs for compiling demo code without GL.
 * The external atlas backend bypasses GL at runtime, but the compiler
 * still needs the symbols to exist. */
#ifndef GL_STUB_H
#define GL_STUB_H

/* Prevent demo-common.h from including real GL/GLFW headers */
#define __glew_h__
#define _glfw3_h_

/* Types */
typedef float GLfloat;
typedef unsigned int GLuint;
typedef int GLint;
typedef unsigned int GLenum;
typedef int GLsizei;
typedef char GLchar;
typedef unsigned char GLboolean;
typedef long GLsizeiptr;

/* Constants */
#define GL_FALSE 0
#define GL_TEXTURE0 0x84C0
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_BUFFER 0x8C2A
#define GL_RGBA16I 0x8D88
#define GL_RGBA_INTEGER 0x8D99
#define GL_SHORT 0x1402
#define GL_STATIC_DRAW 0x88E4
#define GL_NEAREST 0x2600
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_CURRENT_PROGRAM 0x8B8D

/* Function stubs — never called (external atlas backend), but must link */
static inline void glGetIntegerv (GLenum, GLint *) {}
static inline void glGenTextures (GLsizei, GLuint *) {}
static inline void glDeleteTextures (GLsizei, const GLuint *) {}
static inline void glGenBuffers (GLsizei, GLuint *) {}
static inline void glDeleteBuffers (GLsizei, const GLuint *) {}
static inline void glBindBuffer (GLenum, GLuint) {}
static inline void glBufferData (GLenum, GLsizeiptr, const void *, GLenum) {}
static inline void glBufferSubData (GLenum, GLint, GLsizeiptr, const void *) {}
static inline void glBindTexture (GLenum, GLuint) {}
static inline void glActiveTexture (GLenum) {}
static inline void glTexParameteri (GLenum, GLenum, GLint) {}
static inline void glTexImage2D (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *) {}
static inline void glTexSubImage2D (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *) {}
static inline void glTexBuffer (GLenum, GLenum, GLuint) {}
static inline GLint glGetUniformLocation (GLuint, const GLchar *) { return 0; }
static inline void glUniform1i (GLint, GLint) {}
static inline GLenum glGetError () { return 0; }

#endif /* GL_STUB_H */
