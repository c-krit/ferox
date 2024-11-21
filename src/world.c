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

#include "external/ferox_utils.h"

#define STB_DS_IMPLEMENTATION
#include "external/stb_ds.h"

#include "ferox.h"

/* Typedefs ===============================================================> */

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
    frDynArray(frBody *) bodies;
    frRingBuffer(frContextNode) rbf;
    frSpatialHash *hash;
    frContactCacheEntry *cache;
    float accumulator, timestamp;
    frCollisionHandler handler;
    frVector2 gravity;
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
    frRaycastQueryFunc func;
    frWorld *world;
    frRay ray;
    void *ctx;
} frRaycastHashQueryCtx;

/* Private Function Prototypes ============================================> */

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frPreStepWorld()`. 
*/
static bool frPreStepHashQueryCallback(frContextNode ctx);

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frComputeRaycastForWorld()`.
*/
static bool frRaycastHashQueryCallback(frContextNode ctx);

/* Finds all pairs of bodies in `w` that are colliding. */
static void frPreStepWorld(frWorld *w);

/* 
    Clears the accumulated forces on each body in `w`, 
    then clears the spatial hash of `w`. 
*/
static void frPostStepWorld(frWorld *w);

/* Public Functions =======================================================> */

/* 
    Creates a world with the `gravity` vector and `cellSize` 
    for broad-phase collision detection.
*/
frWorld *frCreateWorld(frVector2 gravity, float cellSize) {
    frWorld *result = calloc(1, sizeof *result);

    result->gravity = gravity;
    result->hash = frCreateSpatialHash(cellSize);

    frSetDynArrayCapacity(result->bodies, FR_WORLD_MAX_OBJECT_COUNT);

    frInitRingBuffer(result->rbf, FR_WORLD_MAX_OBJECT_COUNT);

    return result;
}

/* Releases the memory allocated for `w`. */
void frReleaseWorld(frWorld *w) {
    if (w == NULL) return;

    for (int i = 0; i < frGetDynArrayLength(w->bodies); i++)
        frReleaseBody(frGetDynArrayValue(w->bodies, i));

    frReleaseSpatialHash(w->hash);

    frReleaseDynArray(w->bodies);
    frReleaseRingBuffer(w->rbf);

    hmfree(w->cache);

    free(w);
}

/* Erases all rigid bodies from `w`. */
void frClearWorld(frWorld *w) {
    if (w == NULL) return;

    frClearSpatialHash(w->hash);

    frSetDynArrayLength(w->bodies, 0);
}

/* Adds a rigid `b`ody to `w`. */
bool frAddBodyToWorld(frWorld *w, frBody *b) {
    if (w == NULL || b == NULL
        || frGetDynArrayLength(w->bodies) >= FR_WORLD_MAX_OBJECT_COUNT)
        return false;

    return frAddToRingBuffer(w->rbf,
                             ((frContextNode) { .id = FR_OPT_ADD_BODY,
                                                .ctx = b }));
}

/* Removes a rigid `b`ody from `w`. */
bool frRemoveBodyFromWorld(frWorld *w, frBody *b) {
    if (w == NULL || b == NULL) return false;

    return frAddToRingBuffer(w->rbf,
                             ((frContextNode) { .id = FR_OPT_REMOVE_BODY,
                                                .ctx = b }));
}

/* Checks if the given `b`ody is in `w`. */
bool frIsBodyInWorld(const frWorld *w, frBody *b) {
    if (w == NULL || b == NULL) return false;

    for (int i = 0; i < frGetDynArrayLength(w->bodies); i++)
        if (frGetDynArrayValue(w->bodies, i) == b) return true;

    return false;
}

/* Returns a rigid body at the given `i`ndex in `w`. */
frBody *frGetBodyInWorld(const frWorld *w, int i) {
    if (w == NULL || i < 0 || i >= frGetDynArrayLength(w->bodies)) return NULL;

    return frGetDynArrayValue(w->bodies, i);
}

/* Returns the number of rigid bodies in `w`. */
int frGetBodyCountInWorld(const frWorld *w) {
    return (w != NULL) ? frGetDynArrayLength(w->bodies) : 0;
}

/* Returns the gravity acceleration vector of `w`. */
frVector2 frGetWorldGravity(const frWorld *w) {
    return (w != NULL) ? w->gravity : frStructZero(frVector2);
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

    for (int i = 0; i < frGetDynArrayLength(w->bodies); i++) {
        frApplyGravityToBody(frGetDynArrayValue(w->bodies, i), w->gravity);

        frIntegrateForBodyVelocity(frGetDynArrayValue(w->bodies, i), dt);
    }

    int entryCount = hmlen(w->cache);

    for (int j = 0; j < entryCount; j++) {
        frBodyPair key = w->cache[j].key;

        const frCollision *value = &w->cache[j].value;

        for (int k = 0; k < value->count; k++)
            if (w->timestamp - value->contacts[k].timestamp > dt) {
                hmdel(w->cache, key);

                break;
            }
    }

    for (int j = 0; j < hmlen(w->cache); j++)
        frApplyAccumulatedImpulses(w->cache[j].key.first,
                                   w->cache[j].key.second,
                                   &w->cache[j].value);

    float inverseDt = 1.0f / dt;

    for (int i = 0; i < FR_WORLD_ITERATION_COUNT; i++)
        for (int j = 0; j < hmlen(w->cache); j++)
            frResolveCollision(w->cache[j].key.first,
                               w->cache[j].key.second,
                               &w->cache[j].value,
                               inverseDt);

    for (int i = 0; i < frGetDynArrayLength(w->bodies); i++)
        frIntegrateForBodyPosition(frGetDynArrayValue(w->bodies, i), dt);

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

    if (w->timestamp <= 0.0f) {
        w->timestamp = currentTime;

        return;
    }

    float elapsedTime = currentTime - w->timestamp;

    w->timestamp = currentTime, w->accumulator += elapsedTime;

    for (; w->accumulator >= dt; w->accumulator -= dt)
        frStepWorld(w, dt);
}

/* 
    Casts a `ray` against all objects in `w`, 
    then calls `func` for each object that collides with `ray`. 
*/
void frComputeWorldRaycast(frWorld *w,
                           frRay ray,
                           frRaycastQueryFunc func,
                           void *userData) {
    if (w == NULL || func == NULL) return;

    frClearSpatialHash(w->hash);

    for (int i = 0; i < frGetDynArrayLength(w->bodies); i++)
        frInsertIntoSpatialHash(w->hash,
                                frGetBodyAABB(frGetDynArrayValue(w->bodies, i)),
                                i);

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
                       &(frRaycastHashQueryCtx) { .ctx = userData,
                                                  .ray = ray,
                                                  .world = w,
                                                  .func = func });
}

/* Private Functions ======================================================> */

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frPreStepWorld()`. 
*/
static bool frPreStepHashQueryCallback(frContextNode queryResult) {
    frPreStepHashQueryCtx *queryCtx = queryResult.ctx;

    int firstIndex = queryCtx->bodyIndex, secondIndex = queryResult.id;

    if (firstIndex >= secondIndex) return false;

    frWorld *world = queryCtx->world;

    frBody *b1 = frGetDynArrayValue(world->bodies, firstIndex);
    frBody *b2 = frGetDynArrayValue(world->bodies, secondIndex);

    if (frGetBodyInverseMass(b1) + frGetBodyInverseMass(b2) <= 0.0f)
        return false;

    frBodyPair key = { .first = b1, .second = b2 };

    frCollision collision = { .count = 0 };

    if (!frComputeCollision(b1, b2, &collision)) {
        /*
            NOTE: `hmdel()` returns `0` if `key` is not 
            in `world->cache`!
        */

        hmdel(world->cache, key);

        return false;
    }

    frContactCacheEntry *entry = hmgetp_null(world->cache, key);

    const frShape *s1 = frGetBodyShape(b1), *s2 = frGetBodyShape(b2);

    for (int i = 0; i < collision.count; i++)
        collision.contacts[i].timestamp = world->timestamp;

    if (entry != NULL) {
        collision.friction = entry->value.friction;
        collision.restitution = entry->value.restitution;

        for (int i = 0; i < collision.count; i++) {
            int newIndex = i, oldIndex = -1;

            for (int j = 0; j < entry->value.count; j++) {
                int newContactId = collision.contacts[newIndex].id;
                int oldContactId = entry->value.contacts[j].id;

                if (newContactId == oldContactId) {
                    oldIndex = j;

                    break;
                }
            }

            if (oldIndex < 0) continue;

            const frContact *oldContact = &entry->value.contacts[oldIndex];

            frContact *newContact = &collision.contacts[newIndex];

            float oldNormalScalar = oldContact->cache.normalScalar;
            float oldTangentScalar = oldContact->cache.tangentScalar;

            newContact->cache.normalScalar = oldNormalScalar;
            newContact->cache.tangentScalar = oldTangentScalar;
        }
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
static bool frRaycastHashQueryCallback(frContextNode ctxNode) {
    frRaycastHashQueryCtx *queryCtx = ctxNode.ctx;

    const frBody *body = frGetDynArrayValue(queryCtx->world->bodies,
                                            ctxNode.id);

    frRaycastHit raycastHit = { .distance = 0.0f };

    if (!frComputeRaycast(body, queryCtx->ray, &raycastHit)) return false;

    queryCtx->func(raycastHit, queryCtx->ctx);

    return true;
}

/* Finds all pairs of bodies in `w` that are colliding. */
static void frPreStepWorld(frWorld *w) {
    for (int i = 0; i < frGetDynArrayLength(w->bodies); i++)
        frInsertIntoSpatialHash(w->hash,
                                frGetBodyAABB(frGetDynArrayValue(w->bodies, i)),
                                i);

    for (int i = 0; i < frGetDynArrayLength(w->bodies); i++)
        frQuerySpatialHash(w->hash,
                           frGetBodyAABB(frGetDynArrayValue(w->bodies, i)),
                           frPreStepHashQueryCallback,
                           &(frPreStepHashQueryCtx) { .world = w,
                                                      .bodyIndex = i });
}

/* 
    Clears the accumulated forces on each body in `w`, 
    then clears the spatial hash of `w`. 
*/
static void frPostStepWorld(frWorld *w) {
    frContextNode node = { .id = FR_OPT_UNKNOWN };

    while (frRemoveFromRingBuffer(w->rbf, &node)) {
        switch (node.id) {
            case FR_OPT_ADD_BODY:
                frDynArrayPush(w->bodies, node.ctx);

                break;

            case FR_OPT_REMOVE_BODY:
                for (int i = 0; i < frGetDynArrayLength(w->bodies); i++)
                    if (frGetDynArrayValue(w->bodies, i) == node.ctx) {
                        frDynArraySwap(frBody *,
                                       w->bodies,
                                       i,
                                       frGetDynArrayLength(w->bodies) - 1);

                        frSetDynArrayLength(w->bodies,
                                            frGetDynArrayLength(w->bodies) - 1);

                        break;
                    }

                break;

            default:
                break;
        }
    }

    for (int i = 0; i < frGetDynArrayLength(w->bodies); i++)
        frClearBodyForces(frGetDynArrayValue(w->bodies, i));

    frClearSpatialHash(w->hash);
}
