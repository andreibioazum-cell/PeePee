/* Хуки Android и браузерного превью делегируют FaiCraft Engine. */
#include "rbx_internal.h"

void reset(void) { rbx_engine_reset(); }
void init(AAssetManager *assets) { rbx_engine_boot(assets); }
void update(void) { rbx_engine_update((float)dt); }
void draw(Buffer *buffer) { rbx_engine_draw(buffer); }
void touch(float x, float y, int action, int id) { rbx_engine_touch(x, y, action, id); }
