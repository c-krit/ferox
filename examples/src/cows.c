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

#define TARGET_FPS       60

#define SCREEN_WIDTH     1280
#define SCREEN_HEIGHT    800

#define MAX_ENEMY_COUNT  256

// clang-format on

/* Typedefs ================================================================ */

typedef enum _EntityType {
    ENTITY_PLAYER,
    ENTITY_BULLET,
    ENTITY_ENEMY,
    ENTITY_COUNT_
} EntityType;

typedef struct _EntityData {
    EntityType type;
    float attackSpeed;
    float movementSpeed;
    float counter;
} EntityData;

/* Constants =============================================================== */

static const float CELL_SIZE = 4.0f, DELTA_TIME = 1.0f / TARGET_FPS;

static const frMaterial MATERIAL_BULLET = { .density = 2.25f,
                                            .friction = 0.85f,
                                            .restitution = 0.0f };

static const frMaterial MATERIAL_ENEMY = { .density = 0.85f,
                                           .friction = 0.5f,
                                           .restitution = 0.0f };

static const frMaterial MATERIAL_PLAYER = { .density = 1.25f,
                                            .friction = 0.75f,
                                            .restitution = 0.0f };

static const Rectangle SCREEN_BOUNDS = { .width = SCREEN_WIDTH,
                                         .height = SCREEN_HEIGHT };

/* Private Variables ======================================================= */

static EntityData entityData[ENTITY_COUNT_] = {
    { .type = ENTITY_PLAYER, .attackSpeed = 0.1f },
    { .type = ENTITY_BULLET, .movementSpeed = 64.0f },
    { .type = ENTITY_ENEMY, .movementSpeed = 3.5f }
};

static frVertices bulletVertices, playerVertices;

static frWorld *world;

static frBody *player;

static int enemyCount;

/* Private Function Prototypes ============================================= */

static void InitExample(void);
static void UpdateExample(void);
static void DeinitExample(void);

static void DrawCursor(void);

static void UpdateBullets(void);

static void OnPreStep(frBodyPair key, frCollision *value);

/* Public Functions ======================================================== */

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | cows.c");

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

    world = frCreateWorld(frVector2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY,
                                                  0.0f),
                          CELL_SIZE);

    frSetWorldCollisionHandler(world,
                               (frCollisionHandler) {
                                   .preStep = OnPreStep,
                               });

    bulletVertices = (frVertices) {
        .data = { frVector2PixelsToUnits((frVector2) { .x = 0.0f, .y = -7.2f }),
                  frVector2PixelsToUnits((frVector2) { .x = -2.8f, .y = 7.2f }),
                  frVector2PixelsToUnits(
                      (frVector2) { .x = 2.8f, .y = 7.2f }) },
        .count = 3
    };

    playerVertices = (frVertices) {
        .data = { frVector2PixelsToUnits(
                      (frVector2) { .x = 0.0f, .y = -16.0f }),
                  frVector2PixelsToUnits(
                      (frVector2) { .x = -14.0f, .y = 16.0f }),
                  frVector2PixelsToUnits(
                      (frVector2) { .x = 14.0f, .y = 16.0f }) },
        .count = 3
    };

    player = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        frVector2PixelsToUnits((frVector2) { .x = 0.5f * SCREEN_WIDTH,
                                             .y = 0.5f * SCREEN_HEIGHT }),
        frCreatePolygon(FR_API_STRUCT_ZERO(frMaterial), &playerVertices));

    frSetBodyUserData(player, (void *) &entityData[ENTITY_PLAYER]);

    frAddBodyToWorld(world, player);
}

static void UpdateExample(void) {
    for (int i = 0; i < MAX_ENEMY_COUNT - enemyCount; i++) {
        frVector2 position = { .x = 0.5f * SCREEN_WIDTH,
                               .y = 0.5f * SCREEN_HEIGHT };

        while (position.x >= 0.0f * SCREEN_WIDTH
               && position.x <= 1.0f * SCREEN_WIDTH)
            position.x = GetRandomValue(-2.5f * SCREEN_WIDTH,
                                        2.5f * SCREEN_WIDTH);

        while (position.y >= 0.0f * SCREEN_HEIGHT
               && position.y <= 1.0f * SCREEN_HEIGHT)
            position.y = GetRandomValue(-2.5f * SCREEN_HEIGHT,
                                        2.5f * SCREEN_HEIGHT);

        frBody *enemy = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            frVector2PixelsToUnits(position),
            frCreateCircle(MATERIAL_ENEMY, 0.35f * GetRandomValue(3, 5)));

        frSetBodyUserData(enemy, (void *) &entityData[ENTITY_ENEMY]);

        frAddBodyToWorld(world, enemy);

        enemyCount++;
    }

    const int bodyCount = frGetBodyCountForWorld(world);

    for (int i = 0; i < bodyCount; i++) {
        frBody *body = frGetBodyFromWorld(world, i);

        const EntityData *bodyData = frGetBodyUserData(body);

        if (bodyData == NULL || bodyData->type != ENTITY_BULLET) continue;

        frAABB aabb = frGetBodyAABB(body);

        if (CheckCollisionRecs(
                (Rectangle) { .x = frUnitsToPixels(aabb.x),
                              .y = frUnitsToPixels(aabb.y),
                              .width = frUnitsToPixels(aabb.width),
                              .height = frUnitsToPixels(aabb.height) },
                SCREEN_BOUNDS))
            continue;

        frRemoveBodyFromWorld(world, body);
    }

    const Vector2 mousePosition = GetMousePosition();

    frSetBodyAngle(
        player,
        frVector2Angle((frVector2) { .y = -1.0f },
                       frVector2Subtract(frVector2PixelsToUnits((frVector2) {
                                             .x = mousePosition.x,
                                             .y = mousePosition.y }),
                                         frGetBodyPosition(player))));

    UpdateBullets();

    frUpdateWorld(world, DELTA_TIME);

    {
        BeginDrawing();

        ClearBackground(FR_DRAW_COLOR_MATTEBLACK);

        frDrawGrid(SCREEN_BOUNDS,
                   CELL_SIZE,
                   0.25f,
                   ColorAlpha(DARKGRAY, 0.75f));

        const int bodyCount = frGetBodyCountForWorld(world);

        for (int i = 0; i < bodyCount; i++) {
            frBody *body = frGetBodyFromWorld(world, i);

            const EntityData *bodyData = frGetBodyUserData(body);

            if (bodyData == NULL) continue;

            const frVector2 deltaPosition = frVector2Normalize(
                frVector2Subtract(frGetBodyPosition(player),
                                  frGetBodyPosition(body)));

            switch (bodyData->type) {
                case ENTITY_BULLET:
                    frDrawBodyLines(body, 2.0f, ColorAlpha(YELLOW, 0.85f));

                    break;

                case ENTITY_ENEMY:
                    frSetBodyVelocity(
                        body,
                        frVector2ScalarMultiply(deltaPosition,
                                                bodyData->movementSpeed));

                    frDrawBodyLines(body, 2.0f, ColorAlpha(RED, 0.65f));

                    break;

                case ENTITY_PLAYER:
                    frDrawBodyLines(body, 2.0f, ColorAlpha(GREEN, 0.95f));

                    break;
            }
        }

        DrawCursor();

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

static void UpdateBullets(void) {
    EntityData *playerData = &entityData[ENTITY_PLAYER];

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)
        && playerData->counter >= playerData->attackSpeed) {
        frBody *bullet = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            frVector2Transform(playerVertices.data[0],
                               frGetBodyTransform(player)),
            frCreatePolygon(MATERIAL_BULLET, &bulletVertices));

        const Vector2 mousePosition = GetMousePosition();

        const frVector2 direction =
            frVector2Subtract(frVector2PixelsToUnits((frVector2) {
                                  .x = mousePosition.x, .y = mousePosition.y }),
                              frGetBodyPosition(player));

        frSetBodyAngle(bullet,
                       frVector2Angle((frVector2) { .y = -1.0f }, direction));
        frSetBodyUserData(bullet, (void *) &entityData[ENTITY_BULLET]);

        frSetBodyVelocity(
            bullet,
            frVector2ScalarMultiply(frVector2Normalize(direction),
                                    entityData[ENTITY_BULLET].movementSpeed));

        frAddBodyToWorld(world, bullet);

        playerData->counter = 0.0f;
    }

    playerData->counter += GetFrameTime();
}

static void OnPreStep(frBodyPair key, frCollision *value) {
    if (value->count == 0) return;

    const EntityData *bodyData1 = frGetBodyUserData(key.first);
    const EntityData *bodyData2 = frGetBodyUserData(key.second);

    if ((bodyData1->type == ENTITY_BULLET && bodyData2->type == ENTITY_ENEMY)
        || (bodyData1->type == ENTITY_ENEMY
            && bodyData2->type == ENTITY_BULLET)) {
        frBody *bullet = NULL, *enemy = NULL;

        if (bodyData1->type == ENTITY_BULLET)
            bullet = key.first, enemy = key.second;
        else
            bullet = key.second, enemy = key.first;

        frRemoveBodyFromWorld(world, bullet);
        frRemoveBodyFromWorld(world, enemy);

        value->count = 0, enemyCount--;
    }
}