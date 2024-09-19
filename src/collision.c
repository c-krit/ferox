/*
    Copyright (c) 2021-2024 Jaedeok Kim <jdeokkim@protonmail.com>

    Permission is hereby granted, free of charge, to any person obtaining a 
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation 
    the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included 
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.
*/

/* Includes ================================================================ */

#include "ferox.h"

/* Typedefs ================================================================ */

/* A structure that represents an edge of a convex polygon. */
typedef struct frEdge_ {
    frVector2 data[3];
    int indexes[2];
    int count;
} frEdge;

/* Private Function Prototypes ============================================= */

/* 
    Clips `e` so that the dot product of each vertex in `e` 
    and `v` is greater than or equal to `dot`.
*/
static bool frClipEdge(frEdge *e, frVector2 v, float dot);

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` and `s2` are 'circle' collision shapes,
    then stores the collision information to `collision`.
*/
static bool frComputeCollisionCircles(const frShape *s1,
                                      frTransform tx1,
                                      const frShape *s2,
                                      frTransform tx2,
                                      frCollision *collision);

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` is a 'circle' collision shape and `s2` is a 'polygon' 
    collision shape, then stores the collision information to `collision`.
*/
static bool frComputeCollisionCirclePoly(const frShape *s1,
                                         frTransform tx1,
                                         const frShape *s2,
                                         frTransform tx2,
                                         frCollision *collision);

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` and `s2` are 'polygon' collision shapes,
    then stores the collision information to `collision`.
*/
static bool frComputeCollisionPolys(const frShape *s1,
                                    frTransform tx1,
                                    const frShape *s2,
                                    frTransform tx2,
                                    frCollision *collision);

/* Computes the intersection of a circle and a line. */
static bool frComputeIntersectionCircleLine(frVector2 center,
                                            float radius,
                                            frVector2 origin,
                                            frVector2 direction,
                                            float *distance);

/* Computes the intersection of two lines. */
static bool frComputeIntersectionLines(frVector2 origin1,
                                       frVector2 direction1,
                                       frVector2 origin2,
                                       frVector2 direction2,
                                       float *distance);

/* Returns the edge of `s` that is most perpendicular to `v`. */
static frEdge frGetContactEdge(const frShape *s, frTransform tx, frVector2 v);

/* 
    Finds the axis of minimum penetration from `s1` to `s2`,
    then returns its index.
*/
static int frGetSeparatingAxisIndex(const frShape *s1,
                                    frTransform tx1,
                                    const frShape *s2,
                                    frTransform tx2,
                                    float *depth);

/* Returns the index of the vertex farthest along `v`. */
static int
frGetSupportPointIndex(const frVertices *vertices, frTransform tx, frVector2 v);

/* Public Functions ======================================================== */

/* 
    Checks whether `b1` and `b2` are colliding,
    then stores the collision information to `collision`.
*/
bool frComputeCollision(frBody *b1, frBody *b2, frCollision *collision) {
    if (b1 == NULL || b2 == NULL) return false;

    const frShape *s1 = frGetBodyShape(b1);
    frTransform tx1 = frGetBodyTransform(b1);

    const frShape *s2 = frGetBodyShape(b2);
    frTransform tx2 = frGetBodyTransform(b2);

    frShapeType type1 = frGetShapeType(s1);
    frShapeType type2 = frGetShapeType(s2);

    if (type1 == FR_SHAPE_CIRCLE && type2 == FR_SHAPE_CIRCLE)
        return frComputeCollisionCircles(s1, tx1, s2, tx2, collision);
    else if ((type1 == FR_SHAPE_CIRCLE && type2 == FR_SHAPE_POLYGON)
             || (type1 == FR_SHAPE_POLYGON && type2 == FR_SHAPE_CIRCLE))
        return frComputeCollisionCirclePoly(s1, tx1, s2, tx2, collision);
    else if (type1 == FR_SHAPE_POLYGON && type2 == FR_SHAPE_POLYGON)
        return frComputeCollisionPolys(s1, tx1, s2, tx2, collision);
    else
        return false;
}

/* Casts a `ray` against `b`. */
bool frComputeRaycast(const frBody *b, frRay ray, frRaycastHit *raycastHit) {
    if (b == NULL) return false;

    ray.direction = frVector2Normalize(ray.direction);

    const frShape *s = frGetBodyShape(b);
    frTransform tx = frGetBodyTransform(b);

    frShapeType type = frGetShapeType(s);

    float distance = FLT_MAX;

    if (type == FR_SHAPE_CIRCLE) {
        bool intersects = frComputeIntersectionCircleLine(tx.position,
                                                          frGetCircleRadius(s),
                                                          ray.origin,
                                                          ray.direction,
                                                          &distance);

        bool result = (distance >= 0.0f) && (distance <= ray.maxDistance);

        if (raycastHit != NULL) {
            raycastHit->body = (frBody *) b;

            raycastHit->point = frVector2Add(
                ray.origin, frVector2ScalarMultiply(ray.direction, distance));

            raycastHit->normal = frVector2LeftNormal(
                frVector2Subtract(ray.origin, raycastHit->point));

            raycastHit->distance = distance;
            raycastHit->inside = (distance < 0.0f);
        }

        return result;
    } else if (type == FR_SHAPE_POLYGON) {
        const frVertices *vertices = frGetPolygonVertices(s);

        int intersectionCount = 0;

        float minDistance = FLT_MAX;

        for (int j = vertices->count - 1, i = 0; i < vertices->count;
             j = i, i++) {
            frVector2 v1 = frVector2Transform(vertices->data[i], tx);
            frVector2 v2 = frVector2Transform(vertices->data[j], tx);

            frVector2 edgeVector = frVector2Subtract(v1, v2);

            bool intersects = frComputeIntersectionLines(ray.origin,
                                                         ray.direction,
                                                         v2,
                                                         edgeVector,
                                                         &distance);

            if (intersects && distance <= ray.maxDistance) {
                if (minDistance > distance) {
                    minDistance = distance;

                    if (raycastHit != NULL) {
                        raycastHit->point =
                            frVector2Add(ray.origin,
                                         frVector2ScalarMultiply(ray.direction,
                                                                 minDistance));

                        raycastHit->normal = frVector2LeftNormal(edgeVector);
                    }
                }

                intersectionCount++;
            }
        }

        if (raycastHit != NULL) {
            raycastHit->body = (frBody *) b;
            raycastHit->inside = (intersectionCount & 1);
        }

        return (!(raycastHit->inside) && (intersectionCount > 0));
    } else {
        return false;
    }
}

/* Private Functions ======================================================= */

/* 
    Clips `e` so that the dot product of each vertex in `e` 
    and `v` is greater than or equal to `dot`. 
*/
static bool frClipEdge(frEdge *e, frVector2 v, float dot) {
    e->count = 0;

    float dot1 = frVector2Dot(e->data[0], v) - dot;
    float dot2 = frVector2Dot(e->data[1], v) - dot;

    if (dot1 >= 0.0f && dot2 >= 0.0f) {
        e->count = 2;

        return true;
    } else {
        frVector2 edgeVector = frVector2Subtract(e->data[1], e->data[0]);

        frVector2 midPoint = frVector2Add(
            e->data[0],
            frVector2ScalarMultiply(edgeVector, (dot1 / (dot1 - dot2))));

        if (dot1 > 0.0f && dot2 < 0.0f) {
            e->data[1] = midPoint, e->count = 2;

            return true;
        } else if (dot1 < 0.0f && dot2 > 0.0f) {
            e->data[0] = e->data[1], e->data[1] = midPoint, e->count = 2;

            return true;
        } else {
            return false;
        }
    }
}

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` and `s2` are 'circle' collision shapes,
    then stores the collision information to `collision`.
*/
static bool frComputeCollisionCircles(const frShape *s1,
                                      frTransform tx1,
                                      const frShape *s2,
                                      frTransform tx2,
                                      frCollision *collision) {
    frVector2 direction = frVector2Subtract(tx2.position, tx1.position);

    float radiusSum = frGetCircleRadius(s1) + frGetCircleRadius(s2);
    float magnitudeSqr = frVector2MagnitudeSqr(direction);

    if (radiusSum * radiusSum < magnitudeSqr) return false;

    if (collision != NULL) {
        float magnitude = sqrtf(magnitudeSqr);

        if (magnitude <= 0.0f)
            direction.x = 0.0f, direction.y = magnitude = FLT_EPSILON;

        collision->direction = frVector2ScalarMultiply(direction,
                                                       1.0f / magnitude);

        /* TODO: ... */

        collision->contacts[0].point =
            frVector2Transform(frVector2ScalarMultiply(collision->direction,
                                                       frGetCircleRadius(s1)),
                               tx1);

        collision->contacts[0].depth = radiusSum - magnitude;

        collision->contacts[1] = collision->contacts[0];

        collision->count = 1;
    }

    return true;
}

/* 
    Checks whether `s1` and `s2` are colliding,
    assuming `s1` is a 'circle' collision shape and `s2` is a 'polygon' 
    collision shape, then stores the collision information to `collision`.
*/
static bool frComputeCollisionCirclePoly(const frShape *s1,
                                         frTransform tx1,
                                         const frShape *s2,
                                         frTransform tx2,
                                         frCollision *collision) {
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
    frVector2 txCenter = frVector2Rotate(frVector2Subtract(circleTx.position,
                                                           polyTx.position),
                                         -polyTx.angle);

    float radius = frGetCircleRadius(circle), maxDot = -FLT_MAX;

    int maxIndex = -1;

    /*
        NOTE: This will find the edge of the 'polygon' collision shape
        closest to the center of the 'circle' collision shape.
    */
    for (int i = 0; i < vertices->count; i++) {
        float dot = frVector2Dot(normals->data[i],
                                 frVector2Subtract(txCenter,
                                                   vertices->data[i]));

        if (dot > radius) return false;

        if (maxDot < dot) maxDot = dot, maxIndex = i;
    }

    if (maxIndex < 0) return false;

    frVector2 deltaPosition = frVector2Subtract(tx2.position, tx1.position);

    /*
        NOTE: Is the center of the 'circle' collision shape 
        inside the 'polygon' collision shape?
    */
    if (maxDot < 0.0f) {
        if (collision != NULL) {
            collision->direction = frVector2Negate(
                frVector2RotateTx(normals->data[maxIndex], polyTx));

            if (frVector2Dot(deltaPosition, collision->direction) < 0.0f)
                collision->direction = frVector2Negate(collision->direction);

            /* TODO: ... */

            collision->contacts[0].point = frVector2Add(
                circleTx.position,
                frVector2ScalarMultiply(collision->direction, radius));

            collision->contacts[0].depth = radius - maxDot;

            collision->contacts[1] = collision->contacts[0];

            collision->count = 1;
        }
    } else {
        frVector2 v1 = (maxIndex > 0) ? vertices->data[maxIndex - 1]
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

            if (radius * radius < magnitudeSqr) return false;

            if (collision != NULL) {
                float magnitude = sqrtf(magnitudeSqr);

                if (magnitude <= 0.0f) magnitude = FLT_EPSILON;

                collision->direction = frVector2ScalarMultiply(
                        frVector2RotateTx(frVector2Negate(direction), polyTx),
                        1.0f / magnitude);

                if (frVector2Dot(deltaPosition, collision->direction) < 0.0f)
                    collision->direction = frVector2Negate(
                        collision->direction);

                /* TODO: ... */

                collision->contacts[0].point = frVector2Transform(
                    frVector2ScalarMultiply(collision->direction, radius),
                    circleTx);

                collision->contacts[0].depth = radius - magnitude;

                collision->contacts[1] = collision->contacts[0];

                collision->count = 1;
            }
        } else {
            // TODO: ...

            if (collision != NULL) {
                collision->direction = frVector2Negate(
                    frVector2RotateTx(normals->data[maxIndex], polyTx));

                if (frVector2Dot(deltaPosition, collision->direction) < 0.0f)
                    collision->direction = frVector2Negate(
                        collision->direction);

                /* TODO: ... */

                collision->contacts[0].point = frVector2Add(
                    circleTx.position,
                    frVector2ScalarMultiply(collision->direction, radius));

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
static bool frComputeCollisionPolys(const frShape *s1,
                                    frTransform tx1,
                                    const frShape *s2,
                                    frTransform tx2,
                                    frCollision *collision) {
    float maxDepth1 = FLT_MAX, maxDepth2 = FLT_MAX;

    int index1 = frGetSeparatingAxisIndex(s1, tx1, s2, tx2, &maxDepth1);

    if (maxDepth1 >= 0.0f) return false;

    int index2 = frGetSeparatingAxisIndex(s2, tx2, s1, tx1, &maxDepth2);

    if (maxDepth2 >= 0.0f) return false;

    if (collision != NULL) {
        frVector2 direction =
            (maxDepth1 > maxDepth2)
                ? frVector2RotateTx(frGetPolygonNormal(s1, index1), tx1)
                : frVector2RotateTx(frGetPolygonNormal(s2, index2), tx2);

        frVector2 deltaPosition = frVector2Subtract(tx2.position, tx1.position);

        if (frVector2Dot(deltaPosition, direction) < 0.0f)
            direction = frVector2Negate(direction);

        frEdge e1 = frGetContactEdge(s1, tx1, direction);
        frEdge e2 = frGetContactEdge(s2, tx2, frVector2Negate(direction));

        frEdge refEdge = e1, incEdge = e2;

        frTransform refTx = tx1, incTx = tx2;

        frVector2 edgeVector1 = frVector2Subtract(e1.data[1], e1.data[0]);
        frVector2 edgeVector2 = frVector2Subtract(e2.data[1], e2.data[0]);

        float edgeDot1 = frVector2Dot(edgeVector1, direction);
        float edgeDot2 = frVector2Dot(edgeVector2, direction);

        bool refEdgeFlipped = false;

        if (fabsf(edgeDot1) > fabsf(edgeDot2)) {
            refEdge = e2, incEdge = e1;
            refTx = tx2, incTx = tx1;

            refEdgeFlipped = true;
        }

        frVector2 refEdgeVector = frVector2Normalize(
            frVector2Subtract(refEdge.data[1], refEdge.data[0]));

        float refDot1 = frVector2Dot(refEdge.data[0], refEdgeVector);
        float refDot2 = frVector2Dot(refEdge.data[1], refEdgeVector);

        if (!frClipEdge(&incEdge, refEdgeVector, refDot1))
            return false;
        
        if (!frClipEdge(&incEdge, frVector2Negate(refEdgeVector), -refDot2))
            return false;

        frVector2 refEdgeNormal = frVector2RightNormal(refEdgeVector);

        float maxDepth = frVector2Dot(refEdge.data[2], refEdgeNormal);

        float depth1 = frVector2Dot(incEdge.data[0], refEdgeNormal) - maxDepth;
        float depth2 = frVector2Dot(incEdge.data[1], refEdgeNormal) - maxDepth;

        collision->direction = direction;

        // TODO: ...
        uint32_t bitMask = (refEdgeFlipped << 16) | (refEdge.indexes[0] << 8);

        collision->contacts[0].id = bitMask | incEdge.indexes[0];
        collision->contacts[1].id = bitMask | incEdge.indexes[1];

        if (depth1 < 0.0f) {
            collision->contacts[0].id = collision->contacts[1].id;

            collision->contacts[0].point = incEdge.data[1];
            collision->contacts[0].depth = depth2;

            collision->contacts[1] = collision->contacts[0];

            collision->count = 1;
        } else if (depth2 < 0.0f) {
            collision->contacts[0].point = incEdge.data[0];
            collision->contacts[0].depth = depth1;

            collision->contacts[1] = collision->contacts[0];

            collision->count = 1;
        } else {
            collision->contacts[0].point = incEdge.data[0];
            collision->contacts[0].depth = depth1;

            collision->contacts[1].point = incEdge.data[1];
            collision->contacts[1].depth = depth2;

            collision->count = 2;
        }
    }

    return true;
}

/* Computes the intersection of a circle and a line. */
static bool frComputeIntersectionCircleLine(frVector2 center,
                                            float radius,
                                            frVector2 origin,
                                            frVector2 direction,
                                            float *distance) {
    frVector2 originToCenter = frVector2Subtract(center, origin);

    float dot = frVector2Dot(originToCenter, direction);

    float heightSqr = frVector2MagnitudeSqr(originToCenter) - (dot * dot);
    float baseSqr = (radius * radius) - heightSqr;

    if (distance != NULL) *distance = dot - sqrtf(baseSqr);

    return (dot >= 0.0f && baseSqr >= 0.0f);
}

/* Computes the intersection of two lines. */
static bool frComputeIntersectionLines(frVector2 origin1,
                                       frVector2 direction1,
                                       frVector2 origin2,
                                       frVector2 direction2,
                                       float *distance) {
    float rXs = frVector2Cross(direction1, direction2);

    frVector2 qp = frVector2Subtract(origin2, origin1);

    float qpXs = frVector2Cross(qp, direction2);
    float qpXr = frVector2Cross(qp, direction1);

    if (rXs != 0.0f) {
        float inverseRxS = 1.0f / rXs;

        float t = qpXs * inverseRxS, u = qpXr * inverseRxS;

        if ((t >= 0.0f && t <= 1.0f) && (u >= 0.0f && u <= 1.0f)) {
            if (distance != NULL) *distance = t;

            return true;
        }

        return false;
    } else {
        if (qpXr != 0.0f) return 0;

        float rDr = frVector2Dot(direction1, direction1);
        float sDr = frVector2Dot(direction2, direction1);

        float inverseRdR = 1.0f / rDr;

        float qpDr = frVector2Dot(qp, direction1);

        float k, t0 = qpDr * inverseRdR, t1 = t0 + sDr * inverseRdR;

        if (sDr < 0.0f) k = t0, t0 = t1, t1 = k;

        if ((t0 < 0.0f && t1 == 0.0f) || (t0 == 1.0f && t1 > 1.0f)) {
            if (distance != NULL) *distance = (t0 == 1.0f);

            return 1;
        }

        return (t1 >= 0.0f && t0 <= 1.0f);
    }
}

/* Returns the edge of `s` that is most perpendicular to `v`. */
static frEdge frGetContactEdge(const frShape *s, frTransform tx, frVector2 v) {
    const frVertices *vertices = frGetPolygonVertices(s);

    int supportIndex = frGetSupportPointIndex(vertices, tx, v);

    int prevIndex = (supportIndex == 0) ? vertices->count - 1
                                        : supportIndex - 1;
    int nextIndex = (supportIndex == vertices->count - 1) ? 0
                                                          : supportIndex + 1;

    frVector2 prevEdgeVector = frVector2Normalize(
        frVector2Subtract(vertices->data[supportIndex],
                          vertices->data[prevIndex]));

    frVector2 nextEdgeVector = frVector2Normalize(
        frVector2Subtract(vertices->data[supportIndex],
                          vertices->data[nextIndex]));

    v = frVector2Rotate(v, -tx.angle);

    frVector2 supportVertex = frVector2Transform(vertices->data[supportIndex],
                                                 tx);

    if (frVector2Dot(prevEdgeVector, v) < frVector2Dot(nextEdgeVector, v)) {
        frVector2 prevVertex = frVector2Transform(vertices->data[prevIndex],
                                                  tx);

        return (frEdge) { .data = { prevVertex, supportVertex, supportVertex },
                          .indexes = { prevIndex, supportIndex },
                          .count = 2 };
    } else {
        frVector2 nextVertex = frVector2Transform(vertices->data[nextIndex],
                                                  tx);

        return (frEdge) { .data = { supportVertex, nextVertex, supportVertex },
                          .indexes = { supportIndex, nextIndex },
                          .count = 2 };
    }
}

/* Finds the axis of minimum penetration, then returns its index. */
static int frGetSeparatingAxisIndex(const frShape *s1,
                                    frTransform tx1,
                                    const frShape *s2,
                                    frTransform tx2,
                                    float *depth) {
    const frVertices *vertices1 = frGetPolygonVertices(s1);
    const frVertices *vertices2 = frGetPolygonVertices(s2);

    const frVertices *normals1 = frGetPolygonNormals(s1);

    float maxDepth = -FLT_MAX;

    int maxIndex = -1;

    for (int i = 0; i < normals1->count; i++) {
        frVector2 vertex = frVector2Transform(vertices1->data[i], tx1);
        frVector2 normal = frVector2RotateTx(normals1->data[i], tx1);

        int supportIndex = frGetSupportPointIndex(vertices2,
                                                  tx2,
                                                  frVector2Negate(normal));

        if (supportIndex < 0) return supportIndex;

        frVector2 supportPoint =
            frVector2Transform(vertices2->data[supportIndex], tx2);

        float depth = frVector2Dot(normal,
                                   frVector2Subtract(supportPoint, vertex));

        if (maxDepth < depth) maxDepth = depth, maxIndex = i;
    }

    if (depth != NULL) *depth = maxDepth;

    return maxIndex;
}

/* Returns the index of the vertex farthest along `v`. */
static int frGetSupportPointIndex(const frVertices *vertices,
                                  frTransform tx,
                                  frVector2 v) {
    float maxDot = -FLT_MAX;

    int maxIndex = -1;

    v = frVector2Rotate(v, -tx.angle);

    for (int i = 0; i < vertices->count; i++) {
        float dot = frVector2Dot(vertices->data[i], v);

        if (maxDot < dot) maxDot = dot, maxIndex = i;
    }

    return maxIndex;
}
