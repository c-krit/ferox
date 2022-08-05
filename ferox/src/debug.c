/*
    Copyright (c) 2021-2022 jdeokkim

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

#include "ferox.h"

/* | `debug` 모듈 변수... | */

static const float ARROW_HEAD_LENGTH = 16.0f;

/* | `debug` 모듈 함수... | */

#ifndef FEROX_STANDALONE
    /* 강체 `b`의 다각형 꼭짓점 배열을 세계 기준의 픽셀 좌표 배열로 변환한다. */
    static frVertices frGetWorldVerticesInPixels(frBody *b);

    /* 게임 화면에 점 `p1`에서 `p2`로 향하는 화살표를 그린다. */
    void frDrawArrow(Vector2 p1, Vector2 p2, float thick, Color color) {
        if (thick <= 0.0f) return;
        
        p1 = frVec2MetersToPixels(p1);
        p2 = frVec2MetersToPixels(p2);

        Vector2 diff = frVec2Normalize(frVec2Subtract(p1, p2));

        Vector2 normal1 = frVec2LeftNormal(diff);
        Vector2 normal2 = frVec2RightNormal(diff);

        Vector2 h1 = frVec2Add(
            p2, 
            frVec2ScalarMultiply(
                frVec2Normalize(frVec2Add(diff, normal1)), 
                ARROW_HEAD_LENGTH
            )
        );

        Vector2 h2 = frVec2Add(
            p2, 
            frVec2ScalarMultiply(
                frVec2Normalize(frVec2Add(diff, normal2)), 
                ARROW_HEAD_LENGTH
            )
        );
        
        DrawLineEx(p1, p2, thick, color);
        DrawLineEx(p2, h1, thick, color);
        DrawLineEx(p2, h2, thick, color);
    }

    /* 게임 화면에 강체 `b`의 도형을 그린다. */
    void frDrawBody(frBody *b, Color color) {
        if (b == NULL) return;
        
        frShape *s = frGetBodyShape(b);
        
        if (frGetShapeType(s) == FR_SHAPE_CIRCLE) {
            DrawCircleV(
                frVec2MetersToPixels(frGetBodyPosition(b)), 
                frNumberMetersToPixels(frGetCircleRadius(s)), 
                color
            );
        } else if (frGetShapeType(s) == FR_SHAPE_POLYGON) {
            frVertices worldVertices = frGetWorldVerticesInPixels(b);
            
            DrawTriangleFan(worldVertices.data, worldVertices.count, color);
        }
    }

    /* 게임 화면에 강체 `b`의 도형 테두리를 그린다. */
    void frDrawBodyLines(frBody *b, float thick, Color color) {
        if (b == NULL || thick <= 0.0f) return;
        
        frShape *s = frGetBodyShape(b);
        
        if (frGetShapeType(s) == FR_SHAPE_CIRCLE) {
            Vector2 p = frGetBodyPosition(b);
            
            DrawRing(
                frVec2MetersToPixels(p),
                frNumberMetersToPixels(frGetCircleRadius(s)) - thick,
                frNumberMetersToPixels(frGetCircleRadius(s)),
                0.0f,
                360.0f,
                FR_DEBUG_CIRCLE_SEGMENT_COUNT,
                color
            );
        } else if (frGetShapeType(s) == FR_SHAPE_POLYGON) {
            frVertices worldVertices = frGetWorldVerticesInPixels(b);
                
            for (int j = worldVertices.count - 1, i = 0; i < worldVertices.count; j = i, i++)
                DrawLineEx(worldVertices.data[j], worldVertices.data[i], thick, color);
        }
    }

    /* 게임 화면에 강체 `b`의 AABB와 질량 중심을 그린다. */
    void frDrawBodyAABB(frBody *b, float thick, Color color) {
        if (b == NULL || thick <= 0.0f) return;
        
        Rectangle aabb = frGetBodyAABB(b);
        
        DrawRectangleLinesEx(
            (Rectangle) { 
                frNumberMetersToPixels(aabb.x), 
                frNumberMetersToPixels(aabb.y),
                frNumberMetersToPixels(aabb.width),
                frNumberMetersToPixels(aabb.height)
            }, 
            thick, 
            color
        );
        
        DrawCircleV(frVec2MetersToPixels(frGetBodyPosition(b)), 2.0f, color);
    }

    /* 게임 화면에 강체 `b`의 물리량 정보를 그린다. */
    void frDrawBodyProperties(frBody *b, Color color) {
        Vector2 velocity = frGetBodyVelocity(b);
        
        frTransform tx = frGetBodyTransform(b);
        
        DrawTextEx(
            GetFontDefault(),
            TextFormat(
                "m: %fkg, I: %fkg*m²\n"
                "(x, y) [theta]: (%f, %f) [%f rad.]\n"
                "v [omega]: (%f, %f) [%f rad.]\n",
                frGetBodyMass(b), frGetBodyInertia(b),
                tx.position.x, tx.position.y, tx.rotation,
                velocity.x, velocity.y, frGetBodyAngularVelocity(b)
            ), 
            frVec2MetersToPixels(frVec2Add(tx.position, (Vector2) { 1.0f, 1.0f })),
            10.0f, 
            1.0f, 
            color
        );
    }

    /* 게임 화면에 공간 해시맵 `hm`을 그린다. */
    void frDrawSpatialHash(frSpatialHash *hm, float thick, Color color) {
        if (hm == NULL || thick <= 0.0f) return;

        Rectangle bounds = frGetSpatialHashBounds(hm);

        const int vCount = bounds.width * FR_BROADPHASE_INVERSE_CELL_SIZE;
        const int hCount = bounds.height * FR_BROADPHASE_INVERSE_CELL_SIZE;
        
        for (int i = 0; i <= vCount; i++)
            DrawLineEx(
                frVec2MetersToPixels((Vector2) { FR_BROADPHASE_CELL_SIZE * i, 0.0f }),
                frVec2MetersToPixels((Vector2) { FR_BROADPHASE_CELL_SIZE * i, bounds.height }),
                thick,
                color
            );
    
        for (int i = 0; i <= hCount; i++)
            DrawLineEx(
                frVec2MetersToPixels((Vector2) { 0.0f, FR_BROADPHASE_CELL_SIZE * i }), 
                frVec2MetersToPixels((Vector2) { bounds.width, FR_BROADPHASE_CELL_SIZE * i }),
                thick,
                color
            );

        DrawRectangleLinesEx(frRecMetersToPixels(bounds), thick, color);
    }

    /* 강체 `b`의 다각형 꼭짓점 배열을 세계 기준의 픽셀 좌표 배열로 변환한다. */
    static frVertices frGetWorldVerticesInPixels(frBody *b) {
        if (b == NULL) return FR_STRUCT_ZERO(frVertices);

        frVertices vertices = frGetPolygonVertices(frGetBodyShape(b));
        
        for (int i = 0; i < vertices.count; i++)
            vertices.data[i] = frVec2MetersToPixels(frGetWorldPoint(b, vertices.data[i]));
        
        return vertices;
    }
#endif
