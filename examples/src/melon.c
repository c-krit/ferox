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

#define TARGET_FPS        60

#define SCREEN_WIDTH      600
#define SCREEN_HEIGHT     800

#define MAX_RADIUS_COUNT  1
#define MAX_WALL_COUNT    3

// clang-format on

/* Constants =============================================================== */

static const frMaterial MATERIAL_WALL = { .density = 1.25f,
                                          .friction = 0.75f,
                                          .restitution = 0.0f };

static const Rectangle SCREEN_BOUNDS = { .width = SCREEN_WIDTH,
                                         .height = SCREEN_HEIGHT };

static const float MELON_RADII[MAX_RADIUS_COUNT] = { 0.5f };

static const float CELL_SIZE = 4.0f, DELTA_TIME = 1.0f / TARGET_FPS;

/* Private Variables ======================================================= */

static frVertices wallVertices;

static frWorld *world;

static frBody *walls[MAX_WALL_COUNT];

/* Private Function Prototypes ============================================= */

static void InitExample(void);
static void UpdateExample(void);
static void DeinitExample(void);

/* Public Functions ======================================================== */

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | melon.c");

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

    walls[0] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                             .y = 0.95f * SCREEN_HEIGHT }),
        frCreateRectangle(MATERIAL_WALL,
                          frPixelsToUnits(1.0f * SCREEN_WIDTH),
                          frPixelsToUnits(0.1f * SCREEN_HEIGHT)));

    // TODO: ...

    for (int i = 0; i < MAX_WALL_COUNT; i++)
        frAddBodyToWorld(world, walls[i]);
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

        for (int i = 0; i < MAX_WALL_COUNT; i++)
            frDrawBodyLines(walls[i], 1.0f, DARKGRAY);

        DrawFPS(8, 8);

        EndDrawing();
    }
}

static void DeinitExample(void) {
    for (int i = 0; i < MAX_WALL_COUNT; i++)
        frReleaseShape(frGetBodyShape(walls[i]));

    frReleaseWorld(world);
}