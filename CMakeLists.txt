cmake_minimum_required(VERSION 3.16)
project(FaiCraftEngine C)
set(CMAKE_C_STANDARD 99)

# Единственная целевая платформа — Android: библиотека libds_game.so, которую
# забирает APK из game/AndroidManifest.xml. Демо запускает FaiCraft Engine —
# блочный движок на чистом C.
#
# Структура исходников (каждый файл — отдельная единица компиляции):
#   main.c          вход под Android (native activity)
#   core/           рантайм: консоль, ошибки, строки, массивы, клавиатура
#   graphics/       2D-рендер: примитивы, текстуры, текст (+ ttf/)
#   sound/          звук: микшер и вывод через AudioTrack
#   fc/             API создания игр: callbacks и сущности
#   rbx/            движок: lifecycle, рендер, мир, игрок, ввод, сцена, HUD
if(NOT ANDROID)
    message(FATAL_ERROR
        "FaiCraft Engine собирается только под Android. Запусти CMake через "
        "инструментальный файл NDK, например: cmake -B build "
        "-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake "
        "-DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29. "
        "Для превью на ПК смотри tools/preview (обычный gcc, без CMake).")
endif()

add_library(ds_game SHARED
    main.c
    core/log.c
    core/state.c
    core/strings.c
    core/arrays.c
    core/keyboard_android.c
    graphics/gfx_frame.c
    graphics/gfx_draw.c
    graphics/gfx_texture.c
    graphics/gfx_text.c
    graphics/ttf/ttf_outline.c
    graphics/ttf/ttf_font.c
    sound/sound.c
    sound/sound_android.c
    fc/fc_game.c
    fc/fc_scene.c
    rbx/rbx_engine.c
    rbx/rbx_render.c
    rbx/rbx_shapes.c
    rbx/rbx_material.c
    rbx/rbx_terrain.c
    rbx/rbx_world.c
    rbx/rbx_player.c
    rbx/rbx_input.c
    rbx/rbx_scene.c
    rbx/rbx_hud.c
    rbx/rbx_game.c
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(ds_game PRIVATE
    .
    rbx
    ${ANDROID_NDK}/sources/android/native_app_glue)
target_link_libraries(ds_game PRIVATE android log m)
