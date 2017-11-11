#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>

#define SHADER_LAST 0

/* TODO Move somewhere else */
void *mmap_file(const char *path, size_t *length);

GLuint load_shader_file(const char *path, GLenum shader_type);
GLuint link_shader_prog(GLuint shader_id_0, ...);

#endif
