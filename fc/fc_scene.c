/* Простая сцена для игр: игровые объекты с трансформом, скоростью и box-рендером.
 * Её можно использовать вместе с воксельным миром или как слой объектов игры. */
#include "fc/fc_engine.h"
#include "rbx/rbx_internal.h"
#include <math.h>
#include <string.h>

static FcEntity entities[FC_MAX_ENTITIES];
static int next_id = 1;

static int valid_entity(const FcEntity *e) { return e && e->alive && e->id > 0; }
static float safe_half(float value) { return isfinite(value) && value > 0.0f ? value : .5f; }
static int overlaps(float a0, float a1, float b0, float b1) { return a1 > b0 && a0 < b1; }

void fc_scene_clear(void) {
    memset(entities, 0, sizeof(entities));
    next_id = 1;
}
int fc_entity_spawn(const FcEntity *template_entity) {
    for (int i = 0; i < FC_MAX_ENTITIES; i++) if (!entities[i].alive) {
        FcEntity e = template_entity ? *template_entity : (FcEntity){0};
        e.id = next_id++;
        if (next_id <= 0) next_id = 1;
        e.alive = 1;
        if (e.kind == FC_ENTITY_EMPTY) e.kind = FC_ENTITY_BOX;
        e.hx = safe_half(e.hx); e.hy = safe_half(e.hy); e.hz = safe_half(e.hz);
        if (!e.color) e.color = 0xFFFFD54Fu;
        if (!e.flags) e.flags = FC_ENTITY_VISIBLE;
        e.name[FC_NAME_MAX - 1] = '\0';
        entities[i] = e;
        return e.id;
    }
    return 0;
}
FcEntity *fc_entity_get(int id) {
    if (id <= 0) return 0;
    for (int i = 0; i < FC_MAX_ENTITIES; i++) if (entities[i].alive && entities[i].id == id) return &entities[i];
    return 0;
}
const FcEntity *fc_entity_at(int index) {
    if (index < 0) return 0;
    int seen = 0;
    for (int i = 0; i < FC_MAX_ENTITIES; i++) if (entities[i].alive) {
        if (seen++ == index) return &entities[i];
    }
    return 0;
}
int fc_entity_count(void) {
    int count = 0;
    for (int i = 0; i < FC_MAX_ENTITIES; i++) count += entities[i].alive;
    return count;
}
int fc_entity_find(const char *name) {
    if (!name || !name[0]) return 0;
    for (int i = 0; i < FC_MAX_ENTITIES; i++)
        if (entities[i].alive && strcmp(entities[i].name, name) == 0) return entities[i].id;
    return 0;
}
void fc_entity_remove(int id) {
    FcEntity *e = fc_entity_get(id);
    if (e) memset(e, 0, sizeof(*e));
}
void fc_scene_update(float dt) {
    if (!isfinite(dt) || dt <= 0.0f) return;
    if (dt > .1f) dt = .1f;
    for (int i = 0; i < FC_MAX_ENTITIES; i++) if (entities[i].alive) {
        FcEntity *e = &entities[i];
        if (e->update) e->update(e, dt);
        e->x += e->vx * dt; e->y += e->vy * dt; e->z += e->vz * dt;
        e->yaw += e->vyaw * dt;
        if (!isfinite(e->x + e->y + e->z + e->yaw)) memset(e, 0, sizeof(*e));
    }
}
void fc_scene_draw_3d(void) {
    for (int i = 0; i < FC_MAX_ENTITIES; i++) if (valid_entity(&entities[i])) {
        const FcEntity *e = &entities[i];
        if (!(e->flags & FC_ENTITY_VISIBLE) || e->kind != FC_ENTITY_BOX) continue;
        rbx3d_box(e->x, e->y, e->z, e->hx, e->hy, e->hz, e->yaw, e->color);
    }
}
int fc_entity_intersects_player(int id) {
    const FcEntity *e = fc_entity_get(id);
    if (!valid_entity(e)) return 0;
    RbxPlayerState p;
    rbx_engine_player_state(&p);
    return overlaps(e->x - e->hx, e->x + e->hx, p.x - RBX_PLAYER_RADIUS, p.x + RBX_PLAYER_RADIUS) &&
           overlaps(e->y - e->hy, e->y + e->hy, p.y, p.y + RBX_PLAYER_HEIGHT) &&
           overlaps(e->z - e->hz, e->z + e->hz, p.z - RBX_PLAYER_RADIUS, p.z + RBX_PLAYER_RADIUS);
}
