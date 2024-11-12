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

#define TARGET_FPS       60

#define SCREEN_WIDTH     1280
#define SCREEN_HEIGHT    800

#define CURSOR_COOLDOWN  0.5f

#define BORDER_COUNT     4
#define PLATFORM_COUNT   4

// clang-format on

/* Constants =============================================================== */

static const Rectangle SCREEN_BOUNDS = { .width = SCREEN_WIDTH,
                                         .height = SCREEN_HEIGHT };

static const float CELL_SIZE = 1.5f, DELTA_TIME = 1.0f / TARGET_FPS;

/* Private Variables ======================================================= */

static frWorld *world;

static frBody *borders[BORDER_COUNT], *platforms[PLATFORM_COUNT], *cursor;

static Color boxColor = LIGHTGRAY, borderColor = DARKBROWN,
             platformColor = BROWN;

static float cursorCounter;

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
    // SetTargetFPS(TARGET_FPS);

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
                                                  2.0f),
                          CELL_SIZE);

    {
        frMaterial borderMaterial = { .density = 1.25f, .friction = 0.5f };

        borders[0] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                                 .y = -0.05f * SCREEN_HEIGHT }),
            frCreateRectangle(borderMaterial,
                              frPixelsToUnits(1.0f * SCREEN_WIDTH),
                              frPixelsToUnits(0.1f * SCREEN_HEIGHT)));

        borders[1] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            frVector2PixelsToUnits((frVector2) { .x = -0.05f * SCREEN_WIDTH,
                                                 .y = 0.5f * SCREEN_HEIGHT }),
            frCreateRectangle(borderMaterial,
                              frPixelsToUnits(0.1f * SCREEN_WIDTH),
                              frPixelsToUnits(1.0f * SCREEN_HEIGHT)));

        borders[2] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                                 .y = 1.05f * SCREEN_HEIGHT }),
            frCreateRectangle(borderMaterial,
                              frPixelsToUnits(1.0f * SCREEN_WIDTH),
                              frPixelsToUnits(0.1f * SCREEN_HEIGHT)));

        borders[3] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            frVector2PixelsToUnits((frVector2) { .x = 1.05f * SCREEN_WIDTH,
                                                 .y = 0.5f * SCREEN_HEIGHT }),
            frCreateRectangle(borderMaterial,
                              frPixelsToUnits(0.1f * SCREEN_WIDTH),
                              frPixelsToUnits(1.0f * SCREEN_HEIGHT)));

        for (int i = 0; i < BORDER_COUNT; i++) {
            frSetBodyUserData(borders[i], &borderColor);

            frAddBodyToWorld(world, borders[i]);
        }
    }

    {
        frMaterial platformMaterial = { .density = 1.25f, .friction = 0.25f };

        platforms[0] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            frVector2PixelsToUnits((frVector2) { .x = 0.85f * SCREEN_WIDTH,
                                                 .y = 0.25f * SCREEN_HEIGHT }),
            frCreateRectangle(platformMaterial,
                              frPixelsToUnits(0.75f * SCREEN_WIDTH),
                              frPixelsToUnits(0.05f * SCREEN_HEIGHT)));

        frSetBodyAngle(platforms[0], DEG2RAD * -15.0f);

        platforms[1] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            frVector2PixelsToUnits((frVector2) { .x = 0.25f * SCREEN_WIDTH,
                                                 .y = 0.65f * SCREEN_HEIGHT }),
            frCreateRectangle(platformMaterial,
                              frPixelsToUnits(0.75f * SCREEN_WIDTH),
                              frPixelsToUnits(0.05f * SCREEN_HEIGHT)));

        frSetBodyAngle(platforms[1], DEG2RAD * 20.0f);

        platforms[2] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            frVector2PixelsToUnits((frVector2) { .x = 0.95f * SCREEN_WIDTH,
                                                 .y = 0.85f * SCREEN_HEIGHT }),
            frCreateRectangle(platformMaterial,
                              frPixelsToUnits(0.75f * SCREEN_WIDTH),
                              frPixelsToUnits(0.1f * SCREEN_HEIGHT)));

        frSetBodyAngle(platforms[2], DEG2RAD * -60.0f);

        platforms[3] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            frVector2PixelsToUnits((frVector2) { .x = 0.25f * SCREEN_WIDTH,
                                                 .y = 0.35f * SCREEN_HEIGHT }),
            frCreateCircle(platformMaterial, frPixelsToUnits(84.0f)));

        for (int i = 0; i < PLATFORM_COUNT; i++) {
            frSetBodyUserData(platforms[i], &platformColor);

            frAddBodyToWorld(world, platforms[i]);
        }
    }

    {
        frMaterial cursorMaterial = { .density = 0.85f, .friction = 0.25f };

        cursor = frCreateBodyFromShape(
            FR_BODY_KINEMATIC,
            frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                                 .y = 0.5f * SCREEN_HEIGHT }),
            frCreateRectangle(cursorMaterial,
                              frPixelsToUnits(32.0f),
                              frPixelsToUnits(40.0f)));
    }

    HideCursor();

#ifdef PLATFORM_WEB
    // TODO: https://github.com/emscripten-core/emscripten/issues/5446
    emscripten_hide_mouse();
#endif

    SetMousePosition(0.5f * SCREEN_WIDTH, 0.5f * SCREEN_HEIGHT);
}

static void UpdateExample(void) {
    frUpdateWorld(world, DELTA_TIME);

    {
        frVector2 cursorPosition = frGetBodyPosition(cursor);

        cursorPosition.x = frPixelsToUnits(GetMousePosition().x);
        cursorPosition.y = frPixelsToUnits(GetMousePosition().y);

        frSetBodyPosition(cursor, cursorPosition);

        if (GetMouseWheelMove() > 0)
            frSetBodyAngle(cursor, frGetBodyAngle(cursor) - (2.0f * DEG2RAD));
        if (GetMouseWheelMove() < 0)
            frSetBodyAngle(cursor, frGetBodyAngle(cursor) + (2.0f * DEG2RAD));

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)
            && cursorCounter >= CURSOR_COOLDOWN) {
            frBody *box = frCreateBodyFromShape(FR_BODY_DYNAMIC,
                                                cursorPosition,
                                                frGetBodyShape(cursor));

            frSetBodyAngle(box, frGetBodyAngle(cursor));
            frSetBodyUserData(box, &boxColor);

            frAddBodyToWorld(world, box);

            cursorCounter = 0.0f;
        }

        cursorCounter += GetFrameTime();
    }

    {
        BeginDrawing();

        ClearBackground(FR_DRAW_COLOR_MATTEBLACK);

        frDrawGrid(SCREEN_BOUNDS,
                   CELL_SIZE,
                   0.25f,
                   ColorAlpha(DARKGRAY, 0.75f));

        for (int i = 0, j = frGetBodyCountInWorld(world); i < j; i++) {
            const frBody *body = frGetBodyInWorld(world, i);
            const Color *color = frGetBodyUserData(body);

            frDrawBodyLines(body, 1.0f, *color);
        }

        frDrawBodyLines(cursor, 1.0f, WHITE);

        const Font font = GetFontDefault();

        DrawTextEx(font,
                   TextFormat("%d/%d bodies",
                              frGetBodyCountInWorld(world),
                              FR_WORLD_MAX_OBJECT_COUNT),
                   (Vector2) { .x = 8.0f, .y = 32.0f },
                   font.baseSize,
                   2.0f,
                   WHITE);

        DrawFPS(8, 8);

        EndDrawing();
    }
}

static void DeinitExample(void) {
    frReleaseShape(frGetBodyShape(cursor));

    for (int i = 0; i < PLATFORM_COUNT; i++)
        frReleaseShape(frGetBodyShape(platforms[i]));

    for (int i = 0; i < BORDER_COUNT; i++)
        frReleaseShape(frGetBodyShape(borders[i]));

    frReleaseWorld(world);
}