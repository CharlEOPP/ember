/**
 * Ember Engine — minimal OpenGL 4.1 core loader implementation.
 */
#include "glad/gl.h"
#include <string.h>

/* ---- Global function pointer definitions ---- */
PFNGLGENVERTEXARRAYSPROC         glad_glGenVertexArrays         = NULL;
PFNGLBINDVERTEXARRAYPROC         glad_glBindVertexArray         = NULL;
PFNGLDELETEVERTEXARRAYSPROC      glad_glDeleteVertexArrays      = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = NULL;
PFNGLVERTEXATTRIBPOINTERPROC     glad_glVertexAttribPointer     = NULL;

PFNGLGENBUFFERSPROC              glad_glGenBuffers              = NULL;
PFNGLDELETEBUFFERSPROC           glad_glDeleteBuffers           = NULL;
PFNGLBINDBUFFERPROC              glad_glBindBuffer              = NULL;
PFNGLBUFFERDATAPROC              glad_glBufferData              = NULL;
PFNGLBUFFERSUBDATAPROC           glad_glBufferSubData           = NULL;
PFNGLBINDBUFFERBASEPROC          glad_glBindBufferBase          = NULL;

PFNGLCREATESHADERPROC            glad_glCreateShader            = NULL;
PFNGLDELETESHADERPROC            glad_glDeleteShader            = NULL;
PFNGLSHADERSOURCEPROC            glad_glShaderSource            = NULL;
PFNGLCOMPILESHADERPROC           glad_glCompileShader           = NULL;
PFNGLGETSHADERIVPROC             glad_glGetShaderiv             = NULL;
PFNGLGETSHADERINFOLOGPROC        glad_glGetShaderInfoLog        = NULL;
PFNGLCREATEPROGRAMPROC           glad_glCreateProgram           = NULL;
PFNGLDELETEPROGRAMPROC           glad_glDeleteProgram           = NULL;
PFNGLATTACHSHADERPROC            glad_glAttachShader            = NULL;
PFNGLDETACHSHADERPROC            glad_glDetachShader            = NULL;
PFNGLLINKPROGRAMPROC             glad_glLinkProgram             = NULL;
PFNGLUSEPROGRAMPROC              glad_glUseProgram              = NULL;
PFNGLGETPROGRAMIVPROC            glad_glGetProgramiv            = NULL;
PFNGLGETPROGRAMINFOLOGPROC       glad_glGetProgramInfoLog       = NULL;

PFNGLGETUNIFORMLOCATIONPROC      glad_glGetUniformLocation      = NULL;
PFNGLUNIFORM1IPROC               glad_glUniform1i               = NULL;
PFNGLUNIFORM1FPROC               glad_glUniform1f               = NULL;
PFNGLUNIFORM2FVPROC              glad_glUniform2fv              = NULL;
PFNGLUNIFORM3FVPROC              glad_glUniform3fv              = NULL;
PFNGLUNIFORM4FVPROC              glad_glUniform4fv              = NULL;
PFNGLUNIFORMMATRIX3FVPROC        glad_glUniformMatrix3fv        = NULL;
PFNGLUNIFORMMATRIX4FVPROC        glad_glUniformMatrix4fv        = NULL;
PFNGLUNIFORM1IVPROC              glad_glUniform1iv              = NULL;

PFNGLGENTEXTURESPROC             glad_glGenTextures             = NULL;
PFNGLDELETETEXTURESPROC          glad_glDeleteTextures          = NULL;
PFNGLBINDTEXTUREPROC             glad_glBindTexture             = NULL;
PFNGLTEXIMAGE2DPROC              glad_glTexImage2D              = NULL;
PFNGLTEXPARAMETERIPROC           glad_glTexParameteri           = NULL;
PFNGLGENERATEMIPMAPPROC          glad_glGenerateMipmap          = NULL;
PFNGLACTIVETEXTUREPROC           glad_glActiveTexture           = NULL;
PFNGLTEXSUBIMAGE2DPROC           glad_glTexSubImage2D           = NULL;

PFNGLGENFRAMEBUFFERSPROC         glad_glGenFramebuffers         = NULL;
PFNGLDELETEFRAMEBUFFERSPROC      glad_glDeleteFramebuffers      = NULL;
PFNGLBINDFRAMEBUFFERPROC         glad_glBindFramebuffer         = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC    glad_glFramebufferTexture2D    = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC  glad_glCheckFramebufferStatus  = NULL;
PFNGLGENRENDERBUFFERSPROC        glad_glGenRenderbuffers        = NULL;
PFNGLDELETERENDERBUFFERSPROC     glad_glDeleteRenderbuffers     = NULL;
PFNGLBINDRENDERBUFFERPROC        glad_glBindRenderbuffer        = NULL;
PFNGLRENDERBUFFERSTORAGEPROC     glad_glRenderbufferStorage     = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_glFramebufferRenderbuffer = NULL;

PFNGLDRAWELEMENTSPROC            glad_glDrawElements            = NULL;
PFNGLDRAWARRAYSPROC              glad_glDrawArrays              = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC   glad_glDrawElementsInstanced   = NULL;

PFNGLENABLEPROC                  glad_glEnable                  = NULL;
PFNGLDISABLEPROC                 glad_glDisable                 = NULL;
PFNGLCLEARPROC                   glad_glClear                   = NULL;
PFNGLCLEARCOLORPROC              glad_glClearColor              = NULL;
PFNGLVIEWPORTPROC                glad_glViewport                = NULL;
PFNGLSCISSORPROC                 glad_glScissor                 = NULL;
PFNGLBLENDFUNCPROC               glad_glBlendFunc               = NULL;
PFNGLCULLFACEPROC                glad_glCullFace                = NULL;
PFNGLDEPTHFUNCPROC               glad_glDepthFunc               = NULL;
PFNGLDEPTHMASKPROC               glad_glDepthMask               = NULL;
PFNGLGETERRORPROC                glad_glGetError                = NULL;
PFNGLGETSTRINGPROC               glad_glGetString               = NULL;
PFNGLGETINTEGERVPROC             glad_glGetIntegerv             = NULL;

PFNGLDEBUGMESSAGECALLBACKPROC    glad_glDebugMessageCallback    = NULL;
PFNGLDEBUGMESSAGECONTROLPROC     glad_glDebugMessageControl     = NULL;

/* Helper: cast proc address to a function pointer */
typedef void (*GLproc)(void);
static GLproc load_proc(GLADloadfunc load, const char* name) {
    return (GLproc)load(name);
}

#define LOAD(var, name) var = (void*)load(name)

int gladLoadGL(GLADloadfunc load) {
    int major = 0, minor = 0;

    /* Load glGetString first to query the version */
    glad_glGetString   = (PFNGLGETSTRINGPROC)load("glGetString");
    glad_glGetIntegerv = (PFNGLGETINTEGERVPROC)load("glGetIntegerv");
    if (!glad_glGetString || !glad_glGetIntegerv) return 0;

    /* Parse version string */
    const char* version = (const char*)glad_glGetString(0x1F02 /* GL_VERSION */);
    if (!version) return 0;
    sscanf(version, "%d.%d", &major, &minor);
    if (major < 2) return 0;

    /* Vertex arrays */
    glad_glGenVertexArrays         = (PFNGLGENVERTEXARRAYSPROC)        load("glGenVertexArrays");
    glad_glBindVertexArray         = (PFNGLBINDVERTEXARRAYPROC)        load("glBindVertexArray");
    glad_glDeleteVertexArrays      = (PFNGLDELETEVERTEXARRAYSPROC)     load("glDeleteVertexArrays");
    glad_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)load("glEnableVertexAttribArray");
    glad_glVertexAttribPointer     = (PFNGLVERTEXATTRIBPOINTERPROC)    load("glVertexAttribPointer");

    /* Buffers */
    glad_glGenBuffers              = (PFNGLGENBUFFERSPROC)             load("glGenBuffers");
    glad_glDeleteBuffers           = (PFNGLDELETEBUFFERSPROC)          load("glDeleteBuffers");
    glad_glBindBuffer              = (PFNGLBINDBUFFERPROC)             load("glBindBuffer");
    glad_glBufferData              = (PFNGLBUFFERDATAPROC)             load("glBufferData");
    glad_glBufferSubData           = (PFNGLBUFFERSUBDATAPROC)          load("glBufferSubData");
    glad_glBindBufferBase          = (PFNGLBINDBUFFERBASEPROC)         load("glBindBufferBase");

    /* Shaders */
    glad_glCreateShader            = (PFNGLCREATESHADERPROC)           load("glCreateShader");
    glad_glDeleteShader            = (PFNGLDELETESHADERPROC)           load("glDeleteShader");
    glad_glShaderSource            = (PFNGLSHADERSOURCEPROC)           load("glShaderSource");
    glad_glCompileShader           = (PFNGLCOMPILESHADERPROC)          load("glCompileShader");
    glad_glGetShaderiv             = (PFNGLGETSHADERIVPROC)            load("glGetShaderiv");
    glad_glGetShaderInfoLog        = (PFNGLGETSHADERINFOLOGPROC)       load("glGetShaderInfoLog");
    glad_glCreateProgram           = (PFNGLCREATEPROGRAMPROC)          load("glCreateProgram");
    glad_glDeleteProgram           = (PFNGLDELETEPROGRAMPROC)          load("glDeleteProgram");
    glad_glAttachShader            = (PFNGLATTACHSHADERPROC)           load("glAttachShader");
    glad_glDetachShader            = (PFNGLDETACHSHADERPROC)           load("glDetachShader");
    glad_glLinkProgram             = (PFNGLLINKPROGRAMPROC)            load("glLinkProgram");
    glad_glUseProgram              = (PFNGLUSEPROGRAMPROC)             load("glUseProgram");
    glad_glGetProgramiv            = (PFNGLGETPROGRAMIVPROC)           load("glGetProgramiv");
    glad_glGetProgramInfoLog       = (PFNGLGETPROGRAMINFOLOGPROC)      load("glGetProgramInfoLog");

    /* Uniforms */
    glad_glGetUniformLocation      = (PFNGLGETUNIFORMLOCATIONPROC)     load("glGetUniformLocation");
    glad_glUniform1i               = (PFNGLUNIFORM1IPROC)              load("glUniform1i");
    glad_glUniform1f               = (PFNGLUNIFORM1FPROC)              load("glUniform1f");
    glad_glUniform2fv              = (PFNGLUNIFORM2FVPROC)             load("glUniform2fv");
    glad_glUniform3fv              = (PFNGLUNIFORM3FVPROC)             load("glUniform3fv");
    glad_glUniform4fv              = (PFNGLUNIFORM4FVPROC)             load("glUniform4fv");
    glad_glUniformMatrix3fv        = (PFNGLUNIFORMMATRIX3FVPROC)       load("glUniformMatrix3fv");
    glad_glUniformMatrix4fv        = (PFNGLUNIFORMMATRIX4FVPROC)       load("glUniformMatrix4fv");
    glad_glUniform1iv              = (PFNGLUNIFORM1IVPROC)             load("glUniform1iv");

    /* Textures */
    glad_glGenTextures             = (PFNGLGENTEXTURESPROC)            load("glGenTextures");
    glad_glDeleteTextures          = (PFNGLDELETETEXTURESPROC)         load("glDeleteTextures");
    glad_glBindTexture             = (PFNGLBINDTEXTUREPROC)            load("glBindTexture");
    glad_glTexImage2D              = (PFNGLTEXIMAGE2DPROC)             load("glTexImage2D");
    glad_glTexParameteri           = (PFNGLTEXPARAMETERIPROC)          load("glTexParameteri");
    glad_glGenerateMipmap          = (PFNGLGENERATEMIPMAPPROC)         load("glGenerateMipmap");
    glad_glActiveTexture           = (PFNGLACTIVETEXTUREPROC)          load("glActiveTexture");
    glad_glTexSubImage2D           = (PFNGLTEXSUBIMAGE2DPROC)          load("glTexSubImage2D");

    /* Framebuffers */
    glad_glGenFramebuffers         = (PFNGLGENFRAMEBUFFERSPROC)        load("glGenFramebuffers");
    glad_glDeleteFramebuffers      = (PFNGLDELETEFRAMEBUFFERSPROC)     load("glDeleteFramebuffers");
    glad_glBindFramebuffer         = (PFNGLBINDFRAMEBUFFERPROC)        load("glBindFramebuffer");
    glad_glFramebufferTexture2D    = (PFNGLFRAMEBUFFERTEXTURE2DPROC)   load("glFramebufferTexture2D");
    glad_glCheckFramebufferStatus  = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) load("glCheckFramebufferStatus");
    glad_glGenRenderbuffers        = (PFNGLGENRENDERBUFFERSPROC)       load("glGenRenderbuffers");
    glad_glDeleteRenderbuffers     = (PFNGLDELETERENDERBUFFERSPROC)    load("glDeleteRenderbuffers");
    glad_glBindRenderbuffer        = (PFNGLBINDRENDERBUFFERPROC)       load("glBindRenderbuffer");
    glad_glRenderbufferStorage     = (PFNGLRENDERBUFFERSTORAGEPROC)    load("glRenderbufferStorage");
    glad_glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)load("glFramebufferRenderbuffer");

    /* Draw */
    glad_glDrawElements            = (PFNGLDRAWELEMENTSPROC)           load("glDrawElements");
    glad_glDrawArrays              = (PFNGLDRAWARRAYSPROC)             load("glDrawArrays");
    glad_glDrawElementsInstanced   = (PFNGLDRAWELEMENTSINSTANCEDPROC)  load("glDrawElementsInstanced");

    /* State */
    glad_glEnable                  = (PFNGLENABLEPROC)                 load("glEnable");
    glad_glDisable                 = (PFNGLDISABLEPROC)                load("glDisable");
    glad_glClear                   = (PFNGLCLEARPROC)                  load("glClear");
    glad_glClearColor              = (PFNGLCLEARCOLORPROC)             load("glClearColor");
    glad_glViewport                = (PFNGLVIEWPORTPROC)               load("glViewport");
    glad_glScissor                 = (PFNGLSCISSORPROC)                load("glScissor");
    glad_glBlendFunc               = (PFNGLBLENDFUNCPROC)              load("glBlendFunc");
    glad_glCullFace                = (PFNGLCULLFACEPROC)               load("glCullFace");
    glad_glDepthFunc               = (PFNGLDEPTHFUNCPROC)              load("glDepthFunc");
    glad_glDepthMask               = (PFNGLDEPTHMASKPROC)              load("glDepthMask");
    glad_glGetError                = (PFNGLGETERRORPROC)               load("glGetError");

    /* Debug — optional (GL 4.3+), don't fail if unavailable */
    glad_glDebugMessageCallback    = (PFNGLDEBUGMESSAGECALLBACKPROC)   load("glDebugMessageCallback");
    glad_glDebugMessageControl     = (PFNGLDEBUGMESSAGECONTROLPROC)    load("glDebugMessageControl");

    return major * 10000 + minor * 100;
}
