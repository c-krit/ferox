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

#define TARGET_FPS        60

#define SCREEN_WIDTH      600
#define SCREEN_HEIGHT     800

#define CURSOR_COOLDOWN   1.0f

#define BORDER_COUNT      4
#define MELON_KIND_COUNT  4

// clang-format on

/* Typedefs ================================================================ */

typedef struct MelonKind_ {
    int index;
    Color color;
} MelonKind;

/* Constants =============================================================== */

static const Rectangle SCREEN_BOUNDS = { .width = SCREEN_WIDTH,
                                         .height = SCREEN_HEIGHT };

static const MelonKind MELON_KINDS[MELON_KIND_COUNT] = {
    { .index = 0, .color = RED },
    { .index = 1, .color = ORANGE },
    { .index = 2, .color = YELLOW },
    { .index = 3, .color = GREEN }
};

static const float CELL_SIZE = 2.0f, DELTA_TIME = 1.0f / (TARGET_FPS << 1);

/* Private Variables ======================================================= */

static frWorld *world;

static frShape *melonShapes[MELON_KIND_COUNT];

static frBody *cursor, *borders[BORDER_COUNT];

static float cursorCounter = CURSOR_COOLDOWN;

/* Private Function Prototypes ============================================= */

static void InitExample(void);
static void UpdateExample(void);
static void DeinitExample(void);

static void OnPostStep(frBodyPair key, frCollision *value);

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
                                                  1.25f),
                          CELL_SIZE);

    frSetWorldCollisionHandler(world,
                               (frCollisionHandler) {
                                   .postStep = OnPostStep,
                               });

    borders[0] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                             .y = 1.05f * SCREEN_HEIGHT }),
        frCreateRectangle((frMaterial) { .density = 1.25f,
                                         .friction = 0.75f,
                                         .restitution = 0.05f },
                          frPixelsToUnits(1.0f * SCREEN_WIDTH),
                          frPixelsToUnits(0.1f * SCREEN_HEIGHT)));

    borders[1] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits((frVector2) { .x = 1.05f * SCREEN_WIDTH,
                                             .y = 0.5f * SCREEN_HEIGHT }),
        frCreateRectangle((frMaterial) { .density = 1.25f,
                                         .friction = 0.75f,
                                         .restitution = 0.05f },
                          frPixelsToUnits(0.1f * SCREEN_WIDTH),
                          frPixelsToUnits(1.0f * SCREEN_HEIGHT)));

    borders[2] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                             .y = -0.05f * SCREEN_HEIGHT }),
        frCreateRectangle((frMaterial) { .density = 1.25f,
                                         .friction = 0.75f,
                                         .restitution = 0.05f },
                          frPixelsToUnits(1.0f * SCREEN_WIDTH),
                          frPixelsToUnits(0.1f * SCREEN_HEIGHT)));

    borders[3] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        frVector2PixelsToUnits((frVector2) { .x = -0.05f * SCREEN_WIDTH,
                                             .y = 0.5f * SCREEN_HEIGHT }),
        frCreateRectangle((frMaterial) { .density = 1.25f,
                                         .friction = 0.65f,
                                         .restitution = 0.05f },
                          frPixelsToUnits(0.1f * SCREEN_WIDTH),
                          frPixelsToUnits(1.0f * SCREEN_HEIGHT)));

    for (int i = 0; i < BORDER_COUNT; i++)
        frAddBodyToWorld(world, borders[i]);

    for (int i = 0; i < MELON_KIND_COUNT; i++)
        melonShapes[i] = frCreateCircle((frMaterial) { .density = 0.25f
                                                                  / (i + 1),
                                                       .friction = 0.75f,
                                                       .restitution = 0.03f },
                                        0.85f + (0.1f * (i + 3)));

    cursor = frCreateBodyFromShape(FR_BODY_KINEMATIC,
                                   frVector2PixelsToUnits((frVector2) {
                                       .x = 0.5f * SCREEN_WIDTH,
                                       .y = 0.1f * SCREEN_HEIGHT }),
                                   melonShapes[0]);

    frSetBodyUserData(cursor, (void *) &MELON_KINDS[0]);
}

static void UpdateExample(void) {
    const MelonKind *cursorKind = frGetBodyUserData(cursor);

    {
        frVector2 cursorPosition = frGetBodyPosition(cursor);

        cursorPosition.x = frPixelsToUnits((GetMousePosition()).x);

        float cursorRadius = 0.1f + frGetCircleRadius(frGetBodyShape(cursor));

        if (cursorPosition.x < cursorRadius) cursorPosition.x = cursorRadius;

        if (cursorPosition.x > (frPixelsToUnits(SCREEN_WIDTH) - cursorRadius))
            cursorPosition.x = (frPixelsToUnits(SCREEN_WIDTH) - cursorRadius);

        frSetBodyPosition(cursor, cursorPosition);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
            && cursorCounter >= CURSOR_COOLDOWN) {
            frBody *melon =
                frCreateBodyFromShape(FR_BODY_DYNAMIC,
                                      cursorPosition,
                                      melonShapes[cursorKind->index]);

            frSetBodyAngle(melon, DEG2RAD * GetRandomValue(0, 360));
            frSetBodyUserData(melon, (void *) cursorKind);

            frAddBodyToWorld(world, melon);

            int cursorIndex = GetRandomValue(0, MELON_KIND_COUNT - 1);

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

        float cursorAlpha = 0.15f + (cursorCounter >= CURSOR_COOLDOWN) * 0.25f;

        frDrawBodyLines(cursor,
                        2.0f,
                        ColorAlpha(cursorKind->color, cursorAlpha));

        const int bodyCount = frGetBodyCountInWorld(world);

        for (int i = 0; i < bodyCount; i++) {
            frBody *body = frGetBodyInWorld(world, i);

            const MelonKind *melonKind = frGetBodyUserData(body);

            if (melonKind != NULL)
                frDrawBodyLines(body, 2.0f, melonKind->color);
            else
                frDrawBodyLines(body, 1.0f, DARKGRAY);
        }

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
    for (int i = 0; i < MELON_KIND_COUNT; i++)
        frReleaseShape(melonShapes[i]);

    frReleaseBody(cursor);

    frReleaseWorld(world);
}

static void OnPostStep(frBodyPair key, frCollision *value) {
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