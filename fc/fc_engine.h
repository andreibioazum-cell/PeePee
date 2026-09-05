/* FaiCraft Game API — тонкий слой для создания игр поверх блочного движка.
 * Подключай этот заголовок из игровых модулей: сущности, callbacks, мир,
 * игрок и ввод доступны без правок Android runtime. */
#ifndef FC_ENGINE_H
#define FC_ENGINE_H

#include "rbx/rbx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FC_MAX_ENTITIES 256
#define FC_NAME_MAX 32

#define FC_ENTITY_VISIBLE 0x01
#define FC_ENTITY_SOLID   0x02

typedef enum {
    FC_ENTITY_EMPTY = 0,
    FC_ENTITY_BOX = 1
} FcEntityKind;

typedef enum {
    FC_TOUCH_DOWN = 0,
    FC_TOUCH_UP = 1,
    FC_TOUCH_MOVE = 2,
    FC_TOUCH_CANCEL_ALL = 3,
    FC_TOUCH_CANCEL_POINTER = 4
} FcTouchAction;

typedef struct FcEntity FcEntity;
typedef void (*FcEntityUpdate)(FcEntity *entity, float dt);

typedef struct {
    const char *name;
    void (*on_start)(void);
    void (*on_reset)(void);
    void (*on_update)(float dt);
    void (*on_draw_3d)(void);
    void (*on_draw_2d)(void);
    int (*on_key)(const char *key, int down);          /* 1 = событие съедено */
    int (*on_touch)(float x, float y, int action, int pointer_id);
    void (*on_block_changed)(int x, int y, int z, int old_block, int new_block);
} FcGame;

struct FcEntity {
    int id;
    int alive;
    int kind;
    int tag;
    int flags;
    char name[FC_NAME_MAX];
    float x, y, z;
    float hx, hy, hz;
    float yaw;
    float vx, vy, vz;
    float vyaw;
    uint32_t color;
    FcEntityUpdate update;
    void *user;
};

void fc_game_register(const FcGame *game);
const FcGame *fc_game_active(void);
const char *fc_game_name(void);

void fc_scene_clear(void);
int fc_entity_spawn(const FcEntity *template_entity);
FcEntity *fc_entity_get(int id);
const FcEntity *fc_entity_at(int index);
int fc_entity_count(void);
int fc_entity_find(const char *name);
void fc_entity_remove(int id);
void fc_scene_update(float dt);
void fc_scene_draw_3d(void);
int fc_entity_intersects_player(int id);

#ifdef __cplusplus
}
#endif

#endif
