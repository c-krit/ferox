/*
    Copyright (c) 2021-2022 jdeokkim

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ferox.h"
#include "raylib.h"

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

/* | `basic` 모듈 매크로 정의... | */

#define TARGET_FPS      60

#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   600

#define MATTEBLACK      ((Color) { 15, 15, 15, 255 })

/* | `basic` 모듈 변수 및 상수... | */

const float DELTA_TIME = (1.0f / TARGET_FPS) * 200.0f;

static frWorld *world;

static Texture2D logo;

/* | `basic` 모듈 함수... | */

/* 예제 프로그램을 초기화한다. */
static void InitExample(void);

/* 예제 프로그램을 실행한다. */
static void UpdateExample(void);

/* 예제 프로그램에 할당된 메모리를 해제한다. */
static void DeinitExample(void);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | basic.c");

    InitExample();

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(UpdateExample, 0, 1);
#else
    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose()) 
        UpdateExample();
#endif

    DeinitExample();

    CloseWindow();

    return 0;
}

/* 예제 프로그램을 초기화한다. */
static void InitExample(void) {
    const Rectangle bounds = {
        .x = -0.05f * frNumberPixelsToMeters(SCREEN_WIDTH),
        .y = -0.05f * frNumberPixelsToMeters(SCREEN_HEIGHT),
        .width = 1.1f * frNumberPixelsToMeters(SCREEN_WIDTH),
        .height = 1.1f * frNumberPixelsToMeters(SCREEN_HEIGHT)
    };

    // 게임 세계를 생성한다.
    world = frCreateWorld(frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.0f), bounds);

    logo = LoadTexture("../res/images/logo_small.png");
}

/* 예제 프로그램을 실행한다. */
static void UpdateExample(void) {
    frSimulateWorld(world, DELTA_TIME);

    {
        BeginDrawing();
            
        ClearBackground(MATTEBLACK);

        const Vector2 position = {
            0.5f * (SCREEN_WIDTH - logo.width),
            0.5f * (SCREEN_HEIGHT - logo.height)
        };

        DrawTextureV(logo, position, ColorAlpha(WHITE, 0.5f));

        frDrawSpatialHash(frGetWorldSpatialHash(world), 0.25f, DARKGRAY);

        DrawFPS(8, 8);

        EndDrawing();
    }
}

/* 예제 프로그램에 할당된 메모리를 해제한다. */
static void DeinitExample(void) {
    UnloadTexture(logo);
    
    // 게임 세계에 할당된 메모리를 해제한다.
    frReleaseWorld(world);
}