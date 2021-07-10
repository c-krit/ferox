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

void DrawDebugInfo(frBody *b, Vector2 p);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | example_dynamics.c");
    
    Color block1_color = (Color) { 200, 202, 212, 255 };
    Color block2_color = (Color) { 238, 190, 60, 255 };
    Color block3_color = (Color) { 24, 66, 44, 255 };
    
    Color platform_color = (Color) { 32, 32, 32, 255 };
    
    Color ball_color = (Color) { 150, 234, 90, 255 };
    
    Vector2 block1_vertices[4] = {
        (Vector2) { -4.0, -3.0 },
        (Vector2) { -4.0, 3.0 },
        (Vector2) { 4.0, 3.0 },
        (Vector2) { 4.0, -3.0 }
    };
    
    Vector2 block2_vertices[4] = {
        (Vector2) { -8.0, -6.0 },
        (Vector2) { -8.0, 6.0 },
        (Vector2) { 8.0, 6.0 },
        (Vector2) { 8.0, -6.0 }
    };
    
    Vector2 block3_vertices[4] = {
        (Vector2) { -12.0, -8.0 },
        (Vector2) { -12.0, 8.0 },
        (Vector2) { 12.0, 8.0 },
        (Vector2) { 12.0, -8.0 }
    };
    
    Vector2 platform_vertices[4] = {
        (Vector2) { -24.0, -4.0 },
        (Vector2) { -24.0, 4.0 },
        (Vector2) { 24.0, 4.0 },
        (Vector2) { 24.0, -4.0 }
    };
    
    frShape *block1_polygon = frCreatePolygon((frMaterial) { .density = 0.4f }, block1_vertices, 4);
    frShape *block2_polygon = frCreatePolygon((frMaterial) { .density = 0.8f }, block2_vertices, 4);
    frShape *block3_polygon = frCreatePolygon((frMaterial) { .density = 1.2f }, block3_vertices, 4);
    
    frShape *platform_polygon = frCreatePolygon((frMaterial) { .density = 1.0f }, platform_vertices, 4);
    
    frBody *ball = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.35f, (SCREEN_HEIGHT_IN_METERS * 0.85f) - 7.0f },
        frCreateCircle((frMaterial) { .density = 1.0f }, 3.0f)
    );
    
    frBody *block1 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC, 
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.55f, SCREEN_HEIGHT_IN_METERS * 0.2f }, 
        block1_polygon
    );
    
    frBody *block2 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC, 
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.55f, SCREEN_HEIGHT_IN_METERS * 0.32f }, 
        block2_polygon
    );
    
    frBody *block3 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC, 
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.55f, SCREEN_HEIGHT_IN_METERS * 0.38f }, 
        block3_polygon
    );
    
    frBody *platform = frCreateBodyFromShape(
        FR_BODY_STATIC, 
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.5f, SCREEN_HEIGHT_IN_METERS * 0.85f },
        platform_polygon
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

        ClearBackground(RAYWHITE);
        
        Vector2 ball_position = frGetBodyPosition(ball);
        
        if (IsKeyDown(KEY_UP)) frSetBodyPosition(ball, frVec2Add(ball_position, (Vector2) { 0.0f, -0.2f }));
        if (IsKeyDown(KEY_DOWN)) frSetBodyPosition(ball, frVec2Add(ball_position, (Vector2) { 0.0f, 0.2f }));
        if (IsKeyDown(KEY_LEFT)) frSetBodyPosition(ball, frVec2Add(ball_position, (Vector2) { -0.2f, 0.0f }));
        if (IsKeyDown(KEY_RIGHT)) frSetBodyPosition(ball, frVec2Add(ball_position, (Vector2) { 0.2f, 0.0f }));
        
        frDrawBody(platform, platform_color);
        
        frDrawBody(block1, block1_color);
        frDrawBody(block2, block2_color);
        frDrawBody(block3, block3_color);
        frDrawBodyProperties(block3, GRAY);
        
        frDrawBody(ball, ball_color);
        frDrawBodyProperties(ball, GRAY);
        
        frDrawQuadtree(frGetWorldQuadtree(world));
        
        frSimulateWorld(world, 1.0f / 60.0f);
        
        DrawFPS(8, 8);

        EndDrawing();
    }
    
    CloseWindow();

    return 0;
}