#ifndef PHYS_H
#define PHYS_H

#include "vec3.h"
#include "decs.h"

struct phys_comp  {
    struct vec3 pos;
    struct vec3 vel;
    struct vec3 force;
    float mass;
};

/* Drag */

void phys_drag_tick(struct decs *decs, uint64_t eid, void *func_data);

struct phys_drag_ctx {
    struct phys_comp *phys_base;
};

const struct system_reg phys_drag_sys = {
    .name       = "phys_drag",
    .comp_names = STR_ARR("phys"),
    .func       = phys_drag_tick,
};

/* Gravity */

void phys_gravity_tick(struct decs *decs, uint64_t eid, void *func_data);

struct phys_gravity_ctx {
    struct phys_comp *phys_base;
};

const struct system_reg phys_gravity_sys = {
    .name       = "phys_gravity",
    .comp_names = STR_ARR("phys"),
    .func       = phys_gravity_tick,
};

/* Phys */

void phys_tick(struct decs *decs, uint64_t eid, void *func_data);

struct phys_ctx {
    struct phys_comp *phys_base;
};

const struct system_reg phys_sys = {
    .name       = "phys",
    .comp_names = STR_ARR("phys"),
    .func       = phys_tick,
    .dep_names  = STR_ARR("phys_drag", "phys_gravity"),
};

#endif
