#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "phys.h"
#include "phys_sphere_col.h"

static void phys_sphere_col_build_tick(struct decs *decs, uint64_t eid,
                                       void *func_data);

struct phys_sphere_col_build_ctx {
    struct phys_col_world *phys_col_world; /* AUX */
    struct phys_pos_comp *phys_pos_base;
    struct phys_sphere_comp *phys_sphere_base;
};

const struct system_reg phys_sphere_col_build_sys = {
    .name       = "phys_sphere_col_build",
    .comps      = STR_ARR("phys_pos", "phys_sphere_col"),
    .icomps     = STR_ARR("phys_dyn"),
    .func       = phys_sphere_col_build_tick,
    .pre_deps   = STR_ARR("phys_integrate"),
};

static void phys_sphere_col_tick(struct decs *decs, uint64_t eid,
                                 void *func_data);

struct phys_sphere_col_ctx {
    struct phys_col_world *phys_col_world; /* AUX */
    struct phys_pos_comp *phys_pos_base;
    struct phys_dyn_comp *phys_dyn_base;
    struct phys_sphere_comp *phys_sphere_base;
};

const struct system_reg phys_sphere_col_sys = {
    .name       = "phys_sphere_col",
    .comps      = STR_ARR("phys_pos", "phys_dyn", "phys_sphere_col"),
    .func       = phys_sphere_col_tick,
    .pre_deps   = STR_ARR("phys_sphere_col_build"),
    .post_deps  = STR_ARR("phys_post_col"),
};

struct phys_col_sphere {
    struct vec3 c;
    float r;
};

static void phys_sphere_col_build_tick(struct decs *decs, uint64_t eid,
                                       void *func_data)
{
    struct phys_sphere_col_build_ctx *ctx = func_data;
    struct phys_pos_comp *pos = ctx->phys_pos_base + eid;
    struct phys_sphere_comp *sph = ctx->phys_sphere_base + eid;
    struct phys_col_world *world = ctx->phys_col_world;

    if (world->n_spheres + 1 >= world->n_allocd_spheres) {
        if (!world->n_allocd_spheres)
            world->n_allocd_spheres = 1;
        world->n_allocd_spheres *= 2;
        world->spheres = realloc(world->spheres, sizeof(*world->spheres) *
                                                 world->n_allocd_spheres);
    }

    world->spheres[world->n_spheres] = (struct phys_col_sphere) {
        .c = pos->pos,
        .r = sph->r,
    };

    ++world->n_spheres;
}

static inline int phys_sphere_col_test(struct phys_col_sphere *a,
                                       struct vec3 bc, float br)
{
    return vec3_norm2(vec3_sub(a->c, bc)) < (a->r + br) * (a->r + br);
}

static void phys_sphere_col_tick(struct decs *decs, uint64_t eid,
                                 void *func_data)
{
    struct phys_sphere_col_ctx *ctx = func_data;
    struct phys_pos_comp *pos = ctx->phys_pos_base + eid;
    struct phys_dyn_comp *dyn = ctx->phys_dyn_base + eid;
    struct phys_sphere_comp *sph = ctx->phys_sphere_base + eid;
    struct phys_col_world *world = ctx->phys_col_world;
    size_t i;


    for (i = 0; i < world->n_spheres; ++i) {
        if (phys_sphere_col_test(world->spheres + i, pos->pos, sph->r)) {
            /* TODO */
            dyn->vel.y += 0.1f;
        }
    }
}

void phys_col_world_init(struct phys_col_world *world)
{
    memset(world, 0, sizeof(*world));
}

void phys_col_world_tick(struct phys_col_world *world)
{
    world->n_spheres = 0;
}

void phys_col_world_cleanup(struct phys_col_world *world)
{
    free(world->spheres);
}
