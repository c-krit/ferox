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

#define TEXT_FONT_SIZE   20
#define TEXT_STRING_DATA "LEFT-CLICK TO CHANGE THE"   \
                         " SHAPE TYPE OF THE CURSOR!"

void InitExample(void);
void DeinitExample(void);
void UpdateExample(void);

static frShape *circle, *polygon;
static frBody *large_circle, *cursor, *cursor_clone;

static bool use_polygon_cursor = false;

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | arrow.c");

    InitExample();

    while (!WindowShouldClose())
        UpdateExample();

    DeinitExample();
    
    CloseWindow();

    return 0;
}

void InitExample(void) {
    circle = frCreateCircle(FR_STRUCT_ZERO(frMaterial), 4.0f);
    polygon = frCreateRectangle(
        FR_STRUCT_ZERO(frMaterial), 
        12.0f,
        6.0f
    );
    
    large_circle = frCreateBodyFromShape(
        FR_BODY_STATIC,
        FR_FLAG_NONE,
        (Vector2) { 
            SCREEN_WIDTH_IN_METERS * 0.5f, 
            SCREEN_HEIGHT_IN_METERS * 0.5f
        },
        frCreateCircle(FR_STRUCT_ZERO(frMaterial), 10.0f)
    );
    
    cursor = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        FR_FLAG_NONE,
        FR_STRUCT_ZERO(Vector2),
        circle
    );

    cursor_clone = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        FR_FLAG_NONE,
        FR_STRUCT_ZERO(Vector2),
        circle
    );
}

void DeinitExample(void) {
    frReleaseShape(frGetBodyShape(large_circle));
    frReleaseBody(large_circle);
    
    frReleaseShape(circle);
    frReleaseShape(polygon);
    
    frReleaseBody(cursor_clone);
    frReleaseBody(cursor);
}

void UpdateExample(void) {
    frSetBodyPosition(cursor, frVec2PixelsToMeters(GetMousePosition()));
        
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        use_polygon_cursor = !use_polygon_cursor;
        
        if (use_polygon_cursor) {
            frAttachShapeToBody(cursor, polygon);
            frAttachShapeToBody(cursor_clone, polygon);
        } else {
            frAttachShapeToBody(cursor, circle);
            frAttachShapeToBody(cursor_clone, circle);
        }
    } else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        float rotation = DEG2RAD * GetRandomValue(0, 360);
        
        frSetBodyRotation(cursor, rotation);
        frSetBodyRotation(cursor_clone, rotation);
    }

    {
        BeginDrawing();
        
        ClearBackground(RAYWHITE);
        
        frDrawBody(large_circle, GRAY);
        frDrawBodyAABB(large_circle, 1.0f, GREEN);
        
        frCollision collision = frComputeBodyCollision(cursor, large_circle);
        
        if (collision.check) {
            frSetBodyPosition(
                cursor_clone,
                frVec2Add(
                    frGetBodyPosition(cursor),
                    frVec2ScalarMultiply(
                        collision.direction, 
                        -fmaxf(collision.depths[0], collision.depths[1])
                    )
                )
            );

            frDrawBodyLines(cursor_clone, 2.0f, RED);

            for (int i = 0; i < collision.count; i++) {
                DrawRing(
                    frVec2MetersToPixels(collision.points[i]), 
                    4.0f, 
                    6.0f, 
                    0.0f, 
                    360.0f, 
                    32, 
                    RED
                );

                frDrawArrow(
                    frVec2Add(
                        collision.points[i],
                        frVec2ScalarMultiply(
                            collision.direction,
                            -collision.depths[i]
                        )
                    ),
                    collision.points[i],
                    2,
                    RED
                );
            }
        }

        frDrawBody(cursor, Fade(BLACK, 0.75f));
        frDrawBodyAABB(cursor, 1.0f, GREEN);
        
        DrawFPS(8, 8);
        
        DrawTextEx(
            GetFontDefault(),
            TEXT_STRING_DATA,
            (Vector2) { 
                0.5f * (SCREEN_WIDTH - MeasureText(TEXT_STRING_DATA, TEXT_FONT_SIZE)), 
                SCREEN_HEIGHT / 16.0f
            },
            (float) TEXT_FONT_SIZE,
            2.0f,
            Fade(GRAY, 0.85f)
        );

        EndDrawing();
    }
}