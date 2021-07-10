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
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | example_collision.c");
    
    Color ball_color = (Color) { 200, 202, 212, 255 };
    Color platform_color = (Color) { 32, 32, 32, 255 };
    
    Vector2 platform_vertices[3] = {
        (Vector2) { -36.0, -8.0 },
        (Vector2) { -36.0, 8.0 },
        (Vector2) { 36.0, 8.0 },
    };
    
    frShape *ball_circle = frCreateCircle((frMaterial) { .density = 0.5f }, 8);
    frShape *platform_polygon = frCreatePolygon((frMaterial) { .density = 1.0f }, platform_vertices, 3);
                                          
    frBody *ball = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.5f, SCREEN_HEIGHT_IN_METERS * 0.5f },
        ball_circle
    );
    
    frBody *platform = frCreateBodyFromShape(
        FR_BODY_STATIC, 
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.38f, SCREEN_HEIGHT_IN_METERS * 0.75f },
        platform_polygon
    );
    
    frSetBodyRotation(platform, PI / 16.0f);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(RAYWHITE);
        
        frSetBodyPosition(ball, FR_VECTOR_P2M(GetMousePosition()));
        
        frDrawBody(platform, platform_color);
        frDrawBody(ball, ball_color);
        
        frTransform ball_tx = frGetBodyTransform(ball);
        frTransform platform_tx = frGetBodyTransform(platform);
        
        frCollision collision = frComputeCollision(ball_circle, ball_tx, platform_polygon, platform_tx);
        
        if (collision.check) {
            for (int i = 0; i < collision.count; i++)
                DrawCircleV(FR_VECTOR_M2P(collision.points[i]), 4, RED);
        }
        
        DrawTextEx(
            GetFontDefault(),
            FormatText(
                "collision.check: %s\n"
                "collision.direction: (%f, %f)\n"
                "collision.points: (%f, %f), (%f, %f)\n"
                "collision.depths: (%f, %f)\n"
                "collision.count: %d\n",
                (collision.check) ? "true" : "false",
                collision.direction.x, collision.direction.y,
                collision.points[0].x, collision.points[0].y,
                collision.points[1].x, collision.points[1].y,
                collision.depths[0], collision.depths[1],
                collision.count
            ), 
            (Vector2) { 8, 32 },
            10, 
            1, 
            BLACK
        );
        
        DrawFPS(8, 8);

        EndDrawing();
    }
    
    CloseWindow();

    return 0;
}