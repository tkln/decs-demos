#include <alloca.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <unistd.h>

#include "shader.h"

void *mmap_file(const char *path, size_t *length)
{
    void *p = NULL;
    int fd;
    int ret;
    struct stat st;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return NULL;
    }

    ret = fstat(fd, &st);
    if (ret < 0) {
        perror("stat failed");
        goto err_close;
    }

    if (length)
        *length = st.st_size;

    p = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (!p)
        perror("mmap failed");

err_close:
    close(fd);
    return p;
}

GLuint load_shader_file(const char *path, GLenum shader_type)
{
    char *src;
    size_t src_len;
    GLuint shader_id;
    GLint compile_status;
    GLint info_log_len;
    char *info_log;

    src = mmap_file(path, &src_len);
    if (!src) {
        fprintf(stderr, "Loading shader \"%s\" failed\n", path);
        return 0;
    }

    shader_id = glCreateShader(shader_type);
    if (!shader_id)
        goto out_unmap;

    /*
     * XXX The length cast may cause things to overflow into the sign bit, I'll
     * deal with it once I see a shader that long. :-)
     */
    glShaderSource(shader_id, 1, (const char * const *)&src, (GLint *)&src_len);

    glCompileShader(shader_id);

    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_len);

    if (!compile_status) {
        info_log = alloca(info_log_len + 1);
        glGetShaderInfoLog(shader_id, info_log_len, &info_log_len, info_log);
        fprintf(stderr, "Shader compilation of \"%s\" failed:\n%s\n", path,
                info_log);
        shader_id = 0;
        goto out_delete_shader;
    }

    goto out_unmap;

out_delete_shader:
    glDeleteShader(shader_id);
out_unmap:
    munmap(src, src_len);
    return shader_id;
}

GLuint link_shader_prog(GLuint shader_id_0, ...)
{
    va_list shaders;
    GLuint id;
    GLuint shader_prog_id;

    GLint link_status;
    GLint info_log_len;
    char *info_log;

    shader_prog_id = glCreateProgram();

    glAttachShader(shader_prog_id, shader_id_0);

    va_start(shaders, shader_id_0);
    while ((id = va_arg(shaders, GLuint)))
        glAttachShader(shader_prog_id, id);
    va_end(shaders);

    glLinkProgram(shader_prog_id);

    glGetProgramiv(shader_prog_id, GL_LINK_STATUS, &link_status);
    glGetProgramiv(shader_prog_id, GL_INFO_LOG_LENGTH, &info_log_len);
    if (!link_status) {
        info_log = alloca(info_log_len + 1);
        glGetProgramInfoLog(shader_prog_id, info_log_len, &info_log_len, info_log);
        fprintf(stderr, "Shader linking failed:\n%s\n", info_log);
        return 0;
    }

    return shader_prog_id;
}
