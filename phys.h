#ifndef PHYS_H
#define PHYS_H

#include "vec3.h"
#include "decs.h"

struct phys_pos_comp {
    struct vec3 pos;
};

struct phys_dyn_comp  {
    struct vec3 d_pos;
    struct vec3 vel;
    struct vec3 force;
    float mass;
};

const struct system_reg phys_drag_sys;
const struct system_reg phys_gravity_sys;
const struct system_reg phys_integrate_sys;
const struct system_reg phys_wall_col_sys;
const struct system_reg phys_post_col_sys;

#endif
