/* FaiCraft Engine — публичный API блочного движка на чистом C.
 * Runtime по-прежнему вызывает init/update/draw/touch/reset из rbx_game.c,
 * а внешние игры могут использовать rbx_engine_* напрямую. */
#ifndef RBX_H
#define RBX_H

#include "runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RBX_DEFAULT_WORLD_SEED 20260905u
#define RBX_DEFAULT_FOG_START 28.0f
#define RBX_DEFAULT_FOG_END 64.0f
#define RBX_DEFAULT_VIEW_DISTANCE 80.0f
#define RBX_ENGINE_AUTO_Y (-1000000.0f)

typedef enum {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_SAND,
    BLOCK_WATER,
    BLOCK_LOG,
    BLOCK_LEAVES,
    BLOCK_COUNT
} RbxBlock;

enum { CHUNK_SIZE = 16, WORLD_HEIGHT = 40, WATER_LEVEL = 8, WORLD_RADIUS = 4 };

typedef struct {
    uint32_t seed;
    float spawn_x;
    float spawn_y;      /* RBX_ENGINE_AUTO_Y: поставить на поверхность */
    float spawn_z;
    float spawn_yaw;
    float spawn_pitch;
    float fov_deg;
    float fog_start;
    float fog_end;
    float view_distance;
    int fullres_pixel_limit;
    uint32_t sky_top;
    uint32_t sky_bottom;
} RbxEngineConfig;

typedef struct {
    uint32_t seed;
    int chunk_size;
    int world_height;
    int water_level;
    int cache_radius;
    int cache_side;
    int loaded_chunks;
    int mesh_faces;
    int edited_blocks;
} RbxWorldInfo;

typedef struct {
    float x, y, z;
    float yaw, pitch;
    int flying;
    int grounded;
} RbxPlayerState;

typedef struct {
    int hit;
    int x, y, z;
    int face;       /* 0:+Y, 1:-Y, 2:+Z, 3:-Z, 4:+X, 5:-X, -1: внутри */
    int block;
    float distance;
    float hit_x, hit_y, hit_z;
} RbxRaycastHit;

void rbx_engine_default_config(RbxEngineConfig *out);
void rbx_engine_configure(const RbxEngineConfig *config);
const RbxEngineConfig *rbx_engine_config(void);

void rbx_engine_boot(AAssetManager *assets);
void rbx_engine_reset(void);
void rbx_engine_update(float seconds);
void rbx_engine_draw(Buffer *buffer);
void rbx_engine_touch(float x, float y, int action, int pointer_id);
void rbx_engine_key(const char *name, int down);
void rbx_engine_cancel_input(void);

void rbx_engine_world_info(RbxWorldInfo *out);
void rbx_engine_player_state(RbxPlayerState *out);
int rbx_engine_block(int x, int y, int z);
int rbx_engine_set_block(int x, int y, int z, int block);
void rbx_engine_clear_edits(void);
int rbx_engine_raycast(float max_distance, RbxRaycastHit *hit);
int rbx_engine_break_selected(float max_distance);
int rbx_engine_place_selected(int block, float max_distance);

/* Совместимость со старым вводом рантайма. */
void rbx_key(const char *name, int down);
void rbx_cancel_input(void);

#ifdef __cplusplus
}
#endif

#endif
