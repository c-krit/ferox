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

#define BRICK_MATERIAL  ((frMaterial) { 0.75f, 0.0f, 0.5f, 0.75f })
#define CURSOR_MATERIAL ((frMaterial) {  2.0f, 0.0f, 0.5f, 0.75f })
#define WALL_MATERIAL   ((frMaterial) { 1.25f, 0.0f, 0.5f, 0.75f })

void InitExample(void);
void DeinitExample(void);
void UpdateExample(void);

const float DELTA_TIME = (1.0f / TARGET_FPS) * 100.0f;

static frWorld *world;
static frBody *wall, *cursor;

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | bricks.c");
    
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

    wall = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        frVec2PixelsToMeters((Vector2) { 0.5f * SCREEN_WIDTH, SCREEN_HEIGHT - 60.0f }),
        frCreateRectangle(
            WALL_MATERIAL,
            frNumberPixelsToMeters(0.75f * SCREEN_WIDTH), 
            frNumberPixelsToMeters(80.0f)
        )
    );
    
    cursor = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        FR_FLAG_NONE,
        FR_STRUCT_ZERO(Vector2),
        frCreateRectangle(
            CURSOR_MATERIAL, 
            frNumberPixelsToMeters(0.04f * SCREEN_WIDTH), 
            frNumberPixelsToMeters(20.0f)
        )
    );
    
    frAddToWorld(world, wall);
    frAddToWorld(world, cursor);
}

void DeinitExample(void) {
    frReleaseWorld(world);
}

void UpdateExample(void) {
    frSetBodyPosition(cursor, frVec2PixelsToMeters(GetMousePosition()));

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        frBody *brick = frCreateBodyFromShape(
            FR_BODY_DYNAMIC,
            FR_FLAG_NONE,
            frVec2PixelsToMeters((Vector2) { GetMouseX(), GetMouseY() + 10.0f }),
            frCreateRectangle(
                CURSOR_MATERIAL,
                frNumberPixelsToMeters(0.04f * SCREEN_WIDTH), 
                frNumberPixelsToMeters(20.0f)
            )
        );
        
        frAddToWorld(world, brick);
    }
    
    for (int i = 0; i < frGetWorldBodyCount(world); i++) {
        frBody *body = frGetWorldBody(world, i);
        
        if (!frIsInWorldBounds(world, body)) {
            frRemoveFromWorld(world, body);
            frReleaseBody(body);
        }
    }

    frSimulateWorld(world, DELTA_TIME);

    {
        BeginDrawing();
        
        ClearBackground(RAYWHITE);
        
        frDrawBody(wall, BLACK);
        
        for (int i = 2; i < frGetWorldBodyCount(world); i++) {
            frBody *body = frGetWorldBody(world, i);

            frDrawBody(body, RED);
        }
        
        frDrawBody(cursor, Fade(RED, 0.5f));
        
        frDrawSpatialHash(frGetWorldSpatialHash(world), 0.25f, GRAY);
        
        const char *message = TextFormat(
            "%d brick(s)!",
            frGetWorldBodyCount(world) - 2
        );
        
        DrawTextEx(
            GetFontDefault(),
            message,
            (Vector2) { 
                0.5f * (SCREEN_WIDTH - MeasureText(message, 40)), 
                0.125f * SCREEN_HEIGHT
            },
            40.0f,
            2.0f, 
            Fade(GRAY, 0.85f)
        );
        
        DrawFPS(8, 8);

        EndDrawing();
    }
}