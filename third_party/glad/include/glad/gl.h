/**
 * Ember Engine — hand-written minimal OpenGL 4.1 core loader.
 * Covers the function set required for Epic 02 (Foundation).
 * Generated-glad API-compatible: gladLoadGL(proc) returns version int.
 */
#ifndef GLAD_GL_H
#define GLAD_GL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "KHR/khrplatform.h"

/* ---- Platform calling convention -- must come before any typedefs ---- */
#ifndef GLAPIENTRY
#  ifdef _WIN32
#    define GLAPIENTRY __stdcall
#  else
#    define GLAPIENTRY
#  endif
#endif

/* ---- Basic GL types ---- */
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef signed   char  GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef double         GLclampd;
typedef void           GLvoid;
typedef char           GLchar;
typedef ptrdiff_t      GLintptr;
typedef ptrdiff_t      GLsizeiptr;
typedef int64_t        GLint64;
typedef uint64_t       GLuint64;
typedef unsigned short GLhalf;
typedef struct __GLsync* GLsync;

/* ---- Constants ---- */
#define GL_FALSE                          0
#define GL_TRUE                           1
#define GL_ZERO                           0
#define GL_ONE                            1

/* Clearing */
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400

/* Data types */
#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_DOUBLE                         0x140A

/* Primitives */
#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006

/* Buffers */
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_UNIFORM_BUFFER                 0x8A11
#define GL_STATIC_DRAW                    0x88B4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_STREAM_DRAW                    0x88E0
#define GL_STATIC_READ                    0x88B5
#define GL_DYNAMIC_READ                   0x88E9

/* Shader types */
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_GEOMETRY_SHADER                0x8DD9
#define GL_COMPUTE_SHADER                 0x91B9

/* Shader parameters */
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_SHADER_SOURCE_LENGTH           0x8B88

/* Textures */
#define GL_TEXTURE_2D                     0x0DE1
#define GL_TEXTURE_CUBE_MAP               0x8513
#define GL_TEXTURE0                       0x84C0
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_RED                            0x1903
#define GL_RGBA8                          0x8058
#define GL_RGB8                           0x8051
#define GL_R8                             0x8229
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_LINEAR_MIPMAP_LINEAR           0x2703
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_REPEAT                         0x2901
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_CLAMP_TO_BORDER                0x812D

/* Framebuffers */
#define GL_FRAMEBUFFER                    0x8D40
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_RENDERBUFFER                   0x8D41
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_DEPTH_STENCIL                  0x84F9
#define GL_UNSIGNED_INT_24_8              0x84FA

/* Depth / blend */
#define GL_DEPTH_TEST                     0x0B71
#define GL_BLEND                          0x0BE2
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_CULL_FACE                      0x0B44
#define GL_BACK                           0x0405
#define GL_FRONT                          0x0404

/* Misc */
#define GL_VENDOR                         0x1F00
#define GL_RENDERER                       0x1F01
#define GL_VERSION                        0x1F02
#define GL_EXTENSIONS                     0x1F03
#define GL_NO_ERROR                       0
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_OUT_OF_MEMORY                  0x0505

/* Debug (GL 4.3+) */
#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B
#define GL_DEBUG_SOURCE_API               0x8246
#define GL_DEBUG_TYPE_ERROR               0x824C

/* ---- Version info ---- */
#define GLAD_VERSION_MAJOR(v) ((v) / 10000)
#define GLAD_VERSION_MINOR(v) ((v) % 10000 / 100)

/* ---- Proc loader type ---- */
typedef void* (*GLADloadfunc)(const char* name);

/* ---- Function pointer declarations ---- */

/* Vertex arrays (GLAPIENTRY already defined above) */
typedef void  (GLAPIENTRY* PFNGLGENVERTEXARRAYSPROC)         (GLsizei n, GLuint* arrays);
typedef void  (GLAPIENTRY* PFNGLBINDVERTEXARRAYPROC)         (GLuint array);
typedef void  (GLAPIENTRY* PFNGLDELETEVERTEXARRAYSPROC)      (GLsizei n, const GLuint* arrays);
typedef void  (GLAPIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void  (GLAPIENTRY* PFNGLVERTEXATTRIBPOINTERPROC)     (GLuint index, GLint size, GLenum type,
                                                               GLboolean normalized, GLsizei stride,
                                                               const void* pointer);
/* Buffers */
typedef void  (GLAPIENTRY* PFNGLGENBUFFERSPROC)              (GLsizei n, GLuint* buffers);
typedef void  (GLAPIENTRY* PFNGLDELETEBUFFERSPROC)           (GLsizei n, const GLuint* buffers);
typedef void  (GLAPIENTRY* PFNGLBINDBUFFERPROC)              (GLenum target, GLuint buffer);
typedef void  (GLAPIENTRY* PFNGLBUFFERDATAPROC)              (GLenum target, GLsizeiptr size,
                                                               const void* data, GLenum usage);
typedef void  (GLAPIENTRY* PFNGLBUFFERSUBDATAPROC)           (GLenum target, GLintptr offset,
                                                               GLsizeiptr size, const void* data);
typedef void  (GLAPIENTRY* PFNGLBINDBUFFERBASEPROC)          (GLenum target, GLuint index, GLuint buffer);

/* Shaders */
typedef GLuint(GLAPIENTRY* PFNGLCREATESHADERPROC)            (GLenum type);
typedef void  (GLAPIENTRY* PFNGLDELETESHADERPROC)            (GLuint shader);
typedef void  (GLAPIENTRY* PFNGLSHADERSOURCEPROC)            (GLuint shader, GLsizei count,
                                                               const GLchar* const* string,
                                                               const GLint* length);
typedef void  (GLAPIENTRY* PFNGLCOMPILESHADERPROC)           (GLuint shader);
typedef void  (GLAPIENTRY* PFNGLGETSHADERIVPROC)             (GLuint shader, GLenum pname, GLint* params);
typedef void  (GLAPIENTRY* PFNGLGETSHADERINFOLOGPROC)        (GLuint shader, GLsizei bufSize,
                                                               GLsizei* length, GLchar* infoLog);
/* Programs */
typedef GLuint(GLAPIENTRY* PFNGLCREATEPROGRAMPROC)           (void);
typedef void  (GLAPIENTRY* PFNGLDELETEPROGRAMPROC)           (GLuint program);
typedef void  (GLAPIENTRY* PFNGLATTACHSHADERPROC)            (GLuint program, GLuint shader);
typedef void  (GLAPIENTRY* PFNGLDETACHSHADERPROC)            (GLuint program, GLuint shader);
typedef void  (GLAPIENTRY* PFNGLLINKPROGRAMPROC)             (GLuint program);
typedef void  (GLAPIENTRY* PFNGLUSEPROGRAMPROC)              (GLuint program);
typedef void  (GLAPIENTRY* PFNGLGETPROGRAMIVPROC)            (GLuint program, GLenum pname, GLint* params);
typedef void  (GLAPIENTRY* PFNGLGETPROGRAMINFOLOGPROC)       (GLuint program, GLsizei bufSize,
                                                               GLsizei* length, GLchar* infoLog);
/* Uniforms */
typedef GLint (GLAPIENTRY* PFNGLGETUNIFORMLOCATIONPROC)      (GLuint program, const GLchar* name);
typedef void  (GLAPIENTRY* PFNGLUNIFORM1IPROC)               (GLint location, GLint v0);
typedef void  (GLAPIENTRY* PFNGLUNIFORM1FPROC)               (GLint location, GLfloat v0);
typedef void  (GLAPIENTRY* PFNGLUNIFORM2FVPROC)              (GLint location, GLsizei count, const GLfloat* value);
typedef void  (GLAPIENTRY* PFNGLUNIFORM3FVPROC)              (GLint location, GLsizei count, const GLfloat* value);
typedef void  (GLAPIENTRY* PFNGLUNIFORM4FVPROC)              (GLint location, GLsizei count, const GLfloat* value);
typedef void  (GLAPIENTRY* PFNGLUNIFORMMATRIX3FVPROC)        (GLint location, GLsizei count,
                                                               GLboolean transpose, const GLfloat* value);
typedef void  (GLAPIENTRY* PFNGLUNIFORMMATRIX4FVPROC)        (GLint location, GLsizei count,
                                                               GLboolean transpose, const GLfloat* value);
typedef void  (GLAPIENTRY* PFNGLUNIFORM1IVPROC)              (GLint location, GLsizei count, const GLint* value);

/* Textures */
typedef void  (GLAPIENTRY* PFNGLGENTEXTURESPROC)             (GLsizei n, GLuint* textures);
typedef void  (GLAPIENTRY* PFNGLDELETETEXTURESPROC)          (GLsizei n, const GLuint* textures);
typedef void  (GLAPIENTRY* PFNGLBINDTEXTUREPROC)             (GLenum target, GLuint texture);
typedef void  (GLAPIENTRY* PFNGLTEXIMAGE2DPROC)              (GLenum target, GLint level,
                                                               GLint internalformat, GLsizei width,
                                                               GLsizei height, GLint border,
                                                               GLenum format, GLenum type,
                                                               const void* pixels);
typedef void  (GLAPIENTRY* PFNGLTEXPARAMETERIPROC)           (GLenum target, GLenum pname, GLint param);
typedef void  (GLAPIENTRY* PFNGLGENERATEMIPMAPPROC)          (GLenum target);
typedef void  (GLAPIENTRY* PFNGLACTIVETEXTUREPROC)           (GLenum texture);
typedef void  (GLAPIENTRY* PFNGLTEXSUBIMAGE2DPROC)           (GLenum target, GLint level,
                                                               GLint xoffset, GLint yoffset,
                                                               GLsizei width, GLsizei height,
                                                               GLenum format, GLenum type,
                                                               const void* pixels);

/* Framebuffers */
typedef void  (GLAPIENTRY* PFNGLGENFRAMEBUFFERSPROC)         (GLsizei n, GLuint* framebuffers);
typedef void  (GLAPIENTRY* PFNGLDELETEFRAMEBUFFERSPROC)      (GLsizei n, const GLuint* framebuffers);
typedef void  (GLAPIENTRY* PFNGLBINDFRAMEBUFFERPROC)         (GLenum target, GLuint framebuffer);
typedef void  (GLAPIENTRY* PFNGLFRAMEBUFFERTEXTURE2DPROC)    (GLenum target, GLenum attachment,
                                                               GLenum textarget, GLuint texture,
                                                               GLint level);
typedef GLenum(GLAPIENTRY* PFNGLCHECKFRAMEBUFFERSTATUSPROC)  (GLenum target);
typedef void  (GLAPIENTRY* PFNGLGENRENDERBUFFERSPROC)        (GLsizei n, GLuint* renderbuffers);
typedef void  (GLAPIENTRY* PFNGLDELETERENDERBUFFERSPROC)     (GLsizei n, const GLuint* renderbuffers);
typedef void  (GLAPIENTRY* PFNGLBINDRENDERBUFFERPROC)        (GLenum target, GLuint renderbuffer);
typedef void  (GLAPIENTRY* PFNGLRENDERBUFFERSTORAGEPROC)     (GLenum target, GLenum internalformat,
                                                               GLsizei width, GLsizei height);
typedef void  (GLAPIENTRY* PFNGLFRAMEBUFFERRENDERBUFFERPROC) (GLenum target, GLenum attachment,
                                                               GLenum renderbuffertarget,
                                                               GLuint renderbuffer);

/* Draw */
typedef void  (GLAPIENTRY* PFNGLDRAWELEMENTSPROC)            (GLenum mode, GLsizei count, GLenum type,
                                                               const void* indices);
typedef void  (GLAPIENTRY* PFNGLDRAWARRAYSPROC)              (GLenum mode, GLint first, GLsizei count);
typedef void  (GLAPIENTRY* PFNGLDRAWELEMENTSINSTANCEDPROC)   (GLenum mode, GLsizei count, GLenum type,
                                                               const void* indices, GLsizei primcount);

/* State */
typedef void  (GLAPIENTRY* PFNGLENABLEPROC)                  (GLenum cap);
typedef void  (GLAPIENTRY* PFNGLDISABLEPROC)                 (GLenum cap);
typedef void  (GLAPIENTRY* PFNGLCLEARPROC)                   (GLbitfield mask);
typedef void  (GLAPIENTRY* PFNGLCLEARCOLORPROC)              (GLfloat red, GLfloat green,
                                                               GLfloat blue, GLfloat alpha);
typedef void  (GLAPIENTRY* PFNGLVIEWPORTPROC)                (GLint x, GLint y,
                                                               GLsizei width, GLsizei height);
typedef void  (GLAPIENTRY* PFNGLSCISSORPROC)                 (GLint x, GLint y,
                                                               GLsizei width, GLsizei height);
typedef void  (GLAPIENTRY* PFNGLBLENDFUNCPROC)               (GLenum sfactor, GLenum dfactor);
typedef void  (GLAPIENTRY* PFNGLCULLFACEPROC)                (GLenum mode);
typedef void  (GLAPIENTRY* PFNGLDEPTHFUNCPROC)               (GLenum func);
typedef void  (GLAPIENTRY* PFNGLDEPTHMASKPROC)               (GLboolean flag);
typedef GLenum(GLAPIENTRY* PFNGLGETERRORPROC)                (void);
typedef const GLubyte* (GLAPIENTRY* PFNGLGETSTRINGPROC)      (GLenum name);
typedef void  (GLAPIENTRY* PFNGLGETINTEGERVPROC)             (GLenum pname, GLint* data);

/* Debug (GL 4.3) */
typedef void (GLAPIENTRY GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity,
                                       GLsizei length, const GLchar* message, const void* userParam);
typedef void (GLAPIENTRY* PFNGLDEBUGMESSAGECALLBACKPROC)(GLDEBUGPROC callback, const void* userParam);
typedef void (GLAPIENTRY* PFNGLDEBUGMESSAGECONTROLPROC) (GLenum source, GLenum type, GLenum severity,
                                                          GLsizei count, const GLuint* ids,
                                                          GLboolean enabled);

/* ---- Global function pointer variables ---- */
extern PFNGLGENVERTEXARRAYSPROC         glad_glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC         glad_glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC      glad_glDeleteVertexArrays;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC     glad_glVertexAttribPointer;

extern PFNGLGENBUFFERSPROC              glad_glGenBuffers;
extern PFNGLDELETEBUFFERSPROC           glad_glDeleteBuffers;
extern PFNGLBINDBUFFERPROC              glad_glBindBuffer;
extern PFNGLBUFFERDATAPROC              glad_glBufferData;
extern PFNGLBUFFERSUBDATAPROC           glad_glBufferSubData;
extern PFNGLBINDBUFFERBASEPROC          glad_glBindBufferBase;

extern PFNGLCREATESHADERPROC            glad_glCreateShader;
extern PFNGLDELETESHADERPROC            glad_glDeleteShader;
extern PFNGLSHADERSOURCEPROC            glad_glShaderSource;
extern PFNGLCOMPILESHADERPROC           glad_glCompileShader;
extern PFNGLGETSHADERIVPROC             glad_glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC        glad_glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC           glad_glCreateProgram;
extern PFNGLDELETEPROGRAMPROC           glad_glDeleteProgram;
extern PFNGLATTACHSHADERPROC            glad_glAttachShader;
extern PFNGLDETACHSHADERPROC            glad_glDetachShader;
extern PFNGLLINKPROGRAMPROC             glad_glLinkProgram;
extern PFNGLUSEPROGRAMPROC              glad_glUseProgram;
extern PFNGLGETPROGRAMIVPROC            glad_glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC       glad_glGetProgramInfoLog;

extern PFNGLGETUNIFORMLOCATIONPROC      glad_glGetUniformLocation;
extern PFNGLUNIFORM1IPROC               glad_glUniform1i;
extern PFNGLUNIFORM1FPROC               glad_glUniform1f;
extern PFNGLUNIFORM2FVPROC              glad_glUniform2fv;
extern PFNGLUNIFORM3FVPROC              glad_glUniform3fv;
extern PFNGLUNIFORM4FVPROC              glad_glUniform4fv;
extern PFNGLUNIFORMMATRIX3FVPROC        glad_glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVPROC        glad_glUniformMatrix4fv;
extern PFNGLUNIFORM1IVPROC              glad_glUniform1iv;

extern PFNGLGENTEXTURESPROC             glad_glGenTextures;
extern PFNGLDELETETEXTURESPROC          glad_glDeleteTextures;
extern PFNGLBINDTEXTUREPROC             glad_glBindTexture;
extern PFNGLTEXIMAGE2DPROC              glad_glTexImage2D;
extern PFNGLTEXPARAMETERIPROC           glad_glTexParameteri;
extern PFNGLGENERATEMIPMAPPROC          glad_glGenerateMipmap;
extern PFNGLACTIVETEXTUREPROC           glad_glActiveTexture;
extern PFNGLTEXSUBIMAGE2DPROC           glad_glTexSubImage2D;

extern PFNGLGENFRAMEBUFFERSPROC         glad_glGenFramebuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC      glad_glDeleteFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC         glad_glBindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC    glad_glFramebufferTexture2D;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC  glad_glCheckFramebufferStatus;
extern PFNGLGENRENDERBUFFERSPROC        glad_glGenRenderbuffers;
extern PFNGLDELETERENDERBUFFERSPROC     glad_glDeleteRenderbuffers;
extern PFNGLBINDRENDERBUFFERPROC        glad_glBindRenderbuffer;
extern PFNGLRENDERBUFFERSTORAGEPROC     glad_glRenderbufferStorage;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_glFramebufferRenderbuffer;

extern PFNGLDRAWELEMENTSPROC            glad_glDrawElements;
extern PFNGLDRAWARRAYSPROC              glad_glDrawArrays;
extern PFNGLDRAWELEMENTSINSTANCEDPROC   glad_glDrawElementsInstanced;

extern PFNGLENABLEPROC                  glad_glEnable;
extern PFNGLDISABLEPROC                 glad_glDisable;
extern PFNGLCLEARPROC                   glad_glClear;
extern PFNGLCLEARCOLORPROC              glad_glClearColor;
extern PFNGLVIEWPORTPROC                glad_glViewport;
extern PFNGLSCISSORPROC                 glad_glScissor;
extern PFNGLBLENDFUNCPROC               glad_glBlendFunc;
extern PFNGLCULLFACEPROC                glad_glCullFace;
extern PFNGLDEPTHFUNCPROC               glad_glDepthFunc;
extern PFNGLDEPTHMASKPROC               glad_glDepthMask;
extern PFNGLGETERRORPROC                glad_glGetError;
extern PFNGLGETSTRINGPROC               glad_glGetString;
extern PFNGLGETINTEGERVPROC             glad_glGetIntegerv;

extern PFNGLDEBUGMESSAGECALLBACKPROC    glad_glDebugMessageCallback;
extern PFNGLDEBUGMESSAGECONTROLPROC     glad_glDebugMessageControl;

/* ---- Convenience macros (same names as real GL calls) ---- */
#define glGenVertexArrays         glad_glGenVertexArrays
#define glBindVertexArray         glad_glBindVertexArray
#define glDeleteVertexArrays      glad_glDeleteVertexArrays
#define glEnableVertexAttribArray glad_glEnableVertexAttribArray
#define glVertexAttribPointer     glad_glVertexAttribPointer

#define glGenBuffers              glad_glGenBuffers
#define glDeleteBuffers           glad_glDeleteBuffers
#define glBindBuffer              glad_glBindBuffer
#define glBufferData              glad_glBufferData
#define glBufferSubData           glad_glBufferSubData
#define glBindBufferBase          glad_glBindBufferBase

#define glCreateShader            glad_glCreateShader
#define glDeleteShader            glad_glDeleteShader
#define glShaderSource            glad_glShaderSource
#define glCompileShader           glad_glCompileShader
#define glGetShaderiv             glad_glGetShaderiv
#define glGetShaderInfoLog        glad_glGetShaderInfoLog
#define glCreateProgram           glad_glCreateProgram
#define glDeleteProgram           glad_glDeleteProgram
#define glAttachShader            glad_glAttachShader
#define glDetachShader            glad_glDetachShader
#define glLinkProgram             glad_glLinkProgram
#define glUseProgram              glad_glUseProgram
#define glGetProgramiv            glad_glGetProgramiv
#define glGetProgramInfoLog       glad_glGetProgramInfoLog

#define glGetUniformLocation      glad_glGetUniformLocation
#define glUniform1i               glad_glUniform1i
#define glUniform1f               glad_glUniform1f
#define glUniform2fv              glad_glUniform2fv
#define glUniform3fv              glad_glUniform3fv
#define glUniform4fv              glad_glUniform4fv
#define glUniformMatrix3fv        glad_glUniformMatrix3fv
#define glUniformMatrix4fv        glad_glUniformMatrix4fv
#define glUniform1iv              glad_glUniform1iv

#define glGenTextures             glad_glGenTextures
#define glDeleteTextures          glad_glDeleteTextures
#define glBindTexture             glad_glBindTexture
#define glTexImage2D              glad_glTexImage2D
#define glTexParameteri           glad_glTexParameteri
#define glGenerateMipmap          glad_glGenerateMipmap
#define glActiveTexture           glad_glActiveTexture
#define glTexSubImage2D           glad_glTexSubImage2D

#define glGenFramebuffers         glad_glGenFramebuffers
#define glDeleteFramebuffers      glad_glDeleteFramebuffers
#define glBindFramebuffer         glad_glBindFramebuffer
#define glFramebufferTexture2D    glad_glFramebufferTexture2D
#define glCheckFramebufferStatus  glad_glCheckFramebufferStatus
#define glGenRenderbuffers        glad_glGenRenderbuffers
#define glDeleteRenderbuffers     glad_glDeleteRenderbuffers
#define glBindRenderbuffer        glad_glBindRenderbuffer
#define glRenderbufferStorage     glad_glRenderbufferStorage
#define glFramebufferRenderbuffer glad_glFramebufferRenderbuffer

#define glDrawElements            glad_glDrawElements
#define glDrawArrays              glad_glDrawArrays
#define glDrawElementsInstanced   glad_glDrawElementsInstanced

#define glEnable                  glad_glEnable
#define glDisable                 glad_glDisable
#define glClear                   glad_glClear
#define glClearColor              glad_glClearColor
#define glViewport                glad_glViewport
#define glScissor                 glad_glScissor
#define glBlendFunc               glad_glBlendFunc
#define glCullFace                glad_glCullFace
#define glDepthFunc               glad_glDepthFunc
#define glDepthMask               glad_glDepthMask
#define glGetError                glad_glGetError
#define glGetString               glad_glGetString
#define glGetIntegerv             glad_glGetIntegerv

#define glDebugMessageCallback    glad_glDebugMessageCallback
#define glDebugMessageControl     glad_glDebugMessageControl

/* ---- Loader ---- */
/**
 * Load OpenGL function pointers using the provided proc address loader.
 * Returns an encoded version integer: major*10000 + minor*100.
 * Returns 0 on failure.
 * Compatible with glfwGetProcAddress:
 *   int ver = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
 */
int gladLoadGL(GLADloadfunc load);

#ifdef __cplusplus
}
#endif

#endif /* GLAD_GL_H */
