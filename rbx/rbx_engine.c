/* FaiCraft Engine: конфигурация, жизненный цикл и публичные операции мира. */
#include "rbx_internal.h"
#include <math.h>
#include <string.h>

#define DEFAULT_FULLRES_PIXELS 1500000
#define EDIT_REACH 6.0f

double rbx_t_abs;
static RbxEngineConfig cfg;
static int cfg_ready;
static int edit_q_down, edit_e_down;

void rbx_engine_default_config(RbxEngineConfig *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->seed = RBX_DEFAULT_WORLD_SEED;
    out->spawn_x = 8.5f;
    out->spawn_y = RBX_ENGINE_AUTO_Y;
    out->spawn_z = 8.5f;
    out->spawn_yaw = .7f;
    out->spawn_pitch = -.16f;
    out->fov_deg = 66.0f;
    out->fog_start = RBX_DEFAULT_FOG_START;
    out->fog_end = RBX_DEFAULT_FOG_END;
    out->view_distance = RBX_DEFAULT_VIEW_DISTANCE;
    out->fullres_pixel_limit = DEFAULT_FULLRES_PIXELS;
    out->sky_top = 0xFF78B8E8u;
    out->sky_bottom = 0xFFC7E5F5u;
}

static float finite_or(float value, float fallback) { return isfinite(value) ? value : fallback; }

void rbx_engine_configure(const RbxEngineConfig *config) {
    RbxEngineConfig next;
    if (config) next = *config;
    else rbx_engine_default_config(&next);

    next.spawn_x = finite_or(next.spawn_x, 8.5f);
    next.spawn_z = finite_or(next.spawn_z, 8.5f);
    if (!isfinite(next.spawn_y) || next.spawn_y < RBX_ENGINE_AUTO_Y * .5f) next.spawn_y = RBX_ENGINE_AUTO_Y;
    next.spawn_yaw = finite_or(next.spawn_yaw, .7f);
    next.spawn_pitch = finite_or(next.spawn_pitch, -.16f);
    if (!isfinite(next.fov_deg) || next.fov_deg < 30.0f || next.fov_deg > 110.0f) next.fov_deg = 66.0f;
    if (!isfinite(next.fog_start) || next.fog_start < 0.0f) next.fog_start = RBX_DEFAULT_FOG_START;
    if (!isfinite(next.fog_end) || next.fog_end <= next.fog_start + 1.0f) next.fog_end = next.fog_start + 1.0f;
    if (!isfinite(next.view_distance) || next.view_distance < next.fog_end) next.view_distance = next.fog_end;
    if (next.view_distance < 16.0f) next.view_distance = 16.0f;
    if (next.view_distance > 512.0f) next.view_distance = 512.0f;
    if (next.fullres_pixel_limit <= 0) next.fullres_pixel_limit = DEFAULT_FULLRES_PIXELS;

    cfg = next;
    cfg_ready = 1;
    rbx3d_configure(cfg.fog_start, cfg.fog_end, cfg.view_distance);
}

const RbxEngineConfig *rbx_engine_config(void) {
    if (!cfg_ready) rbx_engine_configure(NULL);
    return &cfg;
}

void rbx_engine_cancel_input(void) {
    edit_q_down = edit_e_down = 0;
    rbx_input_reset();
    rbx_key_reset();
}

void rbx_engine_reset(void) {
    const RbxEngineConfig *c = rbx_engine_config();
    rbx_engine_cancel_input();
    rbx_world_build(c->seed);
    rbx_player_spawn();
    float x, z;
    rbx_player_pos(&x, NULL, &z, NULL, NULL);
    rbx_world_update(x, z);
    rbx_t_abs = 0;
    rbx_input_layout();
}

void rbx_engine_boot(AAssetManager *assets) {
    (void)assets;
    const RbxEngineConfig *c = rbx_engine_config();
    snd_load("send.wav");
    rbx_engine_reset();
    ds_log("FaiCraft Engine: seed=%u, chunk=%d, cache=%dx%d, view=%.0f",
           (unsigned)c->seed, CHUNK_SIZE, WORLD_RADIUS * 2 + 1,
           WORLD_RADIUS * 2 + 1, c->view_distance);
}

void rbx_engine_update(float seconds) {
    float d = seconds;
    if (!isfinite(d) || d < 0.0f) d = 0.0f;
    if (d > .05f) d = .05f;
    rbx_t_abs += d;
    rbx_input_layout();
    rbx_player_update(d);
    float x, z;
    rbx_player_pos(&x, NULL, &z, NULL, NULL);
    rbx_world_update(x, z);
}

void rbx_engine_draw(Buffer *buffer) {
    rbx_scene_draw(buffer);
    rbx_hud_draw();
}

void rbx_engine_touch(float x, float y, int action, int pointer_id) {
    rbx_input_touch(x, y, action, pointer_id);
}

void rbx_engine_world_info(RbxWorldInfo *out) {
    if (!out) return;
    int chunks = 0, faces = 0;
    const RbxEngineConfig *c = rbx_engine_config();
    rbx_world_stats(&chunks, &faces);
    memset(out, 0, sizeof(*out));
    out->seed = c->seed;
    out->chunk_size = CHUNK_SIZE;
    out->world_height = WORLD_HEIGHT;
    out->water_level = WATER_LEVEL;
    out->cache_radius = WORLD_RADIUS;
    out->cache_side = WORLD_RADIUS * 2 + 1;
    out->loaded_chunks = chunks;
    out->mesh_faces = faces;
    out->edited_blocks = rbx_world_edit_count();
}

void rbx_engine_player_state(RbxPlayerState *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    rbx_player_pos(&out->x, &out->y, &out->z, NULL, NULL);
    rbx_camera_angles(&out->yaw, &out->pitch);
    out->flying = rbx_player_flying();
    out->grounded = rbx_player_grounded();
}

int rbx_engine_block(int x, int y, int z) { return rbx_world_block(x, y, z); }

int rbx_engine_set_block(int x, int y, int z, int block) {
    return rbx_world_set_block(x, y, z, block);
}

void rbx_engine_clear_edits(void) { rbx_world_clear_edits(); }

int rbx_engine_raycast(float max_distance, RbxRaycastHit *hit) {
    RbxPlayerState p;
    if (!isfinite(max_distance) || max_distance <= 0.0f) max_distance = EDIT_REACH;
    rbx_engine_player_state(&p);
    float cp = cosf(p.pitch);
    return rbx_world_raycast(p.x, p.y + RBX_PLAYER_EYE_HEIGHT, p.z,
                             sinf(p.yaw) * cp, sinf(p.pitch), cosf(p.yaw) * cp,
                             max_distance, hit);
}

int rbx_engine_break_selected(float max_distance) {
    RbxRaycastHit hit;
    if (!rbx_engine_raycast(max_distance, &hit)) return 0;
    if (hit.y < 0 || hit.y >= WORLD_HEIGHT) return 0;
    return rbx_engine_set_block(hit.x, hit.y, hit.z, BLOCK_AIR);
}

static void face_offset(int face, int *dx, int *dy, int *dz) {
    static const int offsets[6][3] = {{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},{1,0,0},{-1,0,0}};
    *dx = *dy = *dz = 0;
    if (face >= 0 && face < 6) { *dx = offsets[face][0]; *dy = offsets[face][1]; *dz = offsets[face][2]; }
}

static int overlaps_player(int x, int y, int z) {
    float px, py, pz;
    rbx_player_pos(&px, &py, &pz, NULL, NULL);
    return x + 1.0f > px - RBX_PLAYER_RADIUS && x < px + RBX_PLAYER_RADIUS &&
           y + 1.0f > py && y < py + RBX_PLAYER_HEIGHT &&
           z + 1.0f > pz - RBX_PLAYER_RADIUS && z < pz + RBX_PLAYER_RADIUS;
}

int rbx_engine_place_selected(int block, float max_distance) {
    if (block <= BLOCK_AIR || block >= BLOCK_COUNT) return 0;
    RbxRaycastHit hit;
    if (!rbx_engine_raycast(max_distance, &hit) || hit.face < 0) return 0;
    int dx, dy, dz;
    face_offset(hit.face, &dx, &dy, &dz);
    int x = hit.x + dx, y = hit.y + dy, z = hit.z + dz;
    if (y < 0 || y >= WORLD_HEIGHT || overlaps_player(x, y, z)) return 0;
    return rbx_engine_set_block(x, y, z, block);
}

static int key_is(const char *name, char lo, char hi) {
    return name && (name[0] == lo || name[0] == hi) && name[1] == '\0';
}

void rbx_engine_key(const char *name, int down) {
    int d = down != 0;
    if (key_is(name, 'q', 'Q')) {
        if (d && !edit_q_down) rbx_engine_break_selected(EDIT_REACH);
        edit_q_down = d;
        return;
    }
    if (key_is(name, 'e', 'E')) {
        if (d && !edit_e_down) rbx_engine_place_selected(BLOCK_GRASS, EDIT_REACH);
        edit_e_down = d;
        return;
    }
    rbx_key_state(name, d);
}

void rbx_key(const char *name, int down) { rbx_engine_key(name, down); }
void rbx_cancel_input(void) { rbx_engine_cancel_input(); }
