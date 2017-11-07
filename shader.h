#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>

/* TODO Move somewhere else */
void *mmap_file(const char *path, size_t *length);

GLuint load_shader_file(const char *path, GLenum shader_type);

#endif
