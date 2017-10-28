#include "phys.h"

void phys_drag_tick(struct decs *decs, uint64_t eid, void *func_data)
{
    struct phys_drag_ctx *ctx = func_data;
    struct phys_comp *phys = ctx->phys_base + eid;


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

static void phys_euler_tick(struct phys_comp *phys, struct vec3 force, float dt)
{
    struct vec3 acc = vec3_muls(force, 1.0f / phys->mass);

    struct vec3 d_vel = vec3_muls(acc, dt);
    struct vec3 d_pos = vec3_muls(phys->vel, dt);

    phys->vel = vec3_add(phys->vel, d_vel);
    phys->pos = vec3_add(phys->pos, d_pos);
}

void phys_tick(struct decs *decs, uint64_t eid, void *func_data)
{
    struct phys_ctx *phys_ctx = func_data;
    struct phys_comp *phys = phys_ctx->phys_base + eid;

    float dt = 1.0f / 60.0f;

    phys_euler_tick(phys, phys->force, dt);

    phys->force = (struct vec3){ 0.0f, 0.0f, 0.0f };

    /* TODO remove these */
    if (phys->pos.y > 1.0f) {
        phys->vel.y *= -0.9f;
    }
    if (phys->pos.x > 1.0f)
        phys->pos.x -= 1.0f;
    if (phys->pos.x < 0.0f)
        phys->pos.x += 1.0f;
}

void phys_gravity_tick(struct decs *decs, uint64_t eid, void *func_data)
{
    struct phys_gravity_ctx *ctx = func_data;
    struct phys_comp *phys = ctx->phys_base + eid;

    phys->force  = vec3_add(phys->force, (struct vec3) { 0.0f, 9.81f, 0.0f });
}


