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

/* Includes ===============================================================> */

#include "ferox.h"

/* Typedefs ===============================================================> */

/* A union that represents the internal data of a collision shape. */
typedef union frShapeData_ {
    struct {
        float radius;
    } circle;
    struct {
        frVertices vertices, normals;
    } polygon;
} frShapeData;

/* 
    A structure that represents a collision shape, 
    which can be attached to a rigid body.
*/
struct frShape_ {
    frShapeType type;
    frShapeData data;
    frMaterial material;
    float area;
};

/* Private Function Prototypes ============================================> */

/* 
    Computes the convex hull for the given `input` points 
    with the gift wrapping (a.k.a. Jarvis march) algorithm.
*/
static void frJarvisMarch(const frVertices *input, frVertices *output);

/* Public Functions =======================================================> */

/* Creates a 'circle' collision shape. */
frShape *frCreateCircle(frMaterial material, float radius) {
    if (radius <= 0.0f) return NULL;

    frShape *result = calloc(1, sizeof *result);

    result->type = FR_SHAPE_CIRCLE;
    result->material = material;

    frSetCircleRadius(result, radius);

    return result;
}

/* Creates a 'rectangle' collision shape. */
frShape *frCreateRectangle(frMaterial material, float width, float height) {
    if (width <= 0.0f || height <= 0.0f) return NULL;

    frShape *result = calloc(1, sizeof *result);

    result->type = FR_SHAPE_POLYGON;
    result->material = material;

    float halfWidth = 0.5f * width, halfHeight = 0.5f * height;

    // NOTE: https://en.cppreference.com/w/c/language/compound_literal
    frSetPolygonVertices(result,
                         &(const frVertices) {
                             .data = { { .x = -halfWidth, .y = -halfHeight },
                                       { .x = -halfWidth, .y = halfHeight },
                                       { .x = halfWidth, .y = halfHeight },
                                       { .x = halfWidth, .y = -halfHeight } },
                             .count = 4 });

    return result;
}

/* Creates a 'convex polygon' collision shape. */
frShape *frCreatePolygon(frMaterial material, const frVertices *vertices) {
    if (vertices == NULL || vertices->count <= 0) return NULL;

    frShape *result = calloc(1, sizeof *result);

    result->type = FR_SHAPE_POLYGON;
    result->material = material;

    frSetPolygonVertices(result, vertices);

    return result;
}

/* Releases the memory allocated by `s`. */
void frReleaseShape(frShape *s) {
    free(s);
}

/* Returns the type of `s`. */
frShapeType frGetShapeType(const frShape *s) {
    return (s != NULL) ? s->type : FR_SHAPE_UNKNOWN;
}

/* Returns the material of `s`. */
frMaterial frGetShapeMaterial(const frShape *s) {
    return (s != NULL) ? s->material : FR_API_STRUCT_ZERO(frMaterial);
}

/* Returns the density of `s`. */
float frGetShapeDensity(const frShape *s) {
    return (s != NULL) ? s->material.density : 0.0f;
}

/* Returns the coefficient of friction of `s`. */
float frGetShapeFriction(const frShape *s) {
    return (s != NULL) ? s->material.friction : 0.0f;
}

/* Returns the coefficient of restitution of `s`. */
float frGetShapeRestitution(const frShape *s) {
    return (s != NULL) ? s->material.restitution : 0.0f;
}

/* Returns the area of `s`. */
float frGetShapeArea(const frShape *s) {
    return (s != NULL) ? s->area : 0.0f;
}

/* Returns the mass of `s`. */
float frGetShapeMass(const frShape *s) {
    return (s != NULL) ? s->material.density * s->area : 0.0f;
}

/* Returns the moment of inertia of `s`. */
float frGetShapeInertia(const frShape *s) {
    if (s == NULL || s->material.density <= 0.0f) return 0.0f;

    if (s->type == FR_SHAPE_CIRCLE) {
        float radius = s->data.circle.radius;

        return 0.5f * frGetShapeMass(s) * (radius * radius);
    } else if (s->type == FR_SHAPE_POLYGON) {
        float numerator = 0.0f, denominator = 0.0f;

        int vertexCount = s->data.polygon.vertices.count;

        // NOTE: https://en.wikipedia.org/wiki/List_of_moments_of_inertia
        for (int j = vertexCount - 1, i = 0; i < vertexCount; j = i, i++) {
            frVector2 v1 = s->data.polygon.vertices.data[j];
            frVector2 v2 = s->data.polygon.vertices.data[i];

            float cross = frVector2Cross(v1, v2),
                  dotSum = (frVector2Dot(v1, v1) + frVector2Dot(v1, v2)
                            + frVector2Dot(v2, v2));

            numerator += (cross * dotSum), denominator += cross;
        }

        return s->material.density * (numerator / (6.0f * denominator));
    } else {
        return 0.0f;
    }
}

/* Returns the AABB (Axis-Aligned Bounding Box) of `s`. */
frAABB frGetShapeAABB(const frShape *s, frTransform tx) {
    frAABB result = FR_API_STRUCT_ZERO(frAABB);

    if (s != NULL) {
        if (s->type == FR_SHAPE_CIRCLE) {
            result.x = tx.position.x - s->data.circle.radius;
            result.y = tx.position.y - s->data.circle.radius;

            result.width = result.height = 2.0f * s->data.circle.radius;
        } else if (s->type == FR_SHAPE_POLYGON) {
            frVector2 minVertex = { .x = FLT_MAX, .y = FLT_MAX };
            frVector2 maxVertex = { .x = -FLT_MAX, .y = -FLT_MAX };

            for (int i = 0; i < s->data.polygon.vertices.count; i++) {
                frVector2 v =
                    frVector2Transform(s->data.polygon.vertices.data[i], tx);

                if (minVertex.x > v.x) minVertex.x = v.x;
                if (minVertex.y > v.y) minVertex.y = v.y;

                if (maxVertex.x < v.x) maxVertex.x = v.x;
                if (maxVertex.y < v.y) maxVertex.y = v.y;
            }

            float deltaX = maxVertex.x - minVertex.x;
            float deltaY = maxVertex.y - minVertex.y;

            result.x = minVertex.x;
            result.y = minVertex.y;

            result.width = deltaX;
            result.height = deltaY;
        }
    }

    return result;
}

/* Returns the radius of `s`, assuming `s` is a 'circle' collision shape. */
float frGetCircleRadius(const frShape *s) {
    return (frGetShapeType(s) == FR_SHAPE_CIRCLE) ? s->data.circle.radius
                                                  : 0.0f;
}

/* 
    Returns a vertex with the given `index` of `s`, 
    assuming `s` is a 'polygon' collision shape. 
*/
frVector2 frGetPolygonVertex(const frShape *s, int index) {
    if (frGetShapeType(s) != FR_SHAPE_POLYGON || index < 0
        || index >= s->data.polygon.vertices.count)
        return FR_API_STRUCT_ZERO(frVector2);

    return s->data.polygon.vertices.data[index];
}

/* Returns the vertices of `s`, assuming `s` is a 'polygon' collision shape. */
const frVertices *frGetPolygonVertices(const frShape *s) {
    return (frGetShapeType(s) == FR_SHAPE_POLYGON) ? &(s->data.polygon.vertices)
                                                   : NULL;
}

/* 
    Returns a normal with the given `index` of `s`, 
    assuming `s` is a 'polygon' collision shape. 
*/
frVector2 frGetPolygonNormal(const frShape *s, int index) {
    if (frGetShapeType(s) != FR_SHAPE_POLYGON || index < 0
        || index >= s->data.polygon.normals.count)
        return FR_API_STRUCT_ZERO(frVector2);

    return s->data.polygon.normals.data[index];
}

/* Returns the normals of `s`, assuming `s` is a 'polygon' collision shape. */
const frVertices *frGetPolygonNormals(const frShape *s) {
    return (frGetShapeType(s) == FR_SHAPE_POLYGON) ? &(s->data.polygon.normals)
                                                   : NULL;
}

/* Sets the type of `s` to `type`. */
void frSetShapeType(frShape *s, frShapeType type) {
    if (s != NULL) s->type = type;
}

/* Sets the `material` of `s`. */
void frSetShapeMaterial(frShape *s, frMaterial material) {
    if (s != NULL) s->material = material;
}

/* Sets the `density` of `s`. */
void frSetShapeDensity(frShape *s, float density) {
    if (s != NULL) s->material.density = density;
}

/* Sets the coefficient of `friction` of `s`. */
void frSetShapeFriction(frShape *s, float friction) {
    if (s != NULL) s->material.friction = friction;
}

/* Sets the coefficient of `restitution` of `s`. */
void frSetShapeRestitution(frShape *s, float restitution) {
    if (s != NULL) s->material.restitution = restitution;
}

/* Sets the `radius` of `s`, assuming `s` is a 'circle' collision shape. */
void frSetCircleRadius(frShape *s, float radius) {
    if (s == NULL || s->type != FR_SHAPE_CIRCLE) return;

    s->data.circle.radius = radius;

    s->area = M_PI * (radius * radius);
}

/* 
    Sets the `width` and `height` of `s`, assuming `s` is a 'rectangle'
    collision shape.
*/
void frSetRectangleDimensions(frShape *s, float width, float height) {
    if (s == NULL || width <= 0.0f || height <= 0.0f) return;

    float halfWidth = 0.5f * width, halfHeight = 0.5f * height;

    // NOTE: https://en.cppreference.com/w/c/language/compound_literal
    frSetPolygonVertices(s,
                         &(const frVertices) {
                             .data = { { .x = -halfWidth, .y = -halfHeight },
                                       { .x = -halfWidth, .y = halfHeight },
                                       { .x = halfWidth, .y = halfHeight },
                                       { .x = halfWidth, .y = -halfHeight } },
                             .count = 4 });
}

/* Sets the `vertices` of `s`, assuming `s` is a 'polygon' collision shape. */
void frSetPolygonVertices(frShape *s, const frVertices *vertices) {
    if (s == NULL || vertices == NULL || vertices->count <= 0) return;

    frVertices newVertices = { .count = 0 };

    frJarvisMarch(vertices, &newVertices);

    {
        s->data.polygon.vertices.count = newVertices.count;
        s->data.polygon.normals.count = newVertices.count;

        for (int i = 0; i < newVertices.count; i++)
            s->data.polygon.vertices.data[i] = newVertices.data[i];

        for (int j = newVertices.count - 1, i = 0; i < newVertices.count;
             j = i, i++)
            s->data.polygon.normals.data[i] = frVector2LeftNormal(
                frVector2Subtract(s->data.polygon.vertices.data[i],
                                  s->data.polygon.vertices.data[j]));
    }

    float twiceAreaSum = 0.0f;

    for (int i = 0; i < s->data.polygon.vertices.count - 1; i++) {
        /*
            NOTE: Divides the convex hull into multiple triangles,
            then computes the area for each triangle.
        */

        float twiceArea = frVector2Cross(
            frVector2Subtract(s->data.polygon.vertices.data[i],
                              s->data.polygon.vertices.data[0]),
            frVector2Subtract(s->data.polygon.vertices.data[i + 1],
                              s->data.polygon.vertices.data[0]));

        twiceAreaSum += twiceArea;
    }

    s->area = fabsf(0.5f * twiceAreaSum);
}

/* Private Functions ======================================================> */

/* 
    Computes the convex hull for the given `input` points 
    with the gift wrapping (a.k.a. Jarvis march) algorithm.
*/
static void frJarvisMarch(const frVertices *input, frVertices *output) {
    if (input == NULL || output == NULL || input->count < 3) return;

    /* 
        NOTE: Since the `input` size is most likely to be small 
        (less than 128?), we do not need advanced convex hull 
        algorithms like Graham scan, Quickhull, etc.
    */

    int lowestIndex = 0;

    for (int i = 1; i < input->count; i++)
        if (input->data[lowestIndex].x > input->data[i].x) lowestIndex = i;

    output->count = 0, output->data[output->count++] = input->data[lowestIndex];

    int currentIndex = lowestIndex, nextIndex = currentIndex;

    for (;;) {
        for (int i = 0; i < input->count; i++) {
            if (i == currentIndex) continue;

            nextIndex = i;

            break;
        }

        for (int i = 0; i < input->count; i++) {
            if (i == currentIndex || i == nextIndex) continue;

            int direction = frVector2CounterClockwise(input->data[currentIndex],
                                                      input->data[i],
                                                      input->data[nextIndex]);

            if (direction < 0) continue;

            float toCandidate = frVector2DistanceSqr(input->data[currentIndex],
                                                     input->data[i]);

            float toNext = frVector2DistanceSqr(input->data[currentIndex],
                                                input->data[nextIndex]);

            if (direction != 0 || (direction == 0 && (toCandidate > toNext)))
                nextIndex = i;
        }

        if (nextIndex == lowestIndex) break;

        currentIndex = nextIndex;

        output->data[output->count++] = input->data[nextIndex];
    }
}
