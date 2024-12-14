#pragma once
#include <string>
#include <fstream>
#include <sstream>

namespace gl {

inline void checkError(const char* location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        WARN("OpenGL error at %s: 0x%x", location, err);
    }
}

inline std::string readShaderFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        WARN("Failed to open shader file: %s", filePath.c_str());
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline GLuint compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        WARN("Shader compilation failed: %s", infoLog);
        return 0;
    }
    return shader;
}

} // namespace gl 
