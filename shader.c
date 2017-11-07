#include <alloca.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "shader.h"

void *mmap_file(const char *path, size_t *length)
{
    void *p;
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
        p = NULL;
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
    if (!src)
        return 0;

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


