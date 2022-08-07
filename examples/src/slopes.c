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

/* | `slopes` 모듈 매크로 정의... | */

#define TARGET_FPS       60

#define SCREEN_WIDTH     800
#define SCREEN_HEIGHT    600

#define MATTEBLACK       ((Color) { 15, 15, 15, 255 })

#define MAX_BALL_COUNT   224
#define MAX_SLOPE_COUNT  4
#define MAX_WALL_COUNT   2

/* | `slopes` 모듈 변수 및 상수... | */

const frMaterial MATERIAL_WALL = {
    .density         = 1.25f, 
    .staticFriction  = 0.75f, 
    .dynamicFriction = 0.5f
};

const frMaterial MATERIAL_SLOPE = {
    .density         = 1.25f, 
    .staticFriction  = 0.75f, 
    .dynamicFriction = 0.5f
};

const frMaterial MATERIAL_BALL = {
    .density         = 1.0f, 
    .staticFriction  = 0.5f,
    .dynamicFriction = 0.35f
};

const float BALL_RADIUS = 9.6f;
const float DELTA_TIME = (1.0f / TARGET_FPS) * 200.0f;

const Vector2 ORIGIN_POS = { BALL_RADIUS, BALL_RADIUS };

static frWorld *world;

static frBody *walls[MAX_WALL_COUNT];
static frBody *slopes[MAX_SLOPE_COUNT];

static RenderTexture ball;
static Texture2D logo;

static int ballCount;

/* | `slopes` 모듈 함수... | */

/* 예제 프로그램을 초기화한다. */
static void InitExample(void);

/* 예제 프로그램을 실행한다. */
static void UpdateExample(void);

/* 예제 프로그램에 할당된 메모리를 해제한다. */
static void DeinitExample(void);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | slopes.c");

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
    const Rectangle bounds = {
        .x = -0.05f * frNumberPixelsToMeters(SCREEN_WIDTH),
        .y = -0.25f * frNumberPixelsToMeters(SCREEN_HEIGHT),
        .width = 1.1f * frNumberPixelsToMeters(SCREEN_WIDTH),
        .height = 1.5f * frNumberPixelsToMeters(SCREEN_HEIGHT)
    };

    // 게임 세계를 생성한다.
    world = frCreateWorld(frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.00001f), bounds);

    {
        // 왼쪽 벽을 생성한다.
        walls[0] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { -0.05f * SCREEN_WIDTH, 0.5f * SCREEN_HEIGHT }),
            frCreateRectangle(
                MATERIAL_WALL,
                frNumberPixelsToMeters(0.1f * SCREEN_WIDTH),
                frNumberPixelsToMeters(SCREEN_HEIGHT)
            )
        );

        // 오른쪽 벽을 생성한다.
        walls[1] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { 1.05f * SCREEN_WIDTH, 0.5f * SCREEN_HEIGHT }),
            frCreateRectangle(
                MATERIAL_WALL,
                frNumberPixelsToMeters(0.1f * SCREEN_WIDTH),
                frNumberPixelsToMeters(SCREEN_HEIGHT)
            )
        );

        for (int i = 0; i < MAX_WALL_COUNT; i++)
            frAddToWorld(world, walls[i]);
    }

    {
        slopes[0] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { 0.8f * SCREEN_WIDTH, 0.15f * SCREEN_HEIGHT }),
            frCreateRectangle(
                MATERIAL_SLOPE,
                frNumberPixelsToMeters(0.65f * SCREEN_WIDTH),
                frNumberPixelsToMeters(0.05f * SCREEN_HEIGHT)
            )
        );

        frSetBodyRotation(slopes[0], DEG2RAD * 170.0f);

        slopes[1] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { 0.2f * SCREEN_WIDTH, 0.36f * SCREEN_HEIGHT }),
            frCreateRectangle(
                MATERIAL_SLOPE,
                frNumberPixelsToMeters(0.65f * SCREEN_WIDTH),
                frNumberPixelsToMeters(0.05f * SCREEN_HEIGHT)
            )
        );

        frSetBodyRotation(slopes[1], DEG2RAD * 15.0f);

        slopes[2] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { 0.7f * SCREEN_WIDTH, 0.7f * SCREEN_HEIGHT }),
            frCreateRectangle(
                MATERIAL_SLOPE,
                frNumberPixelsToMeters(0.45f * SCREEN_WIDTH),
                frNumberPixelsToMeters(0.05f * SCREEN_HEIGHT)
            )
        );

        frSetBodyRotation(slopes[2], DEG2RAD * 155.0f);

        slopes[3] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { 0.25f * SCREEN_WIDTH, 0.82f * SCREEN_HEIGHT }),
            frCreateRectangle(
                MATERIAL_SLOPE,
                frNumberPixelsToMeters(0.3f * SCREEN_WIDTH),
                frNumberPixelsToMeters(0.05f * SCREEN_HEIGHT)
            )
        );

        frSetBodyRotation(slopes[3], DEG2RAD * 25.0f);

        for (int i = 0; i < MAX_SLOPE_COUNT; i++)
            frAddToWorld(world, slopes[i]);
    }

    ball = LoadRenderTexture(2 * BALL_RADIUS, 2 * BALL_RADIUS);

    {
        BeginTextureMode(ball);

        ClearBackground(BLANK);

        const Color color = ColorAlpha(WHITE, 0.9f);

        DrawRing(
            ORIGIN_POS,
            BALL_RADIUS - 2.0f,
            BALL_RADIUS,
            0.0f,
            360.0f,
            16,
            color
        );

        DrawCircleV(ORIGIN_POS, 2.0f, color);

        EndTextureMode();
    }

    logo = LoadTexture("../res/images/logo_small.png");
}

/* 예제 프로그램을 실행한다. */
static void UpdateExample(void) {
    for (int i = 0; i < MAX_BALL_COUNT - ballCount; i++) {
        Vector2 position = FR_STRUCT_ZERO(Vector2);
        
        position.x = GetRandomValue(0.15f * SCREEN_WIDTH, 0.85f * SCREEN_WIDTH);
        position.y = GetRandomValue(-0.2f * SCREEN_HEIGHT, -0.05f * SCREEN_HEIGHT);
        
        frBody *ball = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters(position),
            frCreateCircle(
                MATERIAL_BALL, 
                frNumberPixelsToMeters(BALL_RADIUS)
            )
        );
        
        frAddToWorld(world, ball);

        ballCount++;
    }

    int bodyCount = frGetWorldBodyCount(world);

    for (int i = MAX_WALL_COUNT + MAX_SLOPE_COUNT; i < bodyCount; i++) {
        frBody *body = frGetWorldBody(world, i);

        const Vector2 bodyPos = frGetBodyPosition(body);
        
        if (bodyPos.y > frNumberPixelsToMeters(SCREEN_HEIGHT)) {
            frRemoveFromWorld(world, body);
            frReleaseBody(body);

            ballCount--;
        }
    }

    frSimulateWorld(world, DELTA_TIME);

    {
        BeginDrawing();
            
        ClearBackground(MATTEBLACK);

        const Vector2 position = {
            0.5f * (SCREEN_WIDTH - logo.width),
            0.5f * (SCREEN_HEIGHT - logo.height)
        };

        DrawTextureV(logo, position, ColorAlpha(WHITE, 0.5f));

        for (int i = MAX_WALL_COUNT; i < MAX_WALL_COUNT + MAX_SLOPE_COUNT; i++) {
            frBody *body = frGetWorldBody(world, i);

            frDrawBodyLines(body, 2.0f, ColorAlpha(WHITE, 0.9f));
        }

        const Rectangle sourceRec = (Rectangle) {
            .width = 2.0f * BALL_RADIUS,
            .height = 2.0f * BALL_RADIUS
        };

        bodyCount = frGetWorldBodyCount(world);

        for (int i = MAX_WALL_COUNT + MAX_SLOPE_COUNT; i < bodyCount; i++) {
            frBody *body = frGetWorldBody(world, i);

            const Vector2 bodyPos = frGetBodyPosition(body);

            DrawTexturePro(
                ball.texture,
                sourceRec,
                (Rectangle) {
                    .x = frNumberMetersToPixels(bodyPos.x), 
                    .y = frNumberMetersToPixels(bodyPos.y),
                    .width = sourceRec.width,
                    .height = sourceRec.height
                },
                ORIGIN_POS,
                RAD2DEG * frGetBodyRotation(body),
                WHITE
            );
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