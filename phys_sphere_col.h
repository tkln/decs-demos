#ifndef SPHERE_COL_H
#define SPHERE_COL_H

#include <decs.h>

#include "vec3.h"

struct phys_sphere_comp {
    float r;
};

const struct system_reg phys_sphere_col_build_sys;
const struct system_reg phys_sphere_col_sys;

struct phys_col_sphere;

struct phys_col_world {
    struct phys_col_sphere *spheres;
    size_t n_spheres;
    size_t n_allocd_spheres;
};

void phys_col_world_init(struct phys_col_world *world);

/* 
 * This is meant to run once per game tick, not per entity and therefore it's
 * not registered with decs core as a system and has to be run manually after
 * decs_tick.
 */
void phys_col_world_tick(struct phys_col_world *world);

void phys_col_world_cleanup(struct phys_col_world *world);

#endif
