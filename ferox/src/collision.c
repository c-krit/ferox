﻿/*
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
    float *depth
);

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclesSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 다각형 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclePolySAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 최적화된 분리 축 정리를 이용하여, 다각형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionPolysSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 최적화된 분리 축 정리를 이용하여, 도형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 선분 `e`에서 벡터 `v`와의 내적이 `min_dot`보다 작은 부분을 모두 잘라낸다. */
static frEdge frClipEdgeWithAxis(frEdge e, Vector2 v, float min_dot);

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

/* 강체 `b`에 `o`에서 `v` 방향으로 최대 `max_distance`의 거리까지 진행하는 광선을 투사한다. */
frRaycastHit frComputeBodyRaycast(frBody *b, Vector2 o, Vector2 v, float max_distance) {
    if (b == NULL || frGetBodyShape(b) == NULL) {
        return FR_STRUCT_ZERO(frRaycastHit);
    } else {
        frRaycastHit result = frComputeShapeRaycast(
            frGetBodyShape(b), frGetBodyTransform(b),
            o, v, max_distance
        );

        result.body = b;

        return result;
    }
}

/* 도형 `s`에 `o`에서 `v` 방향으로 최대 `max_distance`의 거리까지 진행하는 광선을 투사한다. */
frRaycastHit frComputeShapeRaycast(frShape *s, frTransform tx, Vector2 o, Vector2 v, float max_distance) {
    if (s == NULL || frGetShapeType(s) == FR_SHAPE_UNKNOWN) {
        return FR_STRUCT_ZERO(frRaycastHit);
    } else {
        frRaycastHit result = { .shape = s, .distance = FLT_MAX };
        
        float distance = FLT_MAX;

        v = frVec2Normalize(v);
        
        if (frGetShapeType(s) == FR_SHAPE_CIRCLE) {
            bool intersects = frComputeIntersectionRayCircle(
                o, v,
                tx.position, frGetCircleRadius(s),
                &distance
            );
            
            result.check = (distance >= 0.0f) && (distance <= max_distance);
            result.inside = (distance < 0.0f);
            
            if (result.check) {
                result.distance = distance;
                
                result.point = frVec2Add(o, frVec2ScalarMultiply(v, result.distance));
                result.normal = frVec2LeftNormal(frVec2Subtract(o, result.point));
            }
            
            return result;
        } else if (frGetShapeType(s) == FR_SHAPE_POLYGON) {
            int intersection_count = 0;
            
            frVertices vertices = frGetPolygonVertices(s);
            
            // 다각형의 변 중에 광선과 교차하는 변이 존재하는지 확인한다.
            for (int j = vertices.count - 1, i = 0; i < vertices.count; j = i, i++) {
                Vector2 e_v = frVec2Subtract(
                    frVec2Transform(vertices.data[i], tx),
                    frVec2Transform(vertices.data[j], tx)
                );
                
                bool intersects = frComputeIntersectionRays(
                    o, v, 
                    frVec2Transform(vertices.data[j], tx), e_v, 
                    &distance
                );
                
                if (intersects && distance <= max_distance) {
                    if (result.distance > distance) {
                        result.distance = distance;
                        
                        result.point = frVec2Add(o, frVec2ScalarMultiply(v, result.distance));
                        result.normal = frVec2LeftNormal(e_v);
                    }
                    
                    intersection_count++;
                }
            }
            
            result.check = (intersection_count > 0);
            result.inside = (intersection_count & 1);
            
            return result;
        }
    }
}

/* 도형 `s1`의 AABB가 `s2`의 AABB와 충돌하는지 확인한다. */
static bool frCheckCollisionAABB(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    return CheckCollisionRecs(frGetShapeAABB(s1, tx1), frGetShapeAABB(s2, tx2));
}

/* 다각형 `s`에서 벡터 `v`와의 내적이 가장 큰 꼭짓점의 인덱스를 반환한다. */
static int frGetPolygonFurthestIndex(frShape *s, frTransform tx, Vector2 v) {
    if (frGetShapeType(s) != FR_SHAPE_POLYGON) return -1;
    
    frVertices vertices = frGetPolygonVertices(s);
    
    float max_dot = -FLT_MAX;
    int result = 0;

    v = frVec2Rotate(v, -tx.rotation);
    
    for (int i = 0; i < vertices.count; i++) {
        float dot = frVec2DotProduct(vertices.data[i], v);
        
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
        
        frVertices vertices = frGetPolygonVertices(s);

        int prev_index = (furthest_index == 0) ? vertices.count - 1 : furthest_index - 1;
        int next_index = (furthest_index == vertices.count - 1) ? 0 : furthest_index + 1;

        Vector2 furthest_vertex = frVec2Transform(vertices.data[furthest_index], tx);

        Vector2 prev_vertex = frVec2Transform(vertices.data[prev_index], tx);
        Vector2 next_vertex = frVec2Transform(vertices.data[next_index], tx);

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
    float *depth
) {
    int result = -1;

    if (frGetShapeType(s1) != FR_SHAPE_POLYGON || frGetShapeType(s2) != FR_SHAPE_POLYGON) 
        return result;
    
    frVertices s1_normals = frGetPolygonNormals(s1);
    
    float max_distance = -FLT_MAX;
    
    for (int i = 0; i < s1_normals.count; i++) {
        Vector2 s1_vertex = frVec2Transform(frGetPolygonVertex(s1, i), tx1);
        Vector2 s1_normal = frVec2Rotate(s1_normals.data[i], tx1.rotation);
        
        int s2_furthest_index = frGetPolygonFurthestIndex(s2, tx2, frVec2Negate(s1_normal));
        Vector2 s2_furthest_vertex = frVec2Transform(frGetPolygonVertex(s2, s2_furthest_index), tx2);
        
        float distance = frVec2DotProduct(s1_normal, frVec2Subtract(s2_furthest_vertex, s1_vertex));
        
        if (max_distance < distance) {
            max_distance = distance;
            result = i;
        }
    }
    
    *depth = max_distance;
    
    return result;
}

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclesSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    frCollision result = FR_STRUCT_ZERO(frCollision);

    if (frGetShapeType(s1) != FR_SHAPE_CIRCLE || frGetShapeType(s2) != FR_SHAPE_CIRCLE)
        return result;
        
    float depth = (frGetCircleRadius(s1) + frGetCircleRadius(s2)) 
        - frVec2Magnitude(frVec2Subtract(tx2.position, tx1.position));
    
    if (depth < 0.0f) return result;
    
    result.check = true;
    
    result.direction = frVec2Normalize(frVec2Subtract(tx2.position, tx1.position));
    
    frEdge e = frGetShapeSignificantEdge(s1, tx1, result.direction);
    
    result.points[0] = result.points[1] = e.points[0];
    result.depths[0] = result.depths[1] = depth;
    result.count = 1;
    
    return result;
}

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 다각형 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclePolySAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    frCollision result = FR_STRUCT_ZERO(frCollision);
    
    if (frGetShapeType(s1) == FR_SHAPE_UNKNOWN || frGetShapeType(s2) == FR_SHAPE_UNKNOWN 
        || frGetShapeType(s1) == frGetShapeType(s2))
        return FR_STRUCT_ZERO(frCollision);
        
    frShape *circle = s1, *polygon = s2;
    frTransform circle_tx = tx1, polygon_tx = tx2;
    
    if (frGetShapeType(s1) == FR_SHAPE_POLYGON && frGetShapeType(s2) == FR_SHAPE_CIRCLE) {
        circle = s2, polygon = s1;
        circle_tx = tx2, polygon_tx = tx1;
    }

    frVertices vertices = frGetPolygonVertices(polygon);
    frVertices normals = frGetPolygonNormals(polygon);
    
    float max_distance = -FLT_MAX;
    int normal_index = -1;
    
    for (int i = 0; i < vertices.count; i++) {
        Vector2 vertex = frVec2Transform(vertices.data[i], polygon_tx);
        Vector2 normal = frVec2Rotate(normals.data[i], polygon_tx.rotation);
        
        float distance = frVec2DotProduct(normal, frVec2Subtract(circle_tx.position, vertex));
        
        if (max_distance < distance) {
            max_distance = distance;
            normal_index = i;
        }
    }
    
    float radius = frGetCircleRadius(circle), depth = radius - max_distance;
    
    if (depth < 0.0f) {
        return result;
    } else if (depth > radius) {
        Vector2 direction = frVec2Negate(
            frVec2Rotate(
                normals.data[normal_index], 
                polygon_tx.rotation
            )
        );
        
        result.check = true;
            
        result.direction = direction;
        result.points[0] = result.points[1] = frVec2Add(
            circle_tx.position,
            frVec2ScalarMultiply(result.direction, radius)
        );
        
        // 다각형이 원의 중심을 포함하고 있다면, 충돌 깊이는 원의 반지름이 된다.
        result.depths[0] = result.depths[1] = radius;
        result.count = 1;
        
        return result;
    } else {
        Vector2 p1 = (normal_index > 0)
            ? frVec2Transform(vertices.data[normal_index - 1], polygon_tx)
            : frVec2Transform(vertices.data[vertices.count - 1], polygon_tx);
        Vector2 p2 = frVec2Transform(vertices.data[normal_index], polygon_tx);

        /*
            1. 원이 `p1`을 포함하는 선분과 만나는가?
            2. 원이 `p1`과 `p2` 사이의 선분과 만나는가?
            3. 원이 `p2`를 포함하는 선분과 만나는가?

            => 벡터의 내적을 통해 확인할 수 있다.
        */

        float v1_dot = frVec2DotProduct(
            frVec2Subtract(circle_tx.position, p1), 
            frVec2Subtract(p2, p1)
        );

        float v2_dot = frVec2DotProduct(
            frVec2Subtract(circle_tx.position, p2), 
            frVec2Subtract(p1, p2)
        );
        
        result.direction = frVec2Negate(
            frVec2Rotate(
                normals.data[normal_index], 
                polygon_tx.rotation
            )
        );
        
        Vector2 diff = frVec2Subtract(p1, circle_tx.position);

        if (v1_dot <= 0.0f) {
            if (frVec2MagnitudeSqr(diff) > radius * radius)
                return result;
                
            result.points[0] = result.points[1] = p1;
        } else if (v2_dot <= 0.0f) {
            diff = frVec2Subtract(p2, circle_tx.position);
        
            if (frVec2MagnitudeSqr(diff) > radius * radius)
                return result;

            result.points[0] = result.points[1] = p2;
        } else {
            if (frVec2DotProduct(diff, result.direction) > radius)
                return result;

            result.points[0] = result.points[1] = frVec2Add(
                circle_tx.position,
                frVec2ScalarMultiply(result.direction, radius)
            );
        }

        result.check = true;

        result.depths[0] = result.depths[1] = depth;
        result.count = 1;

        if (frVec2DotProduct(frVec2Subtract(tx2.position, tx1.position), result.direction) < 0.0f)
            result.direction = frVec2Negate(result.direction);

        return result;
    }
}

/* 최적화된 분리 축 정리를 이용하여, 다각형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionPolysSAT(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    frCollision result = FR_STRUCT_ZERO(frCollision);

    if (frGetShapeType(s1) != FR_SHAPE_POLYGON || frGetShapeType(s2) != FR_SHAPE_POLYGON)
        return result;
    
    float depth1 = FLT_MAX, depth2 = FLT_MAX;
    
    int index1 = frGetSeparatingAxisIndex(s1, tx1, s2, tx2, &depth1);
    if (depth1 >= 0.0f) return result;
    
    int index2 = frGetSeparatingAxisIndex(s2, tx2, s1, tx1, &depth2);
    if (depth2 >= 0.0f) return result;
    
    Vector2 direction = (depth1 > depth2) 
        ? frVec2Rotate(frGetPolygonNormal(s1, index1), tx1.rotation)
        : frVec2Rotate(frGetPolygonNormal(s2, index2), tx2.rotation);
        
    float depth = FR_NUMBER_MAX(depth1, depth2);
    
    if (frVec2DotProduct(frVec2Subtract(tx2.position, tx1.position), direction) < 0.0f) 
        direction = frVec2Negate(direction);
        
    result.check = true;
    
    // 회전 변환이 적용된 벡터는 회전 변환을 적용하기 전의 벡터와 크기가 같다.
    result.direction = direction;
    
    frEdge e1 = frGetShapeSignificantEdge(s1, tx1, direction);
    frEdge e2 = frGetShapeSignificantEdge(s2, tx2, frVec2Negate(direction));
    
    frEdge ref_e = e1, inc_e = e2;
    frTransform ref_tx = tx1, inc_tx = tx2;

    float dot1 = frVec2DotProduct(frVec2Subtract(e1.points[1], e1.points[0]), direction);
    float dot2 = frVec2DotProduct(frVec2Subtract(e2.points[1], e2.points[0]), direction);

    if (fabs(dot1) > fabs(dot2)) {
        ref_e = e2;
        inc_e = e1;

        ref_tx = tx2;
        inc_tx = tx1;
    }

    Vector2 ref_v = frVec2Normalize(frVec2Subtract(ref_e.points[1], ref_e.points[0]));

    float ref_dot1 = frVec2DotProduct(ref_e.points[0], ref_v);
    float ref_dot2 = frVec2DotProduct(ref_e.points[1], ref_v);

    inc_e = frClipEdgeWithAxis(inc_e, ref_v, ref_dot1);
    inc_e = frClipEdgeWithAxis(inc_e, frVec2Negate(ref_v), -ref_dot2);

    Vector2 ref_normal_v = frVec2RightNormal(ref_v);

    float max_depth = frVec2DotProduct(ref_e.points[0], ref_normal_v);

    result.points[0] = inc_e.points[0];
    result.points[1] = inc_e.points[1];

    result.depths[0] = frVec2DotProduct(result.points[0], ref_normal_v) - max_depth;
    result.depths[1] = frVec2DotProduct(result.points[1], ref_normal_v) - max_depth;

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

/* `o1`에서 `v1` 방향으로 진행하는 광선이 `o2`에서 `v2` 방향으로 진행하는 광선과 만나는지 계산한다. */
static bool frComputeIntersectionRays(Vector2 o1, Vector2 v1, Vector2 o2, Vector2 v2, float *distance) {
    bool result = true;
    
    float cross = frVec2CrossProduct(v1, v2);
    
    // 두 광선은 평행하거나 일직선 상에 있다.
    if (frNumberApproxEquals(cross, 0.0f)) {
        *distance = 0.0f;
        
        return false;
    }
    
    Vector2 diff = frVec2Subtract(o2, o1);

    float inverse_cross = 1.0f / cross;

    float t = frVec2CrossProduct(diff, v2) * inverse_cross;
    float u = frVec2CrossProduct(diff, v1) * inverse_cross;

    // 두 광선은 평행하지 않으면서 서로 만나지 않는다.
    if (t < 0.0f || u < 0.0f || u > 1.0f) result = false;

    *distance = t;

    return result;
}

/* `o`에서 `v` 방향으로 진행하는 광선이 중심점이 `c`이고 반지름이 `r`인 원과 만나는지 계산한다. */
static bool frComputeIntersectionRayCircle(Vector2 o, Vector2 v, Vector2 c, float r, float *distance) {
    bool result = true;
    
    Vector2 diff = frVec2Subtract(c, o);
    
    float dot = frVec2DotProduct(diff, v);
    
    float height_sqr = frVec2MagnitudeSqr(diff) - (dot * dot);
    float base_sqr = (r * r) - height_sqr;
    
    // 광선과 원은 서로 만나지 않는다.
    if (dot < 0.0f || base_sqr < 0) result = false;
    
    *distance = dot - sqrtf(base_sqr);
    
    return result;
}