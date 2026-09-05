/* Регистрация игрового модуля: приложение остаётся тем же, меняется только
 * набор callbacks. Если модуль не зарегистрирован, работает sandbox-демо. */
#include "fc/fc_internal.h"

static FcGame active;
static int has_active;

void fc_game_register(const FcGame *game) {
    if (game) { active = *game; has_active = 1; }
    else { active = (FcGame){0}; has_active = 0; }
}
const FcGame *fc_game_active(void) { return has_active ? &active : 0; }
const char *fc_game_name(void) { return has_active && active.name ? active.name : "FaiCraft Sandbox"; }
void fc_game_emit_start(void) { if (has_active && active.on_start) active.on_start(); }
void fc_game_emit_reset(void) { if (has_active && active.on_reset) active.on_reset(); }
void fc_game_emit_update(float dt) { if (has_active && active.on_update) active.on_update(dt); }
void fc_game_emit_draw_3d(void) { if (has_active && active.on_draw_3d) active.on_draw_3d(); }
void fc_game_emit_draw_2d(void) { if (has_active && active.on_draw_2d) active.on_draw_2d(); }
int fc_game_emit_key(const char *key, int down) {
    return has_active && active.on_key ? active.on_key(key, down) : 0;
}
int fc_game_emit_touch(float x, float y, int action, int pointer_id) {
    return has_active && active.on_touch ? active.on_touch(x, y, action, pointer_id) : 0;
}
void fc_game_emit_block_changed(int x, int y, int z, int old_block, int new_block) {
    if (has_active && active.on_block_changed) active.on_block_changed(x, y, z, old_block, new_block);
}
