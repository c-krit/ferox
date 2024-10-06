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

#define TARGET_FPS        60

#define SCREEN_WIDTH      1280
#define SCREEN_HEIGHT     800

#define MAX_OBJECT_COUNT  128

// clang-format on

/* Constants =============================================================== */

static const Rectangle SCREEN_BOUNDS = { .width = SCREEN_WIDTH,
                                         .height = SCREEN_HEIGHT };

static const float CELL_SIZE = 4.0f, DELTA_TIME = 1.0f / TARGET_FPS;

/* Private Variables ======================================================= */

static frWorld *world;

static frBody *player;

/* Private Function Prototypes ============================================= */

static void InitExample(void);
static void UpdateExample(void);
static void DeinitExample(void);

static void DrawCursor(void);

static void OnRaycastQuery(frRaycastHit raycastHit, frContextNode queryResult);

/* Public Functions ======================================================== */

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | raycast.c");

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
    HideCursor();

#ifdef PLATFORM_WEB
    // TODO: https://github.com/emscripten-core/emscripten/issues/5446
    emscripten_hide_mouse();
#endif

    SetMousePosition(0.5f * SCREEN_WIDTH, 0.5f * SCREEN_HEIGHT);

    world = frCreateWorld(FR_WORLD_DEFAULT_GRAVITY, CELL_SIZE);

    player = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                             .y = 0.5f * SCREEN_HEIGHT }),
        frCreatePolygon(FR_API_STRUCT_ZERO(frMaterial),
                        &(const frVertices) {
                            .data = { frVector2PixelsToUnits((frVector2) {
                                          .x = 0.0f, .y = -16.0f }),
                                      frVector2PixelsToUnits((frVector2) {
                                          .x = -14.0f, .y = 16.0f }),
                                      frVector2PixelsToUnits((frVector2) {
                                          .x = 14.0f, .y = 16.0f }) },
                            .count = 3 }));

    frAddBodyToWorld(world, player);

    for (int i = 0; i < MAX_OBJECT_COUNT; i++) {
        frVector2 position = {
            .x = GetRandomValue(0, 1)
                     ? GetRandomValue(0, 0.47f * SCREEN_WIDTH)
                     : GetRandomValue(0.53f * SCREEN_WIDTH, SCREEN_WIDTH),
            .y = GetRandomValue(0, 1)
                     ? GetRandomValue(0, 0.47f * SCREEN_HEIGHT)
                     : GetRandomValue(0.53f * SCREEN_HEIGHT, SCREEN_HEIGHT)
        };

        frBody *body =
            frCreateBodyFromShape(FR_BODY_STATIC,
                                  frVector2PixelsToUnits(position),
                                  frCreateCircle(FR_API_STRUCT_ZERO(frMaterial),
                                                 0.22f * GetRandomValue(2, 4)));

        frAddBodyToWorld(world, body);
    }
}

static void UpdateExample(void) {
    const Vector2 mousePosition = GetMousePosition();

    frSetBodyAngle(
        player,
        frVector2Angle((frVector2) { .y = -1.0f },
                       frVector2Subtract(frVector2PixelsToUnits((frVector2) {
                                             .x = mousePosition.x,
                                             .y = mousePosition.y }),
                                         frGetBodyPosition(player))));

    frVector2 rayOrigin = frVector2Transform(frGetPolygonVertex(frGetBodyShape(
                                                                    player),
                                                                2),
                                             frGetBodyTransform(player));

    frVector2 rayDirection = frVector2Subtract(frVector2PixelsToUnits(
                                                   (frVector2) {
                                                       .x = mousePosition.x,
                                                       .y = mousePosition.y }),
                                               rayOrigin);

    frRay ray = { .origin = frVector2Add(
                      rayOrigin,
                      frVector2ScalarMultiply(frVector2Normalize(rayDirection),
                                              0.25f)),
                  .direction = rayDirection,
                  .maxDistance = frVector2Magnitude(rayDirection) };

    frUpdateWorld(world, DELTA_TIME);

    {
        BeginDrawing();

        ClearBackground(FR_DRAW_COLOR_MATTEBLACK);

        frDrawGrid(SCREEN_BOUNDS,
                   CELL_SIZE,
                   0.25f,
                   ColorAlpha(DARKGRAY, 0.75f));

        const int bodyCount = frGetBodyCountInWorld(world);

        for (int i = 1; i < bodyCount; i++)
            frDrawBodyLines(frGetBodyInWorld(world, i),
                            2.0f,
                            ColorAlpha(LIGHTGRAY, 0.95f));

        Color ringColor = ColorAlpha(YELLOW, 0.85f);

        frComputeWorldRaycast(world, ray, OnRaycastQuery, &ringColor);

        frDrawBodyLines(player, 2.0f, ColorAlpha(GREEN, 0.85f));

        frDrawArrow(rayOrigin,
                    frVector2Add(rayOrigin, rayDirection),
                    1.0f,
                    ColorAlpha(GREEN, 0.85f));

        DrawCursor();

        DrawFPS(8, 8);

        EndDrawing();
    }
}

static void DeinitExample(void) {
    frReleaseWorld(world);
}

static void DrawCursor(void) {
    const Vector2 mousePosition = GetMousePosition();

    DrawLineEx((Vector2) { .x = mousePosition.x - 8.0f, .y = mousePosition.y },
               (Vector2) { .x = mousePosition.x + 8.0f, .y = mousePosition.y },
               2.0f,
               WHITE);

    DrawLineEx((Vector2) { .x = mousePosition.x, .y = mousePosition.y - 8.0f },
               (Vector2) { .x = mousePosition.x, .y = mousePosition.y + 8.0f },
               2.0f,
               WHITE);
}

static void OnRaycastQuery(frRaycastHit raycastHit, frContextNode queryResult) {
    const Color *ringColor = queryResult.ctx;

    frDrawBodyAABB(raycastHit.body, 1.0f, *ringColor);

    Vector2 center = { .x = frUnitsToPixels(raycastHit.point.x),
                       .y = frUnitsToPixels(raycastHit.point.y) };

    DrawRing(center, 6.0f, 8.0f, 0.0f, 360.0f, 16, *ringColor);
}