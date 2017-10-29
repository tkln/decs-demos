#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "decs.h"
#include "vec3.h"
#include "ttf.h"
#include "phys.h"

struct color_comp {
    union {
        struct vec3 color;
        struct {
            float r, g, b;
        };
    };
};

struct comp_ids {
    uint64_t phys;
    uint64_t color;
};

struct render_ctx_aux {
    SDL_Window *win;
    SDL_Renderer *rend;
};

struct render_ctx {
    struct render_ctx_aux *aux;
    struct phys_comp *phys_base;
    struct color_comp *color_base;
};

static const GLfloat triangle_verts[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f,
};

int win_w = 1280, win_h = 720;

static void render_tick(struct decs *decs, uint64_t eid, void *func_data)
{
    struct render_ctx *ctx = func_data;
    struct phys_comp *phys = ctx->phys_base + eid;
    struct color_comp *color = ctx->color_base + eid;

    SDL_SetRenderDrawColor(ctx->aux->rend, 0xff * color->r, 0xff * color->g,
                           0xff * color->b, 0xff);
    SDL_RenderDrawPoint(ctx->aux->rend, phys->pos.x * win_w, phys->pos.y * win_h);

}

static struct system_reg render_sys = {
    .name       = "render",
    .comp_names = STR_ARR("phys", "color"),
    .func       = render_tick,
    .dep_names  = STR_ARR("phys_post_col"),
};

void create_particle(struct decs *decs, struct comp_ids *comp_ids)
{
    struct phys_comp *phys;
    struct color_comp *color;
    uint64_t eid;

    eid = decs_alloc_entity(decs, (1<<comp_ids->phys) | (1<<comp_ids->color));

    phys = decs_get_comp(decs, comp_ids->phys, eid);
    color = decs_get_comp(decs, comp_ids->color, eid);

    phys->pos.x = 0.5f;
    phys->pos.y = 0.25f;
    phys->pos.z = 0.0f;
    phys->vel.x = cos(eid * 0.25) * 0.2f;
    phys->vel.y = sin(eid * 0.25) * 0.2f;
    phys->vel.z = sin(eid * 0.25) * 0.2f;
    phys->force.x = 0.0f;
    phys->force.y = 0.0f;
    phys->force.z = 0.0f;
    phys->mass = 7.0f;
    color->r = sin(eid * 0.01) * 2;
    color->g = cos(eid * 0.03) * 2;
    color->b = eid * 0.02 * 2;
}

static void *mmap_file(const char *path, size_t *length)
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

static GLuint load_shader_file(const char *path, GLenum shader_type)
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

static void render_system_perf_stats(const struct decs *decs)
{
    int pt_size = 16;
    size_t i;
    struct perf_stats *stats;

    ttf_printf(0, 0, "entity count: %zu", decs->n_entities);
    for (i = 0; i < decs->n_systems; ++i) {
        stats = &decs->systems[i].perf_stats;
        ttf_printf(0, pt_size * (1 + i * 6), "%s:", decs->systems[i].name);
        ttf_printf(64, pt_size * (2 + i * 6), "cpu cycles: %lld, (%lld)",
                 stats->cpu_cycles, stats->cpu_cycles / decs->n_entities);
        ttf_printf(64, pt_size * (3 + i * 6), "l3 cache refs: %lld, (%lld)",
                 stats->cache_refs, stats->cache_refs / decs->n_entities);
        ttf_printf(64, pt_size * (4 + i * 6), "l3 cache misses: %lld, (%lld)",
                   stats->cache_misses,
                   stats->cache_misses / decs->n_entities);
        ttf_printf(64, pt_size * (5 + i * 6),
                   "branch instructions: %lld, (%lld)", stats->branch_instrs,
                   stats->branch_instrs / decs->n_entities);
        ttf_printf(64, pt_size * (6 + i * 6), "branch misses: %lld, (%lld)",
                   stats->branch_misses,
                   stats->branch_misses / decs->n_entities);
    }
}

int main(void)
{
    struct decs decs;
    struct comp_ids comp_ids;
    uint64_t render_comps;
    int runnig = 1;
    int i;
    int mx, my;
    int ret = 0;
    const struct system_reg *systems[] = {
        &phys_gravity_sys,
        &phys_drag_sys,
        &phys_pre_col_sys,
        &phys_wall_col_sys,
        &phys_post_col_sys,
        &render_sys,
    };

    struct render_ctx_aux render_ctx_aux;

    SDL_Window *win;
    SDL_Renderer *rend;
    SDL_Event event;
    SDL_GLContext sdl_gl_ctx;

    GLuint vao_id;
    GLuint vbo_id;
    GLuint vs_id;
    GLuint fs_id;
    GLuint shader_prog_id;


    SDL_Init(SDL_INIT_EVERYTHING);

    /* TODO error checks */
    win = SDL_CreateWindow("title", SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED, win_w, win_h,
                           SDL_WINDOW_OPENGL);

    rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdl_gl_ctx = SDL_GL_CreateContext(win);
    glewExperimental = GL_TRUE;
    glewInit();

    render_ctx_aux.rend = rend;
    render_ctx_aux.win = win;
    render_sys.aux_ctx = &render_ctx_aux;

    decs_init(&decs);
    ttf_init(rend, win, NULL);

    vs_id = load_shader_file("./vs.glsl", GL_VERTEX_SHADER);
    if (!vs_id) {
        ret = EXIT_FAILURE;
        goto out_sdl_tear_down;
    }

    fs_id = load_shader_file("./fs.glsl", GL_FRAGMENT_SHADER);
    if (!fs_id) {
        ret = EXIT_FAILURE;
        goto out_sdl_tear_down;
    }

    shader_prog_id = glCreateProgram();
    glAttachShader(shader_prog_id, vs_id);
    glAttachShader(shader_prog_id, fs_id);
    glLinkProgram(shader_prog_id);


    glGenVertexArrays(1, &vao_id);
    glBindVertexArray(vao_id);

    glGenBuffers(1, &vbo_id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_verts), triangle_verts,
                 GL_STATIC_DRAW);

    glUseProgram(shader_prog_id);

    glDisable(GL_DEPTH_TEST);

    comp_ids.phys = decs_register_comp(&decs, "phys", sizeof(struct phys_comp));
    comp_ids.color = decs_register_comp(&decs, "color",
                                        sizeof(struct color_comp));

    for (i = 0; i < sizeof(systems) / sizeof(systems[0]); ++i) {
        ret = decs_register_system(&decs, systems[i], NULL);
        if (ret < 0) {
            fprintf(stderr, "Error occurred while registering system \"%s\"\n",
                    systems[i]->name);
            goto out_sdl_tear_down;
        }
    }


    while (runnig) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                runnig = 0;
        }

        create_particle(&decs, &comp_ids);

        glClearColor(0.0, 0.0, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        //decs_tick(&decs);

        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisableVertexAttribArray(0);

        //render_system_perf_stats(&decs);

        SDL_GL_SwapWindow(win);

        //SDL_RenderPresent(rend);

        SDL_Delay(16);
    }

    decs_cleanup(&decs);

out_sdl_tear_down:
    SDL_GL_DeleteContext(sdl_gl_ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return ret;
}
