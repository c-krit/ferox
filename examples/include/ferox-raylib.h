/*
    Copyright (c) 2021-2023 Jaedeok Kim <jdeokkim@protonmail.com>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef FEROX_RAYLIB_H
#define FEROX_RAYLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes =========================================================================== */

#include "ferox.h"
#include "raylib.h"

/* Macros ============================================================================= */

#define FR_DRAW_ARROW_HEAD_LENGTH     8.0f
#define FR_DRAW_CIRCLE_SEGMENT_COUNT  32

#define FR_DRAW_COLOR_MATTEBLACK      CLITERAL(Color) { 16, 16, 16, 255 }

/* Public Function Prototypes ========================================================= */

/* 
    Draws an arrow that starts from `v1` to `v2` 
    with the given `thick`ness and `color`. 
*/
void frDrawArrow(frVector2 v1, frVector2 v2, float thick, Color color);

/* 
    Draws the AABB (Axis-Aligned Bounding Box) of `b` 
    with the given `thick`ness and `color`.
*/
void frDrawBodyAABB(const frBody *b, float thick, Color color);

/* Draws the outline of `b` with the given `thick`ness and `color`. */
void frDrawBodyLines(const frBody *b, float thick, Color color);

/* 
    Draws a grid within the `bounds`, 
    with the given `cellSize`, `thick`ness and `color`. 
*/
void frDrawGrid(Rectangle bounds, float cellSize, float thick, Color color);

#ifdef __cplusplus
}
#endif

#endif // `FEROX_RAYLIB_H`

#ifdef FEROX_RAYLIB_IMPLEMENTATION

/* Public Functions =================================================================== */

/* 
    Draws an arrow that starts from `v1` to `v2` 
    with the given `thick`ness and `color`. 
*/
void frDrawArrow(frVector2 v1, frVector2 v2, float thick, Color color) {
    if (thick <= 0.0f) return;

    v1 = frVector2UnitsToPixels(v1);
    v2 = frVector2UnitsToPixels(v2);

    frVector2 unitDiff = frVector2Normalize(frVector2Subtract(v1, v2));

    frVector2 leftNormal = frVector2LeftNormal(unitDiff);
    frVector2 rightNormal = frVector2RightNormal(unitDiff);

    frVector2 leftHead = frVector2Add(
        v2, 
        frVector2ScalarMultiply(
            frVector2Normalize(frVector2Add(unitDiff, leftNormal)), 
            FR_DRAW_ARROW_HEAD_LENGTH
        )
    );

    frVector2 rightHead = frVector2Add(
        v2, 
        frVector2ScalarMultiply(
            frVector2Normalize(frVector2Add(unitDiff, rightNormal)), 
            FR_DRAW_ARROW_HEAD_LENGTH
        )
    );

    DrawLineEx(
        (Vector2) { v1.x, v1.y }, 
        (Vector2) { v2.x, v2.y },
        thick, 
        color
    );

    DrawLineEx(
        (Vector2) { v2.x, v2.y },
        (Vector2) { leftHead.x, leftHead.y },
        thick,
        color
    );

    DrawLineEx(
        (Vector2) { v2.x, v2.y }, 
        (Vector2) { rightHead.x, rightHead.y },
        thick, 
        color
    );
}

/* 
    Draws the AABB (Axis-Aligned Bounding Box) of `b` 
    with the given `thick`ness and `color`.
*/
void frDrawBodyAABB(const frBody *b, float thick, Color color) {
    if (b == NULL || thick <= 0.0f) return;
        
    frAABB aabb = frGetBodyAABB(b);
    
    DrawRectangleLinesEx(
        (Rectangle) { 
            frUnitsToPixels(aabb.x), 
            frUnitsToPixels(aabb.y),
            frUnitsToPixels(aabb.width),
            frUnitsToPixels(aabb.height)
        }, 
        thick, 
        color
    );

    frVector2 position = frVector2UnitsToPixels(frGetBodyPosition(b));
    
    DrawCircleV((Vector2) { position.x, position.y }, 2.0f, color);
}

/* Draws the outline of `b` with the given `thick`ness and `color`. */
void frDrawBodyLines(const frBody *b, float thick, Color color) {
    if (b == NULL || thick <= 0.0f) return;
        
    frShape *s = frGetBodyShape(b);

    frTransform tx = frGetBodyTransform(b);
    frVector2 position = frVector2UnitsToPixels(frGetBodyPosition(b));

    if (frGetShapeType(s) == FR_SHAPE_CIRCLE) {
        DrawRing(
            (Vector2) { position.x, position.y },
            frUnitsToPixels(frGetCircleRadius(s)) - thick,
            frUnitsToPixels(frGetCircleRadius(s)),
            0.0f,
            360.0f,
            FR_DRAW_CIRCLE_SEGMENT_COUNT,
            color
        );
    } else if (frGetShapeType(s) == FR_SHAPE_POLYGON) {
        const frVertices *vertices = frGetPolygonVertices(s);

        for (int j = vertices->count - 1, i = 0; i < vertices->count; j = i, i++) {
            frVector2 v1 = frVector2Transform(vertices->data[j], tx);
            frVector2 v2 = frVector2Transform(vertices->data[i], tx);

            v1 = frVector2UnitsToPixels(v1);
            v2 = frVector2UnitsToPixels(v2);

            DrawLineEx(
                (Vector2) { v1.x, v1.y }, 
                (Vector2) { v2.x, v2.y },
                thick, 
                color
            );
        }
    }

    DrawCircleV((Vector2) { position.x, position.y }, 2.0f, color);
}

/* 
    Draws a grid within the `bounds`, 
    with the given `cellSize`, `thick`ness and `color`. 
*/
void frDrawGrid(Rectangle bounds, float cellSize, float thick, Color color) {
    if (cellSize <= 0.0f || thick <= 0.0f) return;

    const float inverseCellSize = 1.0f / cellSize;

    const int vLineCount = bounds.width * inverseCellSize;
    const int hLineCount = bounds.height * inverseCellSize;

    for (int i = 0; i <= vLineCount; i++) {
        DrawLineEx(
            (Vector2) { 
                bounds.x + frUnitsToPixels(cellSize * i), 
                bounds.y 
            },
            (Vector2) { 
                bounds.x + frUnitsToPixels(cellSize * i), 
                bounds.y + bounds.height 
            },
            thick,
            color
        );
    }

    for (int i = 0; i <= hLineCount; i++)
        DrawLineEx(
            (Vector2) { 
                bounds.x, 
                bounds.y + frUnitsToPixels(cellSize * i) 
            },
            (Vector2) { 
                bounds.x + bounds.width, 
                bounds.y + frUnitsToPixels(cellSize * i) 
            },
            thick,
            color
        );

    DrawRectangleLinesEx(bounds, thick, color);
}

#endif // `FEROX_RAYLIB_IMPLEMENTATION`