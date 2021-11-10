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

#define SCREEN_CENTER ((Vector2) { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 })

#define SCREEN_WIDTH_IN_METERS  (frNumberPixelsToMeters(SCREEN_WIDTH))
#define SCREEN_HEIGHT_IN_METERS (frNumberPixelsToMeters(SCREEN_HEIGHT))

#define WORLD_RECTANGLE ((Rectangle) { \
    .width = SCREEN_WIDTH_IN_METERS,   \
    .height = SCREEN_HEIGHT_IN_METERS  \
})

#define SEMO_MATERIAL ((frMaterial) { 2.0f, 0.0f, 1.25f, 1.25f })

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | laser.c");
    
    frWorld *world = frCreateWorld(FR_STRUCT_ZERO(Vector2), WORLD_RECTANGLE);
    
    Vector2 semo_vertices[3] = {
        frVec2PixelsToMeters((Vector2) { 0, -16 }),
        frVec2PixelsToMeters((Vector2) { -14, 16 }),
        frVec2PixelsToMeters((Vector2) { 14, 16 })
    };
    
    frBody *semo = frCreateBodyFromShape(
        FR_BODY_KINEMATIC, 
        frVec2PixelsToMeters(SCREEN_CENTER),
        frCreatePolygon(SEMO_MATERIAL, semo_vertices, 3)
    );
    
    frAddToWorld(world, semo);
    
    for (int i = 0; i < 32; i++) {
        Vector2 position = FR_STRUCT_ZERO(Vector2);
        
        position.x = GetRandomValue(0, 1) 
            ? GetRandomValue(0, 0.45f * SCREEN_WIDTH)
            : GetRandomValue(0.55f * SCREEN_WIDTH, SCREEN_WIDTH);
        
        position.y = GetRandomValue(0, 1)
            ? GetRandomValue(0, 0.45f * SCREEN_HEIGHT)
            : GetRandomValue(0.55f * SCREEN_HEIGHT, SCREEN_HEIGHT);
        
        frBody *ball = frCreateBodyFromShape(
            FR_BODY_STATIC, 
            frVec2PixelsToMeters(position),
            frCreateCircle(FR_DYNAMICS_DEFAULT_MATERIAL, GetRandomValue(1, 3))
        );
        
        frAddToWorld(world, ball);
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        
        ClearBackground(RAYWHITE);
        
        Vector2 mouse_position = GetMousePosition();
        
        frSetBodyRotation(
            semo, 
            frVec2Angle(
                (Vector2) { .y = -1 },
                frVec2Subtract(
                    mouse_position, 
                    frVec2MetersToPixels(frGetBodyPosition(semo))
                )
            )
        );
        
        for (int i = 1; i < frGetWorldBodyCount(world); i++) {
            frBody *body = frGetWorldBody(world, i);
            
            frDrawBodyLines(body, BLACK);
            
            Vector2 position_diff = frVec2Subtract(
                frVec2PixelsToMeters(mouse_position), 
                frVec2PixelsToMeters(SCREEN_CENTER)
            );
            
            frRaycastHit raycast = frComputeRaycast(
                frGetBodyShape(body),
                frGetBodyTransform(body),
                frVec2PixelsToMeters(SCREEN_CENTER),
                position_diff,
                frVec2Magnitude(position_diff)
            );
            
            if (!raycast.check) continue;
            
            DrawCircleLines(
                frNumberMetersToPixels(raycast.point.x),
                frNumberMetersToPixels(raycast.point.y),
                6, 
                RED
            );
        }
        
        DrawLineEx(SCREEN_CENTER, mouse_position, 1, RED);
        
        frDrawBody(semo, DARKGRAY);
        frDrawBodyAABB(semo, GREEN);
        
        frSimulateWorld(world, (1.0f / TARGET_FPS) * 100);
        
        DrawFPS(8, 8);

        EndDrawing();
    }
    
    frReleaseWorldBodies(world);
    frReleaseWorld(world);
    
    CloseWindow();

    return 0;
}