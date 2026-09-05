/* От первого лица: сцена берёт параметры камеры/неба из FaiCraft Engine. */
#include "rbx_internal.h"
#include "fc/fc_internal.h"

void rbx_scene_draw(Buffer *buffer) {
    const RbxEngineConfig *cfg = rbx_engine_config();
    float x, y, z, yaw, pitch;
    rbx_player_pos(&x, &y, &z, NULL, NULL);
    rbx_camera_angles(&yaw, &pitch);
    long pixels = (long)screen_w * (long)screen_h;
    int scale = pixels > cfg->fullres_pixel_limit ? 2 : 1;
    rbx3d_configure(cfg->fog_start, cfg->fog_end, cfg->view_distance);
    if (!rbx3d_begin(buffer, scale, x, y + RBX_PLAYER_EYE_HEIGHT, z, yaw, pitch, cfg->fov_deg)) return;
    rbx3d_sky(cfg->sky_top, cfg->sky_bottom);
    rbx_world_draw();
    fc_scene_draw_3d();
    fc_game_emit_draw_3d();
    rbx3d_end();
}
