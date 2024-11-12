/*
    Copyright (c) 2021-2024 Jaedeok Kim <jdeokkim@protonmail.com>

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
#include "ferox_raylib.h"

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

/* Macros ================================================================== */

// clang-format off

#define TARGET_FPS     60

#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600

#define BOX_COUNT      10

// clang-format on

/* Constants =============================================================== */

static const Rectangle SCREEN_BOUNDS = { .width = SCREEN_WIDTH,
                                         .height = SCREEN_HEIGHT };

static const float BOX_WIDTH = 1.25f, BOX_HEIGHT = 1.25f;

static const float CELL_SIZE = 1.5f, DELTA_TIME = 1.0f / (TARGET_FPS << 1);

/* Private Variables ======================================================= */

static frWorld *world;

static frShape *boxShape, *groundShape;

static frBody *boxes[BOX_COUNT], *ground;

/* Private Function Prototypes ============================================= */

static void InitExample(void);
static void UpdateExample(void);
static void DeinitExample(void);

/* Public Functions ======================================================== */

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | " __FILE__);

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
                                                  1.0f),
                          CELL_SIZE);

    ground = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                             .y = 0.85f * SCREEN_HEIGHT }),
        frCreateRectangle((frMaterial) { .density = 1.25f, .friction = 0.75f },
                          frPixelsToUnits(0.75f * SCREEN_WIDTH),
                          frPixelsToUnits(0.1f * SCREEN_HEIGHT)));

    frAddBodyToWorld(world, ground);

    boxShape = frCreateRectangle((frMaterial) { .density = 1.0f,
                                                .friction = 0.75f },
                                 BOX_WIDTH,
                                 BOX_HEIGHT);

    for (int i = 0; i < BOX_COUNT; i++) {
        boxes[i] = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            frVector2PixelsToUnits((frVector2) {
                .x = 0.5f * SCREEN_WIDTH,
                .y = (0.74f * SCREEN_HEIGHT)
                     - (i * (frUnitsToPixels(BOX_HEIGHT) + 1.0f)) }),
            boxShape);

        frAddBodyToWorld(world, boxes[i]);
    }
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

        for (int i = 0; i < BOX_COUNT; i++)
            frDrawBodyLines(boxes[i], 1.0f, ColorAlpha(RED, 0.85f));

        DrawFPS(8, 8);

        EndDrawing();
    }
}

static void DeinitExample(void) {
    frReleaseShape(groundShape), frReleaseShape(boxShape);

    frReleaseWorld(world);
}