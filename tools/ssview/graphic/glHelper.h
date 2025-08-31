#pragma once

// System
#include <cstdio>
#include <cstdlib>

// Internal
#include "bs.h"
#include "bsColor.h"
#ifdef __linux__
#include "glLinux.h"
#endif
#ifdef _WIN32
#include "glWindows.h"
#endif

namespace gfx
{

// Helper OpenGL debugging function. Just call it after a bunch of GL commands
// in order to check for issues
inline void
checkGlError(const char* filename, int lineNbr)
{
    GLenum err = 0;
    while ((err = glGetError()) != GL_NO_ERROR) {
        const char* errorKind = nullptr;
        switch (err) {
            case GL_INVALID_OPERATION:
                errorKind = "INVALID_OPERATION";
                break;
            case GL_INVALID_ENUM:
                errorKind = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                errorKind = "INVALID_VALUE";
                break;
            case GL_OUT_OF_MEMORY:
                errorKind = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                errorKind = "INVALID_FRAMEBUFFER_OPERATION";
                break;
            default:
                errorKind = "(UNKNOWN ERROR TYPE)";
        }
        printf("GL_%s: at line %d of file %s\n", errorKind, lineNbr, filename);
    }
}

// Helper OpenGL debugging function. Just call it after compilation of the
// shaders. For linking, call bsGlCheckError instead
inline void
checkGlShaderCompilation(const char* filename, int lineNbr, int shaderId)
{
    // Get the compilation status
    GLint isCompiled = 0;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isCompiled);
    // Error detected?
    if (isCompiled == GL_FALSE) {
        // Get the length of the error message (which includes the ending NULL
        // character)
        GLint maxLength = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength);
        // Get the error message
        GLchar* errorLog = (GLchar*)malloc(maxLength);
        asserted(errorLog);
        glGetShaderInfoLog(shaderId, maxLength, &maxLength, errorLog);
        // Display and quit (no shader = fatal error)
        printf("GL shader compilation error (file %s line %d) : %s", filename, lineNbr, errorLog);
        free(errorLog);
        exit(1);
    }
}

#ifdef WITH_GL_CHECK
#define GL_CHECK()                     gfx::checkGlError(__FILE__, __LINE__)
#define GL_CHECK_COMPILATION(shaderId) gfx::checkGlShaderCompilation(__FILE__, __LINE__, shaderId)
#else
#define GL_CHECK()
#define GL_CHECK_COMPILATION(shaderId)
#endif  // WITH_GL_CHECK

class GlProgramVAO
{
   public:
    GlProgramVAO();
    ~GlProgramVAO();

    void install(const GLchar* vertexShaderSrc, const GLchar* fragmentShaderSrc);
    void deinstall();
    int  getId() const { return _programId; }
    int  getIboId() const { return _iboId; }
    int  getVboId() const { return _vboId; }
    int  getVaoId() const { return _vaoId; }

   private:
    int      _programId, _vertShaderId, _fragShaderId;
    uint32_t _vboId, _vaoId, _iboId;
};

// Data for 2D shader
struct Draw2d {
    float     x, y;      // Screen pos
    float     u, v;      // Texture coord (if used)
    bsColor_t color;     // Plain color
    float     alpha;     // Alpha multiplier
    uint32_t  mode;      // bitfield: bit0 = with texture  (if texture, then bit1 = bold  bit2 = italic)
    uint32_t  reserved;  // Alignment...
};

};  // namespace gfx
