#include <stdarg.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <GL/glew.h>

#include "ttf.h"
#include "shader.h"


static const char default_font_path[] =
		"/usr/share/fonts/dejavu/DejaVuSansMono.ttf";

static int pt_size = 12;
static TTF_Font *font;
static SDL_Renderer *rend;
static SDL_Window *win;

static GLuint tex_id;

static GLuint tex_quad_vao_id;
static GLuint tex_quad_vert_pos_vbo_id;
static GLuint tex_quad_tex_pos_vbo_id;

static GLuint vs_id;
static GLuint fs_id;
static GLuint shader_prog_id;

enum {
    VA_IDX_VERT_POS,
    VA_IDX_TEX_POS,
};

int ttf_init(SDL_Renderer *renderer, SDL_Window *window, const char *font_path)
{
    GLint link_status;
    GLint info_log_len;
    char *info_log;

    TTF_Init();

    rend = renderer;
    win = window;

    if (!font_path)
        font_path = default_font_path;

    font = TTF_OpenFont(font_path, pt_size);
    if (font == NULL) {
        fprintf(stderr, "Could not open font: %s", font_path);
        return -1;
    }

    vs_id = load_shader_file("./tex_quad_vs.glsl", GL_VERTEX_SHADER);
    if (!vs_id)
        return -1;

    fs_id = load_shader_file("./tex_quad_fs.glsl", GL_FRAGMENT_SHADER);
    if (!fs_id)
        return -1;

    shader_prog_id = glCreateProgram();
    glAttachShader(shader_prog_id, vs_id);
    glAttachShader(shader_prog_id, fs_id);
    glLinkProgram(shader_prog_id);

    glGetProgramiv(shader_prog_id, GL_LINK_STATUS, &link_status);
    glGetProgramiv(shader_prog_id, GL_INFO_LOG_LENGTH, &info_log_len);
    if (!link_status) {
        info_log = alloca(info_log_len + 1);
        glGetProgramInfoLog(shader_prog_id, info_log_len, &info_log_len, info_log);
        fprintf(stderr, "Shader linking failed:\n%s\n", info_log);
        return -1;
    }

    glGenVertexArrays(1, &tex_quad_vao_id);
    glBindVertexArray(tex_quad_vao_id);

    glGenBuffers(1, &tex_quad_vert_pos_vbo_id);
    glGenBuffers(1, &tex_quad_tex_pos_vbo_id);

    glGenTextures(1, &tex_id);

    static const GLfloat tex_coords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
    };

    glEnableVertexAttribArray(VA_IDX_TEX_POS);
    glBindBuffer(GL_ARRAY_BUFFER, tex_quad_tex_pos_vbo_id);
    glVertexAttribPointer(VA_IDX_TEX_POS, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords,
                 GL_STATIC_DRAW);

    return 0;
}

void ttf_render(unsigned x, unsigned y, const char *str)
{
    SDL_Rect rect;
    SDL_Surface *surf;
    SDL_Color color = {255, 255, 255, 0};
    SDL_Color bg_color = {0, 0, 0, 0};

    surf = TTF_RenderText_Blended(font, str, color);
    rect.x = x;
    rect.y = y;
    rect.w = surf->w;
    rect.h = surf->h;

    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, surf->pixels);

    GLfloat quad_verts[] = {
        x, y,
        x + surf->w, y,
        x, y + surf->h,
        x + surf->w, y + surf->h,
     };

    glBindVertexArray(tex_quad_vao_id);

    glEnableVertexAttribArray(VA_IDX_VERT_POS);
    glBindBuffer(GL_ARRAY_BUFFER, tex_quad_vert_pos_vbo_id);
    glVertexAttribPointer(VA_IDX_VERT_POS, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts,
                 GL_STATIC_DRAW);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glEnableVertexAttribArray(VA_IDX_TEX_POS);
    glEnableVertexAttribArray(VA_IDX_VERT_POS);

    glUseProgram(shader_prog_id);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(VA_IDX_VERT_POS);
    glDisableVertexAttribArray(VA_IDX_TEX_POS);

    SDL_FreeSurface(surf);
}

int ttf_printf(unsigned x, unsigned y, const char *fmt, ...)
{
    va_list sz_vargs, vargs;
    int sz;
    char *buf;

    va_start(sz_vargs, fmt);
    va_copy(vargs, sz_vargs);
    sz = vsnprintf(NULL, 0, fmt, sz_vargs) + 1;
    va_end(sz_vargs);

    buf = malloc(sz);
    sz = vsnprintf(buf, sz, fmt, vargs);
    va_end(vargs);

    ttf_render(x, y, buf);
    free(buf);

    return sz;
}
