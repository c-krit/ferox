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

/* | `bricks` 모듈 매크로 정의... | */

#define TARGET_FPS      60

#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   600

#define MATTEBLACK      ((Color) { 15, 15, 15, 255 })

#define MAX_WALL_COUNT  4

/* | `bricks` 모듈 변수 및 상수... | */

const frMaterial MATERIAL_BRICK = { 
    .density         = 1.0f, 
    .staticFriction  = 0.5f, 
    .dynamicFriction = 0.35f
};

const frMaterial MATERIAL_WALL = { 
    .density         = 1.25f, 
    .staticFriction  = 0.75f, 
    .dynamicFriction = 0.5f
};

const float BRICK_WIDTH = 64.0f, BRICK_HEIGHT = 48.0f;
const float DELTA_TIME = (1.0f / TARGET_FPS) * 200.0f;

const Rectangle SOURCE_REC = { .width = BRICK_WIDTH, .height = BRICK_HEIGHT };
const Vector2 ORIGIN_POS = { 0.5f * BRICK_WIDTH, 0.5f * BRICK_HEIGHT };

static frWorld *world;

static frBody *walls[MAX_WALL_COUNT];
static frBody *cursor;

static RenderTexture brick;
static Texture2D logo;

/* | `bricks` 모듈 함수... | */

/* 예제 프로그램을 초기화한다. */
static void InitExample(void);

/* 예제 프로그램을 실행한다. */
static void UpdateExample(void);

/* 예제 프로그램에 할당된 메모리를 해제한다. */
static void DeinitExample(void);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | bricks.c");

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
        .y = -0.05f * frNumberPixelsToMeters(SCREEN_HEIGHT),
        .width = 1.1f * frNumberPixelsToMeters(SCREEN_WIDTH),
        .height = 1.1f * frNumberPixelsToMeters(SCREEN_HEIGHT)
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

        // 아래쪽 벽을 생성한다.
        walls[1] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, 1.05f * SCREEN_HEIGHT }),
            frCreateRectangle(
                MATERIAL_WALL,
                frNumberPixelsToMeters(SCREEN_WIDTH),
                frNumberPixelsToMeters(0.1f * SCREEN_HEIGHT)
            )
        );

        // 오른쪽 벽을 생성한다.
        walls[2] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { 1.05f * SCREEN_WIDTH, 0.5f * SCREEN_HEIGHT }),
            frCreateRectangle(
                MATERIAL_WALL,
                frNumberPixelsToMeters(0.1f * SCREEN_WIDTH),
                frNumberPixelsToMeters(SCREEN_HEIGHT)
            )
        );

        // 위쪽 벽을 생성한다.
        walls[3] = frCreateBodyFromShape(
            FR_BODY_STATIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, -0.05f * SCREEN_HEIGHT }),
            frCreateRectangle(
                MATERIAL_WALL,
                frNumberPixelsToMeters(SCREEN_WIDTH),
                frNumberPixelsToMeters(0.1f * SCREEN_HEIGHT)
            )
        );

        for (int i = 0; i < MAX_WALL_COUNT; i++)
            frAddToWorld(world, walls[i]);

        cursor = frCreateBodyFromShape(
            FR_BODY_KINEMATIC,
            FR_FLAG_NONE,
            FR_STRUCT_ZERO(Vector2),
            frCreateRectangle(
                MATERIAL_BRICK, 
                frNumberPixelsToMeters(BRICK_WIDTH), 
                frNumberPixelsToMeters(BRICK_HEIGHT)
            )
        );
        
        frAddToWorld(world, cursor);
    }

    brick = LoadRenderTexture(BRICK_WIDTH, BRICK_HEIGHT);

    {
        BeginTextureMode(brick);

        ClearBackground(BLANK);

        const Color color = ColorAlpha(WHITE, 0.9f);

        DrawRectangleLinesEx(SOURCE_REC, 2.0f, color);
        DrawCircleV(ORIGIN_POS, 2.0f, color);

        EndTextureMode();
    }

    SetTextureFilter(brick.texture, TEXTURE_FILTER_BILINEAR);

    logo = LoadTexture("../res/images/logo_small.png");
}

/* 예제 프로그램을 실행한다. */
static void UpdateExample(void) {
    frSetBodyPosition(cursor, frVec2PixelsToMeters(GetMousePosition()));

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        frBody *brick = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { GetMouseX(), GetMouseY() + 8.0f }),
            frCreateRectangle(
                MATERIAL_BRICK,
                frNumberPixelsToMeters(BRICK_WIDTH), 
                frNumberPixelsToMeters(BRICK_HEIGHT)
            )
        );

        frSetBodyRotation(brick, frGetBodyRotation(cursor));
        frSetBodyRotation(cursor, DEG2RAD * GetRandomValue(0, 360));
        
        frAddToWorld(world, brick);
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

        const int bodyCount = frGetWorldBodyCount(world);

        for (int i = MAX_WALL_COUNT; i < bodyCount; i++) {
            frBody *body = frGetWorldBody(world, i);

            const Vector2 bodyPos = frGetBodyPosition(body);

            const Rectangle destRec = {
                .x = frNumberMetersToPixels(bodyPos.x), 
                .y = frNumberMetersToPixels(bodyPos.y),
                .width = BRICK_WIDTH,
                .height = BRICK_HEIGHT
            };

            DrawTexturePro(
                brick.texture,
                SOURCE_REC,
                destRec,
                ORIGIN_POS,
                RAD2DEG * frGetBodyRotation(body),
                ColorAlpha(WHITE, (body == cursor) ? 0.5f : 1.0f)
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
    UnloadRenderTexture(brick);
    UnloadTexture(logo);
    
    // 게임 세계에 할당된 메모리를 해제한다.
    frReleaseWorld(world);
}