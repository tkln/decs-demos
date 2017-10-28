#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <SDL2/SDL.h>

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

    SDL_Init(SDL_INIT_EVERYTHING);

    /* TODO error checks */
    win = SDL_CreateWindow("title", SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED, win_w, win_h, 0);
    rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    render_ctx_aux.rend = rend;
    render_ctx_aux.win = win;
    render_sys.aux_ctx = &render_ctx_aux;

    decs_init(&decs);
    ttf_init(rend, win, NULL);

    comp_ids.phys = decs_register_comp(&decs, "phys", sizeof(struct phys_comp));
    comp_ids.color = decs_register_comp(&decs, "color",
                                        sizeof(struct color_comp));

    for (i = 0; i < sizeof(systems) / sizeof(systems[0]); ++i) {
        ret = decs_register_system(&decs, systems[i], NULL);
        if (ret < 0) {
            fprintf(stderr, "Error occurred while registering system \"%s\"\n",
                    systems[i]->name);
            goto sys_reg_fail;
        }
    }

    while (runnig) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                runnig = 0;
        }

        create_particle(&decs, &comp_ids);

        SDL_SetRenderDrawColor(rend, 0x0, 0x0, 0x0, 0xff);
        SDL_RenderClear(rend);

        decs_tick(&decs);
        render_system_perf_stats(&decs);

        SDL_RenderPresent(rend);

        SDL_Delay(16);
    }

    decs_cleanup(&decs);

sys_reg_fail:
    SDL_DestroyWindow(win);
    SDL_Quit();

    return ret;
}
