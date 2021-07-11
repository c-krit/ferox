/*
    Copyright (c) 2021 jdeokkim

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

#define SCREEN_WIDTH_IN_METERS (SCREEN_WIDTH / FR_GLOBAL_PIXELS_PER_METER)
#define SCREEN_HEIGHT_IN_METERS (SCREEN_HEIGHT / FR_GLOBAL_PIXELS_PER_METER)

#define BLOCK1_COLOR (GetColor(0xC8CAD4FF))
#define BLOCK2_COLOR (GetColor(0xEEBF3CFF))
#define BLOCK3_COLOR (GetColor(0x18402CFF))

#define PLATFORM_COLOR (GetColor(0x2B2B2BFF))

#define BALL_COLOR (GetColor(0x1C1C1CFF))

#define BLOCK1_MATERIAL ((frMaterial) { 0.45f, 0.0f, 0.5f, 0.35f })
#define BLOCK2_MATERIAL ((frMaterial) { 0.65f, 0.0f, 0.5f, 0.35f })
#define BLOCK3_MATERIAL ((frMaterial) { 0.85f, 0.0f, 0.5f, 0.35f })

#define PLATFORM_MATERIAL ((frMaterial) { 1.0f, 0.0f, 0.35f, 0.2f })

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | example_dynamics.c");
    
    Vector2 block1_vertices[4] = {
        (Vector2) { -2.0, -1.5 },
        (Vector2) { -2.0, 1.5 },
        (Vector2) { 2.0, 1.5 },
        (Vector2) { 2.0, -1.5 }
    };
    
    Vector2 block2_vertices[4] = {
        (Vector2) { -4.0, -3.0 },
        (Vector2) { -4.0, 3.0 },
        (Vector2) { 4.0, 3.0 },
        (Vector2) { 4.0, -3.0 }
    };
    
    Vector2 block3_vertices[4] = {
        (Vector2) { -6.0, -4.0 },
        (Vector2) { -6.0, 4.0 },
        (Vector2) { 6.0, 4.0 },
        (Vector2) { 6.0, -4.0 }
    };
    
    Vector2 platform_vertices[4] = {
        (Vector2) { -12.0, -2.0 },
        (Vector2) { -12.0, 2.0 },
        (Vector2) { 12.0, 2.0 },
        (Vector2) { 12.0, -2.0 }
    };
    
    frShape *block1_polygon = frCreatePolygon(BLOCK1_MATERIAL, block1_vertices, 4);
    frShape *block2_polygon = frCreatePolygon(BLOCK2_MATERIAL, block2_vertices, 4);
    frShape *block3_polygon = frCreatePolygon(BLOCK3_MATERIAL, block3_vertices, 4);
    
    frShape *platform_polygon = frCreatePolygon(PLATFORM_MATERIAL, platform_vertices, 4);
    
    frBody *block1 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC, 
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.55f, SCREEN_HEIGHT_IN_METERS * 0.2f }, 
        block1_polygon
    );
    
    frBody *block2 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC, 
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.55f, SCREEN_HEIGHT_IN_METERS * 0.26f }, 
        block2_polygon
    );
    
    frBody *block3 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC, 
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.55f, SCREEN_HEIGHT_IN_METERS * 0.32f }, 
        block3_polygon
    );
    
    frBody *platform = frCreateBodyFromShape(
        FR_BODY_STATIC, 
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.5f, SCREEN_HEIGHT_IN_METERS * 0.85f },
        platform_polygon
    );
    
    frBody *ball = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.35f, (SCREEN_HEIGHT_IN_METERS * 0.85f) - 3.0f },
        frCreateCircle(FR_STRUCT_ZERO(frMaterial), 1.0f)
    );
    
    frWorld *world = frCreateWorld(
        frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.00001f),
        (Rectangle) { 0, 0, SCREEN_WIDTH_IN_METERS, SCREEN_HEIGHT_IN_METERS }
    );
    
    frAddToWorld(world, block1);
    frAddToWorld(world, block2);
    frAddToWorld(world, block3);
    
    frAddToWorld(world, platform);
    
    frAddToWorld(world, ball);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(FR_DEBUG_BACKGROUND_COLOR);
        
        Vector2 ball_position = frGetBodyPosition(ball);
        
        if (IsKeyDown(KEY_UP)) frSetBodyPosition(ball, frVec2Add(ball_position, (Vector2) { 0.0f, -0.15f }));
        if (IsKeyDown(KEY_DOWN)) frSetBodyPosition(ball, frVec2Add(ball_position, (Vector2) { 0.0f, 0.15f }));
        if (IsKeyDown(KEY_LEFT)) frSetBodyPosition(ball, frVec2Add(ball_position, (Vector2) { -0.15f, 0.0f }));
        if (IsKeyDown(KEY_RIGHT)) frSetBodyPosition(ball, frVec2Add(ball_position, (Vector2) { 0.15f, 0.0f }));
        
        frDrawBody(platform, PLATFORM_COLOR);
        frDrawBodyAABB(platform, GREEN);
        
        frDrawBody(block1, BLOCK1_COLOR);
        frDrawBodyAABB(block1, GREEN);
        
        frDrawBody(block2, BLOCK2_COLOR);
        frDrawBodyAABB(block2, GREEN);
        
        frDrawBody(block3, BLOCK3_COLOR);
        frDrawBodyAABB(block3, GREEN);
        frDrawBodyProperties(block3, WHITE);
        
        frDrawBody(ball, BALL_COLOR);
        frDrawBodyAABB(ball, GREEN);
        frDrawBodyProperties(ball, WHITE);
        
        frDrawQuadtree(frGetWorldQuadtree(world));
        
        frSimulateWorld(world, 1.0f / 60.0f);
        
        DrawFPS(8, 8);

        EndDrawing();
    }
    
    frReleaseWorldBodies(world);
    frReleaseWorld(world);
    
    CloseWindow();

    return 0;
}