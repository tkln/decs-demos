#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>

#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "decs.h"
#include "vec3.h"
#include "ttf.h"
#include "phys.h"
#include "shader.h"
#include "decs/decs.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

struct color_comp {
    union {
        struct vec3 color;
        struct {
            float r, g, b;
        };
    };
};

struct comp_ids {
    uint64_t phys_pos;
    uint64_t phys_dyn;
    uint64_t color;
};

static const GLfloat triangle_verts[] = {
    -0.04f, -0.04f, 0.0f,
     0.04f, -0.04f, 0.0f,
    -0.04f,  0.04f, 0.0f,
     0.04f,  0.04f, 0.0f,
};

int win_w = 1280, win_h = 720;

void create_particle(struct decs *decs, struct comp_ids *comp_ids,
                     struct vec3 spawn_point)
{
    struct phys_pos_comp *phys_pos;
    struct phys_dyn_comp *phys_dyn;
    struct color_comp *color;
    uint64_t eid;

    eid = decs_alloc_entity(decs, (1<<comp_ids->phys_pos) |
                                  (1<<comp_ids->phys_dyn) |
                                  (1<<comp_ids->color));

    phys_pos = decs_get_comp(decs, comp_ids->phys_pos, eid);
    phys_dyn = decs_get_comp(decs, comp_ids->phys_dyn, eid);
    color = decs_get_comp(decs, comp_ids->color, eid);

    *color = (struct color_comp) {
        sinf(eid * 0.001f) * 1 + 1.0f,
        cosf(eid * 0.003f) * 0.25f + 0.50f,
        sinf(eid * 0.002f) * 0.5f + 1.5f,
    };

    eid += rand();

    *phys_pos = (struct phys_pos_comp) {
        .pos = spawn_point,
    };
    *phys_dyn = (struct phys_dyn_comp) {
        .vel = (struct vec3) {
            cosf(eid * 0.05f) * 0.5f,
            sinf(eid * 0.05f) * 0.5f,
            0.0f,
        },
        .force = { 0.0f, 0.0f, 0.0f },
        .mass = 7.0f
    };
}

static void render_system_perf_stats(const struct decs *decs)
{
    unsigned pt_size = 16;
    unsigned i, j;
    struct perf_stats *stats;

    struct {
        const char *name;
        const size_t offset;
    } prints[] = {
        {
            .name = "cpu cycles:     ",
            .offset = offsetof(struct perf_stats, cpu_cycles),
        }, {
            .name = "l3 cache refs:  ",
            .offset = offsetof(struct perf_stats, cache_refs),
        }, {
            .name = "l3 cache misses:",
            .offset = offsetof(struct perf_stats, cpu_cycles),
        }, {
            .name = "branch instrs:  ",
            .offset = offsetof(struct perf_stats, cache_refs),
        }, {
            .name = "branchs misses: ",
            .offset = offsetof(struct perf_stats, cpu_cycles),
        }
    };

    const unsigned n_prints = ARRAY_SIZE(prints) + 1;

    ttf_printf(0, 0, "entity count: %zu", decs->n_entities);
    for (i = 0; i < decs->n_systems; ++i) {
        stats = &decs->systems[i].perf_stats;
        ttf_printf(0, pt_size * (1 + i * n_prints), "%s:", decs->systems[i].name);
        for (j = 0; j < ARRAY_SIZE(prints); ++j) {
            long long val = ((long long *)stats)[j];
            ttf_printf(64, pt_size * (2 + j + i * n_prints), "%s %lld, (%.2f)",
                       prints[j].name, val, (double)val / decs->n_entities);
        }
    }
}

int main(void)
{
    struct decs decs;
    struct comp_ids comp_ids;
    int running = 1;
    int n_particles = 0;
    int i;
    int ret = 0;
    struct vec3 spawn_point = { 0.0f, 0.25f, 0.0f };
    int particle_rate = 20;
    const struct system_reg *systems[] = {
        &phys_gravity_sys,
        &phys_drag_sys,
        &phys_integrate_sys,
        &phys_wall_col_sys,
        &phys_post_col_sys,
    };

    SDL_Window *win;
    SDL_Renderer *rend;
    SDL_Event event;
    SDL_GLContext sdl_gl_ctx;

    /* TODO Clean these up */

    GLuint vao_id;
    GLuint vertex_vbo_id;
    GLuint particle_pos_vbo_id;
    GLuint particle_color_vbo_id;
    GLuint vs_id;
    GLuint fs_id;
    GLuint shader_prog_id;

    enum {
        VA_IDX_VERT,
        VA_IDX_POS,
        VA_IDX_COLOR
    };

    SDL_Init(SDL_INIT_EVERYTHING);

    /* TODO error checks */
    win = SDL_CreateWindow("title", SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED, win_w, win_h,
                           SDL_WINDOW_OPENGL);

    rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    SDL_GL_SetSwapInterval(1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdl_gl_ctx = SDL_GL_CreateContext(win);
    glewExperimental = GL_TRUE;
    glewInit();

    glDisable(GL_DEPTH_TEST); /* XXX */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    decs_init(&decs);
    ttf_init(rend, win, NULL);

    vs_id = load_shader_file("./particle_vs.glsl", GL_VERTEX_SHADER);
    if (!vs_id) {
        ret = EXIT_FAILURE;
        goto out_sdl_tear_down;
    }

    fs_id = load_shader_file("./particle_fs.glsl", GL_FRAGMENT_SHADER);
    if (!fs_id) {
        ret = EXIT_FAILURE;
        goto out_sdl_tear_down;
    }

    shader_prog_id = link_shader_prog(fs_id, vs_id, SHADER_LAST);
    if (!shader_prog_id) {
        ret = EXIT_FAILURE;
        goto out_sdl_tear_down;
    }

    glGenVertexArrays(1, &vao_id);
    glBindVertexArray(vao_id);

    glGenBuffers(1, &vertex_vbo_id);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_verts), triangle_verts,
                 GL_STATIC_DRAW);

    glGenBuffers(1, &particle_pos_vbo_id);
    glBindBuffer(GL_ARRAY_BUFFER, particle_pos_vbo_id);

    glGenBuffers(1, &particle_color_vbo_id);
    glBindBuffer(GL_ARRAY_BUFFER, particle_color_vbo_id);

    glUseProgram(shader_prog_id);

    glEnableVertexAttribArray(VA_IDX_VERT);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo_id);
    glVertexAttribPointer(VA_IDX_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(VA_IDX_POS);
    glBindBuffer(GL_ARRAY_BUFFER, particle_pos_vbo_id);
    glVertexAttribPointer(VA_IDX_POS, 3, GL_FLOAT, GL_FALSE,
                          sizeof(struct phys_dyn_comp), 0);

    glEnableVertexAttribArray(VA_IDX_COLOR);
    glBindBuffer(GL_ARRAY_BUFFER, particle_color_vbo_id);
    glVertexAttribPointer(VA_IDX_COLOR, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glVertexAttribDivisor(VA_IDX_VERT, 0); /* Vertices aren't instanced */
    /* Particle positions and colors are unique to each instance */
    glVertexAttribDivisor(VA_IDX_POS, 1);
    glVertexAttribDivisor(VA_IDX_COLOR, 1);

    comp_ids.phys_pos = decs_register_comp(&decs, "phys_pos",
                                           sizeof(struct phys_pos_comp));
    comp_ids.phys_dyn = decs_register_comp(&decs, "phys_dyn",
                                           sizeof(struct phys_dyn_comp));
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

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_MOUSEBUTTONDOWN:
                spawn_point.x = event.button.x * 2.0f / win_h - 1.0f *
                                (win_w / (float)win_h);
                spawn_point.y = -event.button.y * 2.0f / win_h + 1.0f;
                break;
            case SDL_MOUSEWHEEL:
                particle_rate += event.wheel.y;
                printf("%d p/s\n", 60 * particle_rate);
                break;
            default:
                break;
            }
        }

        for (i = 0; i < particle_rate; ++i)
            create_particle(&decs, &comp_ids, spawn_point);

        decs_tick(&decs);

        n_particles = decs.n_entities;

        glBindVertexArray(vao_id);
        glBindBuffer(GL_ARRAY_BUFFER, particle_pos_vbo_id);
        glBufferData(GL_ARRAY_BUFFER,
                     n_particles * sizeof(struct phys_pos_comp), NULL,
                     GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        n_particles * sizeof(struct phys_pos_comp),
                        decs.comps[comp_ids.phys_pos].data);
        glVertexAttribPointer(VA_IDX_POS, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, particle_color_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, n_particles * sizeof(struct vec3), NULL,
                     GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, n_particles * sizeof(struct vec3),
                        decs.comps[comp_ids.color].data);
        glVertexAttribPointer(VA_IDX_COLOR, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnableVertexAttribArray(VA_IDX_VERT);
        glEnableVertexAttribArray(VA_IDX_POS);
        glEnableVertexAttribArray(VA_IDX_COLOR);

        glUseProgram(shader_prog_id);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, n_particles);

        glDisableVertexAttribArray(VA_IDX_VERT);
        glDisableVertexAttribArray(VA_IDX_POS);
        glDisableVertexAttribArray(VA_IDX_COLOR);

        render_system_perf_stats(&decs);

        SDL_GL_SwapWindow(win);
    }

    decs_cleanup(&decs);

out_sdl_tear_down:
    SDL_GL_DeleteContext(sdl_gl_ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return ret;
}
