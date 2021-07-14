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

/* | `geometry` 모듈 구조체... | */

/* 충돌 감지용 도형을 나타내는 구조체. */
typedef struct frShape {
    frShapeType type;
    frMaterial material;
    float area;
    union {
        struct {
            float radius;
        } circle;
        struct {
            struct {
                Vector2 data[FR_GEOMETRY_MAX_VERTEX_COUNT];
                int count;
            } vertices;
            struct {
                Vector2 data[FR_GEOMETRY_MAX_VERTEX_COUNT];
                int count;
            } normals;
        } polygon;
    };
} frShape;

/* | `geometry` 모듈 함수... | */

/* 다각형 `s`의 무게중심을 계산한다. */
static void frComputeCentroid(frShape *s);

/* 다각형 `s`를 볼록 다각형으로 변형한다. */
static void frComputeConvex(frShape *s);

/* 꼭짓점 배열 `vertices`로 만들 수 있는 가장 큰 볼록 다각형의 꼭짓점 배열을 반환한다. */
static Vector2 *frJarvisMarch(Vector2 *vertices, int count, int *new_count);

/* 반지름이 `radius`인 원을 나타내는 도형 구조체의 메모리 주소를 반환한다. */
frShape *frCreateCircle(frMaterial material, float radius) {
    frShape *result = frCreateShape();
    
    result->type = FR_SHAPE_CIRCLE;
    result->area = PI * (radius * radius);
    result->material = material;
    
    result->circle.radius = radius;
    
    return result;
}

/* 시작점 `p1`과 끝점 `p2`로 구성된 직사각형을 나타내는 도형 구조체의 메모리 주소를 반환한다. */
frShape *frCreateRectangle(frMaterial material, Vector2 p1, Vector2 p2) {
    frShape *result = frCreateShape();
    
    result->type = FR_SHAPE_POLYGON;
    result->material = material;
    result->area = -FLT_MAX;
    
    frSetRectangleVertices(result, p1, p2);
    
    return result;
}

/* 꼭짓점 배열이 `vertices`이고 꼭짓점 개수가 `count`인 다각형을 나타내는 도형 구조체의 메모리 주소를 반환한다. */
frShape *frCreatePolygon(frMaterial material, Vector2 *vertices, int count) {
    frShape *result = frCreateShape();
    
    if (vertices == NULL || count < 2 || count > FR_GEOMETRY_MAX_VERTEX_COUNT) 
        return NULL;
    
    result->type = FR_SHAPE_POLYGON;
    result->material = material;
    result->area = -FLT_MAX;
    
    frSetPolygonVertices(result, vertices, count);
    
    return result;
}

/* 형태가 정해지지 않은 도형 구조체의 메모리 주소를 반환한다. */
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
    if (s != NULL) free(s);
}

/* 원 `s`의 반지름을 반환한다. */
float frGetCircleRadius(frShape *s) {
    return (s != NULL && s->type == FR_SHAPE_CIRCLE) ? s->circle.radius : 0.0f;
}

/* 다각형 `s`의 꼭짓점 배열의 메모리 주소를 반환한다. */
Vector2 *frGetPolygonVertices(frShape *s, int *vertex_count) {
    if (s == NULL) return NULL;
    
    if (vertex_count != NULL) *vertex_count = s->polygon.vertices.count;
    return s->polygon.vertices.data;
}

/* 다각형 `s`의 각 변과 수직인 모든 변이 저장된 배열의 메모리 주소를 반환한다. */
Vector2 *frGetPolygonNormals(frShape *s, int *normal_count) {
    if (s == NULL) return NULL;
    
    if (normal_count != NULL) *normal_count = s->polygon.normals.count;
    return s->polygon.normals.data;
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

/* 도형 `s`의 넓이를 반환한다. */
float frGetShapeArea(frShape *s) {
    return (s != NULL && s->area >= 0.0f) ? s->area : 0.0f;
}

/* 도형 `s`의 Z축을 기준으로 한 관성 모멘트를 반환한다. */
float frGetShapeInertia(frShape *s) {
    if (s == NULL || s->type == FR_SHAPE_UNKNOWN) return 0.0f;
    
    float result = 0.0f;
    
    if (s->type == FR_SHAPE_CIRCLE) {
        result = 0.5f * frGetShapeMass(s) * (frGetCircleRadius(s) * frGetCircleRadius(s));
    } else if (s->type == FR_SHAPE_POLYGON) {
        float x_inertia = 0.0f;
        float y_inertia = 0.0f;
        
        // https://en.wikipedia.org/wiki/Second_moment_of_area#Any_polygon
        for (int j = s->polygon.vertices.count - 1, i = 0; i < s->polygon.vertices.count; j = i, i++) {
            Vector2 v1 = s->polygon.vertices.data[j];
            Vector2 v2 = s->polygon.vertices.data[i];
            
            float cross = frVec2CrossProduct(v1, v2);
            
            x_inertia += cross * ((v1.y * v1.y) + (v1.y * v2.y) + (v2.y * v2.y));
            y_inertia += cross * ((v1.x * v1.x) + (v1.x * v2.x) + (v2.x * v2.x));
        }
        
        result = fabs((x_inertia + y_inertia) / 12.0f);
    }
    
    return s->material.density * result;
}

/* 도형 `s`의 질량을 반환한다. */
float frGetShapeMass(frShape *s) {
    return (s != NULL) ? s->material.density * frGetShapeArea(s) : 0.0f;
}

/* 도형 `s`의 재질을 반환한다. */
frMaterial frGetShapeMaterial(frShape *s) {
    return (s != NULL) ? s->material : FR_STRUCT_ZERO(frMaterial);
}

/* 도형 `s`의 종류를 반환한다. */
frShapeType frGetShapeType(frShape *s) {
    return (s != NULL) ? s->type : FR_SHAPE_UNKNOWN;
}

/* 원 `s`의 반지름을 `radius`로 변경한다. */
void frSetCircleRadius(frShape *s, float radius) {
    if (s != NULL && s->type == FR_SHAPE_CIRCLE) s->circle.radius = radius;
}

/* 직사각형 `s`의 시작점과 끝점을 각각 `p1`과 `p2`로 변경한다. */
void frSetRectangleVertices(frShape *s, Vector2 p1, Vector2 p2) {
    if (s == NULL || s->type != FR_SHAPE_POLYGON) return;
    
    s->polygon.vertices.count = 4;
    s->polygon.normals.count = 4;
    
    for (int i = 0; i < 4; i++)
        s->polygon.vertices.data[i] = (Vector2) {
            (i == 0 || i == 1) ? p1.x : p2.x,
            (i == 0 || i == 3) ? p1.y : p2.y
        };
    
    frComputeCentroid(s);
    
    for (int i = 0; i < 4; i++)
        s->polygon.normals.data[i] = frVec2LeftNormal(
            frVec2Subtract(
                s->polygon.vertices.data[i], 
                s->polygon.vertices.data[i + 1]
            )
        );
}

/* 다각형 `s`의 꼭짓점 배열을 꼭짓점 개수 `count`개의 배열 `vertices`로 변경한다. */
void frSetPolygonVertices(frShape *s, Vector2 *vertices, int count) {
    if (s == NULL || s->type != FR_SHAPE_POLYGON) return;
    
    s->polygon.vertices.count = count;
    s->polygon.normals.count = count;

    for (int i = 0; i < count; i++)
        s->polygon.vertices.data[i] = vertices[i];

    frComputeConvex(s);
    frComputeCentroid(s);

    for (int j = count - 1, i = 0; i < count; j = i, i++)
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
        
        float r = frGetCircleRadius(s);
        
        return (delta_x * delta_x) + (delta_y * delta_y) <= r * r;
    } else if (s->type == FR_SHAPE_POLYGON) {
        frRaycastHit raycast = frComputeRaycast(s, tx, p, (Vector2) { .x = 1.0f }, FLT_MAX);
        
        return raycast.inside;
    }
}

/* 다각형 `s`의 무게중심을 계산한다. */
static void frComputeCentroid(frShape *s) {
    if (s == NULL || s->type != FR_SHAPE_POLYGON) return;
    
    /*
        1. 다각형에서 임의의 꼭짓점 `v[0]`을 선택한다.
        2. `v[0]`와 다각형에서 연속하는 두 꼭짓점 (`v[i]`, `v[i + 1]`)을 선택한다.
        3. 세 꼭짓점이 이루는 삼각형의 넓이 `A(0)`와 무게중심 `C(0)`를 계산한다.
        4. 다각형의 무게중심은 `((A(0) * C(0)) + (A(1) + C(1)) + ...) / (A(0) + A(1) + ...)`이 된다.
    */ 
    
    Vector2 result = FR_STRUCT_ZERO(Vector2);
    
    float twice_area_sum = 0.0f;
    
    for (int i = 0; i < s->polygon.vertices.count - 1; i++) {
        float twice_area = frVec2CrossProduct(
            frVec2Subtract(s->polygon.vertices.data[i], s->polygon.vertices.data[0]),
            frVec2Subtract(s->polygon.vertices.data[i + 1], s->polygon.vertices.data[0])
        );
        
        Vector2 thrice_centroid = frVec2Add(
            s->polygon.vertices.data[0],
            frVec2Add(s->polygon.vertices.data[i], s->polygon.vertices.data[i + 1])
        );
        
        result = frVec2Add(result, frVec2ScalarMultiply(thrice_centroid, twice_area));
        
        twice_area_sum += twice_area;
    }
    
    result = frVec2ScalarMultiply(result, 1.0f / (3.0f * twice_area_sum));
    
    if (s->area < 0.0f) s->area = fabs(twice_area_sum / 2.0f);
    
    for (int i = 0; i < s->polygon.vertices.count; i++)
        s->polygon.vertices.data[i] = frVec2Subtract(s->polygon.vertices.data[i], result);
}

/* 다각형 `s`를 볼록 다각형으로 변형한다. */
static void frComputeConvex(frShape *s) {
    if (s == NULL || s->type != FR_SHAPE_POLYGON) return;
    
    int new_count = 0;
        
    Vector2 *vertices = frJarvisMarch(
        s->polygon.vertices.data,
        s->polygon.vertices.count,
        &new_count
    );
    
    for (int i = 0; i < new_count; i++)
        s->polygon.vertices.data[i] = vertices[i];
    
    s->polygon.vertices.count = new_count;
    
    free(vertices);
}

/* 꼭짓점 배열 `vertices`로 만들 수 있는 가장 큰 볼록 다각형의 꼭짓점 배열을 반환한다. */
static Vector2 *frJarvisMarch(Vector2 *vertices, int count, int *new_count) {
    int leftmost_index = 0, pivot_index = 0, next_index = 0;
    int vertex_index = 0;
    
    if (vertices == NULL || count == 0 || new_count == NULL) return NULL;
    
    Vector2 *result = calloc(FR_GEOMETRY_MAX_VERTEX_COUNT, sizeof(Vector2));
    
    // 주어진 꼭짓점 배열에서 X좌표 값이 가장 작은 꼭짓점 L을 찾는다.
    for (int i = 1; i < count; i++)
        if (vertices[leftmost_index].x > vertices[i].x)
            leftmost_index = i;
    
    result[vertex_index++] = vertices[leftmost_index];
    
    // 기준점 P를 방금 찾은 꼭짓점 L로 설정한다.
    pivot_index = leftmost_index;
    
    while (1) {
        // 기준점 P의 바로 다음에 오는 꼭짓점 Q를 찾는다.
        for (int i = 0; i < count; i++) {
            if (i == pivot_index)
                continue;
            
            next_index = i;
            
            break;
        }
        
        // 기준점 P와 꼭짓점 Q 사이에 오는 꼭짓점 R을 찾는다.
        for (int i = 0; i < count; i++) {
            if (i == pivot_index || i == next_index)
                continue;
            
            // 기준점 P, 꼭짓점 R과 꼭짓점 Q가 반시계 방향으로 정렬되어 있는지 확인한다.
            if (frVec2CCW(vertices[pivot_index], vertices[i], vertices[next_index]))
                next_index = i;
        }
        
        // 꼭짓점 Q가 시작점인 꼭짓점 L이 되면 탐색을 종료한다.
        if (next_index == leftmost_index)
            break;
        
        pivot_index = next_index;
        
        // 새로 찾은 꼭짓점을 배열에 저장한다.
        result[vertex_index++] = vertices[next_index];
    }
    
    *new_count = vertex_index;
    
    return result;
}