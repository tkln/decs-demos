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

static inline int phys_sphere_col_test(struct phys_col_sphere a,
                                       struct phys_col_sphere b)
{
    return vec3_norm2(vec3_sub(a.c, b.c)) < (a.r + b.r) * (a.r + b.r);
}

static void phys_sphere_col_tick(struct decs *decs, uint64_t eid,
                                 void *func_data)
{
    struct phys_sphere_col_ctx *ctx = func_data;
    struct phys_pos_comp *pos = ctx->phys_pos_base + eid;
    struct phys_dyn_comp *dyn = ctx->phys_dyn_base + eid;
    struct phys_sphere_comp *sph = ctx->phys_sphere_base + eid;
    struct phys_col_world *world = ctx->phys_col_world;
    struct phys_col_sphere sph_a = {
        .c = vec3_add(pos->pos, dyn->d_pos),
        .r = sph->r,
    };
    struct phys_col_sphere sph_b;
    struct vec3 n;
    struct vec3 v;
    size_t i;
    bool clear;

    for (i = 0, clear = true; i < world->n_spheres && clear; ++i) {
        sph_b = world->spheres[i];
        if (phys_sphere_col_test(sph_b, sph_a)) {
            n = vec3_normalize(vec3_sub(sph_b.c, sph_a.c));
            v = dyn->vel;

            dyn->d_pos = vec3_muls(dyn->d_pos, -0.1f);
            dyn->vel = vec3_sub(v, vec3_muls(n, 2 * vec3_dot(v, n)));
            clear = false;
        }
    }

    /*
     * When a collision was detected, the entity was backed out along -d_pos.
     * In order to properly resolve the collision, it has to be verified that
     * the new position is also valid. The loop below backs the entity out
     * until no collisions can be found
     */
    while (!clear) {
        sph_a.c = vec3_add(pos->pos, dyn->d_pos);
        for (i = 0, clear = true; i < world->n_spheres && clear; ++i) {
            sph_b = world->spheres[i];
            if (phys_sphere_col_test(sph_b, sph_a)) {
                n = vec3_normalize(vec3_sub(sph_b.c, sph_a.c));
                v = dyn->vel;

                dyn->d_pos = vec3_add(dyn->d_pos, dyn->d_pos);
                clear = false;
            }
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
