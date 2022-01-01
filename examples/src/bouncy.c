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

#define FLOOR_MATERIAL  ((frMaterial) { 1.25f, 1.25f, 0.25f, 0.25f })

#define BALL1_MATERIAL ((frMaterial) { 0.3f, 0.85f, 0.65f, 0.5f })
#define BALL2_MATERIAL ((frMaterial) { 0.2f, 0.65f, 0.65f, 0.5f })
#define BALL3_MATERIAL ((frMaterial) { 0.1f, 0.5f, 0.65f, 0.5f })

#define CURSOR_MATERIAL ((frMaterial) { 2.5f, 0.5f, 0.75f, 0.75f })

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | bouncy.c");
    
    frWorld *world = frCreateWorld(
        frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.00001f),
        WORLD_RECTANGLE
    );
    
    Vector2 floor_vertices[4] = {
        frVec2PixelsToMeters((Vector2) { -0.35f * SCREEN_WIDTH, -25 }),
        frVec2PixelsToMeters((Vector2) { -0.35f * SCREEN_WIDTH, 25 }),
        frVec2PixelsToMeters((Vector2) { 0.35f * SCREEN_WIDTH, 25 }),
        frVec2PixelsToMeters((Vector2) { 0.35f * SCREEN_WIDTH, -25 })
    };
    
    frBody *floor = frCreateBodyFromShape(
        FR_BODY_STATIC, 
        frVec2PixelsToMeters((Vector2) { SCREEN_WIDTH / 2, SCREEN_HEIGHT - 80 }),
        frCreatePolygon(FLOOR_MATERIAL, floor_vertices, 4)
    );
    
    frAddToWorld(world, floor);
    
    frBody *ball1 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC, 
        frVec2PixelsToMeters((Vector2) { 0.25f * SCREEN_WIDTH, SCREEN_HEIGHT / 4 }),
        frCreateCircle(BALL1_MATERIAL, 1.5f)
    );
    
    frBody *ball2 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC, 
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, SCREEN_HEIGHT / 4 }),
        frCreateCircle(BALL2_MATERIAL, 2.0f)
    );
    
    frBody *ball3 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC, 
        frVec2PixelsToMeters((Vector2) { 0.75f * SCREEN_WIDTH, SCREEN_HEIGHT / 4 }),
        frCreateCircle(BALL3_MATERIAL, 2.75f)
    );
    
    frAddToWorld(world, ball1);
    frAddToWorld(world, ball2);
    frAddToWorld(world, ball3);
    
    frBody *cursor = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        FR_STRUCT_ZERO(Vector2),
        frCreateCircle(CURSOR_MATERIAL, 0.5f)
    );
    
    frAddToWorld(world, cursor);

    while (!WindowShouldClose()) {  
        BeginDrawing();
        
        ClearBackground(RAYWHITE);
        
        frSetBodyPosition(cursor, frVec2PixelsToMeters(GetMousePosition()));
        
        frDrawBody(floor, BLACK);
        
        frDrawBody(ball1, RED);
        frDrawBody(ball2, RED);
        frDrawBody(ball3, RED);
        
        frDrawBody(cursor, Fade(RED, 0.5f));
        
        frDrawSpatialHash(frGetWorldSpatialHash(world));
        
        frSimulateWorld(world, (1.0f / TARGET_FPS) * 100);
        
        DrawFPS(8, 8);

        EndDrawing();
    }
    
    frReleaseWorld(world);
    
    CloseWindow();

    return 0;
}