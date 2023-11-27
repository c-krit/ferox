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

#define CURSOR_COOLDOWN   0.5f

#define MAX_WALL_COUNT    3
#define MELON_KIND_COUNT  4

// clang-format on

/* Typedefs ================================================================ */

typedef struct _MelonKind {
    int index;
    Color color;
} MelonKind;

/* Constants =============================================================== */

static const frMaterial MATERIAL_WALL = { .density = 1.25f,
                                          .friction = 0.5f,
                                          .restitution = 0.05f };

static const Rectangle SCREEN_BOUNDS = { .width = SCREEN_WIDTH,
                                         .height = SCREEN_HEIGHT };

static const MelonKind MELON_KINDS[MELON_KIND_COUNT] = {
    { .index = 0, .color = RED },
    { .index = 1, .color = ORANGE },
    { .index = 2, .color = YELLOW },
    { .index = 3, .color = GREEN }
};

static const float CELL_SIZE = 2.0f, DELTA_TIME = 1.0f / (2.0f * TARGET_FPS);

/* Private Variables ======================================================= */

static frWorld *world;

static frShape *melonShapes[MELON_KIND_COUNT];

static frBody *cursor, *walls[MAX_WALL_COUNT];

static float cursorCounter;

/* Private Function Prototypes ============================================= */

static void InitExample(void);
static void UpdateExample(void);
static void DeinitExample(void);

static void OnPreStep(frBodyPair key, frCollision *value);

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
                                                  1.0f),
                          CELL_SIZE);

    frSetWorldCollisionHandler(world,
                               (frCollisionHandler) {
                                   .preStep = OnPreStep,
                               });

    for (int i = 0; i < MELON_KIND_COUNT; i++)
        melonShapes[i] = frCreateCircle((frMaterial) { .density = 2.5f
                                                                  / (i + 1),
                                                       .friction = 0.35f,
                                                       .restitution = 0.05f },
                                        0.18f * (i + 3));

    walls[0] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                             .y = 0.95f * SCREEN_HEIGHT }),
        frCreateRectangle(MATERIAL_WALL,
                          frPixelsToUnits(1.0f * SCREEN_WIDTH),
                          frPixelsToUnits(0.1f * SCREEN_HEIGHT)));

    const frVertices wallVertices1 = {
        .data = { frVector2PixelsToUnits(
                      (frVector2) { .x = -80.0f, .y = 400.0f }),
                  frVector2PixelsToUnits(
                      (frVector2) { .x = -80.0f, .y = -400.0f }),
                  frVector2PixelsToUnits(
                      (frVector2) { .x = 120.0f, .y = 400.0f }),
                  frVector2PixelsToUnits(
                      (frVector2) { .x = 0.0f, .y = -400.0f }) },
        .count = 4
    };

    walls[1] = frCreateBodyFromShape(FR_BODY_STATIC,
                                     frVector2PixelsToUnits((frVector2) {
                                         .x = 0.02f * SCREEN_WIDTH,
                                         .y = 0.5f * SCREEN_HEIGHT }),
                                     frCreatePolygon(MATERIAL_WALL,
                                                     &wallVertices1));

    const frVertices wallVertices2 = {
        .data = { frVector2PixelsToUnits(
                      (frVector2) { .x = 0.0f, .y = -400.0f }),
                  frVector2PixelsToUnits(
                      (frVector2) { .x = -120.0f, .y = 400.0f }),
                  frVector2PixelsToUnits(
                      (frVector2) { .x = 80.0f, .y = 400.0f }),
                  frVector2PixelsToUnits(
                      (frVector2) { .x = 80.0f, .y = -400.0f }) },
        .count = 4
    };

    walls[2] = frCreateBodyFromShape(FR_BODY_STATIC,
                                     frVector2PixelsToUnits((frVector2) {
                                         .x = 0.98f * SCREEN_WIDTH,
                                         .y = 0.5f * SCREEN_HEIGHT }),
                                     frCreatePolygon(MATERIAL_WALL,
                                                     &wallVertices2));

    for (int i = 0; i < MAX_WALL_COUNT; i++)
        frAddBodyToWorld(world, walls[i]);

    cursor = frCreateBodyFromShape(FR_BODY_KINEMATIC,
                                   frVector2PixelsToUnits((frVector2) {
                                       .x = 0.5f * SCREEN_WIDTH,
                                       .y = 0.15f * SCREEN_HEIGHT }),
                                   melonShapes[0]);

    frSetBodyUserData(cursor, (void *) &MELON_KINDS[0]);

    cursorCounter = CURSOR_COOLDOWN;
}

static void UpdateExample(void) {
    const MelonKind *cursorKind = frGetBodyUserData(cursor);

    {
        frVector2 cursorPosition = frGetBodyPosition(cursor);

        if (IsKeyDown(KEY_LEFT)) cursorPosition.x -= 0.2f;
        if (IsKeyDown(KEY_RIGHT)) cursorPosition.x += 0.2f;

        if (cursorPosition.x < frPixelsToUnits(0.1f * SCREEN_WIDTH))
            cursorPosition.x = frPixelsToUnits(0.1f * SCREEN_WIDTH);

        if (cursorPosition.x > frPixelsToUnits(0.9f * SCREEN_WIDTH))
            cursorPosition.x = frPixelsToUnits(0.9f * SCREEN_WIDTH);

        frSetBodyPosition(cursor, cursorPosition);

        if (IsKeyPressed(KEY_SPACE) && cursorCounter >= CURSOR_COOLDOWN) {
            frBody *melon =
                frCreateBodyFromShape(FR_BODY_DYNAMIC,
                                      cursorPosition,
                                      melonShapes[cursorKind->index]);

            frSetBodyAngle(melon, DEG2RAD * GetRandomValue(0, 360));
            frSetBodyUserData(melon, (void *) cursorKind);

            frAddBodyToWorld(world, melon);

            const int cursorIndex = GetRandomValue(0, MELON_KIND_COUNT - 1);

            frSetBodyShape(cursor, melonShapes[cursorIndex]);
            frSetBodyUserData(cursor, (void *) &MELON_KINDS[cursorIndex]);

            cursorCounter = 0.0f;
        }

        cursorCounter += GetFrameTime();
    }

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

        const int bodyCount = frGetBodyCountForWorld(world);

        for (int i = MAX_WALL_COUNT; i < bodyCount; i++) {
            frBody *melon = frGetBodyFromWorld(world, i);

            const MelonKind *melonKind = frGetBodyUserData(melon);

            frDrawBodyLines(melon, 2.0f, melonKind->color);
        }

        frDrawBodyLines(cursor, 2.0f, ColorAlpha(cursorKind->color, 0.5f));

        const Font font = GetFontDefault();

        DrawTextEx(font,
                   TextFormat("%d/%d bodies",
                              frGetBodyCountForWorld(world),
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
    for (int i = 0; i < MELON_KIND_COUNT; i++)
        frReleaseShape(melonShapes[i]);

    for (int i = 0; i < MAX_WALL_COUNT; i++)
        frReleaseShape(frGetBodyShape(walls[i]));

    frReleaseBody(cursor);
    frReleaseWorld(world);
}

static void OnPreStep(frBodyPair key, frCollision *value) {
    const MelonKind *bodyData1 = frGetBodyUserData(key.first);
    const MelonKind *bodyData2 = frGetBodyUserData(key.second);

    if (key.first == cursor || key.second == cursor || bodyData1 == NULL
        || bodyData2 == NULL || bodyData1->index != bodyData2->index
        || bodyData1->index >= MELON_KIND_COUNT - 1)
        return;

    frVector2 bodyPosition1 = frGetBodyPosition(key.first);
    frVector2 bodyPosition2 = frGetBodyPosition(key.second);

    frVector2 newPosition = (bodyPosition1.y < bodyPosition2.y) ? bodyPosition1
                                                                : bodyPosition2;

    int newIndex = bodyData1->index + 1;

    frRemoveBodyFromWorld(world, key.first);
    frRemoveBodyFromWorld(world, key.second);

    frBody *newMelon = frCreateBodyFromShape(FR_BODY_DYNAMIC,
                                             newPosition,
                                             melonShapes[newIndex]);

    frSetBodyUserData(newMelon, (void *) &MELON_KINDS[newIndex]);

    frAddBodyToWorld(world, newMelon);

    value->count = 0;
}