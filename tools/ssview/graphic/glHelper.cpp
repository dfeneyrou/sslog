#include "glHelper.h"

namespace gfx
{

GlProgramVAO::GlProgramVAO() : _programId(0), _vertShaderId(0), _fragShaderId(0), _vboId(0), _vaoId(0), _iboId(0) {}

GlProgramVAO::~GlProgramVAO() { deinstall(); }

void
GlProgramVAO::install(const GLchar* vertexShaderSrc, const GLchar* fragmentShaderSrc)
{
    asserted(_programId == 0);
    // Create the place holders
    _programId    = glCreateProgram();
    _vertShaderId = glCreateShader(GL_VERTEX_SHADER);
    _fragShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Compile shaders
    glShaderSource(_vertShaderId, 1, &vertexShaderSrc, nullptr);
    glShaderSource(_fragShaderId, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(_vertShaderId);
    GL_CHECK_COMPILATION(_vertShaderId);
    glCompileShader(_fragShaderId);
    GL_CHECK_COMPILATION(_fragShaderId);

    // Link shaders
    glAttachShader(_programId, _vertShaderId);
    glAttachShader(_programId, _fragShaderId);
    glLinkProgram(_programId);
    GL_CHECK();

    glGenBuffers(1, &_vboId);
    glGenBuffers(1, &_iboId);

    glGenVertexArrays(1, &_vaoId);
    glBindVertexArray(_vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, _vboId);
    GL_CHECK();
}

void
GlProgramVAO::deinstall()
{
    if (_vaoId) {
        glDeleteVertexArrays(1, &_vaoId);
        GL_CHECK();
    }
    if (_vboId) {
        glDeleteBuffers(1, &_vboId);
        GL_CHECK();
    }
    if (_iboId) {
        glDeleteBuffers(1, &_iboId);
        GL_CHECK();
    }
    _vaoId = _vboId = _iboId = 0;

    if (_programId && _vertShaderId) {
        glDetachShader(_programId, _vertShaderId);
        GL_CHECK();
    }
    if (_vertShaderId) {
        glDeleteShader(_vertShaderId);
        GL_CHECK();
    }
    _vertShaderId = 0;

    if (_programId && _fragShaderId) {
        glDetachShader(_programId, _fragShaderId);
        GL_CHECK();
    }
    if (_fragShaderId) {
        glDeleteShader(_fragShaderId);
        GL_CHECK();
    }
    _fragShaderId = 0;

    if (_programId) {
        glDeleteProgram(_programId);
        GL_CHECK();
    }
    _programId = 0;
}

};  // namespace gfx
