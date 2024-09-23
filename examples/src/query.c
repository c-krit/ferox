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
#include "ferox-raylib.h"

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

/* Macros ================================================================== */

// clang-format off

#define TARGET_FPS             60

#define SCREEN_WIDTH           1280
#define SCREEN_HEIGHT          800

#define CURSOR_SIZE_IN_PIXELS  128.0f

#define MAX_OBJECT_COUNT       256

// clang-format on

/* Constants =============================================================== */

static const Rectangle SCREEN_BOUNDS = { .width = SCREEN_WIDTH,
                                         .height = SCREEN_HEIGHT };

static const float CELL_SIZE = 2.0f, DELTA_TIME = 1.0f / TARGET_FPS;

/* Private Variables ======================================================= */

static frSpatialHash *hash;

static frBody *bodies[MAX_OBJECT_COUNT];

static Color primaryColor, secondaryColor;

/* Private Function Prototypes ============================================= */

static void InitExample(void);
static void UpdateExample(void);
static void DeinitExample(void);

static void DrawCursorBounds(void);
static frAABB GetCursorBounds(void);

static bool OnHashQuery(frIndexedData arg);

/* Public Functions ======================================================== */

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | query.c");

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
    hash = frCreateSpatialHash(CELL_SIZE);

    primaryColor = ColorAlpha(LIGHTGRAY, 0.35f);
    secondaryColor = ColorAlpha(LIME, 0.85f);

    for (int i = 0; i < MAX_OBJECT_COUNT; i++) {
        frVector2 position = {
            .x = GetRandomValue(0.02f * SCREEN_WIDTH, 0.98f * SCREEN_WIDTH),
            .y = GetRandomValue(0.02f * SCREEN_HEIGHT, 0.98f * SCREEN_HEIGHT)
        };

        bodies[i] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            frVector2PixelsToUnits(position),
            frCreateRectangle(FR_API_STRUCT_ZERO(frMaterial),
                              0.35f * GetRandomValue(1, 3),
                              0.35f * GetRandomValue(1, 3)));

        frSetBodyAngle(bodies[i], DEG2RAD * GetRandomValue(0, 360));
    }

    HideCursor();

#ifdef PLATFORM_WEB
    // TODO: https://github.com/emscripten-core/emscripten/issues/5446
    emscripten_hide_mouse();
#endif

    SetMousePosition(0.5f * SCREEN_WIDTH, 0.5f * SCREEN_HEIGHT);
}

static void UpdateExample(void) {
    {
        frClearSpatialHash(hash);

        for (int i = 0; i < MAX_OBJECT_COUNT; i++) {
            frSetBodyUserData(bodies[i], (void *) &primaryColor);

            frInsertIntoSpatialHash(hash, frGetBodyAABB(bodies[i]), i);
        }

        frQuerySpatialHash(hash, GetCursorBounds(), OnHashQuery, NULL);
    }

    {
        BeginDrawing();

        ClearBackground(FR_DRAW_COLOR_MATTEBLACK);

        frDrawGrid(SCREEN_BOUNDS,
                   CELL_SIZE,
                   0.25f,
                   ColorAlpha(DARKGRAY, 0.75f));

        for (int i = 0; i < MAX_OBJECT_COUNT; i++) {
            const Color *color = frGetBodyUserData(bodies[i]);

            frDrawBodyLines(bodies[i], 2.0f, *color);
        }

        DrawCursorBounds();

        DrawFPS(8, 8);

        EndDrawing();
    }
}

static void DeinitExample(void) {
    for (int i = 0; i < MAX_OBJECT_COUNT; i++)
        frReleaseBody(bodies[i]);

    frReleaseSpatialHash(hash);
}

static void DrawCursorBounds(void) {
    const Vector2 mousePosition = GetMousePosition();

    Rectangle bounds = { .x = mousePosition.x - 0.5f * CURSOR_SIZE_IN_PIXELS,
                         .y = mousePosition.y - 0.5f * CURSOR_SIZE_IN_PIXELS,
                         .width = CURSOR_SIZE_IN_PIXELS,
                         .height = CURSOR_SIZE_IN_PIXELS };

    Color color = ColorAlpha(GREEN, 0.85f);

    DrawLineEx((Vector2) { .x = mousePosition.x - 4.0f, .y = mousePosition.y },
               (Vector2) { .x = mousePosition.x + 4.0f, .y = mousePosition.y },
               2.0f,
               color);

    DrawLineEx((Vector2) { .x = mousePosition.x, .y = mousePosition.y - 4.0f },
               (Vector2) { .x = mousePosition.x, .y = mousePosition.y + 4.0f },
               2.0f,
               color);

    DrawRectangleLinesEx(bounds, 2.0f, color);
}

static frAABB GetCursorBounds(void) {
    const Vector2 mousePosition = GetMousePosition();

    return (frAABB) {
        .x = frPixelsToUnits(mousePosition.x - 0.5f * CURSOR_SIZE_IN_PIXELS),
        .y = frPixelsToUnits(mousePosition.y - 0.5f * CURSOR_SIZE_IN_PIXELS),
        .width = frPixelsToUnits(CURSOR_SIZE_IN_PIXELS),
        .height = frPixelsToUnits(CURSOR_SIZE_IN_PIXELS)
    };
}

static bool OnHashQuery(frIndexedData arg) {
    frSetBodyUserData(bodies[arg.idx], (void *) &secondaryColor);

    return true;
}