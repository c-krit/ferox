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

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define SCREEN_WIDTH_IN_METERS  (frNumberPixelsToMeters(SCREEN_WIDTH))
#define SCREEN_HEIGHT_IN_METERS (frNumberPixelsToMeters(SCREEN_HEIGHT))

#define WORLD_RECTANGLE ((Rectangle) { \
    .width = SCREEN_WIDTH_IN_METERS,   \
    .height = SCREEN_HEIGHT_IN_METERS  \
})

#define WALL_MATERIAL  ((frMaterial) { 1.25f, 0.0f, 0.85f, 0.5f })
#define CIRCLE_MATERIAL  ((frMaterial) { 2.0f, 0.0f, 0.85f, 0.75f })

#define MAX_CIRCLE_COUNT 100

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | circles.c");
    
    frWorld *world = frCreateWorld(
        frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.00001f),
        WORLD_RECTANGLE
    );

    Vector2 wall1_vertices[3] = {
        frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH, -0.45f * SCREEN_HEIGHT }),
        frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH, 0.45f * SCREEN_HEIGHT }),
        frVec2PixelsToMeters((Vector2) { 0.1f * SCREEN_WIDTH, 0.45f * SCREEN_HEIGHT }),
    };
    
    Vector2 wall2_vertices[4] = {
        frVec2PixelsToMeters((Vector2) { -0.5f * SCREEN_WIDTH, -40 }),
        frVec2PixelsToMeters((Vector2) { -0.5f * SCREEN_WIDTH, 40 }),
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, 40 }),
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, -40 })
    };

    Vector2 wall3_vertices[3] = {
        frVec2PixelsToMeters((Vector2) { 0.1f * SCREEN_WIDTH, -0.45f * SCREEN_HEIGHT }),
        frVec2PixelsToMeters((Vector2) { -0.1f * SCREEN_WIDTH, 0.45f * SCREEN_HEIGHT }),
        frVec2PixelsToMeters((Vector2) { 0.1f * SCREEN_WIDTH, 0.45f * SCREEN_HEIGHT }),
    };

    frBody *wall1 = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.1f * SCREEN_WIDTH, 0.55f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall1_vertices, 3)
    );
    
    frBody *wall2 = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { SCREEN_WIDTH / 2, SCREEN_HEIGHT - 40 }),
        frCreatePolygon(WALL_MATERIAL, wall2_vertices, 4)
    );

    frBody *wall3 = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.9f * SCREEN_WIDTH, 0.55f * SCREEN_HEIGHT }),
        frCreatePolygon(WALL_MATERIAL, wall3_vertices, 3)
    );
    
    frAddToWorld(world, wall1);
    frAddToWorld(world, wall2);
    frAddToWorld(world, wall3);

    for (int i = 0; i < MAX_CIRCLE_COUNT; i++) {
        Vector2 position = FR_STRUCT_ZERO(Vector2);
        
        position.x = GetRandomValue(5, 13) * 0.05f * SCREEN_WIDTH;
        position.y = GetRandomValue(0.25f * SCREEN_HEIGHT, 0.35f * SCREEN_HEIGHT);
        
        frBody *circle = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters(position),
            frCreateCircle(CIRCLE_MATERIAL, GetRandomValue(1, 2))
        );
        
        frAddToWorld(world, circle);
    }

    while (!WindowShouldClose()) {       
        BeginDrawing();
        
        ClearBackground(RAYWHITE);
        
        frDrawBody(wall1, BLACK);
        frDrawBody(wall2, BLACK);
        frDrawBody(wall3, BLACK);
        
        for (int i = 3; i < frGetWorldBodyCount(world); i++)
            frDrawBodyLines(frGetWorldBody(world, i), 2, RED);
        
        frDrawSpatialHash(frGetWorldSpatialHash(world));
        
        frSimulateWorld(world, (1.0f / TARGET_FPS) * 100);
        
        DrawFPS(8, 8);

        EndDrawing();
    }
    
    frReleaseWorld(world);
    
    CloseWindow();

    return 0;
}