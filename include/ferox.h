/*
    Copyright (c) 2021-2023 Jaedeok Kim <jdeokkim@protonmail.com>

    Permission is hereby granted, free of charge, to any person obtaining a 
    copyof this software and associated documentation files (the "Software"),
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

#ifndef FEROX_H
#define FEROX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ================================================================ */

#define _USE_MATH_DEFINES
#include <math.h>

#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* Macros ================================================================== */

/* Compiler-specific attribute for a function that must be inlined. */
#ifdef _MSC_VER
    #define FR_API_INLINE __forceinline
#elif defined(__GNUC__)
    #if defined(__STRICT_ANSI__)
        #define FR_API_INLINE __inline__ __attribute__((always_inline))
    #else
        #define FR_API_INLINE inline __attribute__((always_inline))
    #endif
#else
    #define FR_API_INLINE inline
#endif

/* Empty-initializes the given object. */
#define FR_API_STRUCT_ZERO(T) ((T) { 0 })

/* User-Defined Macros ===================================================== */

// clang-format off

/* Defines the maximum number of vertices for a convex polygon. */
#define FR_GEOMETRY_MAX_VERTEX_COUNT  8

/* Defines how many pixels represent a unit of length (meter). */
#define FR_GEOMETRY_PIXELS_PER_UNIT   32.0f

/* Defines the 'bias factor' for the Baumgarte stabilization scheme. */
#define FR_WORLD_BAUMGARTE_FACTOR     0.2f

/* Defines the 'slop' for the Baumgarte stabilization scheme. */
#define FR_WORLD_BAUMGARTE_SLOP       0.01f

/* Defines the default gravity acceleration vector for a world. */
#define FR_WORLD_DEFAULT_GRAVITY      ((frVector2) { .y = 9.8f })

/* Defines the iteration count for the constraint solver. */
#define FR_WORLD_ITERATION_COUNT      12

/* Defines the maximum number of objects in a world. */
#define FR_WORLD_MAX_OBJECT_COUNT     2048

// clang-format on

/* Typedefs ================================================================ */

/* A structure that represents a two-dimensional vector. */
typedef struct frVector2_ {
    float x, y;
} frVector2;

/* A structure that represents an axis-aligned bounding box. */
typedef struct frAABB_ {
    float x, y, width, height;
} frAABB;

/* 
    A structure that represents a collision shape, 
    which can be attached to a rigid body.
*/
typedef struct frShape_ frShape;

/* A structure that represents a rigid body. */
typedef struct frBody_ frBody;

/* A structure that represents a simulation container. */
typedef struct frWorld_ frWorld;

/* (From 'broad-phase.c') ================================================== */

/* A structure that represents a spatial hash. */
typedef struct frSpatialHash_ frSpatialHash;

/* A callback function type for `frQuerySpatialHash()`. */
typedef bool (*frHashQueryFunc)(int index, void *ctx);

/* (From 'collision.c') ==================================================== */

/* A structure that represents the contact points of two colliding bodies. */
typedef struct frCollision_ {
    float friction;
    float restitution;
    frVector2 direction;
    struct {
        uint32_t id;
        frVector2 point;
        float depth;
        struct {
            float normalMass, normalScalar;
            float tangentMass, tangentScalar;
        } cache;
    } contacts[2];
    int count;
} frCollision;

/* A structure that represents a ray. */
typedef struct frRay_ {
    frVector2 origin;
    frVector2 direction;
    float maxDistance;
} frRay;

/* A struct that represents the information about a raycast hit. */
typedef struct frRaycastHit_ {
    frBody *body;
    frVector2 point;
    frVector2 normal;
    float distance;
    bool inside;
} frRaycastHit;

/* (From 'geometry.c') ===================================================== */

/* An enumeration that represents the type of a collision shape. */
typedef enum frShapeType_ {
    FR_SHAPE_UNKNOWN,
    FR_SHAPE_CIRCLE,
    FR_SHAPE_POLYGON
} frShapeType;

/* 
    A structure that represents the physical quantities 
    of a collision shape. 
*/
typedef struct frMaterial_ {
    float density;
    float friction;
    float restitution;
} frMaterial;

/* A structure that represents the vertices of a convex polygon. */
typedef struct frVertices_ {
    frVector2 data[FR_GEOMETRY_MAX_VERTEX_COUNT];
    int count;
} frVertices;

/* (From 'rigid-body.c') =================================================== */

/* An enumeration that represents the type of a rigid body. */
typedef enum frBodyType_ {
    FR_BODY_UNKNOWN,
    FR_BODY_STATIC,
    FR_BODY_KINEMATIC,
    FR_BODY_DYNAMIC
} frBodyType;

/* An enumeration that represents a property flag of a rigid body. */
typedef enum frBodyFlag_ {
    FR_FLAG_NONE,
    FR_FLAG_INFINITE_MASS,
    FR_FLAG_INFINITE_INERTIA
} frBodyFlag;

/* A data type that represents the property flags of a rigid body. */
typedef uint_fast8_t frBodyFlags;

/*
    A structure that represents the position of an object in meters,
    the rotation data of an object and the angle of an object in radians.
*/
typedef struct frTransform_ {
    frVector2 position;
    struct {
        float _sin, _cos;
    } rotation;
    float angle;
} frTransform;

/* (From 'world.c') ======================================================== */

/* A structure that represents a pair of two rigid bodies. */
typedef struct frBodyPair_ {
    frBody *first, *second;
} frBodyPair;

/* A callback function type for a collision event. */
typedef void (*frCollisionEventFunc)(frBodyPair key, frCollision *value);

/* A structure that represents the collision event callback functions. */
typedef struct frCollisionHandler_ {
    frCollisionEventFunc preStep, postStep;
} frCollisionHandler;

/* A callback function type for `frComputeRaycastForWorld()`. */
typedef void (*frRaycastQueryFunc)(frRaycastHit raycastHit);

/* Public Function Prototypes ============================================== */

/* (From 'broad-phase.c') ================================================== */

/* Creates a new spatial hash with the given `cellSize`. */
frSpatialHash *frCreateSpatialHash(float cellSize);

/* Releases the memory allocated for `sh`. */
void frReleaseSpatialHash(frSpatialHash *sh);

/* Erases all elements from `sh`. */
void frClearSpatialHash(frSpatialHash *sh);

/* Returns the cell size of `sh`. */
float frGetSpatialHashCellSize(const frSpatialHash *sh);

/* Inserts a `key`-`value` pair into `sh`. */
void frInsertIntoSpatialHash(frSpatialHash *sh, frAABB key, int value);

/* Query `sh` for any objects that overlap the given `aabb`. */
void frQuerySpatialHash(frSpatialHash *sh,
                        frAABB aabb,
                        frHashQueryFunc func,
                        void *ctx);

/* (From 'collision.c') ==================================================== */

/* 
    Checks whether `b1` and `b2` are colliding,
    then stores the collision information to `collision`.
*/
bool frComputeCollision(frBody *b1, frBody *b2, frCollision *collision);

/* Casts a `ray` against `b`. */
bool frComputeRaycast(const frBody *b, frRay ray, frRaycastHit *raycastHit);

/* (From 'geometry.c') ===================================================== */

/* Creates a 'circle' collision shape. */
frShape *frCreateCircle(frMaterial material, float radius);

/* Creates a 'rectangle' collision shape. */
frShape *frCreateRectangle(frMaterial material, float width, float height);

/* Creates a 'convex polygon' collision shape. */
frShape *frCreatePolygon(frMaterial material, const frVertices *vertices);

/* Releases the memory allocated for `s`. */
void frReleaseShape(frShape *s);

/* Returns the type of `s`. */
frShapeType frGetShapeType(const frShape *s);

/* Returns the material of `s`. */
frMaterial frGetShapeMaterial(const frShape *s);

/* Returns the density of `s`. */
float frGetShapeDensity(const frShape *s);

/* Returns the coefficient of friction of `s`. */
float frGetShapeFriction(const frShape *s);

/* Returns the coefficient of restitution of `s`. */
float frGetShapeRestitution(const frShape *s);

/* Returns the area of `s`. */
float frGetShapeArea(const frShape *s);

/* Returns the mass of `s`. */
float frGetShapeMass(const frShape *s);

/* Returns the moment of inertia of `s`. */
float frGetShapeInertia(const frShape *s);

/* Returns the AABB (Axis-Aligned Bounding Box) of `s`. */
frAABB frGetShapeAABB(const frShape *s, frTransform tx);

/* Returns the radius of `s`, assuming `s` is a 'circle' collision shape. */
float frGetCircleRadius(const frShape *s);

/* 
    Returns a vertex with the given `index` of `s`, 
    assuming `s` is a 'polygon' collision shape. 
*/
frVector2 frGetPolygonVertex(const frShape *s, int index);

/* Returns the vertices of `s`, assuming `s` is a 'polygon' collision shape. */
const frVertices *frGetPolygonVertices(const frShape *s);

/* 
    Returns a normal with the given `index` of `s`, 
    assuming `s` is a 'polygon' collision shape. 
*/
frVector2 frGetPolygonNormal(const frShape *s, int index);

/* Returns the normals of `s`, assuming `s` is a 'polygon' collision shape. */
const frVertices *frGetPolygonNormals(const frShape *s);

/* Sets the type of `s` to `type`. */
void frSetShapeType(frShape *s, frShapeType type);

/* Sets the `material` of `s`. */
void frSetShapeMaterial(frShape *s, frMaterial material);

/* Sets the `density` of `s`. */
void frSetShapeDensity(frShape *s, float density);

/* Sets the coefficient of `friction` of `s`. */
void frSetShapeFriction(frShape *s, float friction);

/* Sets the coefficient of `restitution` of `s`. */
void frSetShapeRestitution(frShape *s, float restitution);

/* Sets the `radius` of `s`, assuming `s` is a 'circle' collision shape. */
void frSetCircleRadius(frShape *s, float radius);

/* 
    Sets the `width` and `height` of `s`, assuming `s` is a 'rectangle'
    collision shape.
*/
void frSetRectangleDimensions(frShape *s, float width, float height);

/* Sets the `vertices` of `s`, assuming `s` is a 'polygon' collision shape. */
void frSetPolygonVertices(frShape *s, const frVertices *vertices);

/* (From 'rigid-body.c') =================================================== */

/* Creates a rigid body at `position`. */
frBody *frCreateBody(frBodyType type, frVector2 position);

/* Creates a rigid body at `position`, then attaches `s` to it. */
frBody *frCreateBodyFromShape(frBodyType type, frVector2 position, frShape *s);

/* Releases the memory allocated for `b`. */
void frReleaseBody(frBody *b);

/* Returns the type of `b`. */
frBodyType frGetBodyType(const frBody *b);

/* Returns the property flags of `b`. */
frBodyFlags frGetBodyFlags(const frBody *b);

/* Returns the collision shape of `b`. */
frShape *frGetBodyShape(const frBody *b);

/* Returns the transform of `b`. */
frTransform frGetBodyTransform(const frBody *b);

/* Returns the position of `b`. */
frVector2 frGetBodyPosition(const frBody *b);

/* Returns the angle of `b`, in radians. */
float frGetBodyAngle(const frBody *b);

/* Returns the mass of `b`. */
float frGetBodyMass(const frBody *b);

/* Returns the inverse mass of `b`. */
float frGetBodyInverseMass(const frBody *b);

/* Returns the moment of inertia of `b`. */
float frGetBodyInertia(const frBody *b);

/* Returns the inverse moment of inertia of `b`. */
float frGetBodyInverseInertia(const frBody *b);

/* Returns the gravity scale of `b`. */
float frGetBodyGravityScale(const frBody *b);

/* Returns the velocity of `b`. */
frVector2 frGetBodyVelocity(const frBody *b);

/* Returns the angular velocity of `b`. */
float frGetBodyAngularVelocity(const frBody *b);

/* Returns the AABB (Axis-Aligned Bounding Box) of `b`. */
frAABB frGetBodyAABB(const frBody *b);

/* Returns the user data of `b`. */
void *frGetBodyUserData(const frBody *b);

/* Sets the `type` of `b`. */
void frSetBodyType(frBody *b, frBodyType type);

/* Sets the property `flags` of `b`. */
void frSetBodyFlags(frBody *b, frBodyFlags flags);

/* 
    Attaches the collision `s`hape to `b`. If `s` is `NULL`, 
    it will detach the current collision shape from `b`.
*/
void frSetBodyShape(frBody *b, frShape *s);

/* Sets the transform of `b` to `tx`. */
void frSetBodyTransform(frBody *b, frTransform tx);

/* Sets the `position` of `b`. */
void frSetBodyPosition(frBody *b, frVector2 position);

/* Sets the `angle` of `b`, in radians. */
void frSetBodyAngle(frBody *b, float angle);

/* Sets the gravity `scale` of `b`. */
void frSetBodyGravityScale(frBody *b, float scale);

/* Sets the velocity of `b` to `v`. */
void frSetBodyVelocity(frBody *b, frVector2 v);

/* Sets the `angularVelocity` of `b`. */
void frSetBodyAngularVelocity(frBody *b, float angularVelocity);

/* Sets the user data of `b` to `ctx`. */
void frSetBodyUserData(frBody *b, void *ctx);

/* Checks if the given `point` lies inside `b`. */
bool frBodyContainsPoint(const frBody *b, frVector2 point);

/* Clears accumulated forces on `b`. */
void frClearBodyForces(frBody *b);

/* Applies a `force` at a `point` on `b`. */
void frApplyForceToBody(frBody *b, frVector2 point, frVector2 force);

/* Applies a gravity force to `b` with the `g`ravity acceleration vector. */
void frApplyGravityToBody(frBody *b, frVector2 g);

/* Applies an `impulse` at a `point` on `b`. */
void frApplyImpulseToBody(frBody *b, frVector2 point, frVector2 impulse);

/* Applies accumulated impulses to `b1` and `b2`. */
void frApplyAccumulatedImpulses(frBody *b1, frBody *b2, frCollision *ctx);

/* 
    Calculates the acceleration of `b` from the accumulated forces,
    then integrates the acceleration over `dt` to calculate the 
    velocity of `b`.
*/
void frIntegrateForBodyVelocity(frBody *b, float dt);

/* 
    Integrates the velocity of `b` over `dt` 
    to calculate the position of `b`. 
*/
void frIntegrateForBodyPosition(frBody *b, float dt);

/* Resolves the collision between `b1` and `b2`. */
void frResolveCollision(frBody *b1,
                        frBody *b2,
                        frCollision *ctx,
                        float inverseDt);

/* (From 'timer.c') ======================================================== */

/* Returns the current time of the monotonic clock, in seconds. */
float frGetCurrentTime(void);

/* (From 'world.c') ======================================================== */

/* 
    Creates a world with the `gravity` vector and `cellSize` 
    for broad-phase collision detection.
*/
frWorld *frCreateWorld(frVector2 gravity, float cellSize);

/* Releases the memory allocated for `w`. */
void frReleaseWorld(frWorld *w);

/* Erases all rigid bodies from `w`. */
void frClearWorld(frWorld *w);

/* Adds a rigid `b`ody to `w`. */
bool frAddBodyToWorld(frWorld *w, frBody *b);

/* Removes a rigid `b`ody from `w`. */
bool frRemoveBodyFromWorld(frWorld *w, frBody *b);

/* Returns a rigid body with the given `index` from `w`. */
frBody *frGetBodyFromWorld(const frWorld *w, int index);

/* Returns the number of rigid bodies in `w`. */
int frGetBodyCountForWorld(const frWorld *w);

/* Returns the gravity acceleration vector of `w`. */
frVector2 frGetWorldGravity(const frWorld *w);

/* Sets the collision event `handler` of `w`. */
void frSetWorldCollisionHandler(frWorld *w, frCollisionHandler handler);

/* Sets the `gravity` acceleration vector of `w`. */
void frSetWorldGravity(frWorld *w, frVector2 gravity);

/* Proceeds the simulation over the time step `dt`, in seconds. */
void frStepWorld(frWorld *w, float dt);

/* 
    Proceeds the simulation over the time step `dt`, in seconds,
    which will always run independent of the framerate.
*/
void frUpdateWorld(frWorld *w, float dt);

/* 
    Casts a `ray` against all objects in `w`, 
    then calls `func` for each object that collides with `ray`. 
*/
void frComputeRaycastForWorld(frWorld *w, frRay ray, frRaycastQueryFunc func);

/* Inline Functions ======================================================== */

/* Adds `v1` and `v2`. */
FR_API_INLINE frVector2 frVector2Add(frVector2 v1, frVector2 v2) {
    return (frVector2) { .x = v1.x + v2.x, .y = v1.y + v2.y };
}

/* Subtracts `v2` from `v1`. */
FR_API_INLINE frVector2 frVector2Subtract(frVector2 v1, frVector2 v2) {
    return (frVector2) { .x = v1.x - v2.x, .y = v1.y - v2.y };
}

/* Returns the negated vector of `v`. */
FR_API_INLINE frVector2 frVector2Negate(frVector2 v) {
    return (frVector2) { .x = -v.x, .y = -v.y };
}

/* Multiplies `v` by `k`. */
FR_API_INLINE frVector2 frVector2ScalarMultiply(frVector2 v, float k) {
    return (frVector2) { .x = v.x * k, .y = v.y * k };
}

/* Returns the dot product of `v1` and `v2`. */
FR_API_INLINE float frVector2Dot(frVector2 v1, frVector2 v2) {
    return (v1.x * v2.x) + (v1.y * v2.y);
}

/* Returns the magnitude of the cross product of `v1` and `v2`. */
FR_API_INLINE float frVector2Cross(frVector2 v1, frVector2 v2) {
    // NOTE: This is also known as the "perpendicular dot product."
    return (v1.x * v2.y) - (v1.y * v2.x);
}

/* Returns the squared magnitude of `v`. */
FR_API_INLINE float frVector2MagnitudeSqr(frVector2 v) {
    return (v.x * v.x) + (v.y * v.y);
}

/* Returns the magnitude of `v`. */
FR_API_INLINE float frVector2Magnitude(frVector2 v) {
    return sqrtf(frVector2MagnitudeSqr(v));
}

/* Returns the squared distance between `v1` and `v2`. */
FR_API_INLINE float frVector2DistanceSqr(frVector2 v1, frVector2 v2) {
    return (v2.x - v1.x) * (v2.x - v1.x) + (v2.y - v1.y) * (v2.y - v1.y);
}

/* Returns the distance between `v1` and `v2`. */
FR_API_INLINE float frVector2Distance(frVector2 v1, frVector2 v2) {
    return sqrtf(frVector2DistanceSqr(v1, v2));
}

/* Converts `v` to a unit vector. */
FR_API_INLINE frVector2 frVector2Normalize(frVector2 v) {
    const float magnitude = frVector2Magnitude(v);

    return (magnitude > 0.0f) ? frVector2ScalarMultiply(v, 1.0f / magnitude)
                              : v;
}

/* Returns the left normal vector of `v`. */
FR_API_INLINE frVector2 frVector2LeftNormal(frVector2 v) {
    return frVector2Normalize((frVector2) { .x = -v.y, .y = v.x });
}

/* Returns the right normal vector of `v`. */
FR_API_INLINE frVector2 frVector2RightNormal(frVector2 v) {
    return frVector2Normalize((frVector2) { .x = v.y, .y = -v.x });
}

/* Rotates `v` through the `angle` about the origin of a coordinate plane. */
FR_API_INLINE frVector2 frVector2Rotate(frVector2 v, float angle) {
    const float _sin = sinf(angle);
    const float _cos = cosf(angle);

    return (frVector2) { .x = v.x * _cos - v.y * _sin,
                         .y = v.x * _sin + v.y * _cos };
}

/* Rotates `v` through `tx` about the origin of a coordinate plane. */
FR_API_INLINE frVector2 frVector2RotateTx(frVector2 v, frTransform tx) {
    return (frVector2) { v.x * tx.rotation._cos - v.y * tx.rotation._sin,
                         v.x * tx.rotation._sin + v.y * tx.rotation._cos };
}

/* Transforms `v` through `tx` about the origin of a coordinate plane. */
FR_API_INLINE frVector2 frVector2Transform(frVector2 v, frTransform tx) {
    return (frVector2) {
        tx.position.x + (v.x * tx.rotation._cos - v.y * tx.rotation._sin),
        tx.position.y + (v.x * tx.rotation._sin + v.y * tx.rotation._cos)
    };
}

/* Returns the angle between `v1` and `v2`, in radians. */
FR_API_INLINE float frVector2Angle(frVector2 v1, frVector2 v2) {
    return atan2f(v2.y, v2.x) - atan2f(v1.y, v1.x);
}

/*
    Returns ​a negative integer value if `v1, `v2` and `v3` form 
    a clockwise angle, a positive integer value if `v1, `v2` and `v3` form
    a counter-clockwise angle and zero if `v1, `v2` and `v3` are collinear.
*/
FR_API_INLINE int
frVector2CounterClockwise(frVector2 v1, frVector2 v2, frVector2 v3) {
    /*
       `v1`
        *
         \
          \
           \
            *-----------*
           `v2`        `v3`
    */

    const float lhs = (v2.y - v1.y) * (v3.x - v1.x);
    const float rhs = (v3.y - v1.y) * (v2.x - v1.x);

    // NOTE: Compares the slopes of two line equations.
    return (lhs > rhs) - (lhs < rhs);
}

/* Converts each component of `v` (in pixels) to units. */
FR_API_INLINE frVector2 frVector2PixelsToUnits(frVector2 v) {
    return (FR_GEOMETRY_PIXELS_PER_UNIT > 0.0f)
               ? frVector2ScalarMultiply(v, 1.0f / FR_GEOMETRY_PIXELS_PER_UNIT)
               : FR_API_STRUCT_ZERO(frVector2);
}

/* Converts each component of `v` (in units) to pixels. */
FR_API_INLINE frVector2 frVector2UnitsToPixels(frVector2 v) {
    return (FR_GEOMETRY_PIXELS_PER_UNIT > 0.0f)
               ? frVector2ScalarMultiply(v, FR_GEOMETRY_PIXELS_PER_UNIT)
               : FR_API_STRUCT_ZERO(frVector2);
}

/* Converts `k` (in pixels) to units. */
FR_API_INLINE float frPixelsToUnits(float k) {
    return (FR_GEOMETRY_PIXELS_PER_UNIT > 0.0f)
               ? (k / FR_GEOMETRY_PIXELS_PER_UNIT)
               : 0.0f;
}

/* Converts `k` (in units) to pixels. */
FR_API_INLINE float frUnitsToPixels(float k) {
    return (FR_GEOMETRY_PIXELS_PER_UNIT > 0.0f)
               ? (k * FR_GEOMETRY_PIXELS_PER_UNIT)
               : 0.0f;
}

#ifdef __cplusplus
}
#endif

#endif  // `FEROX_H`
