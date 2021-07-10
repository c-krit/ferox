/*
    Copyright (c) 2021 jdeokkim

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

/* | `debug` 모듈 함수... | */

#ifdef HAVE_RAYLIB
    /* 게임 화면에 강체 `b`의 AABB와 질량 중심을 그린다. */
    static void frDrawBodyAABB(frBody *b);

    /* 게임 화면에 쿼드 트리 `tree`의 경계 범위를 그린다. */
    static void frDrawQuadtreeBounds(frQuadtree *tree);

    /* 강체 `b`의 다각형 꼭짓점 배열을 세계 기준 좌표 배열로 변환한다. */
    static int frGetWorldVertices(frBody *b, Vector2 *result);

    /* 게임 화면에 강체 `b`의 도형을 그린다. */
    void frDrawBody(frBody *b, Color color) {
        if (b == NULL || frGetBodyShape(b) == NULL) return;
        
        frShape *s = frGetBodyShape(b);
        
        if (frGetShapeType(s) == FR_SHAPE_CIRCLE) {
            DrawCircleV(FR_VECTOR_M2P(frGetBodyPosition(b)), FR_NUMBER_M2P(frGetCircleRadius(s)), color);
        } else if (frGetShapeType(s) == FR_SHAPE_POLYGON) {
            Vector2 world_vertices[FR_GEOMETRY_MAX_VERTEX_COUNT] = { 0 };
            
            int world_vertex_count = frGetWorldVertices(b, world_vertices);
            
            DrawTriangleFan(world_vertices, world_vertex_count, color);
        }
        
        frDrawBodyAABB(b);
    }

    /* 게임 화면에 강체 `b`의 도형 테두리를 그린다. */
    void frDrawBodyLines(frBody *b, Color color) {
        if (b == NULL || frGetBodyShape(b) == NULL) return;
        
        frShape *s = frGetBodyShape(b);
        
        if (frGetShapeType(s) == FR_SHAPE_CIRCLE) {
            Vector2 p = frGetBodyPosition(b);
            DrawCircleLines(p.x, p.y, frGetCircleRadius(s), color);
        } else if (frGetShapeType(s) == FR_SHAPE_POLYGON) {
            Vector2 world_vertices[FR_GEOMETRY_MAX_VERTEX_COUNT] = { 0 };
            
            int world_vertex_count = frGetWorldVertices(b, world_vertices);
                
            for (int j = world_vertex_count - 1, i = 0; i < world_vertex_count; j = i, i++)
                DrawLineEx(world_vertices[i], world_vertices[j], 2, color);
        }
        
        frDrawBodyAABB(b);
    }

    /* 게임 화면에 강체 `b`의 물리량 정보를 그린다. */
    void frDrawBodyProperties(frBody *b, Color color) {
        float mass = frGetBodyMass(b);
        float inertia = frGetBodyInertia(b);
        
        Vector2 velocity = frGetBodyVelocity(b);
        float angular_velocity = frGetBodyAngularVelocity(b);
        
        frTransform tx = frGetBodyTransform(b);
        
        DrawTextEx(
            GetFontDefault(),
            FormatText(
                "m: %fkg, I: %fkg*m²\n"
                "(x, y) [theta]: (%f, %f) [%f rad.]\n"
                "v [omega]: (%f, %f) [%f rad.]\n",
                mass, inertia,
                tx.position.x, tx.position.y, tx.rotation,
                velocity.x, velocity.y, angular_velocity
            ), 
            FR_VECTOR_M2P(frVec2Add(tx.position, (Vector2) { 1, 1 })),
            10, 
            1, 
            color
        );
    }

    /* 게임 화면에 쿼드 트리 `tree`를 그린다. */
    void frDrawQuadtree(frQuadtree *tree) {
        if (frGetQuadtreeDepth(tree) == 0 && frIsQuadtreeLeaf(tree)) {
            frDrawQuadtreeBounds(tree);
        } else {
            if (!frIsQuadtreeLeaf(tree)) {
                frDrawQuadtreeBounds(tree);

                for (int i = 0; i < 4; i++)
                    frDrawQuadtree(frGetQuadtreeChild(tree, i));
            }
        }
    }
    
    /* 무작위 색상을 반환한다. */
    Color frGetRandomColor(void) {
        return ColorFromHSV(GetRandomValue(0, 360), 1, 1);
    }

    /* 게임 화면에 강체 `b`의 AABB와 질량 중심을 그린다. */
    static void frDrawBodyAABB(frBody *b) {
        if (b == NULL) return;
        
        Rectangle aabb = frGetBodyAABB(b);
        
        DrawRectangleLinesEx(
            (Rectangle) { 
                FR_NUMBER_M2P(aabb.x), 
                FR_NUMBER_M2P(aabb.y),
                FR_NUMBER_M2P(aabb.width),
                FR_NUMBER_M2P(aabb.height)
            }, 
            1, 
            GREEN
        );
        
        DrawCircleV(FR_VECTOR_M2P(frGetBodyPosition(b)), 2, BLACK);
    }

    /* 게임 화면에 쿼드 트리 `tree`의 경계 범위를 그린다. */
    static void frDrawQuadtreeBounds(frQuadtree *tree) {
        Rectangle bounds = frGetQuadtreeBounds(tree);
        
        Vector2 h1 = (Vector2) { bounds.x, (2 * bounds.y + bounds.height) / 2 };
        Vector2 h2 = (Vector2) { bounds.x + bounds.width, (2 * bounds.y + bounds.height) / 2 };
        
        Vector2 v1 = (Vector2) { (2 * bounds.x + bounds.width) / 2, bounds.y };
        Vector2 v2 = (Vector2) { (2 * bounds.x + bounds.width) / 2, bounds.y + bounds.height };

        DrawLineEx(FR_VECTOR_M2P(h1), FR_VECTOR_M2P(h2), 0.25f, BLACK);
        DrawLineEx(FR_VECTOR_M2P(v1), FR_VECTOR_M2P(v2), 0.25f, BLACK);
    }

    /* 강체 `b`의 다각형 꼭짓점 배열을 세계 기준 좌표 배열로 변환한다. */
    static int frGetWorldVertices(frBody *b, Vector2 *result) {
        if (b == NULL || result == NULL) return 0;
        
        int vertex_count = 0;
            
        Vector2 *vertices = frGetPolygonVertices(frGetBodyShape(b), &vertex_count);
        
        if (vertices != NULL) {
            for (int i = 0; i < vertex_count; i++)
                result[i] = FR_VECTOR_M2P(frGetWorldPoint(b, vertices[i]));
        }
        
        return vertex_count;
    }
#endif