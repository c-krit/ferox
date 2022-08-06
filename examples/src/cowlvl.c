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

/* | `cowlvl` 모듈 매크로 정의... | */

#define TARGET_FPS       60

#define SCREEN_WIDTH     1024
#define SCREEN_HEIGHT    768

#define MATTEBLACK       ((Color) { 15, 15, 15, 255 })

#define MAX_ENEMY_COUNT  128

/* | `cowlvl` 모듈 자료형 정의... | */

typedef enum ObjType {
    OBJ_TYPE_PLAYER,
    OBJ_TYPE_BULLET,
    OBJ_TYPE_ENEMY,
    OBJ_TYPE_COUNT_
} ObjType;

typedef struct ObjData {
    ObjType type;
    /* TODO: ... */
} ObjData;

/* | `cowlvl` 모듈 변수 및 상수... | */

const frMaterial MATERIAL_PLAYER = { 
    .density         = 1.5f, 
    .staticFriction  = 0.5f, 
    .dynamicFriction = 0.25f
};

const frMaterial MATERIAL_BULLET = { 
    .density         = 2.0f, 
    .staticFriction  = 0.5f, 
    .dynamicFriction = 0.25f
};

const frMaterial MATERIAL_ENEMY = { 
    .density         = 0.85f, 
    .staticFriction  = 0.5f, 
    .dynamicFriction = 0.25f
};

const ObjData userData[OBJ_TYPE_COUNT_] = {
    { .type = OBJ_TYPE_PLAYER },
    { .type = OBJ_TYPE_BULLET },
    { .type = OBJ_TYPE_ENEMY  }
};

const float DELTA_TIME = (1.0f / TARGET_FPS) * 100.0f;
const float FIRE_RATE = 0.1f * TARGET_FPS, PLAYER_SPEED = 0.028f;

const float ENEMY_IMPULSE_MULTIPLIER = 0.0001f;
const float BULLET_IMPULSE_MULTIPLIER = 0.02f;

static frWorld *world;
static frBody *player;

static frVertices playerVertices, bulletVertices;

static Texture2D logo;

static int enemyCount;

/* | `cowlvl` 모듈 함수... | */

/* 예제 프로그램을 초기화한다. */
static void InitExample(void);

/* 예제 프로그램을 실행한다. */
static void UpdateExample(void);

/* 예제 프로그램에 할당된 메모리를 해제한다. */
static void DeinitExample(void);

/* 두 강체 사이의 충돌 해결 직전에 호출되는 함수. */
static void onCollisionPreSolve(frCollision *collision);

/* 게임 화면 내에 커서를 그린다. */
static void DrawCustomCursor(Vector2 position);

/* 사용자 입력에 따라 플레이어를 움직인다. */
static void MovePlayer(void);

/* 플레이어가 발사한 투사체들을 업데이트한다. */
static void UpdateBullets(void);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | cowlvl.c");

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
    HideCursor();

#ifdef PLATFORM_WEB
    // TODO: https://github.com/emscripten-core/emscripten/issues/5446
    emscripten_hide_mouse();
#endif

    const Rectangle bounds = {
        .y = -0.25f * frNumberPixelsToMeters(SCREEN_HEIGHT),
        .width = 1.0f * frNumberPixelsToMeters(SCREEN_WIDTH),
        .height = 1.5f * frNumberPixelsToMeters(SCREEN_HEIGHT)
    };

    // 게임 세계를 생성한다.
    world = frCreateWorld(FR_STRUCT_ZERO(Vector2), bounds);

    // 충돌 이벤트 처리 함수를 설정한다.
    frSetWorldCollisionHandler(
        world,
        (frCollisionHandler) { 
            .preSolve = onCollisionPreSolve
        }
    );

    {
        playerVertices = (frVertices) {
            .data = {
                frVec2PixelsToMeters((Vector2) {  0.0f, -18.0f }),
                frVec2PixelsToMeters((Vector2) { -16.0f, 18.0f }),
                frVec2PixelsToMeters((Vector2) {  16.0f, 18.0f })
            },
            .count = 3
        };

        bulletVertices = (frVertices) {
            .data = {
                frVec2PixelsToMeters((Vector2) {  0.0f, -7.2f }),
                frVec2PixelsToMeters((Vector2) { -2.8f,  7.2f }),
                frVec2PixelsToMeters((Vector2) {  2.8f,  7.2f })
            },
            .count = 3
        };

        player = frCreateBodyFromShape(
            FR_BODY_KINEMATIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, 0.88f * SCREEN_HEIGHT }),
            frCreatePolygon(MATERIAL_PLAYER, playerVertices)
        );
    }

    frSetBodyUserData(player, (void *) &userData[OBJ_TYPE_PLAYER]);

    frAddToWorld(world, player);

    logo = LoadTexture("../res/images/logo_small.png");
}

/* 예제 프로그램을 실행한다. */
static void UpdateExample(void) {
    if (enemyCount < MAX_ENEMY_COUNT) {
        for (int i = 0; i < MAX_ENEMY_COUNT - enemyCount; i++) {
            Vector2 position = FR_STRUCT_ZERO(Vector2);
            
            position.x = GetRandomValue(0.05f * SCREEN_WIDTH, 0.95f * SCREEN_WIDTH);
            position.y = GetRandomValue(-0.25f * SCREEN_HEIGHT, -0.05f * SCREEN_HEIGHT);
            
            frBody *enemy = frCreateBodyFromShape(
                FR_BODY_DYNAMIC,
                FR_FLAG_INFINITE_INERTIA,
                frVec2PixelsToMeters(position),
                frCreateCircle(MATERIAL_ENEMY, 0.5f * GetRandomValue(2, 5))
            );
            
            frSetBodyUserData(enemy, (void *) &userData[OBJ_TYPE_ENEMY]);
            
            frAddToWorld(world, enemy);

            enemyCount++;
        }
    }

    for (int i = 0; i < frGetWorldBodyCount(world); i++) {
        frBody *body = frGetWorldBody(world, i);
        
        if (!frIsInWorldBounds(world, body)) {
            const ObjData *userData = frGetBodyUserData(body);

            if (userData->type == OBJ_TYPE_ENEMY) enemyCount--;

            frRemoveFromWorld(world, body);
            frReleaseBody(body);
        }
    }

    frSimulateWorld(world, DELTA_TIME);

    MovePlayer();
    UpdateBullets();

    {
        BeginDrawing();
            
        ClearBackground(MATTEBLACK);

        const Vector2 position = {
            0.5f * (SCREEN_WIDTH - logo.width),
            0.5f * (SCREEN_HEIGHT - logo.height)
        };

        DrawTextureV(logo, position, ColorAlpha(WHITE, 0.25f));

        for (int i = 0; i < frGetWorldBodyCount(world); i++) {
            frBody *body = frGetWorldBody(world, i);
            
            const ObjData *objData = frGetBodyUserData(body);

            if (objData == NULL) continue;

            const Vector2 bodyPos = frGetBodyPosition(body);

            const Vector2 enemyDirection = frVec2Normalize(
                frVec2Subtract(
                    frGetBodyPosition(player), 
                    frGetBodyPosition(body)
                )
            );

            switch (objData->type) {
                case OBJ_TYPE_PLAYER:
                    frDrawBodyLines(body, 2.0f, ColorAlpha(GREEN, 0.95f));

                    break;

                case OBJ_TYPE_BULLET:
                    frDrawBodyLines(body, 2.0f, ColorAlpha(LIME, 0.85f));

                    break;

                case OBJ_TYPE_ENEMY:
                    frApplyImpulse(
                        body, 
                        frVec2ScalarMultiply(
                            enemyDirection, 
                            ENEMY_IMPULSE_MULTIPLIER
                        )
                    );

                    frDrawArrow(
                        bodyPos,
                        frVec2Add(
                            bodyPos,
                            frVec2ScalarMultiply(
                                enemyDirection,
                                1.3f * frGetCircleRadius(frGetBodyShape(body))
                            )
                        ),
                        2.0f,
                        ColorAlpha(MAROON, 0.95f)
                    );

                    frDrawBodyLines(body, 2.0f, ColorAlpha(RED, 0.65f));

                    break;

                default:
                    /* no-op */

                    break;
            }
        }

        frDrawSpatialHash(frGetWorldSpatialHash(world), 0.25f, DARKGRAY);

        const Font font = GetFontDefault();

        DrawTextEx(
            font, 
            TextFormat("%d bodies", frGetWorldBodyCount(world)),
            (Vector2) { 8.0f, 32.0f },
            font.baseSize,
            2.0f,
            WHITE
        );

        DrawCustomCursor(GetMousePosition());

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

/* 두 강체 사이의 충돌 해결 직전에 호출되는 함수. */
static void onCollisionPreSolve(frCollision *collision) {
    frBody *b1 = collision->cache.bodies[0];
    frBody *b2 = collision->cache.bodies[1];
            
    ObjData *objData1 = frGetBodyUserData(b1);
    ObjData *objData2 = frGetBodyUserData(b2);

    if ((objData1->type == OBJ_TYPE_BULLET && objData2->type == OBJ_TYPE_ENEMY)
       || (objData1->type == OBJ_TYPE_ENEMY && objData2->type == OBJ_TYPE_BULLET)) {
        frBody *bullet = NULL, *enemy = NULL;

        if (objData1->type == OBJ_TYPE_BULLET) {
            bullet = b1;
            enemy = b2;
        } else {
            bullet = b2;
            enemy = b1;
        }

        collision->check = false;
        
        frSetBodyPosition(bullet, (Vector2) { -FLT_MAX, -FLT_MAX });
        frSetBodyPosition(enemy, (Vector2) { -FLT_MAX, -FLT_MAX });
    }
}

/* 게임 화면 내에 커서를 그린다. */
static void DrawCustomCursor(Vector2 position) {
    DrawLineEx(
        (Vector2) { position.x - 8.0f, position.y },
        (Vector2) { position.x + 8.0f, position.y },
        2.0f,
        GREEN
    );
    
    DrawLineEx(
        (Vector2) { position.x, position.y - 8.0f },
        (Vector2) { position.x, position.y + 8.0f },
        2.0f,
        GREEN
    );
}

/* 사용자 입력에 따라 플레이어를 움직인다. */
static void MovePlayer(void) {
    Vector2 position = frGetBodyPosition(player);
    Vector2 velocity = frGetBodyVelocity(player);
    
    if (IsKeyDown(KEY_A)) {
        velocity.x = -PLAYER_SPEED;
        
        if (IsKeyPressed(KEY_LEFT_SHIFT)) position.x -= 4.0f;
    } else if (IsKeyDown(KEY_D)) {
        velocity.x = PLAYER_SPEED;

        if (IsKeyPressed(KEY_LEFT_SHIFT)) position.x += 4.0f;
    } else velocity.x = 0.0f;

    Rectangle halfAABB = frGetBodyAABB(player);

    halfAABB.width *= 0.5f;

    if (position.x <= halfAABB.width)
        position.x = halfAABB.width;
    else if (position.x >= frNumberPixelsToMeters(SCREEN_WIDTH) - halfAABB.width)
        position.x = frNumberPixelsToMeters(SCREEN_WIDTH) - halfAABB.width;
    
    frSetBodyPosition(player, position);
    frSetBodyVelocity(player, velocity);

    const Vector2 direction = frVec2Subtract(
        frVec2PixelsToMeters(GetMousePosition()), 
        position
    );

    frSetBodyRotation(player, frVec2Angle((Vector2) { .y = -1.0f }, direction));
}

/* 플레이어가 발사한 투사체들을 업데이트한다. */
static void UpdateBullets(void) {
    static int frameCounter = FIRE_RATE;

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && frameCounter >= FIRE_RATE) {
        frBody *bullet = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            FR_FLAG_INFINITE_INERTIA,
            frGetWorldPoint(player, playerVertices.data[0]),
            frCreatePolygon(MATERIAL_BULLET, bulletVertices)
        );

        const Vector2 direction = frVec2Subtract(
            frVec2PixelsToMeters(GetMousePosition()), 
            frGetBodyPosition(player)
        );
        
        frSetBodyRotation(bullet, frVec2Angle((Vector2) { .y = -1.0f }, direction));
        frSetBodyUserData(bullet, (void *) &userData[OBJ_TYPE_BULLET]);
        
        frApplyImpulse(
            bullet, 
            frVec2ScalarMultiply(
                frVec2Normalize(direction), 
                BULLET_IMPULSE_MULTIPLIER
            )
        );
        
        frAddToWorld(world, bullet);

        frameCounter = 0;
    }
    
    frameCounter++;
}