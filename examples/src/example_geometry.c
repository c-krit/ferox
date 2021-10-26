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

#define SCREEN_WIDTH_IN_METERS (frNumberPixelsToMeters(SCREEN_WIDTH))
#define SCREEN_HEIGHT_IN_METERS (frNumberPixelsToMeters(SCREEN_HEIGHT))

#define TRIANGLE_COLOR (GetColor(0xC8CAD4FF))

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | example_geometry.c");
    
    Vector2 triangle_vertices[3] = {
        (Vector2) { 0.0, -6.0 },
        (Vector2) { -4.0, 6.0 },
        (Vector2) { 4.0, 6.0 }
    };
    
    frBody *triangle = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        (Vector2) { SCREEN_WIDTH_IN_METERS * 0.5f, SCREEN_HEIGHT_IN_METERS * 0.5f },
        frCreatePolygon((frMaterial) { .density = 1.0f }, triangle_vertices, 3)
    );
    
    frWorld *world = frCreateWorld(
        frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.0f),
        (Rectangle) { 0, 0, SCREEN_WIDTH_IN_METERS, SCREEN_HEIGHT_IN_METERS }
    );
    
    frAddToWorld(world, triangle);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(FR_DEBUG_BACKGROUND_COLOR);
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
            frSetBodyPosition(triangle, frVec2PixelsToMeters(GetMousePosition()));
        
        frSetBodyRotation(
            triangle, 
            frVec2Angle(
                (Vector2) { 0, -1 }, 
                frVec2Subtract(
                    GetMousePosition(), 
                    frVec2MetersToPixels(frGetBodyPosition(triangle))
                )
            )
        );
    
        frDrawBody(triangle, TRIANGLE_COLOR);
        frDrawBodyAABB(triangle, TRIANGLE_COLOR);
        frDrawBodyProperties(triangle, GRAY);
        
        frDrawSpatialHash(frGetWorldSpatialHash(world));
        
        frSimulateWorld(world, (1.0f / 60.0f) * 100);
        
        DrawFPS(8, 8);

        EndDrawing();
    }
    
    frReleaseWorld(world);
    
    frReleaseShape(frGetBodyShape(triangle));
    frReleaseBody(triangle);
    
    CloseWindow();

    return 0;
}