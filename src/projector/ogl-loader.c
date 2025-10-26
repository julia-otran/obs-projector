#include <string.h>

#include "ogl-loader.h"
#include "debug.h"
#include "shaders.h"

const char* get_shader_data(char *name) {
    if (strcmp("blend.fragment.shader", name) == 0) {
        return BLEND_FRAGMENT_SHADER;
    }
    if (strcmp("blend.vertex.shader", name) == 0) {
        return BLEND_VERTEX_SHADER;
    }
    if (strcmp("blur.fragment.shader", name) == 0) {
        return BLUR_FRAGMENT_SHADER;
    }
    if (strcmp("blur.vertex.shader", name) == 0) {
        return BLUR_VERTEX_SHADER;
    }
    if (strcmp("color-corrector.fragment.shader", name) == 0) {
        return COLOR_CORRECTOR_FRAGMENT_SHADER;
    }
    if (strcmp("color-corrector.vertex.shader", name) == 0) {
        return COLOR_CORRECTOR_VERTEX_SHADER;
    }
    if (strcmp("direct.fragment.shader", name) == 0) {
        return DIRECT_FRAGMENT_SHADER;
    }
    if (strcmp("direct.vertex.shader", name) == 0) {
        return DIRECT_VERTEX_SHADER;
    }
    
    log_debug("Failed getting shader: %s. Source not found", name);
    return "";
}

GLuint loadShader(GLuint type, char *name) {
    GLchar* shader_code[1] = { NULL };
    GLint shader_size[1] = { 0 };

    shader_code[0] = (GLchar*)get_shader_data(name);

    if (shader_code[0] == NULL) {
        log_debug("Shader load fail: '%s' not found.\n", name);
        return 0;
    }

    shader_size[0] = strlen(shader_code[0]);

    GLuint shaderID = glCreateShader(type);

    glShaderSource(shaderID, 1, (const GLchar * const *) shader_code, shader_size);
    glCompileShader(shaderID);

    GLint compileStatus;

    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus == 0) {
        GLsizei len;
        GLchar log_buffer[255];

        log_debug("Failed compiling shader: %s\n", name);
        log_debug("Shader SRC:\n\n")
        log_debug("--->\n%s<---\n", shader_code[0]);

        glGetShaderInfoLog(shaderID, sizeof(log_buffer) - 1, &len, (GLchar*) &log_buffer);

        log_buffer[len - 1] = 0;

        log_debug("%s\n", log_buffer);
    } else {
        log_debug("Shader compiled: %s\n", name);
    }

    return shaderID;
}

void tex_set_default_params() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
