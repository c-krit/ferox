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

#define WORLD_RECTANGLE ((Rectangle) { \
    .width = SCREEN_WIDTH_IN_METERS,   \
    .height = SCREEN_HEIGHT_IN_METERS  \
})

#define WALL_MATERIAL    ((frMaterial) { 2.5f, 0.0f, 0.65f, 0.85f })
#define CIRCLE_MATERIAL  ((frMaterial) { 8.5f, 0.0f, 0.65f, 0.85f })

#define MAX_CIRCLE_COUNT  192
#define MAX_WALL_COUNT    3

const float DELTA_TIME = (1.0f / TARGET_FPS) * 100.0f;

static frWorld *world;

static frBody *circles[MAX_CIRCLE_COUNT], *walls[MAX_WALL_COUNT];

void InitExample(void);
void DeinitExample(void);
void UpdateExample(void);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | circles.c");
    
    InitExample();

    while (!WindowShouldClose())
        UpdateExample();
    
    DeinitExample();
    
    CloseWindow();

    return 0;
}

void InitExample(void) {
    world = frCreateWorld(
        frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.00001f),
        WORLD_RECTANGLE
    );

    const frVertices wall1_vertices = {
        .data = {
            frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH, -0.45f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH,  0.45f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) {  0.1f * SCREEN_WIDTH,  0.45f * SCREEN_HEIGHT })
        },
        .count = 3
    };

    const frVertices wall3_vertices = {
        .data = {
            frVec2PixelsToMeters((Vector2) {  0.1f * SCREEN_WIDTH, -0.45f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH,  0.45f * SCREEN_HEIGHT }),
            frVec2PixelsToMeters((Vector2) {  0.1f * SCREEN_WIDTH,  0.45f * SCREEN_HEIGHT })
        },
        .count = 3
    };

    walls[0] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.1f * SCREEN_WIDTH, 0.55f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall1_vertices)
    );
    
    walls[1] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, SCREEN_HEIGHT - 40.0f }),
        frCreateRectangle(
            WALL_MATERIAL,
            frNumberPixelsToMeters(SCREEN_WIDTH),
            frNumberPixelsToMeters(80.0f)
        )
    );

    walls[2] = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.9f * SCREEN_WIDTH, 0.55f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall3_vertices)
    );

    for (int i = 0; i < MAX_WALL_COUNT; i++)
        frAddToWorld(world, walls[i]);

    for (int i = 0; i < MAX_CIRCLE_COUNT; i++) {
        Vector2 position = FR_STRUCT_ZERO(Vector2);
        
        position.x = GetRandomValue(0.25f * SCREEN_WIDTH, 0.75f * SCREEN_WIDTH);
        position.y = GetRandomValue(0.18f * SCREEN_HEIGHT, 0.36f * SCREEN_HEIGHT);
        
        frBody *circle = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters(position),
            frCreateCircle(CIRCLE_MATERIAL, 0.45f * GetRandomValue(2, 3))
        );
        
        frAddToWorld(world, circle);
    }
}

void DeinitExample(void) {
    frReleaseWorld(world);
}

void UpdateExample(void) {
    frSimulateWorld(world, DELTA_TIME);
    
    {
        BeginDrawing();
        
        ClearBackground(RAYWHITE);
        
        for (int i = 0; i < MAX_WALL_COUNT; i++)
            frDrawBody(walls[i], BLACK);
        
        for (int i = MAX_WALL_COUNT; i < frGetWorldBodyCount(world); i++)
            frDrawBodyLines(frGetWorldBody(world, i), 2.0f, RED);
        
        frDrawSpatialHash(frGetWorldSpatialHash(world), 0.25f, GRAY);
        
        DrawFPS(8, 8);

        EndDrawing();
    }
}