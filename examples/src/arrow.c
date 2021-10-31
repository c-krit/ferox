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

#define SCREEN_WIDTH_IN_METERS  (frNumberPixelsToMeters(SCREEN_WIDTH))
#define SCREEN_HEIGHT_IN_METERS (frNumberPixelsToMeters(SCREEN_HEIGHT))

#define WORLD_RECTANGLE ((Rectangle) { \
    .width = SCREEN_WIDTH_IN_METERS,   \
    .height = SCREEN_HEIGHT_IN_METERS  \
})

#define EXAMPLE_STRING "LEFT-CLICK TO CHANGE THE SHAPE TYPE OF THE CURSOR"

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "c-krit/ferox | arrow.c");
    
    Vector2 polygon_vertices[4] = {
        (Vector2) { -6.0, -3.0 },
        (Vector2) { -6.0, 3.0 },
        (Vector2) { 6.0, 3.0 },
        (Vector2) { 6.0, -3.0 }
    };
    
    frShape *circle = frCreateCircle(FR_STRUCT_ZERO(frMaterial), 4);
    frShape *polygon = frCreatePolygon(FR_STRUCT_ZERO(frMaterial), polygon_vertices, 4);
    
    frBody *large_circle = frCreateBodyFromShape(
        FR_BODY_STATIC,
        (Vector2) { 
            SCREEN_WIDTH_IN_METERS * 0.5f, 
            SCREEN_HEIGHT_IN_METERS * 0.5f
        },
        frCreateCircle(FR_STRUCT_ZERO(frMaterial), 10)
    );
    
    frBody *cursor = frCreateBodyFromShape(
        FR_BODY_KINEMATIC,
        FR_STRUCT_ZERO(Vector2),
        circle
    );

    bool use_polygon_cursor = false;

    while (!WindowShouldClose()) {
        BeginDrawing();
        
        ClearBackground(RAYWHITE);
        
        frSetBodyPosition(cursor, frVec2PixelsToMeters(GetMousePosition()));
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            use_polygon_cursor = !use_polygon_cursor;
            
            if (use_polygon_cursor) frAttachShapeToBody(cursor, polygon);
            else frAttachShapeToBody(cursor, circle);
        }
        
        frDrawBody(large_circle, GRAY);
        frDrawBodyAABB(large_circle, GREEN);
        
        frDrawBody(cursor, Fade(BLACK, 0.85f));
        frDrawBodyAABB(cursor, GREEN);
        
        frCollision collision = frComputeCollision(
            frGetBodyShape(cursor), 
            frGetBodyTransform(cursor), 
            frGetBodyShape(large_circle), 
            frGetBodyTransform(large_circle)
        );
        
        if (collision.check) {
            for (int i = 0; i < collision.count; i++) {         
                DrawCircleLines(
                    frNumberMetersToPixels(collision.points[i].x), 
                    frNumberMetersToPixels(collision.points[i].y), 
                    4, 
                    GREEN
                );
                
                DrawLineEx(
                    frVec2MetersToPixels(collision.points[i]),
                    frVec2MetersToPixels(
                        frVec2Add(
                            collision.points[i],
                            frVec2ScalarMultiply(
                                collision.direction,
                                -collision.depths[i]
                            )
                        )
                    ),
                    2,
                    GREEN
                );
            }
        }
        
        DrawFPS(8, 8);
        
        DrawTextEx(
            GetFontDefault(),
            EXAMPLE_STRING,
            (Vector2) { 
                (SCREEN_WIDTH - MeasureText(EXAMPLE_STRING, 20)) / 2, 
                SCREEN_HEIGHT / 16
            },
            20,
            2, 
            Fade(GRAY, 0.85f)
        );

        EndDrawing();
    }
    
    frReleaseShape(frGetBodyShape(large_circle));
    frReleaseBody(large_circle);
    
    frReleaseShape(circle);
    frReleaseShape(polygon);
    
    frReleaseBody(cursor);
    
    CloseWindow();

    return 0;
}