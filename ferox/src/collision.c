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

/* | `collision` 모듈 구조체... | */

/* 선분을 나타내는 구조체. */
typedef struct frEdge {
    Vector2 points[2];
    int count;
} frEdge;

/* | `collision` 모듈 함수... | */

/* 도형 `s1`의 AABB가 `s2`의 AABB와 충돌하는지 확인한다. */
static bool frCheckCollisionAABB(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 다각형 `s`에서 벡터 `v`와의 내적이 가장 큰 꼭짓점의 인덱스를 반환한다. */
static int frGetPolygonFurthestIndex(frShape *s, frTransform tx, Vector2 v);

/* 도형 `s`에서 벡터 `v`와 가장 수직에 가까운 변을 반환한다. */
static frEdge frGetShapeSignificantEdge(frShape *s, frTransform tx, Vector2 v);

/* 다각형 `s1`에서 `s2`로의 충돌에서 충돌 방향과 최대 충돌 깊이를 계산한다. */
static int frGetSeparatingAxisIndex(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2, 
    float *distance
);

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclesSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 다각형 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclePolySAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 최적화된 분리 축 정리를 이용하여, 다각형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionPolysSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 최적화된 분리 축 정리를 이용하여, 도형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 선분 `e1`에서 선분 `e2`의 바깥쪽 영역에 해당되는 부분을 모두 잘라낸다. */
static frEdge frClipEdge(frEdge e1, frEdge e2);

/* 선분 `e`에서 벡터 `v`와의 내적이 `min_dot`보다 작은 부분을 모두 잘라낸다. */
static frEdge frClipEdgeWithAxis(frEdge e, Vector2 v, float min_dot);

/* 다각형 `s`에서 선분 `e`의 바깥쪽 영역에 해당되는 부분을 모두 잘라낸다. */
static frShape *frClipPolygon(frShape *s, frEdge e);

/* 도형 `s1`에서 `s2`로의 충돌에서 최초 충돌 지점과 충돌 깊이를 계산한다. */
static frCollision frComputeCollisionManifold(
    frShape *s1, frTransform tx1,
    frShape *s2, frTransform tx2,
    Vector2 direction, float depth
);

/* `o1`에서 `v1` 방향으로 진행하는 광선이 `o2`에서 `v2` 방향으로 진행하는 광선과 만나는지 계산한다. */
static bool frComputeIntersectionRays(Vector2 o1, Vector2 v1, Vector2 o2, Vector2 v2, float *distance);

/* `o`에서 `v` 방향으로 진행하는 광선이 중심점이 `c`이고 반지름이 `r`인 원과 만나는지 계산한다. */
static bool frComputeIntersectionRayCircle(Vector2 o, Vector2 v, Vector2 c, float r, float *distance);

/* 도형 `s1`에서 `s2`로의 충돌을 계산한다. */
frCollision frComputeCollision(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    return frCheckCollisionAABB(s1, tx1, s2, tx2)
        ? frComputeCollisionSAT(s1, tx1, s2, tx2)
        : FR_STRUCT_ZERO(frCollision);
}

/* `p`에서 `v` 방향으로 최대 `max_distance`의 거리까지 진행하는 광선을 도형 `s`에 투사한다. */
frRaycastHit frComputeRaycast(frShape *s, frTransform tx, Vector2 p, Vector2 v, float max_distance) {
    frRaycastHit result = { .shape = s, .distance = FLT_MAX };
    
    v = frVec2Normalize(v);
    
    float distance = FLT_MAX;
    
    if (frGetShapeType(s) == FR_SHAPE_UNKNOWN) {
        return result;
    } else if (frGetShapeType(s) == FR_SHAPE_CIRCLE) {
        result.check = frComputeIntersectionRayCircle(p, v, tx.position, frGetCircleRadius(s), &distance);
        
        if (distance < 0.0f) {
            result.check = false;
            result.inside = true;
        } else if (distance > max_distance) {
            result.check = false;
        }
        
        if (distance < 0.0f || distance > max_distance) {
            result.check = false;
            if (distance < 0.0f) result.inside = true;
        }
        
        if (result.check) {
            result.distance = distance;
            
            result.normal = frVec2LeftNormal(frVec2Subtract(p, result.point));
            result.point = frVec2Add(p, frVec2ScalarMultiply(v, result.distance));
        }
        
        return result;
    } else if (frGetShapeType(s) == FR_SHAPE_POLYGON) {
        int intersection_count = 0;
        int vertex_count = -1;
        
        Vector2 *vertices = frGetPolygonVertices(s, &vertex_count);
        
        // 다각형의 변 중에 광선과 교차하는 변이 존재하는지 확인한다.
        for (int j = vertex_count - 1, i = 0; i < vertex_count; j = i, i++) {
            Vector2 e_v = frVec2Subtract(
                frVec2Transform(vertices[i], tx),
                frVec2Transform(vertices[j], tx)
            );
            
            bool intersects = frComputeIntersectionRays(p, v, frVec2Transform(vertices[j], tx), e_v, &distance);
            
            if (intersects && distance <= max_distance) {
                if (result.distance > distance) {
                    result.distance = distance;
                    
                    result.normal = frVec2LeftNormal(e_v);
                    result.point = frVec2Add(p, frVec2ScalarMultiply(v, result.distance));
                }
                
                intersection_count++;
            }
        }
        
        if (intersection_count <= 0) {
            result.check = false;
        } else {
            if (intersection_count & 1) result.inside = true;
            else result.check = true;
        }
        
        if (result.inside) result.distance = 0.0f;
        
        return result;
    }
}

/* Sutherland-Hodgman 다각형 절단 알고리즘을 이용하여, 다각형 `s1`을 `s2`에 맞게 절단한다. */
frShape *frSutherlandHodgman(frShape *s1, frShape *s2) {
    if (frGetShapeType(s1) != FR_SHAPE_POLYGON || frGetShapeType(s2) != FR_SHAPE_POLYGON) 
        return s1;
    
    frShape *result = s1;
    
    int vertex_count = -1;
    Vector2 *vertices = frGetPolygonVertices(s2, &vertex_count);
    
    // 다각형 `s1`에서 `s2`의 각 변의 바깥쪽에 위치한 모든 부분을 삭제한다.
    for (int j = vertex_count - 1, i = 0; i < vertex_count; j = i, i++) {
        if (result == NULL) break;
        
        result = frClipPolygon(result, (frEdge) { vertices[j], vertices[i], 2 });
    }
    
    return result;
}

/* 도형 `s1`의 AABB가 `s2`의 AABB와 충돌하는지 확인한다. */
static bool frCheckCollisionAABB(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    return CheckCollisionRecs(frGetShapeAABB(s1, tx1), frGetShapeAABB(s2, tx2));
}

/* 다각형 `s`에서 벡터 `v`와의 내적이 가장 큰 꼭짓점의 인덱스를 반환한다. */
static int frGetPolygonFurthestIndex(frShape *s, frTransform tx, Vector2 v) {
    if (frGetShapeType(s) != FR_SHAPE_POLYGON) return -1;
    
    v = frVec2Rotate(v, -tx.rotation);
    
    int result = 0;
    int vertex_count = -1;
        
    Vector2 *vertices = frGetPolygonVertices(s, &vertex_count);
    
    float max_dot = -FLT_MAX;
    
    for (int i = 0; i < vertex_count; i++) {
        float dot = frVec2DotProduct(vertices[i], v);
        
        if (max_dot < dot) {
            max_dot = dot;
            result = i;
        }
    }
    
    return result;
}

/* 도형 `s`에서 벡터 `v`와 가장 수직에 가까운 변을 반환한다. */
static frEdge frGetShapeSignificantEdge(frShape *s, frTransform tx, Vector2 v) {
    frEdge result = FR_STRUCT_ZERO(frEdge);
    
    if (s == NULL || frGetShapeType(s) == FR_SHAPE_UNKNOWN) return result;
    
    if (frGetShapeType(s) == FR_SHAPE_CIRCLE) {
        result.points[0] = result.points[1] = frVec2Transform(
            frVec2ScalarMultiply(v, frGetCircleRadius(s)), 
            tx
        );
        
        result.count = 1;
        
        return result;
    } else if (frGetShapeType(s) == FR_SHAPE_POLYGON) {
        int furthest_index = frGetPolygonFurthestIndex(s, tx, v);
        int vertex_count = -1;
        
        Vector2 *vertices = frGetPolygonVertices(s, &vertex_count);

        int prev_index = (furthest_index == 0) ? vertex_count - 1 : furthest_index - 1;
        int next_index = (furthest_index == vertex_count - 1) ? 0 : furthest_index + 1;

        Vector2 furthest_vertex = frVec2Transform(vertices[furthest_index], tx);

        Vector2 prev_vertex = frVec2Transform(vertices[prev_index], tx);
        Vector2 next_vertex = frVec2Transform(vertices[next_index], tx);

        Vector2 left_v = frVec2Normalize(frVec2Subtract(furthest_vertex, prev_vertex));
        Vector2 right_v = frVec2Normalize(frVec2Subtract(furthest_vertex, next_vertex));

        if (frVec2DotProduct(left_v, v) < frVec2DotProduct(right_v, v)) {
            result.points[0] = prev_vertex;
            result.points[1] = furthest_vertex;
        } else {
            result.points[0] = furthest_vertex;
            result.points[1] = next_vertex;
        }
        
        result.count = 2;

        return result;
    }
}

/* 다각형 `s1`에서 `s2`로의 충돌에서 충돌 방향과 최대 충돌 깊이를 계산한다. */
static int frGetSeparatingAxisIndex(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2, 
    float *distance
) {
    if (frGetShapeType(s1) != FR_SHAPE_POLYGON || frGetShapeType(s2) != FR_SHAPE_POLYGON 
        || distance == NULL) return -1;
    
    Vector2 *s1_vertices = frGetPolygonVertices(s1, NULL);
    Vector2 *s2_vertices = frGetPolygonVertices(s2, NULL);
    
    int s1_normal_count = -1;
    
    Vector2 *s1_normals = frGetPolygonNormals(s1, &s1_normal_count);
    
    int result = -1;
    float max_distance = -FLT_MAX;
    
    for (int i = 0; i < s1_normal_count; i++) {
        Vector2 s1_vertex = frVec2Transform(s1_vertices[i], tx1);
        Vector2 s1_normal = frVec2Rotate(s1_normals[i], tx1.rotation);
        
        int s2_furthest_index = frGetPolygonFurthestIndex(s2, tx2, frVec2Negate(s1_normal));
        Vector2 s2_furthest_vertex = frVec2Transform(s2_vertices[s2_furthest_index], tx2);
        
        float distance = frVec2DotProduct(s1_normal, frVec2Subtract(s2_furthest_vertex, s1_vertex));
        
        if (max_distance < distance) {
            max_distance = distance;
            result = i;
        }
    }
    
    *distance = max_distance;
    
    return result;
}

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclesSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    if (frGetShapeType(s1) != FR_SHAPE_CIRCLE || frGetShapeType(s2) != FR_SHAPE_CIRCLE)
        return FR_STRUCT_ZERO(frCollision);
    
    Vector2 direction = frVec2Subtract(tx2.position, tx1.position);
    float depth = (frGetCircleRadius(s1) + frGetCircleRadius(s2)) - frVec2Magnitude(direction);
    
    if (depth < 0.0f) return FR_STRUCT_ZERO(frCollision);
    
    return frComputeCollisionManifold(s1, tx1, s2, tx2, direction, depth);
}

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 다각형 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclePolySAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    frShapeType s1_type = frGetShapeType(s1);
    frShapeType s2_type = frGetShapeType(s2);
    
    if (s1_type == FR_SHAPE_UNKNOWN || s2_type == FR_SHAPE_UNKNOWN || s1_type == s2_type)
        return FR_STRUCT_ZERO(frCollision);
        
    frShape *circle = s1;
    frShape *polygon = s2;
    
    frTransform circle_tx = tx1;
    frTransform polygon_tx = tx2;
    
    if (s1_type == FR_SHAPE_POLYGON && s2_type == FR_SHAPE_CIRCLE) {
        circle = s2;
        polygon = s1;
        
        circle_tx = tx2;
        polygon_tx = tx1;
    }
    
    int vertex_count = -1;
    
    Vector2 *vertices = frGetPolygonVertices(polygon, &vertex_count);
    Vector2 *normals = frGetPolygonNormals(polygon, NULL);
    
    int normal_index = -1;
    float max_distance = -FLT_MAX;
    
    for (int i = 0; i < vertex_count; i++) {
        Vector2 vertex = frVec2Transform(vertices[i], polygon_tx);
        
        float distance = frVec2DotProduct(
            frVec2Rotate(normals[i], polygon_tx.rotation), 
            frVec2Subtract(circle_tx.position, vertex)
        );
        
        if (max_distance < distance) {
            max_distance = distance;
            normal_index = i;
        }
    }
    
    Vector2 direction = FR_STRUCT_ZERO(Vector2);
    float depth = frGetCircleRadius(circle) - max_distance;
    
    // 원과 다각형은 서로 만나지 않는다.
    if (depth < 0.0f) return FR_STRUCT_ZERO(frCollision);
    
    Vector2 p1 = (normal_index > 0)
        ? frVec2Transform(vertices[normal_index - 1], polygon_tx)
        : frVec2Transform(vertices[vertex_count - 1], polygon_tx);
    Vector2 p2 = frVec2Transform(vertices[normal_index], polygon_tx);
    
    /*
        1. 원이 `p1`을 포함하는 선분과 만나는가?
        2. 원이 `p1`과 `p2` 사이의 선분과 만나는가?
        3. 원이 `p2`를 포함하는 선분과 만나는가?
        
        => 벡터의 내적을 통해 확인할 수 있다.
    */
    
    float v1_dot = frVec2DotProduct(frVec2Subtract(circle_tx.position, p1), frVec2Subtract(p2, p1));
    float v2_dot = frVec2DotProduct(frVec2Subtract(circle_tx.position, p2), frVec2Subtract(p1, p2));
    
    if (v1_dot <= 0.0f) direction = frVec2Normalize(frVec2Subtract(p1, circle_tx.position));
    else if (v2_dot <= 0.0f) direction = frVec2Normalize(frVec2Subtract(p2, circle_tx.position));
    else direction = frVec2Normalize(frVec2Negate(frVec2Rotate(normals[normal_index], polygon_tx.rotation)));
    
    if (frVec2DotProduct(frVec2Subtract(tx2.position, tx1.position), direction) < 0.0f) 
        direction = frVec2Negate(direction);
    
    return frComputeCollisionManifold(s1, tx1, s2, tx2, direction, depth);
}

/* 최적화된 분리 축 정리를 이용하여, 다각형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionPolysSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    if (frGetShapeType(s1) != FR_SHAPE_POLYGON || frGetShapeType(s2) != FR_SHAPE_POLYGON)
        return FR_STRUCT_ZERO(frCollision);
        
    Vector2 direction = FR_STRUCT_ZERO(Vector2);
    float depth = -FLT_MAX;
    
    float distance1 = -FLT_MAX, distance2 = -FLT_MAX;
    
    int index1 = frGetSeparatingAxisIndex(s1, tx1, s2, tx2, &distance1);
    if (distance1 >= 0.0f) return FR_STRUCT_ZERO(frCollision);
    
    int index2 = frGetSeparatingAxisIndex(s2, tx2, s1, tx1, &distance2);
    if (distance2 >= 0.0f) return FR_STRUCT_ZERO(frCollision);

    Vector2 *s1_normals = frGetPolygonNormals(s1, NULL);
    Vector2 *s2_normals = frGetPolygonNormals(s2, NULL);
    
    direction = (distance1 > distance2) 
        ? frVec2Rotate(s1_normals[index1], tx1.rotation) 
        : frVec2Rotate(s2_normals[index2], tx2.rotation);
        
    depth = FR_NUMBER_MAX(distance1, distance2);
    
    if (frVec2DotProduct(frVec2Subtract(tx2.position, tx1.position), direction) < 0.0f) 
        direction = frVec2Negate(direction);
    
    return frComputeCollisionManifold(s1, tx1, s2, tx2, direction, depth);
}

/* 최적화된 분리 축 정리를 이용하여, 도형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    frShapeType s1_type = frGetShapeType(s1);
    frShapeType s2_type = frGetShapeType(s2);
    
    if (s1_type == FR_SHAPE_UNKNOWN || s2_type == FR_SHAPE_UNKNOWN)
        return FR_STRUCT_ZERO(frCollision);
    else if (s1_type == FR_SHAPE_CIRCLE && s2_type == FR_SHAPE_CIRCLE)
        return frComputeCollisionCirclesSAT(s1, tx1, s2, tx2);
    else if (s1_type == FR_SHAPE_CIRCLE && s2_type == FR_SHAPE_POLYGON
        || s1_type == FR_SHAPE_POLYGON && s2_type == FR_SHAPE_CIRCLE)
        return frComputeCollisionCirclePolySAT(s1, tx1, s2, tx2);
    else if (s1_type == FR_SHAPE_POLYGON && s2_type == FR_SHAPE_POLYGON)
        return frComputeCollisionPolysSAT(s1, tx1, s2, tx2);
}

/* 선분 `e1`에서 선분 `e2`의 바깥쪽 영역에 해당되는 부분을 모두 잘라낸다. */
static frEdge frClipEdge(frEdge e1, frEdge e2) {
    frEdge result = FR_STRUCT_ZERO(frEdge);
    
    // `e1`의 시작점과 끝점이 선분 `e2`의 안쪽에 있는지 확인한다.
    bool inside_p1 = frVec2CCW(e1.points[0], e2.points[0], e2.points[1]);
    bool inside_p2 = frVec2CCW(e1.points[1], e2.points[0], e2.points[1]);
        
    if (inside_p1 && inside_p2) {
        result.points[0] = e1.points[1];
        result.points[1] = e1.points[1];
        
        result.count = 1;
    } else {
        Vector2 e1_v = frVec2Subtract(e1.points[1], e1.points[0]);
        Vector2 e2_v = frVec2Subtract(e2.points[1], e2.points[0]);
        
        float distance = FLT_MAX;
        
        frComputeIntersectionRays(e1.points[0], e1_v, e2.points[0], e2_v, &distance);

        Vector2 midpoint = frVec2Add(
            e1.points[0], 
            frVec2ScalarMultiply(
                e1_v,
                distance
            )
        );
        
        if (inside_p1 && !inside_p2) {
            result.points[0] = midpoint;
            result.points[1] = midpoint;

            result.count = 1;
        } else if (!inside_p1 && inside_p2) {
            result.points[0] = midpoint;
            result.points[1] = e1.points[1];

            result.count = 2;
        }
    }
    
    return result;
}

/* 선분 `e`에서 벡터 `v`와의 내적이 `min_dot`보다 작은 부분을 모두 잘라낸다. */
static frEdge frClipEdgeWithAxis(frEdge e, Vector2 v, float min_dot) {
    frEdge result = FR_STRUCT_ZERO(frEdge);
    
    float p1_dot = frVec2DotProduct(e.points[0], v) - min_dot;
    float p2_dot = frVec2DotProduct(e.points[1], v) - min_dot;
    
    bool inside_p1 = p1_dot >= 0.0f;
    bool inside_p2 = p2_dot >= 0.0f;
    
    if (inside_p1 && inside_p2) {
        result.points[0] = e.points[0];
        result.points[1] = e.points[1];
        
        result.count = 2;
    } else {
        Vector2 e_v = frVec2Subtract(e.points[1], e.points[0]);
        
        float distance = p1_dot / (p1_dot - p2_dot);
        
        Vector2 midpoint = frVec2Add(
            e.points[0], 
            frVec2ScalarMultiply(
                e_v,
                distance
            )
        );
        
        if (inside_p1 && !inside_p2) {
            result.points[0] = e.points[0];
            result.points[1] = midpoint;

            result.count = 2;
        } else if (!inside_p1 && inside_p2) {
            result.points[0] = e.points[1];
            result.points[1] = midpoint;

            result.count = 2;
        }
    }
    
    return result;
}

/* 다각형 `s`에서 선분 `e`의 바깥쪽 영역에 해당되는 부분을 모두 잘라낸다. */
static frShape *frClipPolygon(frShape *s, frEdge e) {
    if (frGetShapeType(s) != FR_SHAPE_POLYGON || e.count < 2) return NULL;
    
    frShape *result = frCreateShape();
    frSetShapeType(result, FR_SHAPE_POLYGON);
    
    int vertex_count = -1, new_vertex_count = 0;
    
    Vector2 *vertices = frGetPolygonVertices(s, &vertex_count);
    Vector2 new_vertices[FR_GEOMETRY_MAX_VERTEX_COUNT];
    
    for (int j = vertex_count - 1, i = 0; i < vertex_count; j = i, i++) {
        frEdge new_e = frClipEdge((frEdge) { vertices[j], vertices[i], 2 }, e);
        
        for (int k = 0; k < new_e.count; k++)
            new_vertices[new_vertex_count++] = new_e.points[i];
    }
    
    frSetPolygonVertices(result, new_vertices, new_vertex_count);
    
    return result;
}

/* 도형 `s1`에서 `s2`로의 충돌에서 최초 충돌 지점과 충돌 깊이를 계산한다. */
static frCollision frComputeCollisionManifold(
    frShape *s1, frTransform tx1,
    frShape *s2, frTransform tx2,
    Vector2 direction, float depth
) {
    frCollision result = { .check = true, .direction = direction };
    
    frShapeType s1_type = frGetShapeType(s1);
    frShapeType s2_type = frGetShapeType(s2);
    
    if (s1_type == FR_SHAPE_UNKNOWN || s2_type == FR_SHAPE_UNKNOWN) {
        return result;
    } else if (frGetShapeType(s1) == FR_SHAPE_CIRCLE && s2_type == FR_SHAPE_CIRCLE) {
        frEdge s1_e = frGetShapeSignificantEdge(s1, tx1, direction);
        
        result.points[0] = result.points[1] = s1_e.points[0];
        result.depths[0] = result.depths[1] = depth;
        
        result.count = 1;
        
        return result;
    } else if (s1_type == FR_SHAPE_CIRCLE && s2_type == FR_SHAPE_POLYGON 
               || s1_type == FR_SHAPE_POLYGON && s2_type == FR_SHAPE_CIRCLE) {
        frEdge s1_e = frGetShapeSignificantEdge(s1, tx1, direction);
        frEdge s2_e = frGetShapeSignificantEdge(s2, tx2, frVec2Negate(direction));
        
        frEdge circle_e = s1_e;
        frEdge polygon_e = s2_e;
        
        if (frGetShapeType(s1) == FR_SHAPE_POLYGON && frGetShapeType(s2) == FR_SHAPE_CIRCLE) {
            circle_e = s2_e;
            polygon_e = s1_e;
        }
        
        result.points[0] = result.points[1] = circle_e.points[0];
        result.depths[0] = result.depths[1] = depth;
        
        result.count = 1;
        
        return result;
    } else if (s1_type == FR_SHAPE_POLYGON && s2_type == FR_SHAPE_POLYGON) {
        frEdge s1_e = frGetShapeSignificantEdge(s1, tx1, direction);
        frEdge s2_e = frGetShapeSignificantEdge(s2, tx2, frVec2Negate(direction));

        frEdge ref_e = s1_e;
        frEdge inc_e = s2_e;
        
        frTransform ref_tx = tx1;
        frTransform inc_tx = tx2;

        float dot1 = frVec2DotProduct(frVec2Subtract(s1_e.points[1], s1_e.points[0]), direction);
        float dot2 = frVec2DotProduct(frVec2Subtract(s2_e.points[1], s2_e.points[0]), direction);

        if (fabs(dot1) > fabs(dot2)) {
            ref_e = s2_e;
            inc_e = s1_e;
            
            ref_tx = tx2;
            inc_tx = tx1;
        }

        Vector2 ref_v = frVec2Normalize(frVec2Subtract(ref_e.points[1], ref_e.points[0]));

        float ref_dot1 = frVec2DotProduct(ref_e.points[0], ref_v);
        float ref_dot2 = frVec2DotProduct(ref_e.points[1], ref_v);

        inc_e = frClipEdgeWithAxis(inc_e, ref_v, ref_dot1);
        inc_e = frClipEdgeWithAxis(inc_e, frVec2Negate(ref_v), -ref_dot2);

        Vector2 refn_v = frVec2RightNormal(ref_v);

        float max_depth = frVec2DotProduct(ref_e.points[0], refn_v);

        result.points[0] = inc_e.points[0];
        result.points[1] = inc_e.points[1];

        result.depths[0] = frVec2DotProduct(result.points[0], refn_v) - max_depth;
        result.depths[1] = frVec2DotProduct(result.points[1], refn_v) - max_depth;
    
        result.count = 2;

        if (result.depths[0] < 0.0f) {
            result.points[0] = result.points[1];
            result.depths[0] = result.depths[1];

            result.count = 1;
        } else if (result.depths[1] < 0.0f) {
            result.points[1] = result.points[0];
            result.depths[1] = result.depths[0];

            result.count = 1;
        }

        return result;
    }
}

/* `o1`에서 `v1` 방향으로 진행하는 광선이 `o2`에서 `v2` 방향으로 진행하는 광선과 만나는지 계산한다. */
static bool frComputeIntersectionRays(Vector2 o1, Vector2 v1, Vector2 o2, Vector2 v2, float *distance) {
    bool result = true;
    
    Vector2 o1_to_o2 = frVec2Subtract(o2, o1);
    
    float cross = frVec2CrossProduct(v1, v2);
    
    float t = frVec2CrossProduct(o1_to_o2, v2) / cross;
    float u = frVec2CrossProduct(o1_to_o2, v1) / cross;
    
    // 두 광선은 서로 만나지 않는다.
    if (frApproxEquals(cross, 0.0f) || (t < 0.0f || u < 0.0f || u > 1.0f)) result = false;
    
    *distance = t;
    
    return result;
}

/* `o`에서 `v` 방향으로 진행하는 광선이 중심점이 `c`이고 반지름이 `r`인 원과 만나는지 계산한다. */
static bool frComputeIntersectionRayCircle(Vector2 o, Vector2 v, Vector2 c, float r, float *distance) {
    bool result = true;
    
    Vector2 o_to_c = frVec2Subtract(c, o);
    
    float dot = frVec2DotProduct(o_to_c, v);
    float base_sqr = frVec2MagnitudeSqr(o_to_c) - (dot * dot);
    
    // 광선과 원은 서로 만나지 않는다.
    if (dot < 0.0f || (r * r) < base_sqr) result = false;
    
    *distance = dot - (sqrtf((r * r) - base_sqr));
    
    return result;
}