/* Публичный слой FaiCraft Engine: конфиг, статистика, правки и raycast. */
#include "rbx/rbx_internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%s:%d: %s\n",__func__,__LINE__,#x); exit(1); } } while(0)
#define CLOSE(a,b) CHECK(fabsf((a)-(b)) < .0005f)

int screen_w = 960, screen_h = 540;
double dt;
int snd_load(const char *s) { (void)s; return 1; }
int snd_play(const char *s) { (void)s; return 1; }
void ds_log(const char *s,...) { (void)s; }
void ds_runtime_error(const char *s,...) { fprintf(stderr,"%s\n",s); abort(); }
void rbx3d_configure(float fog_start,float fog_end,float far_z) { (void)fog_start; (void)fog_end; (void)far_z; }
int rbx3d_visible(float x,float y,float z,float hx,float hy,float hz) { (void)x;(void)y;(void)z;(void)hx;(void)hy;(void)hz; return 0; }
void rbx3d_block_face(float x,float y,float z,int face,int block) { (void)x;(void)y;(void)z;(void)face;(void)block; }
void rbx_scene_draw(Buffer *buffer) { (void)buffer; }
void rbx_hud_draw(void) {}

static int first_air_above(int x, int z) {
    int y = rbx_terrain_height(x, z) + 1;
    if (y <= WATER_LEVEL) y = WATER_LEVEL + 1;
    while (y < WORLD_HEIGHT && rbx_engine_block(x, y, z) != BLOCK_AIR) y++;
    CHECK(y < WORLD_HEIGHT);
    return y;
}

static void test_config_and_stats(void) {
    RbxEngineConfig cfg;
    rbx_engine_default_config(&cfg);
    cfg.seed = 12345u;
    cfg.spawn_x = 32.5f;
    cfg.spawn_z = -15.5f;
    cfg.spawn_yaw = 1.0f;
    cfg.spawn_pitch = .25f;
    cfg.fov_deg = 74.0f;
    cfg.fog_start = 10.0f;
    cfg.fog_end = 40.0f;
    cfg.view_distance = 96.0f;
    rbx_engine_configure(&cfg);
    rbx_engine_reset();

    RbxWorldInfo info;
    rbx_engine_world_info(&info);
    CHECK(info.seed == 12345u);
    CHECK(info.chunk_size == CHUNK_SIZE && info.world_height == WORLD_HEIGHT);
    CHECK(info.cache_side == WORLD_RADIUS * 2 + 1);
    CHECK(info.loaded_chunks == info.cache_side * info.cache_side);
    CHECK(info.mesh_faces > 0 && info.edited_blocks == 0);

    RbxPlayerState p;
    rbx_engine_player_state(&p);
    CLOSE(p.x, cfg.spawn_x); CLOSE(p.z, cfg.spawn_z);
    CLOSE(p.y, rbx_terrain_height((int)floorf(cfg.spawn_x), (int)floorf(cfg.spawn_z)) + 1.002f);
    CLOSE(p.yaw, cfg.spawn_yaw); CLOSE(p.pitch, cfg.spawn_pitch);
    CHECK(!p.flying && p.grounded);
    puts("PASS engine config: seed, spawn, render knobs and world stats are public");
}

static void test_edits_survive_streaming(void) {
    int x = 40, z = -12, y = first_air_above(x, z);
    CHECK(rbx_engine_set_block(x, y, z, BLOCK_STONE));
    CHECK(rbx_engine_block(x, y, z) == BLOCK_STONE);
    RbxWorldInfo info;
    rbx_engine_world_info(&info);
    CHECK(info.edited_blocks == 1);

    rbx_world_update(1024.5f, -1024.5f);
    CHECK(rbx_engine_block(x, y, z) == BLOCK_STONE);
    rbx_world_update((float)x + .5f, (float)z + .5f);
    CHECK(rbx_engine_block(x, y, z) == BLOCK_STONE);
    CHECK(rbx_engine_set_block(x, y, z, rbx_terrain_block(x, y, z)));
    rbx_engine_world_info(&info);
    CHECK(info.edited_blocks == 0 && rbx_engine_block(x, y, z) == rbx_terrain_block(x, y, z));
    puts("PASS engine edits: set_block patches meshes and survives chunk streaming");
}

static void test_raycast_and_actions(void) {
    RbxEngineConfig cfg;
    rbx_engine_default_config(&cfg);
    cfg.spawn_yaw = 0.0f;
    cfg.spawn_pitch = 0.0f;
    rbx_engine_configure(&cfg);
    rbx_engine_reset();
    RbxPlayerState p;
    rbx_engine_player_state(&p);
    int x = (int)floorf(p.x), y = (int)floorf(p.y + RBX_PLAYER_EYE_HEIGHT), z = (int)floorf(p.z) + 4;
    CHECK(rbx_engine_block(x, y, z - 1) == BLOCK_AIR);
    CHECK(rbx_engine_set_block(x, y, z, BLOCK_STONE));

    RbxRaycastHit hit;
    CHECK(rbx_engine_raycast(10.0f, &hit));
    CHECK(hit.hit && hit.x == x && hit.y == y && hit.z == z && hit.face == 3 && hit.block == BLOCK_STONE);
    CHECK(hit.distance > 3.0f && hit.distance < 4.0f);
    CHECK(rbx_engine_break_selected(10.0f));
    CHECK(rbx_engine_block(x, y, z) == BLOCK_AIR);

    CHECK(rbx_engine_set_block(x, y, z, BLOCK_STONE));
    CHECK(rbx_engine_place_selected(BLOCK_DIRT, 10.0f));
    CHECK(rbx_engine_block(x, y, z - 1) == BLOCK_DIRT);
    puts("PASS engine raycast: selected block can be broken and adjacent block placed");
}

int main(void) {
    test_config_and_stats();
    test_edits_survive_streaming();
    test_raycast_and_actions();
    return 0;
}
