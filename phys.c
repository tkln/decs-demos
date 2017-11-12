#include "phys.h"

void phys_drag_tick(struct decs *, uint64_t, void *);
void phys_gravity_tick(struct decs *, uint64_t, void *);
void phys_pre_col_tick(struct decs *, uint64_t, void *);
void phys_wall_col_tick(struct decs *, uint64_t, void *);
void phys_post_col_tick(struct decs *, uint64_t, void *);

struct phys_drag_ctx {
    struct phys_dyn_comp *phys_base;
};

struct phys_gravity_ctx {
    struct phys_dyn_comp *phys_base;
};

struct phys_ctx {
    struct phys_pos_comp *phys_pos_base;
    struct phys_dyn_comp *phys_dyn_base;
};

const struct system_reg phys_drag_sys = {
    .name       = "phys_drag",
    .comp_names = STR_ARR("phys_dyn"),
    .func       = phys_drag_tick,
};

const struct system_reg phys_gravity_sys = {
    .name       = "phys_gravity",
    .comp_names = STR_ARR("phys_dyn"),
    .func       = phys_gravity_tick,
};

const struct system_reg phys_pre_col_sys = {
    .name       = "phys_pre_col",
    .comp_names = STR_ARR("phys_pos", "phys_dyn"),
    .func       = phys_pre_col_tick,
    .dep_names  = STR_ARR("phys_drag", "phys_gravity"),
};

const struct system_reg phys_wall_col_sys = {
    .name       = "phys_wall_col",
    .comp_names = STR_ARR("phys_pos", "phys_dyn"),
    .func       = phys_wall_col_tick,
    .dep_names  = STR_ARR("phys_pre_col"),
};

const struct system_reg phys_post_col_sys = {
    .name       = "phys_post_col",
    .comp_names = STR_ARR("phys_pos", "phys_dyn"),
    .func       = phys_post_col_tick,
    .dep_names  = STR_ARR("phys_wall_col"),
};

void phys_drag_tick(struct decs *decs, uint64_t eid, void *func_data)
{
    struct phys_drag_ctx *ctx = func_data;
    struct phys_dyn_comp *phys = ctx->phys_base + eid;


#if 1
    const float fluid_density = 1.2f; /* Air */
#else
    const float fluid_density = 999.9f; /* Water */
#endif
    const float area = 10.10f;
    const float drag_coef = 0.47f; // Sphere
    const float total_drag_coef = -0.5f * fluid_density * drag_coef * area;
    const struct vec3 vel2 = vec3_spow2(phys->vel);
    struct vec3 drag_force = vec3_muls(vel2, total_drag_coef);

    phys->force = vec3_add(phys->force, drag_force);
}

void phys_gravity_tick(struct decs *decs, uint64_t eid, void *func_data)
{
    struct phys_gravity_ctx *ctx = func_data;
    struct phys_dyn_comp *phys = ctx->phys_base + eid;

    phys->force.y -= 9.81f;
}

static void phys_euler_tick(struct phys_dyn_comp *phys, struct vec3 force,
                            float dt)
{
    struct vec3 acc = vec3_muls(force, 1.0f / phys->mass);

    struct vec3 d_vel = vec3_muls(acc, dt);

    phys->vel = vec3_add(phys->vel, d_vel);
    phys->d_pos = vec3_muls(phys->vel, dt);
}

void phys_pre_col_tick(struct decs *decs, uint64_t eid, void *func_data)
{
    struct phys_ctx *phys_ctx = func_data;
    struct phys_dyn_comp *phys_dyn = phys_ctx->phys_dyn_base + eid;

    float dt = 1.0f / 60.0f;

    phys_euler_tick(phys_dyn, phys_dyn->force, dt);

    phys_dyn->force = (struct vec3){ 0.0f, 0.0f, 0.0f };
}

void phys_wall_col_tick(struct decs *decs, uint64_t eid, void *func_data)
{
    struct phys_ctx *phys_ctx = func_data;
    struct phys_pos_comp *phys = phys_ctx->phys_pos_base + eid;
    struct phys_dyn_comp *phys_dyn = phys_ctx->phys_dyn_base + eid;
    struct vec3 pos = vec3_add(phys->pos, phys_dyn->d_pos);
    const float epsilon = 0.005f;

    if (pos.y > 1.0f || pos.y < -1.0f) {
        phys_dyn->vel.y *= -0.9f;
        phys->pos.y *= 1.0f - epsilon;
    }
#if 0
    if (pos.x > 1.0f)
        phys->pos.x -= 1.0f;
    if (pos.x < -1.0f)
        phys->pos.x += 1.0f;
#endif
}

void phys_post_col_tick(struct decs *decs, uint64_t eid, void *func_data)
{
    struct phys_ctx *phys_ctx = func_data;
    struct phys_pos_comp *phys_pos = phys_ctx->phys_pos_base + eid;
    struct phys_dyn_comp *phys_dyn = phys_ctx->phys_dyn_base + eid;

    phys_pos->pos = vec3_add(phys_pos->pos, phys_dyn->d_pos);
}

