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

/* NOTE: `STB_DS_IMPLEMENTATION` is already defined in 'broad-phase.c' */
#include "external/stb_ds.h"

#include "ferox.h"

/* Typedefs ============================================================================= */

/* A structure that represents the key-value pair of the contact cache. */
typedef struct _frContactCacheEntry {
    frBodyPair key;
    frCollision value;
} frContactCacheEntry;

/* A structure that represents a simulation container. */
struct _frWorld {
    frVector2 gravity;
    frBody **bodies;
    frSpatialHash *hash;
    frContactCacheEntry *cache;
    frCollisionHandler handler;
    float accumulator, timestamp;
};

/* A structure that represents the context data for `frPreStepHashQueryCallback()`. */
typedef struct _frPreStepHashQueryCtx {
    frWorld *world;
    int bodyIndex;
} frPreStepHashQueryCtx;

/* A structure that represents the context data for `frRaycastHashQueryCallback()`. */
typedef struct _frRaycastHashQueryCtx {
    frRay ray;
    frWorld *world;
    frRaycastQueryFunc func;
} frRaycastHashQueryCtx;

/* Private Function Prototypes ========================================================== */

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frPreStepWorld()`. 
*/
static bool frPreStepHashQueryCallback(int otherIndex, void *ctx);

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frComputeRaycastForWorld()`.
*/
static bool frRaycastHashQueryCallback(int bodyIndex, void *ctx);

/* Finds all pairs of bodies in `w` that are colliding. */
static void frPreStepWorld(frWorld *w);

/* Clears the spatial hash for `w`. */
static void frPostStepWorld(frWorld *w);

/* Public Functions ===================================================================== */

/* 
    Creates a world with the `gravity` vector and `cellSize` 
    for broad-phase collision detection.
*/
frWorld *frCreateWorld(frVector2 gravity, float cellSize) {
    frWorld *result = calloc(1, sizeof *result);

    result->gravity = gravity;
    result->hash = frCreateSpatialHash(cellSize);

    arrsetcap(result->bodies, FR_WORLD_MAX_OBJECT_COUNT);

    return result;
}

/* Releases the memory allocated for `w`. */
void frReleaseWorld(frWorld *w) {
    if (w == NULL) return;

    for (int i = 0; i < arrlen(w->bodies); i++)
        frReleaseBody(w->bodies[i]);

    frReleaseSpatialHash(w->hash);

    arrfree(w->bodies), hmfree(w->cache);

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
    if (w == NULL || b == NULL || arrlen(w->bodies) >= FR_WORLD_MAX_OBJECT_COUNT)
        return false;

    arrput(w->bodies, b);

    return true;
}

/* Removes a rigid `b`ody from `w`. */
bool frRemoveBodyFromWorld(frWorld *w, frBody *b) {
    if (w == NULL || b == NULL) return false;
    
    for (int i = 0; i < arrlen(w->bodies); i++) {
        if (w->bodies[i] == b) {
            // NOTE: `O(1)` performance!
            arrdelswap(w->bodies, i);

            return true;
        }
    }
    
    return false;
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

    for (int i = 0; i < arrlen(w->cache); i++)
        if (w->handler.preStep != NULL) 
            w->handler.preStep(w->cache[i].key, &w->cache[i].value);

    for (int i = 0; i < arrlen(w->bodies); i++) {
        frApplyGravityToBody(w->bodies[i], w->gravity);

        frIntegrateForBodyVelocity(w->bodies[i], dt);
    }

    for (int i = 0; i < hmlen(w->cache); i++)
        frApplyAccumulatedImpulses(
            w->cache[i].key.first,
            w->cache[i].key.second,
            &w->cache[i].value
        );

    const float inverseDt = 1.0f / dt;

    for (int i = 0; i < FR_WORLD_ITERATION_COUNT; i++)
        for (int j = 0; j < hmlen(w->cache); j++)
            frResolveCollision(
                w->cache[j].key.first,
                w->cache[j].key.second,
                &w->cache[j].value,
                inverseDt
            );

    for (int i = 0; i < arrlen(w->bodies); i++)
        frIntegrateForBodyPosition(w->bodies[i], dt);

    for (int i = 0; i < arrlen(w->cache); i++)
        if (w->handler.postStep != NULL) 
            w->handler.postStep(w->cache[i].key, &w->cache[i].value);

    frPostStepWorld(w);
}

/* 
    Proceeds the simulation over the time step `dt`, in seconds,
    which will always run independent of the framerate.
*/
void frUpdateWorld(frWorld *w, float dt) {
    if (w == NULL || dt <= 0.0f) return;

    double currentTime = frGetCurrentTime();
    double elapsedTime = currentTime - w->timestamp;

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
        frInsertToSpatialHash(w->hash, frGetBodyAABB(w->bodies[i]), i);

    frVector2 minVertex = ray.origin, maxVertex = frVector2Add(
        ray.origin, 
        frVector2ScalarMultiply(
            frVector2Normalize(ray.direction), 
            ray.maxDistance
        )
    );

    frQuerySpatialHash(
        w->hash,
        (frAABB) {
            .x = fminf(minVertex.x, maxVertex.x),
            .y = fminf(minVertex.y, maxVertex.y),
            .width = fabsf(maxVertex.x - minVertex.x),
            .height = fabsf(maxVertex.y - minVertex.y)
        },
        frRaycastHashQueryCallback,
        &(frRaycastHashQueryCtx) {
            .ray = ray, .world = w, .func = func
        }
    );
}

/* Private Functions ==================================================================== */

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frPreStepWorld()`. 
*/
static bool frPreStepHashQueryCallback(int otherBodyIndex, void *ctx) {
    frPreStepHashQueryCtx *queryCtx = ctx;
    
    if (otherBodyIndex <= queryCtx->bodyIndex) return false;

    frBody *b1 = queryCtx->world->bodies[queryCtx->bodyIndex];
    frBody *b2 = queryCtx->world->bodies[otherBodyIndex];

    const frBodyPair key = { .first = b1, .second = b2 };

    frShape *s1 = frGetBodyShape(b1), *s2 = frGetBodyShape(b2);
    frTransform tx1 = frGetBodyTransform(b1), tx2 = frGetBodyTransform(b2);

    frCollision collision = { .count = 0 };

    if (!frComputeCollision(s1, tx1, s2, tx2, &collision)) {
        // NOTE: `hmdel()` returns `0` if `key` is not in `queryCtx->world->cache`!
        hmdel(queryCtx->world->cache, key);

        return false;
    }

    frContactCacheEntry *entry = hmgetp_null(queryCtx->world->cache, key);

    if (entry != NULL) {
        collision.friction = entry->value.friction;
        collision.restitution = entry->value.restitution;

        for (int i = 0; i < collision.count; i++) {
            int k = -1;

            for (int j = 0; j < entry->value.count; j++) {
                const int edgeId = entry->value.contacts[j].edgeId;

                if (collision.contacts[i].edgeId == edgeId) {
                    k = j;

                    break;
                }
            }

            if (k < 0) continue;

            const float accNormalScalar = entry->value.contacts[k].cache.normalScalar;
            const float accTangentScalar = entry->value.contacts[k].cache.tangentScalar;

            collision.contacts[i].cache.normalScalar = accNormalScalar;
            collision.contacts[i].cache.tangentScalar = accTangentScalar;
        }
    } else {
        collision.friction = frGetShapeFriction(s1) * frGetShapeFriction(s2);
        collision.restitution = fminf(frGetShapeRestitution(s1), frGetShapeRestitution(s2));

        if (collision.friction <= 0.0f) collision.friction = 0.0f;
        if (collision.restitution <= 0.0f) collision.restitution = 0.0f;
    }
    
    hmputs(
        queryCtx->world->cache, 
        ((frContactCacheEntry) { 
            .key = key, .value = collision 
        })
    );

    return true;
}

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frComputeRaycastForWorld()`.
*/
static bool frRaycastHashQueryCallback(int bodyIndex, void *ctx) {
    frRaycastHashQueryCtx *queryCtx = ctx;

    frRaycastHit raycastHit = { .distance = 0.0f };

    if (!frComputeRaycast(
        queryCtx->world->bodies[bodyIndex],
        queryCtx->ray,
        &raycastHit
    )) return false;

    queryCtx->func(raycastHit);

    return true;
}

/* Finds all pairs of bodies in `w` that are colliding. */
static void frPreStepWorld(frWorld *w) {
    for (int i = 0; i < arrlen(w->bodies); i++)
        frInsertToSpatialHash(w->hash, frGetBodyAABB(w->bodies[i]), i);

    for (int i = 0; i < arrlen(w->bodies); i++)
        frQuerySpatialHash(
            w->hash, 
            frGetBodyAABB(w->bodies[i]), 
            frPreStepHashQueryCallback, 
            &(frPreStepHashQueryCtx) {
                .world = w, .bodyIndex = i
            }
        );
}

/* Clears the spatial hash for `w`. */
static void frPostStepWorld(frWorld *w) {
    for (int i = 0; i < arrlen(w->bodies); i++)
        frClearBodyForces(w->bodies[i]);

    frClearSpatialHash(w->hash);
}