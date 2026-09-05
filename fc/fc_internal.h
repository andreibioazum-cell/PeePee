#ifndef FC_INTERNAL_H
#define FC_INTERNAL_H
#include "fc/fc_engine.h"

void fc_game_emit_start(void);
void fc_game_emit_reset(void);
void fc_game_emit_update(float dt);
void fc_game_emit_draw_3d(void);
void fc_game_emit_draw_2d(void);
int fc_game_emit_key(const char *key, int down);
int fc_game_emit_touch(float x, float y, int action, int pointer_id);
void fc_game_emit_block_changed(int x, int y, int z, int old_block, int new_block);

#endif
