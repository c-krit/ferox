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

/* NOTE: `STB_DS_IMPLEMENTATION` is already defined in 'broad-phase.c' */
#include "external/stb_ds.h"

#include "ferox.h"

/* Typedefs ================================================================ */

/* A structure that represents the type of an operation for a world. */
typedef enum frWorldOpType_ {
    FR_OPT_UNKNOWN,
    FR_OPT_ADD_BODY,
    FR_OPT_REMOVE_BODY
} frWorldOpType;

/* A structure that represents the key-value pair of the contact cache. */
typedef struct frContactCacheEntry_ {
    frBodyPair key;
    frCollision value;
} frContactCacheEntry;

/* A structure that represents a simulation container. */
struct frWorld_ {
    frVector2 gravity;
    frBody **bodies;
    frRingBuffer *rbf;
    frSpatialHash *hash;
    frContactCacheEntry *cache;
    frCollisionHandler handler;
    float accumulator, timestamp;
};

/* 
    A structure that represents the context data 
    for `frPreStepHashQueryCallback()`. 
*/
typedef struct frPreStepHashQueryCtx_ {
    frWorld *world;
    int bodyIndex;
} frPreStepHashQueryCtx;

/*
    A structure that represents the context data 
    for `frRaycastHashQueryCallback()`.
*/
typedef struct frRaycastHashQueryCtx_ {
    frRay ray;
    frWorld *world;
    frRaycastQueryFunc func;
} frRaycastHashQueryCtx;

/* Private Function Prototypes ============================================= */

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frPreStepWorld()`. 
*/
static bool frPreStepHashQueryCallback(frIndexedData arg);

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frComputeRaycastForWorld()`.
*/
static bool frRaycastHashQueryCallback(frIndexedData arg);

/* Finds all pairs of bodies in `w` that are colliding. */
static void frPreStepWorld(frWorld *w);

/* 
    Clears the accumulated forces on each body in `w`, 
    then clears the spatial hash of `w`. 
*/
static void frPostStepWorld(frWorld *w);

/* Public Functions ======================================================== */

/* 
    Creates a world with the `gravity` vector and `cellSize` 
    for broad-phase collision detection.
*/
frWorld *frCreateWorld(frVector2 gravity, float cellSize) {
    frWorld *result = calloc(1, sizeof *result);

    result->gravity = gravity;
    result->rbf = frCreateRingBuffer(FR_WORLD_MAX_OBJECT_COUNT);
    result->hash = frCreateSpatialHash(cellSize);

    arrsetcap(result->bodies, FR_WORLD_MAX_OBJECT_COUNT);

    return result;
}

/* Releases the memory allocated for `w`. */
void frReleaseWorld(frWorld *w) {
    if (w == NULL) return;

    for (int i = 0; i < arrlen(w->bodies); i++)
        frReleaseBody(w->bodies[i]);

    arrfree(w->bodies), hmfree(w->cache);

    frReleaseSpatialHash(w->hash);
    frReleaseRingBuffer(w->rbf);

    free(w);
}

/* Erases all rigid bodies from `w`. */
void frClearWorld(frWorld *w) {
    if (w == NULL) return;

    frClearSpatialHash(w->hash);

    arrsetlen(w->bodies, 0);
}

/* Adds a rigid `b`ody to `w`. */
bool frAddBodyToWorld(frWorld *w, frBody *b) {
    if (w == NULL || b == NULL
        || arrlen(w->bodies) >= FR_WORLD_MAX_OBJECT_COUNT)
        return false;

    return frAddValueToRingBuffer(w->rbf,
                                  (frIndexedData) { .idx = FR_OPT_ADD_BODY,
                                                    .data = b });
}

/* Removes a rigid `b`ody from `w`. */
bool frRemoveBodyFromWorld(frWorld *w, frBody *b) {
    if (w == NULL || b == NULL) return false;

    return frAddValueToRingBuffer(w->rbf,
                                  (frIndexedData) { .idx = FR_OPT_REMOVE_BODY,
                                                    .data = b });
}

/* Returns a rigid body with the given `index` from `w`. */
frBody *frGetBodyFromWorld(const frWorld *w, int index) {
    if (w == NULL || index < 0 || index >= arrlen(w->bodies)) return NULL;

    return w->bodies[index];
}

/* Returns the number of rigid bodies in `w`. */
int frGetBodyCountForWorld(const frWorld *w) {
    return (w != NULL) ? arrlen(w->bodies) : 0;
}

/* Returns the gravity acceleration vector of `w`. */
frVector2 frGetWorldGravity(const frWorld *w) {
    return (w != NULL) ? w->gravity : FR_API_STRUCT_ZERO(frVector2);
}

/* Sets the collision event `handler` of `w`. */
void frSetWorldCollisionHandler(frWorld *w, frCollisionHandler handler) {
    if (w != NULL) w->handler = handler;
}

/* Sets the `gravity` acceleration vector of `w`. */
void frSetWorldGravity(frWorld *w, frVector2 gravity) {
    if (w != NULL) w->gravity = gravity;
}

/* Proceeds the simulation over the time step `dt`, in seconds. */
void frStepWorld(frWorld *w, float dt) {
    if (w == NULL || dt <= 0.0f) return;

    frPreStepWorld(w);

    for (int j = 0; j < hmlen(w->cache); j++) {
        frCollision *collision = &w->cache[j].value;

        if (w->handler.preStep != NULL && collision->count > 0)
            w->handler.preStep(w->cache[j].key, collision);
    }

    for (int i = 0; i < arrlen(w->bodies); i++) {
        frApplyGravityToBody(w->bodies[i], w->gravity);

        frIntegrateForBodyVelocity(w->bodies[i], dt);
    }

    for (int j = 0; j < hmlen(w->cache); j++)
        frApplyAccumulatedImpulses(w->cache[j].key.first,
                                   w->cache[j].key.second,
                                   &w->cache[j].value);

    const float inverseDt = 1.0f / dt;

    for (int i = 0; i < FR_WORLD_ITERATION_COUNT; i++)
        for (int j = 0; j < hmlen(w->cache); j++)
            frResolveCollision(w->cache[j].key.first,
                               w->cache[j].key.second,
                               &w->cache[j].value,
                               inverseDt);

    for (int i = 0; i < arrlen(w->bodies); i++)
        frIntegrateForBodyPosition(w->bodies[i], dt);

    for (int j = 0; j < hmlen(w->cache); j++) {
        frCollision *collision = &w->cache[j].value;

        if (w->handler.postStep != NULL && collision->count > 0)
            w->handler.postStep(w->cache[j].key, collision);
    }

    frPostStepWorld(w);
}

/* 
    Proceeds the simulation over the time step `dt`, in seconds,
    which will always run independent of the framerate.
*/
void frUpdateWorld(frWorld *w, float dt) {
    if (w == NULL || dt <= 0.0f) return;

    float currentTime = frGetCurrentTime();
    float elapsedTime = currentTime - w->timestamp;

    w->timestamp = currentTime, w->accumulator += elapsedTime;

    for (; w->accumulator >= dt; w->accumulator -= dt)
        frStepWorld(w, dt);
}

/* 
    Casts a `ray` against all objects in `w`, 
    then calls `func` for each object that collides with `ray`. 
*/
void frComputeRaycastForWorld(frWorld *w, frRay ray, frRaycastQueryFunc func) {
    if (w == NULL || func == NULL) return;

    frClearSpatialHash(w->hash);

    for (int i = 0; i < arrlen(w->bodies); i++)
        frInsertIntoSpatialHash(w->hash, frGetBodyAABB(w->bodies[i]), i);

    frVector2 minVertex = ray.origin,
              maxVertex = frVector2Add(
                  ray.origin,
                  frVector2ScalarMultiply(frVector2Normalize(ray.direction),
                                          ray.maxDistance));

    frQuerySpatialHash(w->hash,
                       (frAABB) { .x = fminf(minVertex.x, maxVertex.x),
                                  .y = fminf(minVertex.y, maxVertex.y),
                                  .width = fabsf(maxVertex.x - minVertex.x),
                                  .height = fabsf(maxVertex.y - minVertex.y) },
                       frRaycastHashQueryCallback,
                       &(frRaycastHashQueryCtx) {
                           .ray = ray, .world = w, .func = func });
}

/* Private Functions ======================================================= */

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frPreStepWorld()`. 
*/
static bool frPreStepHashQueryCallback(frIndexedData arg) {
    frPreStepHashQueryCtx *queryCtx = arg.data;

    if (arg.idx <= queryCtx->bodyIndex) return false;

    frBody *b1 = queryCtx->world->bodies[queryCtx->bodyIndex];
    frBody *b2 = queryCtx->world->bodies[arg.idx];

    if (frGetBodyInverseMass(b1) + frGetBodyInverseMass(b2) <= 0.0f)
        return false;

    frBodyPair key = { .first = b1, .second = b2 };

    frShape *s1 = frGetBodyShape(b1), *s2 = frGetBodyShape(b2);

    frCollision collision = { .count = 0 };

    if (!frComputeCollision(b1, b2, &collision)) {
        /*
            NOTE: `hmdel()` returns `0` if `key` is not 
            in `queryCtx->world->cache`!
        */

        hmdel(queryCtx->world->cache, key);

        return false;
    }

    frContactCacheEntry *entry = hmgetp_null(queryCtx->world->cache, key);

    if (entry != NULL) {
        collision.friction = entry->value.friction;
        collision.restitution = entry->value.restitution;

        /* TODO: ... */
    } else {
        collision.friction = 0.5f
                             * (frGetShapeFriction(s1)
                                + frGetShapeFriction(s2));
        collision.restitution = fminf(frGetShapeRestitution(s1),
                                      frGetShapeRestitution(s2));

        if (collision.friction < 0.0f) collision.friction = 0.0f;
        if (collision.restitution < 0.0f) collision.restitution = 0.0f;
    }

    hmputs(queryCtx->world->cache,
           ((frContactCacheEntry) { .key = key, .value = collision }));

    return true;
}

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frComputeRaycastForWorld()`.
*/
static bool frRaycastHashQueryCallback(frIndexedData arg) {
    frRaycastHashQueryCtx *queryCtx = arg.data;

    frRaycastHit raycastHit = { .distance = 0.0f };

    if (!frComputeRaycast(queryCtx->world->bodies[arg.idx],
                          queryCtx->ray,
                          &raycastHit))
        return false;

    queryCtx->func(raycastHit);

    return true;
}

/* Finds all pairs of bodies in `w` that are colliding. */
static void frPreStepWorld(frWorld *w) {
    frIndexedData value = { .idx = FR_OPT_UNKNOWN };

    while (frRemoveValueFromRingBuffer(w->rbf, &value)) {
        switch (value.idx) {
            case FR_OPT_ADD_BODY:
                arrput(w->bodies, value.data);

                break;

            case FR_OPT_REMOVE_BODY:
                for (int i = 0; i < arrlen(w->bodies); i++) {
                    if (w->bodies[i] == value.data) {
                        arrdelswap(w->bodies, i);

                        break;
                    }
                }

                break;

            default:
                break;
        }
    }

    for (int i = 0; i < arrlen(w->bodies); i++)
        frInsertIntoSpatialHash(w->hash, frGetBodyAABB(w->bodies[i]), i);

    for (int i = 0; i < arrlen(w->bodies); i++)
        frQuerySpatialHash(w->hash,
                           frGetBodyAABB(w->bodies[i]),
                           frPreStepHashQueryCallback,
                           &(frPreStepHashQueryCtx) { .world = w,
                                                      .bodyIndex = i });
}

/* 
    Clears the accumulated forces on each body in `w`, 
    then clears the spatial hash of `w`. 
*/
static void frPostStepWorld(frWorld *w) {
    for (int i = 0; i < arrlen(w->bodies); i++)
        frClearBodyForces(w->bodies[i]);

    frClearSpatialHash(w->hash);
}
