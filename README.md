# FaiCraft Engine

Горизонтальный блочный движок на **чистом C** для Android и браузерного
превью. Ветка с «недо-Майнкрафтом» перенесена сюда целиком и превращена из
одного демо в переиспользуемый слой: публичный API, конфиг, статистика мира,
raycast, чтение/запись блоков и базовые действия «сломать/поставить».

## Что уже есть

- Процедурный воксельный ландшафт: трава, земля, камень, песок, вода, брёвна и
  листва.
- Чанки **16×16×40**, скользящий кэш **9×9 чанков** и бесконечная детерминированная
  генерация по seed.
- Слой правок поверх генератора: `set_block` меняет блоки, пересобирает меши и
  сохраняет изменения при выгрузке/возврате чанка до reset.
- Raycast из камеры игрока и низкоуровневый raycast мира — основа для добычи,
  строительства, триггеров и редактора.
- Софтверный 3D-рендер: z-buffer, near/frustum clipping, перспективные UV через
  `1/z`, туман, процедурные текстуры и mip-уровни.
- Игрок от первого лица: ходьба, прыжок, вода, столкновения с вокселями и
  переключаемый полёт.
- Runtime-хуки `init/update/draw/touch/reset` оставлены совместимыми с текущим
  Android-приложением.

Пока это именно движок и демо-песочница: инвентарь, крафт, сохранение мира на
диск и полноценные игровые правила ещё не добавлены.

## Управление в демо

- **Android / touch:** левый чёрный стик — движение, правая сторона — обзор,
  круглая кнопка — прыжок, белая кнопка сверху — полёт.
- **ПК / превью:**
  - `W A S D` — движение;
  - стрелки или drag справа — камера;
  - `Space` — прыжок;
  - `F` — включить/выключить полёт;
  - `Q` — сломать блок под прицелом;
  - `E` — поставить блок травы на выбранную грань;
  - в полёте `Space` / `Shift` поднимают / опускают игрока.

HUD минимален: стик без подложки, толстое чёрное кольцо, белый прицел, кнопка
полёта и условная кнопка прыжка. На Android включён `sensorLandscape`.

## API для создания игр

Для игровых модулей теперь есть отдельный слой **FaiCraft Game API**:
`fc/fc_engine.h`. Он отделяет игру от Android/preview runtime и даёт готовые
строительные блоки:

- `FcGame` — callbacks игры: `on_start`, `on_reset`, `on_update`, `on_draw_3d`,
  `on_draw_2d`, `on_key`, `on_touch`, `on_block_changed`;
- `FcEntity` — простые игровые объекты со сценой, transform, velocity,
  цветом, callback обновления и box-рендером;
- `fc_scene_*` / `fc_entity_*` — создание, поиск, удаление, обновление,
  отрисовка и проверка пересечения с игроком;
- низкоуровневый блочный API остаётся в `rbx/rbx.h`: конфиг движка, мир,
  игрок, raycast, установка/разрушение блоков.

Минимальный игровой модуль выглядит так:

```c
#include "fc/fc_engine.h"
#include <string.h>

static int beacon_id;

static void spin(FcEntity *e, float dt) { e->yaw += dt * 1.7f; }

static void on_reset(void) {
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
    if (beacon_id && fc_entity_intersects_player(beacon_id))
        fc_entity_remove(beacon_id);
}

void register_my_game(void) {
    static const FcGame game = {
        .name = "Beacon Collector",
        .on_reset = on_reset,
        .on_update = on_update,
    };
    fc_game_register(&game);
}
```

Полный пример лежит в `examples/fc_minigame.c`. Чтобы собрать свою игру,
добавь её `.c` файл в `CMakeLists.txt` и/или `tools/preview/build.sh`, вызови
регистрацию игры до `init`, а дальше используй обычный runtime.

## Низкоуровневый блочный API

```c
#include "rbx/rbx.h"

RbxEngineConfig cfg;
rbx_engine_default_config(&cfg);
cfg.seed = 12345;
cfg.spawn_x = 32.5f;
cfg.spawn_z = -15.5f;
cfg.fov_deg = 74.0f;
rbx_engine_configure(&cfg);
rbx_engine_boot(assets);

/* игровой цикл */
rbx_engine_update(dt_seconds);
rbx_engine_draw(&frame);

/* мир */
int block = rbx_engine_block(x, y, z);
rbx_engine_set_block(x, y, z, BLOCK_STONE);

/* выбор блока из камеры */
RbxRaycastHit hit;
if (rbx_engine_raycast(6.0f, &hit)) {
    rbx_engine_break_selected(6.0f);
    rbx_engine_place_selected(BLOCK_GRASS, 6.0f);
}
```

Основные структуры:

- `RbxEngineConfig` — seed, spawn, FOV, небо, туман, дальность видимости и лимит
  пикселей для full-res рендера;
- `RbxWorldInfo` — размеры чанка/мира, параметры кэша, количество загруженных
  чанков, граней мешей и правок;
- `RbxPlayerState` — позиция, yaw/pitch, режим полёта и grounded-состояние;
- `RbxRaycastHit` — координаты блока, грань попадания и точка пересечения.

## Генерация и рендер

- Seed + мировые координаты полностью определяют базовый блок. Отрицательные
  координаты и стыки деревьев между чанками покрыты тестами.
- В память попадает только окно **9×9** вокруг игрока; возврат к старому месту
  восстанавливает тот же рельеф и применяет накопленные правки.
- Меш чанка хранит только открытые грани. Внутренние поверхности блоков не
  отправляются в растеризатор.
- 3D рисуется в полном разрешении до заданного `fullres_pixel_limit`, выше — в
  половинном с плавным апскейлом. HUD и TTF-текст остаются в разрешении экрана.

## Превью без Android

```sh
tools/preview/build.sh
./preview --port 8090 --assets game/assets
# http://localhost:8090
```

По умолчанию кадр горизонтальный, **960×540**. Можно передать `--w` и `--h`.
Сервер слушает `0.0.0.0`, поэтому подходит для Arena Live Preview.

## Проверки

```sh
tools/tests/run.sh
SANITIZE=1 tools/tests/run.sh
```

Набор регрессий:

- `render.c`: туман, билинейный апскейл, z-buffer, clipping, направление камеры,
  текстуры и mip-уровни;
- `controls.c`: ходьба, прыжок, полёт, мультитач, отмена жестов, коллизии,
  HUD и reset;
- `world.c`: seed, материалы, отрицательные координаты, границы чанков/деревьев,
  ограниченный кэш и восстановление;
- `engine.c`: публичный конфиг, game callbacks, entity scene, `set_block`,
  правки при streaming, raycast, break/place.

Sanitizer-режим включает ASan, UBSan и проверку переполнений float-to-int.
Сборки и временные данные находятся в игнорируемом `build-tests/`.

## Структура

```text
main.c                     Android NativeActivity, цикл и ввод
runtime.h                  общий API рантайма
core/                      состояние, строки, массивы, лог, Android-клавиатура
graphics/                  2D/HUD, текст, текстуры, TTF-движок
sound/                     WAV, микшер, Android AudioTrack
fc/
├── fc_engine.h            API для игровых модулей: callbacks и сущности
├── fc_game.c              регистрация/диспетчеризация игры
└── fc_scene.c             простая entity-сцена поверх 3D-рендера
rbx/
├── rbx.h                  низкоуровневый API FaiCraft Engine
├── rbx_engine.c           конфиг, lifecycle, stats, raycast, break/place
├── rbx_terrain.c          шум высот, слои, вода и деревья
├── rbx_world.c            кэш чанков, слой правок и меши открытых граней
├── rbx_render.c           отсечение, растеризатор, UV, глубина, апскейл
├── rbx_shapes.c           кубы и грани блоков
├── rbx_material.c         процедурные текстуры и mip-уровни
├── rbx_player.c           ходьба/прыжок/полёт, коллизии, камера
├── rbx_input.c            мультитач, стик, прыжок, переключатель
├── rbx_scene.c            камера первого лица и блочная сцена
├── rbx_hud.c              минимальные органы управления
└── rbx_game.c             совместимые хуки init/update/draw/touch/reset
game/                      AndroidManifest, Java-мост, шрифт и звуки
tools/preview/             нативная сборка для браузерного превью
tools/tests/               регрессионные тесты без Android
examples/                  пример игрового модуля на FaiCraft Game API
```

Каждый `.c` — отдельная единица компиляции, не больше 500 строк, без
`include .c`.

## APK

Сборка APK выполняется существующим GitHub Actions workflow: CMake + NDK,
`arm64-v8a` и `armeabi-v7a`, Android API 29+. Имя артефакта старого workflow
сохранено для совместимости: **Enjoer-Messenger-APK-Android10-14-arm64-arm32**.

## Шрифт

`ChillRoundGothic_Heavy` — SIL OFL (Warren2060/ChillRoundGothic).
