/* Пример игры поверх FaiCraft Engine. Чтобы встроить её в APK, добавь файл
 * в CMakeLists.txt/tools/preview/build.sh и вызови register_minigame() до init. */
#include "fc/fc_engine.h"
#include <string.h>

static int beacon_id;

static void spin(FcEntity *e, float dt) { e->yaw += dt * 1.7f; }
static void on_reset(void) {
    fc_scene_clear();
    FcEntity beacon = {0};
    beacon.kind = FC_ENTITY_BOX;
    beacon.flags = FC_ENTITY_VISIBLE;
    beacon.x = 12.5f; beacon.y = 15.0f; beacon.z = 12.5f;
    beacon.hx = .35f; beacon.hy = .35f; beacon.hz = .35f;
    beacon.color = 0xFFFFD54Fu;
    beacon.update = spin;
    strcpy(beacon.name, "golden_beacon");
    beacon_id = fc_entity_spawn(&beacon);
}
static void on_update(float dt) {
    (void)dt;
    if (beacon_id && fc_entity_intersects_player(beacon_id)) fc_entity_remove(beacon_id);
}
static void on_block_changed(int x, int y, int z, int old_block, int new_block) {
    (void)x; (void)y; (void)z; (void)old_block; (void)new_block;
}

void register_minigame(void) {
    static const FcGame game = {
        .name = "Beacon Collector",
        .on_reset = on_reset,
        .on_update = on_update,
        .on_block_changed = on_block_changed
    };
    fc_game_register(&game);
}
