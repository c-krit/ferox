﻿/*
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

/* | `geometry` 모듈 자료형 정의... | */

/* 충돌 감지용 도형을 나타내는 구조체. */
typedef struct frShape {
    frShapeType type;
    frMaterial material;
    bool is_rect;
    float area;
    union {
        struct {
            float radius;
        } circle;
        struct {
            frVertices vertices;
            frVertices normals;
        } polygon;
    };
} frShape;

/* | `geometry` 모듈 함수... | */

/* 도형 `s`의 넓이를 계산한다. */
static void frComputeArea(frShape *s);

/* 다각형 `s`를 볼록 다각형으로 변형한다. */
static void frComputeConvex(frShape *s);

/* 꼭짓점 배열 `vertices`로 만들 수 있는 가장 큰 볼록 다각형의 꼭짓점 배열을 반환한다. */
static frVertices frJarvisMarch(frVertices *vertices);

/* 반지름이 `radius`인 원을 나타내는 도형을 생성한다. */
frShape *frCreateCircle(frMaterial material, float radius) {
    frShape *result = frCreateShape();
    
    result->type = FR_SHAPE_CIRCLE;
    result->material = material;
    
    frSetCircleRadius(result, radius);
    
    return result;
}

/* 가로 길이가 `width`이고 세로 길이가 `height`인 직사각형을 나타내는 도형을 생성한다. */
frShape *frCreateRectangle(frMaterial material, float width, float height) {
    frShape *result = frCreateShape();

    const float half_width = 0.5f * width, half_height = 0.5f * height;

    frVertices vertices = {
        .data = {
            (Vector2) { -half_width, -half_height },
            (Vector2) { -half_width, half_height },
            (Vector2) { half_width, half_height },
            (Vector2) { half_width, -half_height }
        },
        .count = 4
    };

    result->type = FR_SHAPE_POLYGON;
    result->material = material;
    result->is_rect = true;
    
    frSetPolygonVertices(result, vertices);
    
    return result;
}

/* 꼭짓점 배열이 `vertices`인 다각형을 나타내는 도형을 생성한다. */
frShape *frCreatePolygon(frMaterial material, frVertices vertices) {
    frShape *result = frCreateShape();

    if (vertices.count > FR_GEOMETRY_MAX_VERTEX_COUNT)
        vertices.count = FR_GEOMETRY_MAX_VERTEX_COUNT;
    
    result->type = FR_SHAPE_POLYGON;
    result->material = material;
    
    frSetPolygonVertices(result, vertices);
    
    return result;
}

/* 형태가 정해지지 않은 도형을 생성한다. */
frShape *frCreateShape(void) {
    frShape *result = calloc(1, sizeof(frShape));
    
    result->type = FR_SHAPE_UNKNOWN;
    
    return result;
}

/* 도형 `s`와 형태가 같은 새로운 도형을 반환한다. */
frShape *frCloneShape(frShape *s) {
    if (s == NULL || s->type == FR_SHAPE_UNKNOWN) return NULL;
    
    frShape *result = frCreateShape();
    
    result->type = s->type;
    result->material = s->material;
    result->is_rect = s->is_rect;
    result->area = s->area;
    
    if (result->type == FR_SHAPE_CIRCLE) {
        result->circle.radius = s->circle.radius;
    } else if (result->type == FR_SHAPE_POLYGON) {
        result->polygon.vertices.count = s->polygon.vertices.count;
        result->polygon.normals.count = s->polygon.normals.count;
        
        for (int i = 0; i < s->polygon.vertices.count; i++)
            result->polygon.vertices.data[i] = s->polygon.vertices.data[i];
        
        for (int i = 0; i < s->polygon.normals.count; i++)
            result->polygon.normals.data[i] = s->polygon.normals.data[i];
    }
    
    return result;
}

/* 도형 `s`에 할당된 메모리를 해제한다. */
void frReleaseShape(frShape *s) {
    free(s);
}

/* 구조체 `frShape`의 크기를 반환한다. */
size_t frGetShapeStructSize(void) {
    return sizeof(frShape);
}

/* 도형 `s`의 종류를 반환한다. */
frShapeType frGetShapeType(frShape *s) {
    return (s != NULL) ? s->type : FR_SHAPE_UNKNOWN;
}

/* 도형 `s`의 재질을 반환한다. */
frMaterial frGetShapeMaterial(frShape *s) {
    return (s != NULL) ? s->material : FR_STRUCT_ZERO(frMaterial);
}

/* 도형 `s`의 넓이를 반환한다. */
float frGetShapeArea(frShape *s) {
    return (s != NULL && s->area >= 0.0f) ? s->area : 0.0f;
}

/* 도형 `s`의 질량을 반환한다. */
float frGetShapeMass(frShape *s) {
    return (s != NULL) ? s->material.density * frGetShapeArea(s) : 0.0f;
}

/* 도형 `s`의 관성 모멘트를 반환한다. */
float frGetShapeInertia(frShape *s) {
    if (s == NULL || s->type == FR_SHAPE_UNKNOWN) return 0.0f;
    
    float result = 0.0f;
    
    if (s->type == FR_SHAPE_CIRCLE) {
        result = 0.5f * frGetShapeMass(s) * (frGetCircleRadius(s) * frGetCircleRadius(s));
    } else if (s->type == FR_SHAPE_POLYGON) {
        float x_inertia = 0.0f, y_inertia = 0.0f;
        
        // https://en.wikipedia.org/wiki/Second_moment_of_area#Any_polygon
        for (int j = s->polygon.vertices.count - 1, i = 0; i < s->polygon.vertices.count; j = i, i++) {
            Vector2 v1 = s->polygon.vertices.data[j];
            Vector2 v2 = s->polygon.vertices.data[i];
            
            float cross = frVec2CrossProduct(v1, v2);
            
            x_inertia += cross * ((v1.y * v1.y) + (v1.y * v2.y) + (v2.y * v2.y));
            y_inertia += cross * ((v1.x * v1.x) + (v1.x * v2.x) + (v2.x * v2.x));
        }
        
        result = fabsf((x_inertia + y_inertia) / 12.0f);
    }
    
    return s->material.density * result;
}

/* 도형 `s`의 AABB를 반환한다. */
Rectangle frGetShapeAABB(frShape *s, frTransform tx) {
    Rectangle result = FR_STRUCT_ZERO(Rectangle);
    
    if (s == NULL || s->type == FR_SHAPE_UNKNOWN) 
        return result;
    
    if (s->type == FR_SHAPE_CIRCLE) {
        result.x = tx.position.x - (s->circle.radius);
        result.y = tx.position.y - (s->circle.radius);
        
        result.width = result.height = 2 * (s->circle.radius);
        
        return result;
    } else if (s->type == FR_SHAPE_POLYGON) {
        Vector2 min_vertex = { FLT_MAX, FLT_MAX };
        Vector2 max_vertex = { -FLT_MAX, -FLT_MAX };
        
        // AABB의 X좌표와 Y좌표의 최솟값과 최댓값을 구한다.
        for (int i = 0; i < s->polygon.vertices.count; i++) {
            Vector2 vertex = frVec2Transform(s->polygon.vertices.data[i], tx);
            
            if (min_vertex.x > vertex.x) min_vertex.x = vertex.x;
            if (min_vertex.y > vertex.y) min_vertex.y = vertex.y;
                
            if (max_vertex.x < vertex.x) max_vertex.x = vertex.x;
            if (max_vertex.y < vertex.y) max_vertex.y = vertex.y;
        }
        
        float delta_x = max_vertex.x - min_vertex.x;
        float delta_y = max_vertex.y - min_vertex.y;
        
        result.x = min_vertex.x;
        result.y = min_vertex.y;
        
        result.width = delta_x;
        result.height = delta_y;
        
        return result;
    }
}

/* 원 `s`의 반지름을 반환한다. */
float frGetCircleRadius(frShape *s) {
    return (s != NULL && s->type == FR_SHAPE_CIRCLE) ? s->circle.radius : 0.0f;
}

/* 직사각형 `s`의 가로 및 세로 길이를 반환한다. */
Vector2 frGetRectangleDimensions(frShape *s) {
    if (!frIsShapeRectangle(s)) return FR_STRUCT_ZERO(Vector2);

    return (Vector2) { 
        s->polygon.vertices.data[2].x - s->polygon.vertices.data[1].x,
        s->polygon.vertices.data[1].y - s->polygon.vertices.data[0].y
    };
}

/* 다각형 `s`의 `index + 1`번째 꼭짓점을 반환한다. */
Vector2 frGetPolygonVertex(frShape *s, int index) {
    return (s != NULL && index >= 0 && index < s->polygon.vertices.count)
        ? s->polygon.vertices.data[index]
        : FR_STRUCT_ZERO(Vector2);
}

/* 다각형 `s`의 `index + 1`번째 법선 벡터를 반환한다. */
Vector2 frGetPolygonNormal(frShape *s, int index) {
    return (s != NULL && index >= 0 && index < s->polygon.normals.count)
        ? s->polygon.normals.data[index]
        : FR_STRUCT_ZERO(Vector2);
}

/* 다각형 `s`의 꼭짓점 배열을 반환한다. */
frVertices frGetPolygonVertices(frShape *s) {
    return (s != NULL) ? s->polygon.vertices : FR_STRUCT_ZERO(frVertices);
}

/* 다각형 `s`의 법선 벡터 배열을 반환한다. */
frVertices frGetPolygonNormals(frShape *s) {
    return (s != NULL) ? s->polygon.normals : FR_STRUCT_ZERO(frVertices);
}

/* 다각형 `s`가 직사각형인지 확인한다. */
bool frIsShapeRectangle(frShape *s) {
    return (s != NULL) && (s->type == FR_SHAPE_POLYGON) && s->is_rect;
}

/* 원 `s`의 반지름을 `radius`로 변경한다. */
void frSetCircleRadius(frShape *s, float radius) {
    if (s == NULL || s->type != FR_SHAPE_CIRCLE) return; 
    
    s->circle.radius = radius;
    
    frComputeArea(s);
}

/* 직사각형 `s`의 가로 및 세로 길이를 `wh`의 X값과 Y값으로 설정한다. */
void frSetRectangleDimensions(frShape *s, Vector2 wh) {
    if (!frIsShapeRectangle(s) || wh.x <= 0.0f || wh.y <= 0.0f) return;

    wh = frVec2ScalarMultiply(wh, 0.5f);

    frVertices vertices = {
        .data = {
            (Vector2) { -wh.x, -wh.y },
            (Vector2) { -wh.x, wh.y },
            (Vector2) { wh.x, wh.y },
            (Vector2) { wh.x, -wh.y }
        },
        .count = 4
    };

    frSetPolygonVertices(s, vertices);
}

/* 다각형 `s`의 꼭짓점 배열을 `vertices`로 변경한다. */
void frSetPolygonVertices(frShape *s, frVertices vertices) {
    if (s == NULL || s->type != FR_SHAPE_POLYGON) return;
    
    s->polygon.vertices.count = vertices.count;
    // s->polygon.normals.count = (!s->is_rect) ? vertices.count : 2;
    s->polygon.normals.count = vertices.count;

    for (int i = 0; i < vertices.count; i++)
        s->polygon.vertices.data[i] = vertices.data[i];
    
    // 직사각형은 이미 볼록 다각형이므로 변형하지 않는다.
    if (!s->is_rect) frComputeConvex(s);

    frComputeArea(s);

    for (int j = vertices.count - 1, i = 0; i < vertices.count; j = i, i++)
        s->polygon.normals.data[i] = frVec2LeftNormal(
            frVec2Subtract(
                s->polygon.vertices.data[i], 
                s->polygon.vertices.data[j]
            )
        );
}

/* 도형 `s`의 재질을 `material`로 설정한다. */
void frSetShapeMaterial(frShape *s, frMaterial material) {
    if (s != NULL) s->material = material;
}

/* 도형 `s`의 종류를 `type`으로 변경한다. */
void frSetShapeType(frShape *s, frShapeType type) {
    if (s != NULL && s->type != type) s->type = type;
}

/* 점 `p`가 도형 `s`의 내부에 있는지 확인한다. */
bool frShapeContainsPoint(frShape *s, frTransform tx, Vector2 p) {
    if (s == NULL || s->type == FR_SHAPE_UNKNOWN) return false;
    
    Rectangle aabb = frGetShapeAABB(s, tx);
    
    // `p`가 `s`의 AABB 내부에 있는지 먼저 확인한다.
    if (p.x < aabb.x || p.x > aabb.x + aabb.width
        || p.y < aabb.y || p.y > aabb.y + aabb.height)
        return false;
    
    if (s->type == FR_SHAPE_CIRCLE) {
        float delta_x = p.x - tx.position.x;
        float delta_y = p.y - tx.position.y;
        
        float radius = frGetCircleRadius(s);
        
        return (delta_x * delta_x) + (delta_y * delta_y) <= radius * radius;
    } else if (s->type == FR_SHAPE_POLYGON) {
        frRaycastHit raycast = frComputeShapeRaycast(
            s, tx, 
            (frRay) { p, (Vector2) { .x = 1.0f }, FLT_MAX }
        );
        
        return raycast.inside;
    }
}

/* 도형 `s`의 넓이를 계산한다. */
static void frComputeArea(frShape *s) {
    if (s == NULL || s->type == FR_SHAPE_UNKNOWN) {
        s->area = 0.0f;
    } else if (s->type == FR_SHAPE_CIRCLE) {
        s->area = PI * (s->circle.radius * s->circle.radius);
    } else if (s->type == FR_SHAPE_POLYGON) {
        if (s->is_rect) {
            frVertices *vertices = &(s->polygon.vertices);

            float width = vertices->data[2].x - vertices->data[1].x;
            float height = vertices->data[1].y - vertices->data[0].y;

            s->area = width * height;
        } else {
            float twice_area_sum = 0.0f;

            for (int i = 0; i < s->polygon.vertices.count - 1; i++) {
                float twice_area = frVec2CrossProduct(
                    frVec2Subtract(s->polygon.vertices.data[i], s->polygon.vertices.data[0]),
                    frVec2Subtract(s->polygon.vertices.data[i + 1], s->polygon.vertices.data[0])
                );

                Vector2 thrice_centroid = frVec2Add(
                    s->polygon.vertices.data[0],
                    frVec2Add(
                        s->polygon.vertices.data[i], 
                        s->polygon.vertices.data[i + 1]
                    )
                );

                twice_area_sum += twice_area;
            }

            s->area = fabsf(0.5f * twice_area_sum);
        }
    }
}

/* 다각형 `s`를 볼록 다각형으로 변형한다. */
static void frComputeConvex(frShape *s) {
    if (s == NULL || s->type != FR_SHAPE_POLYGON) return;
    
    frVertices result = frJarvisMarch(&(s->polygon.vertices));
    
    for (int i = 0; i < result.count; i++)
        s->polygon.vertices.data[i] = result.data[i];
    
    s->polygon.vertices.count = result.count;
}

/* 꼭짓점 배열 `vertices`로 만들 수 있는 가장 큰 볼록 다각형의 꼭짓점 배열을 반환한다. */
static frVertices frJarvisMarch(frVertices *vertices) {
    if (vertices == NULL || vertices->count == 0) return FR_STRUCT_ZERO(frVertices);

    frVertices result = FR_STRUCT_ZERO(frVertices);

    int leftmost_index = 0, pivot_index = 0, next_index = 0, vertex_count = 0;
    
    // 주어진 꼭짓점 배열에서 X좌표 값이 가장 작은 꼭짓점 L을 찾는다.
    for (int i = 1; i < vertices->count; i++)
        if (vertices->data[leftmost_index].x > vertices->data[i].x)
            leftmost_index = i;
    
    result.data[vertex_count++] = vertices->data[leftmost_index];
    
    // 기준점 P를 방금 찾은 꼭짓점 L로 설정한다.
    pivot_index = leftmost_index;
    
    while (1) {
        // 기준점 P의 바로 다음에 오는 꼭짓점 Q를 찾는다.
        for (int i = 0; i < vertices->count; i++) {
            if (i == pivot_index)
                continue;
            
            next_index = i;
            
            break;
        }
        
        // 기준점 P와 꼭짓점 Q 사이에 오는 꼭짓점 R을 찾는다.
        for (int i = 0; i < vertices->count; i++) {
            if (i == pivot_index || i == next_index)
                continue;
            
            // 기준점 P, 꼭짓점 R과 꼭짓점 Q가 반시계 방향으로 정렬되어 있는지 확인한다.
            if (frVec2CCW(vertices->data[pivot_index], vertices->data[i], vertices->data[next_index]))
                next_index = i;
        }
        
        // 꼭짓점 Q가 시작점인 꼭짓점 L이 되면 탐색을 종료한다.
        if (next_index == leftmost_index)
            break;
        
        pivot_index = next_index;
        
        // 새로 찾은 꼭짓점을 배열에 저장한다.
        result.data[vertex_count++] = vertices->data[next_index];
    }
    
    result.count = vertex_count;
    
    return result;
}
