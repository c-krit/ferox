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

/* | `smash` 모듈 매크로 정의... | */

#define TARGET_FPS       60

#define SCREEN_WIDTH     800
#define SCREEN_HEIGHT    600

#define MATTEBLACK       ((Color) { 15, 15, 15, 255 })

#define WIDTH_IN_BOXES   12
#define HEIGHT_IN_BOXES  12

/* | `smash` 모듈 자료형 정의... | */

typedef struct Box {
    frBody *body;
    RenderTexture rtx;
} Box;

/* | `smash` 모듈 변수 및 상수... | */

const frMaterial MATERIAL_BALL = { 
    .density         = 0.5f, 
    .staticFriction  = 0.35f, 
    .dynamicFriction = 0.15f
};

const frMaterial MATERIAL_BOX = { 
    .density         = 0.75f, 
    .staticFriction  = 0.5f, 
    .dynamicFriction = 0.15f
};

const float BALL_RADIUS = 1.5f;
const float DELTA_TIME = (1.0f / TARGET_FPS) * 100.0f;

static frWorld *world;
static frBody *ball;

static Texture2D logo, rlLogo;

static Box boxes[WIDTH_IN_BOXES * HEIGHT_IN_BOXES];

static float boxWidth, boxHeight;
static float halfBoxWidth, halfBoxHeight;

/* | `smash` 모듈 함수... | */

/* 예제 프로그램을 초기화한다. */
static void InitExample(void);

/* 예제 프로그램을 실행한다. */
static void UpdateExample(void);

/* 예제 프로그램에 할당된 메모리를 해제한다. */
static void DeinitExample(void);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | smash.c");

    InitExample();

#if defined(PLATFORM_WEB)
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
        .y = -0.05f * frNumberPixelsToMeters(SCREEN_HEIGHT),
        .width = 1.1f * frNumberPixelsToMeters(SCREEN_WIDTH),
        .height = 1.1f * frNumberPixelsToMeters(SCREEN_HEIGHT)
    };

    // 게임 세계를 생성한다.
    world = frCreateWorld(FR_STRUCT_ZERO(Vector2), bounds);

    logo = LoadTexture("../res/images/logo_small.png");
    rlLogo = LoadTexture("../res/images/raylib.png");

    {
        boxWidth = rlLogo.width / WIDTH_IN_BOXES;
        boxHeight = rlLogo.height / HEIGHT_IN_BOXES;

        halfBoxWidth = 0.5f * boxWidth;
        halfBoxHeight = 0.5f * boxHeight;

        for (int i = 0; i < WIDTH_IN_BOXES * HEIGHT_IN_BOXES; i++) {
            const Vector2 offset = {
                (i % WIDTH_IN_BOXES) * boxWidth,
                (i / WIDTH_IN_BOXES) * boxHeight
            };

            const Vector2 position = {
                0.5f * (SCREEN_WIDTH - rlLogo.width) + offset.x + halfBoxWidth,
                0.5f * (SCREEN_HEIGHT - rlLogo.height) + offset.y + halfBoxHeight
            };

            boxes[i].body = frCreateBodyFromShape(
                FR_BODY_DYNAMIC,
                FR_FLAG_NONE,
                frVec2PixelsToMeters(position),
                frCreateRectangle(
                    MATERIAL_BOX,
                    frNumberPixelsToMeters(boxWidth),
                    frNumberPixelsToMeters(boxHeight)
                )
            );

            frAddToWorld(world, boxes[i].body);

            boxes[i].rtx = LoadRenderTexture(boxWidth, boxHeight);

            {
                BeginTextureMode(boxes[i].rtx);

                ClearBackground(BLANK);

                DrawTextureRec(
                    rlLogo,
                    (Rectangle) {
                        .x = offset.x,
                        .y = offset.y,
                        .width = boxWidth, 
                        .height = -boxHeight
                    },
                    FR_STRUCT_ZERO(Vector2),
                    WHITE
                );

                EndTextureMode();
            }
        }
    }

    ball = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        FR_FLAG_INFINITE_INERTIA,
        (Vector2) { -BALL_RADIUS, -BALL_RADIUS },
        frCreateCircle(MATERIAL_BALL, BALL_RADIUS)
    );

    frAddToWorld(world, ball);
}

/* 예제 프로그램을 실행한다. */
static void UpdateExample(void) {
    const Vector2 screenCenter = { 
        0.5f * SCREEN_WIDTH, 
        0.5f * SCREEN_HEIGHT 
    };

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        const Vector2 position = frVec2PixelsToMeters(GetMousePosition());

        Vector2 impulse = frVec2ScalarMultiply(
            frVec2Subtract(frVec2PixelsToMeters(screenCenter), position),
            0.05f
        );

        frSetBodyVelocity(ball, FR_STRUCT_ZERO(Vector2));
        frSetBodyPosition(ball, position);
        
        frApplyImpulse(ball, impulse);
    }

    for (int i = 0; i < WIDTH_IN_BOXES * HEIGHT_IN_BOXES; i++) {
        if (!frIsInWorldBounds(world, boxes[i].body)) {
            frRemoveFromWorld(world, boxes[i].body);
            frReleaseBody(boxes[i].body);

            boxes[i].body = NULL;
        }
    }

    frSimulateWorld(world, DELTA_TIME);

    {
        BeginDrawing();
            
        ClearBackground(MATTEBLACK);

        for (int i = 0; i < WIDTH_IN_BOXES * HEIGHT_IN_BOXES; i++) {
            if (boxes[i].body == NULL) continue;

            Vector2 bodyPos = frGetBodyPosition(boxes[i].body);

            const Rectangle sourceRec = { 
                .width = boxWidth, 
                .height = boxHeight 
            };

            const Rectangle destRec = {
                .x = frNumberMetersToPixels(bodyPos.x), 
                .y = frNumberMetersToPixels(bodyPos.y),
                .width = boxWidth,
                .height = boxHeight
            };

            const Vector2 originPos = { halfBoxWidth, halfBoxHeight };

            DrawTexturePro(
                boxes[i].rtx.texture,
                sourceRec,
                destRec,
                originPos,
                RAD2DEG * frGetBodyRotation(boxes[i].body),
                WHITE
            );
        }

        const Color color = ColorAlpha(GRAY, 0.85f);

        frDrawBodyLines(ball, 2.0f, color);

        frDrawArrow(
            frVec2PixelsToMeters(GetMousePosition()),
            frVec2PixelsToMeters(screenCenter),
            2.0f,
            color
        );

        DrawRing(
            GetMousePosition(),
            frNumberMetersToPixels(BALL_RADIUS) - 2.0f,
            frNumberMetersToPixels(BALL_RADIUS),
            0.0f,
            360.0f,
            32,
            color
        );

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
    UnloadTexture(rlLogo);
    
    // 게임 세계에 할당된 메모리를 해제한다.
    frReleaseWorld(world);
}