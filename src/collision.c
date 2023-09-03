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

/* Includes ============================================================================= */

#include <float.h>

#include "ferox.h"

/* Typedefs ============================================================================= */

/* A structure that represents an edge of a convex polygon. */
typedef struct _frEdge {
    frVector2 data[2];
    int indexes[2];
} frEdge;

/* Private Function Prototypes ========================================================== */

/* 
    Clips `e` so that the dot product of each vertex in `e` 
    and `v` is greater than or equal to `dot`. 
*/
static void frClipEdge(frEdge *e, frVector2 v, float dot);

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` and `s2` are 'circle' collision shapes,
    then stores the collision information to `collision`.
*/
static bool frComputeCollisionCircles(
    const frShape *s1, frTransform tx1, 
    const frShape *s2, frTransform tx2,
    frCollision *collision
);

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` is a 'circle' collision shape and `s2` is a 'polygon' collision shape,
    then stores the collision information to `collision`.
*/
static bool frComputeCollisionCirclePoly(
    const frShape *s1, frTransform tx1, 
    const frShape *s2, frTransform tx2,
    frCollision *collision
);

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` and `s2` are 'polygon' collision shapes,
    then stores the collision information to `collision`.
*/
static bool frComputeCollisionPolys(
    const frShape *s1, frTransform tx1, 
    const frShape *s2, frTransform tx2,
    frCollision *collision
);

/* Returns the edge of `s` that is most perpendicular to `v`. */
static frEdge frGetContactEdge(const frShape *s, frTransform tx, frVector2 v);

/* Finds the axis of minimum penetration from `s1` to `s2`, then returns its index. */
static int frGetSeparatingAxisIndex(
    const frShape *s1, frTransform tx1, 
    const frShape *s2, frTransform tx2,
    float *depth
);

/* Finds the vertex farthest along `v`, then returns its index. */
static int frGetSupportPointIndex(
    const frVertices *vertices, 
    frTransform tx, frVector2 v
);

/* Public Functions ===================================================================== */

/* 
    Checks whether `s1` and `s2` are colliding,
    then stores the collision information to `collision`.
*/
bool frComputeCollision(
    const frShape *s1, frTransform tx1, 
    const frShape *s2, frTransform tx2,
    frCollision *collision
) {
    if (s1 == NULL || s2 == NULL) return false;

    frShapeType t1 = frGetShapeType(s1);
    frShapeType t2 = frGetShapeType(s2);

    if (t1 == FR_SHAPE_CIRCLE && t2 == FR_SHAPE_CIRCLE)
        return frComputeCollisionCircles(s1, tx1, s2, tx2, collision);
    else if ((t1 == FR_SHAPE_CIRCLE && t2 == FR_SHAPE_POLYGON) 
        || (t1 == FR_SHAPE_POLYGON && t2 == FR_SHAPE_CIRCLE))
        return frComputeCollisionCirclePoly(s1, tx1, s2, tx2, collision);
    else if (t1 == FR_SHAPE_POLYGON && t2 == FR_SHAPE_POLYGON)
        return frComputeCollisionPolys(s1, tx1, s2, tx2, collision);
    else return false;
}

/* Private Functions ==================================================================== */

/* 
    Clips `e` so that the dot product of each vertex in `e` 
    and `v` is greater than or equal to `dot`. 
*/
static void frClipEdge(frEdge *e, frVector2 v, float dot) {
    float dot1 = frVector2Dot(e->data[0], v) - dot;
    float dot2 = frVector2Dot(e->data[1], v) - dot;

    bool inside1 = (dot1 >= 0.0f), inside2 = (dot2 >= 0.0f);

    if (inside1 && inside2) return;

    frVector2 edgeVector = frVector2Subtract(e->data[1], e->data[0]);
    
    frVector2 midpoint = frVector2Add(
        e->data[0], frVector2ScalarMultiply(edgeVector, (dot1 / (dot1 - dot2)))
    );
    
    if (inside1 && !inside2)
        e->data[1] = midpoint;
    else if (!inside1 && inside2)
        e->data[0] = e->data[1], e->data[1] = midpoint;
}

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` and `s2` are 'circle' collision shapes,
    then stores the collision information to `collision`.
*/
static bool frComputeCollisionCircles(
    const frShape *s1, frTransform tx1, 
    const frShape *s2, frTransform tx2,
    frCollision *collision
) {
    frVector2 direction = frVector2Subtract(tx2.position, tx1.position);

    float radiusSum = frGetCircleRadius(s1) + frGetCircleRadius(s2);
    float magnitudeSqr = frVector2MagnitudeSqr(direction);

    if (radiusSum * radiusSum < magnitudeSqr) return false;

    if (collision != NULL) {
        float magnitude = sqrtf(magnitudeSqr);

        collision->direction = (magnitude > 0.0f)
            ? frVector2ScalarMultiply(direction, 1.0f / magnitude)
            : (frVector2) { .x = 1.0f };

        collision->contacts[0].edgeId = 0;

        collision->contacts[0].point = frVector2Transform(
            frVector2ScalarMultiply(collision->direction, frGetCircleRadius(s1)), tx1
        );

        collision->contacts[0].depth = (magnitude > 0.0f)
            ? radiusSum - magnitude
            : frGetCircleRadius(s1);

        collision->contacts[1] = collision->contacts[0];

        collision->count = 1;
    }

    return true;
}

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` is a 'circle' collision shape and `s2` is a 'polygon' collision shape,
    then stores the collision information to `collision`.
*/
static bool frComputeCollisionCirclePoly(
    const frShape *s1, frTransform tx1, 
    const frShape *s2, frTransform tx2,
    frCollision *collision
) {
    frShape *circle, *poly;
    frTransform circleTx, polyTx;

    if (frGetShapeType(s1) == FR_SHAPE_CIRCLE) {
        circle = (frShape *) s1, poly = (frShape *) s2;
        circleTx = tx1, polyTx = tx2;
    } else {
        circle = (frShape *) s2, poly = (frShape *) s1;
        circleTx = tx2, polyTx = tx1;
    }

    const frVertices *vertices = frGetPolygonVertices(poly);
    const frVertices *normals = frGetPolygonNormals(poly);
    
    /*
        NOTE: `txCenter` refers to the center of the 'circle' collision shape
        transformed to the local space of the 'polygon' collision shape.
    */
    frVector2 txCenter = frVector2Rotate(
        frVector2Subtract(circleTx.position, polyTx.position), 
        -polyTx.angle
    );

    float radius = frGetCircleRadius(circle), maxDot = -FLT_MAX;

    int maxIndex = -1;

    /*
        NOTE: This will find the edge of the 'polygon' collision shape
        closest to the center of the 'circle' collision shape.
    */
    for (int i = 0; i < vertices->count; i++) {
        float dot = frVector2Dot(
            normals->data[i], 
            frVector2Subtract(txCenter, vertices->data[i])
        );

        if (dot > radius) return false;
        
        if (maxDot < dot) maxDot = dot, maxIndex = i;
    }

    if (maxIndex < 0) return false;

    /*
        NOTE: Is the center of the 'circle' collision shape 
        inside the 'polygon' collision shape?
    */
    if (maxDot < 0.0f) {
        if (collision != NULL) {
            collision->direction = frVector2Negate(
                frVector2RotateTx(normals->data[maxIndex], polyTx)
            );

            frVector2 deltaPosition = frVector2Subtract(tx2.position, tx1.position);

            if (frVector2Dot(deltaPosition, collision->direction) < 0.0f)
                collision->direction = frVector2Negate(collision->direction);

            collision->contacts[0].edgeId = 0;

            collision->contacts[0].point = frVector2Add(
                circleTx.position,
                frVector2ScalarMultiply(collision->direction, radius)
            );

            collision->contacts[0].depth = radius - maxDot;

            collision->contacts[1] = collision->contacts[0];

            collision->count = 1;
        }
    } else {
        frVector2 v1 = (maxIndex > 0)
            ? vertices->data[maxIndex - 1]
            : vertices->data[vertices->count - 1];
        
        frVector2 v2 = vertices->data[maxIndex];

        frVector2 edgeVector = frVector2Subtract(v2, v1);

        frVector2 v1ToCenter = frVector2Subtract(txCenter, v1);
        frVector2 v2ToCenter = frVector2Subtract(txCenter, v2);

        float v1Dot = frVector2Dot(v1ToCenter, edgeVector);
        float v2Dot = frVector2Dot(v2ToCenter, frVector2Negate(edgeVector));

        /*
            NOTE: This means the center of the 'circle' collision shape
            does not lie on the line segment from `v1` to `v2`.
        */
        if (v1Dot <= 0.0f || v2Dot <= 0.0f) {
            frVector2 direction = (v1Dot <= 0.0f) ? v1ToCenter : v2ToCenter;

            float magnitudeSqr = frVector2MagnitudeSqr(direction);

            if (magnitudeSqr > radius * radius) return false;

            if (collision != NULL) {
                float magnitude = sqrtf(magnitudeSqr);

                collision->direction = (magnitude > 0.0f)
                    ? frVector2ScalarMultiply(
                        frVector2RotateTx(frVector2Negate(direction), polyTx), 
                        1.0f / magnitude
                    )
                    : FR_API_STRUCT_ZERO(frVector2);

                frVector2 deltaPosition = frVector2Subtract(tx2.position, tx1.position);

                if (frVector2Dot(deltaPosition, collision->direction) < 0.0f)
                    collision->direction = frVector2Negate(collision->direction);

                collision->contacts[0].edgeId = 0;

                collision->contacts[0].point = frVector2Transform(
                    frVector2ScalarMultiply(collision->direction, radius), circleTx
                );

                collision->contacts[0].depth = (magnitude > 0.0f)
                    ? radius - magnitude
                    : radius;

                collision->contacts[1] = collision->contacts[0];

                collision->count = 1;
            }
        } else {
            if (collision != NULL) {
                collision->direction = frVector2Negate(
                    frVector2RotateTx(normals->data[maxIndex], polyTx)
                );

                frVector2 deltaPosition = frVector2Subtract(tx2.position, tx1.position);

                if (frVector2Dot(deltaPosition, collision->direction) < 0.0f)
                    collision->direction = frVector2Negate(collision->direction);

                collision->contacts[0].edgeId = 0;

                collision->contacts[0].point = frVector2Add(
                    circleTx.position,
                    frVector2ScalarMultiply(collision->direction, radius)
                );

                collision->contacts[0].depth = radius - maxDot;

                collision->contacts[1] = collision->contacts[0];

                collision->count = 1;
            }
        }
    }

    return true;
}

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` and `s2` are 'polygon' collision shapes,
    then stores the collision information to `collision`.
*/
static bool frComputeCollisionPolys(
    const frShape *s1, frTransform tx1, 
    const frShape *s2, frTransform tx2,
    frCollision *collision
) {
    float maxDepth1 = FLT_MAX, maxDepth2 = FLT_MAX;

    int index1 = frGetSeparatingAxisIndex(s1, tx1, s2, tx2, &maxDepth1);

    if (maxDepth1 >= 0.0f) return false;
    
    int index2 = frGetSeparatingAxisIndex(s2, tx2, s1, tx1, &maxDepth2);

    if (maxDepth2 >= 0.0f) return false;

    if (collision != NULL) {
        frVector2 direction = (maxDepth1 > maxDepth2)
            ? frVector2RotateTx(frGetPolygonNormal(s1, index1), tx1)
            : frVector2RotateTx(frGetPolygonNormal(s2, index2), tx2);

        frVector2 deltaPosition = frVector2Subtract(tx2.position, tx1.position);

        if (frVector2Dot(deltaPosition, direction) < 0.0f)
            direction = frVector2Negate(direction);

        collision->direction = direction;

        frEdge edge1 = frGetContactEdge(s1, tx1, direction);
        frEdge edge2 = frGetContactEdge(s2, tx2, frVector2Negate(direction));

        frEdge refEdge = edge1, incEdge = edge2;

        frTransform refTx = tx1, incTx = tx2;

        frVector2 edgeVector1 = frVector2Subtract(edge1.data[1], edge1.data[0]);
        frVector2 edgeVector2 = frVector2Subtract(edge2.data[1], edge2.data[0]);

        const float edgeDot1 = frVector2Dot(edgeVector1, direction);
        const float edgeDot2 = frVector2Dot(edgeVector2, direction);

        bool incEdgeFlipped = false;

        if (fabsf(edgeDot1) > fabsf(edgeDot2)) {
            refEdge = edge2, incEdge = edge1;
            refTx = tx2, incTx = tx1;
            
            incEdgeFlipped = true;
        }

        frVector2 refEdgeVector = frVector2Normalize(
            frVector2Subtract(refEdge.data[1], refEdge.data[0])
        );

        const float refDot1 = frVector2Dot(refEdge.data[0], refEdgeVector);
        const float refDot2 = frVector2Dot(refEdge.data[1], refEdgeVector);

        frClipEdge(&incEdge, refEdgeVector, refDot1);
        frClipEdge(&incEdge, frVector2Negate(refEdgeVector), -refDot2);

        frVector2 refEdgeNormal = frVector2RightNormal(refEdgeVector);

        const float maxDepth = frVector2Dot(refEdge.data[0], refEdgeNormal);

        const float depth1 = frVector2Dot(incEdge.data[0], refEdgeNormal) - maxDepth;
        const float depth2 = frVector2Dot(incEdge.data[1], refEdgeNormal) - maxDepth;

        collision->contacts[0].edgeId = (!incEdgeFlipped) 
            ? FR_GEOMETRY_MAX_VERTEX_COUNT + incEdge.indexes[0]
            : incEdge.indexes[0];

        collision->contacts[1].edgeId = collision->contacts[0].edgeId;

        if (depth1 < 0.0f) {
            collision->contacts[0].point = collision->contacts[1].point = incEdge.data[1];
            collision->contacts[0].depth = collision->contacts[1].depth = depth2;

            collision->count = 1;
        } else if (depth2 < 0.0f) {
            collision->contacts[0].point = collision->contacts[1].point = incEdge.data[0];
            collision->contacts[0].depth = collision->contacts[1].depth = depth1;

            collision->count = 1;
        } else {
            collision->contacts[0].point = incEdge.data[0];
            collision->contacts[1].point = incEdge.data[1];

            collision->contacts[0].depth = depth1;
            collision->contacts[1].depth = depth2;

            collision->count = 2;
        }
    }

    return true;
}

/* Returns the edge of `s` that is most perpendicular to `v`. */
static frEdge frGetContactEdge(const frShape *s, frTransform tx, frVector2 v) {
    const frVertices *vertices = frGetPolygonVertices(s);

    int supportIndex = frGetSupportPointIndex(vertices, tx, v);

    int prevIndex = (supportIndex == 0) ? vertices->count - 1 : supportIndex - 1;
    int nextIndex = (supportIndex == vertices->count - 1) ? 0 : supportIndex + 1;

    frVector2 prevEdgeVector = frVector2Normalize(
        frVector2Subtract(vertices->data[supportIndex], vertices->data[prevIndex])
    );

    frVector2 nextEdgeVector = frVector2Normalize(
        frVector2Subtract(vertices->data[supportIndex], vertices->data[nextIndex])
    );

    v = frVector2Rotate(v, -tx.angle);

    if (frVector2Dot(prevEdgeVector, v) < frVector2Dot(nextEdgeVector, v)) {
        return (frEdge) { 
            .data = {
                frVector2Transform(vertices->data[prevIndex], tx),
                frVector2Transform(vertices->data[supportIndex], tx)
            },
            .indexes = { prevIndex, supportIndex }
        };
    } else {
        return (frEdge) {
            .data = {
                frVector2Transform(vertices->data[supportIndex], tx),
                frVector2Transform(vertices->data[nextIndex], tx)
            },
            .indexes = { supportIndex, nextIndex }
        };
    }
}

/* Finds the axis of minimum penetration, then returns its index. */
static int frGetSeparatingAxisIndex(
    const frShape *s1, frTransform tx1, 
    const frShape *s2, frTransform tx2,
    float *depth
) {
    const frVertices *vertices1 = frGetPolygonVertices(s1);
    const frVertices *vertices2 = frGetPolygonVertices(s2);

    const frVertices *normals1 = frGetPolygonNormals(s1);

    float maxDepth = -FLT_MAX;

    int maxIndex = -1;

    for (int i = 0; i < normals1->count; i++) {
        frVector2 vertex = frVector2Transform(vertices1->data[i], tx1);
        frVector2 normal = frVector2RotateTx(normals1->data[i], tx1);

        int supportIndex = frGetSupportPointIndex(vertices2, tx2, frVector2Negate(normal));

        if (supportIndex < 0) return supportIndex;

        frVector2 supportPoint = frVector2Transform(vertices2->data[supportIndex], tx2);

        float depth = frVector2Dot(normal, frVector2Subtract(supportPoint, vertex));

        if (maxDepth < depth) maxDepth = depth, maxIndex = i;
    }

    if (depth != NULL) *depth = maxDepth;

    return maxIndex;
}

/* Finds the vertex farthest along `v`, then returns its index. */
static int frGetSupportPointIndex(
    const frVertices *vertices, 
    frTransform tx, frVector2 v
) {
    float maxDot = -FLT_MAX;
    
    int maxIndex = -1;

    v = frVector2Rotate(v, -tx.angle);

    for (int i = 0; i < vertices->count; i++) {
        float dot = frVector2Dot(vertices->data[i], v);
        
        if (maxDot < dot) maxDot = dot, maxIndex = i;
    }

    return maxIndex;
}