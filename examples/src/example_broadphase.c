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

#define WORLD_RECTANGLE ((Rectangle) { 0, 0, SCREEN_WIDTH_IN_METERS, SCREEN_HEIGHT_IN_METERS })

#define BODY_MATERIAL ((frMaterial) { 0.5f, 0.0f, 0.5f, 0.5f })

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | example_broadphase.c");
    
    frWorld *world = frCreateWorld(
        frVec2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY, 0.0000001f),
        WORLD_RECTANGLE
    );

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(FR_DEBUG_BACKGROUND_COLOR);
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            frBody *body = frCreateBodyFromShape(
                FR_BODY_DYNAMIC, 
                FR_VECTOR_P2M(GetMousePosition()),
                frCreateCircle(
                    BODY_MATERIAL,
                    0.1f * GetRandomValue(6, 12)
                )
            );
            
            frAddToWorld(world, body);
        }
        
        for (int i = 0; i < frGetWorldBodyCount(world); i++) {
            frBody *body = frGetWorldBody(world, i);
            
            if (!CheckCollisionRecs(frGetBodyAABB(body), WORLD_RECTANGLE))
                frRemoveFromWorld(world, body);
        }
        
        for (int i = 0; i < frGetWorldBodyCount(world); i++)
            frDrawBodyLines(frGetWorldBody(world, i), GREEN);
        
        frDrawSpatialHash(frGetWorldSpatialHash(world));
        
        frSimulateWorld(world, (1.0f / 60.0f) * 100);
        
        DrawTextEx(
            GetFontDefault(),
            TextFormat(
                "bodies.count: %d\n",
                frGetWorldBodyCount(world)
            ),
            (Vector2) { 8, 32 },
            10, 
            1, 
            WHITE
        );
        
        DrawFPS(8, 8);

        EndDrawing();
    }
    
    frReleaseWorldBodies(world);
    frReleaseWorld(world);
    
    CloseWindow();

    return 0;
}