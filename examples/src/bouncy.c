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

#define TARGET_FPS 60

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

#define SCREEN_WIDTH_IN_METERS  (frNumberPixelsToMeters(SCREEN_WIDTH))
#define SCREEN_HEIGHT_IN_METERS (frNumberPixelsToMeters(SCREEN_HEIGHT))

#define WORLD_RECTANGLE ((Rectangle) {      \
    .x = -0.05f * SCREEN_WIDTH_IN_METERS,   \
    .width = 1.1f * SCREEN_WIDTH_IN_METERS, \
    .height = SCREEN_HEIGHT_IN_METERS       \
})

#define CIRCLE_MATERIAL ((frMaterial) { 0.05f, 0.85f, 0.5f, 0.35f })
#define WALL_MATERIAL   ((frMaterial) { 2.25f, 0.85f, 0.75f, 0.5f })

#define MAX_CIRCLE_COUNT  216
#define MAX_WALL_COUNT    3

#define CIRCLE_RADIUS     9.6f

static void InitExample(void);
static void DeinitExample(void);
static void UpdateExample(void);

const float DELTA_TIME = (1.0f / TARGET_FPS) * 100.0f;

static RenderTexture rtx;

static frWorld *world;

static frBody *circles[MAX_CIRCLE_COUNT];
static frBody *walls[MAX_WALL_COUNT];

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | bouncy.c");
    
    InitExample();

    while (!WindowShouldClose())
        UpdateExample();

    DeinitExample();
    
    CloseWindow();

    return 0;
}

void InitExample(void) {
    rtx = LoadRenderTexture(2 * CIRCLE_RADIUS, 2 * CIRCLE_RADIUS);

    {
        BeginTextureMode(rtx);

        ClearBackground(BLANK);

        DrawRing(
            (Vector2) { CIRCLE_RADIUS, CIRCLE_RADIUS },
            CIRCLE_RADIUS - 2.0f,
            CIRCLE_RADIUS,
            0.0f,
            360.0f,
            16,
            RED
        );

        EndTextureMode();
    }

    SetTextureFilter(rtx.texture, TEXTURE_FILTER_BILINEAR);

    world = frCreateWorld(
        frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.00001f),
        WORLD_RECTANGLE
    );

    const frVertices wall_vertices = {
        .data = {
            frVec2PixelsToMeters((Vector2) { -0.2f * SCREEN_WIDTH, -0.18f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) { -0.2f * SCREEN_WIDTH,  0.18f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) {  0.2f * SCREEN_WIDTH,  0.18f * SCREEN_HEIGHT })
        },
        .count = 3
    };

    walls[0] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, 0.93f * SCREEN_HEIGHT }),
        frCreateRectangle(
            WALL_MATERIAL, 
            frNumberPixelsToMeters(SCREEN_WIDTH), 
            frNumberPixelsToMeters(0.15f * SCREEN_HEIGHT)
        )
    );

    walls[1] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.2f * SCREEN_WIDTH, 0.68f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall_vertices)
    );

    walls[2] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 1.05f * SCREEN_WIDTH, 0.5f * SCREEN_HEIGHT }),
        frCreateRectangle(
            WALL_MATERIAL, 
            frNumberPixelsToMeters(0.1f * SCREEN_WIDTH), 
            frNumberPixelsToMeters(SCREEN_HEIGHT)
        )
    );
    
    for (int i = 0; i < MAX_WALL_COUNT; i++)
        frAddToWorld(world, walls[i]);

    for (int i = 0; i < MAX_CIRCLE_COUNT; i++) {
        Vector2 position = {
            GetRandomValue(0.25f * SCREEN_WIDTH, 0.75f * SCREEN_WIDTH),
            GetRandomValue(0.1f * SCREEN_HEIGHT, 0.35f * SCREEN_HEIGHT)
        };

        circles[i] = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters(position),
            frCreateCircle(
                CIRCLE_MATERIAL, 
                frNumberPixelsToMeters(CIRCLE_RADIUS)
            )
        );

        frAddToWorld(world, circles[i]);
    }
}

void DeinitExample(void) {
    frReleaseWorld(world);

    UnloadRenderTexture(rtx);
}

void UpdateExample(void) {
    for (int i = 0; i < frGetWorldBodyCount(world); i++) {
        frBody *body = frGetWorldBody(world, i);
        
        if (frGetBodyType(body) != FR_BODY_STATIC 
            && !frIsInWorldBounds(world, body)) 
            frRemoveFromWorld(world, body);
    }

    frSimulateWorld(world, DELTA_TIME);

    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        for (int i = 0; i < MAX_WALL_COUNT; i++)
            frDrawBody(walls[i], BLACK);

        const Rectangle source = (Rectangle) {
            .width = 2.0f * CIRCLE_RADIUS,
            .height = 2.0f * CIRCLE_RADIUS
        };

        for (int i = 0; i < MAX_CIRCLE_COUNT; i++) {
            Vector2 position = frGetBodyPosition(circles[i]);

            DrawTexturePro(
                rtx.texture,
                source,
                (Rectangle) {
                    .x = frNumberMetersToPixels(position.x), 
                    .y = frNumberMetersToPixels(position.y),
                    .width = source.width,
                    .height = source.height
                },
                (Vector2) { CIRCLE_RADIUS, CIRCLE_RADIUS },
                RAD2DEG * frGetBodyRotation(circles[i]),
                WHITE
            );
        }

        frDrawSpatialHash(frGetWorldSpatialHash(world), 0.25f, GRAY);

        DrawFPS(8, 8);

        EndDrawing();
    }
}