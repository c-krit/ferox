/*
    Copyright (c) 2021-2023 Jaedeok Kim <jdeokkim@protonmail.com>

    Permission is hereby granted, free of charge, to any person obtaining a 
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation 
    the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included 
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.
*/

/* Includes ================================================================ */

#include "ferox.h"
#include "raylib.h"

#define FEROX_RAYLIB_IMPLEMENTATION
#include "ferox-raylib.h"

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

/* Macros ================================================================== */

// clang-format off

#define TARGET_FPS     60

#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600

// clang-format on

/* Constants =============================================================== */

static const float CELL_SIZE = 4.0f, DELTA_TIME = 1.0f / TARGET_FPS;

static const Rectangle SCREEN_BOUNDS = { .width = SCREEN_WIDTH,
                                         .height = SCREEN_HEIGHT };

/* Private Variables ======================================================= */

static frWorld *world;

static frBody *box, *ground;

/* Private Function Prototypes ============================================= */

static void InitExample(void);
static void UpdateExample(void);
static void DeinitExample(void);

/* Public Functions ======================================================== */

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

/* Private Functions ======================================================= */

static void InitExample(void) {
    world = frCreateWorld(frVector2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY,
                                                  2.5f),
                          CELL_SIZE);

    ground = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                             .y = 0.85f * SCREEN_HEIGHT }),
        frCreateRectangle((frMaterial) { .density = 1.25f, .friction = 0.5f },
                          frPixelsToUnits(0.75f * SCREEN_WIDTH),
                          frPixelsToUnits(0.1f * SCREEN_HEIGHT)));

    frAddBodyToWorld(world, ground);

    box = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                             .y = 0.35f * SCREEN_HEIGHT }),
        frCreateRectangle((frMaterial) { .density = 1.0f, .friction = 0.35f },
                          frPixelsToUnits(45.0f),
                          frPixelsToUnits(45.0f)));

    // frSetBodyAngle(box, DEG2RAD * 25.0f);

    frAddBodyToWorld(world, box);
}

static void UpdateExample(void) {
    frUpdateWorld(world, DELTA_TIME);

    {
        BeginDrawing();

        ClearBackground(FR_DRAW_COLOR_MATTEBLACK);

        frDrawGrid(SCREEN_BOUNDS,
                   CELL_SIZE,
                   0.25f,
                   ColorAlpha(DARKGRAY, 0.75f));

        frDrawBodyLines(ground, 1.0f, GRAY);

        frDrawBodyLines(box, 1.0f, ColorAlpha(RED, 0.85f));
        // frDrawBodyAABB(box, 1.0f, ColorAlpha(GREEN, 0.25f));

        DrawFPS(8, 8);

        EndDrawing();
    }
}

static void DeinitExample(void) {
    frReleaseShape(frGetBodyShape(ground));
    frReleaseShape(frGetBodyShape(box));

    frReleaseWorld(world);
}