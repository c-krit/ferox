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

/* A structure that represents the context data for `frPreStepQueryCallback()`. */
typedef struct _frPreStepQueryContext {
    frWorld *world;
    int bodyIndex;
} frPreStepQueryContext;

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
    float accumulator, timestamp;
};

/* Private Function Prototypes ========================================================== */

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frPreStepWorld()`. 
*/
static void frPreStepQueryCallback(int otherIndex, void *ctx);

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

/* Sets the `gravity` acceleration vector of `w`. */
void frSetWorldGravity(frWorld *w, frVector2 gravity) {
    if (w != NULL) w->gravity = gravity;
}

/* Proceeds the simulation over the time step `dt`, in seconds. */
void frStepWorld(frWorld *w, float dt) {
    if (w == NULL || dt <= 0.0f) return;

    frPreStepWorld(w);

    for (int i = 0; i < arrlen(w->bodies); i++) {
        frApplyGravityToBody(w->bodies[i], w->gravity);

        frIntegrateForBodyVelocity(w->bodies[i], dt);
    }

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

/* Private Functions ==================================================================== */

/* 
    A callback function for `frQuerySpatialHash()` 
    that will be called during `frPreStepWorld()`. 
*/
static void frPreStepQueryCallback(int otherBodyIndex, void *ctx) {
    frPreStepQueryContext *queryCtx = ctx;
    
    if (otherBodyIndex <= queryCtx->bodyIndex) return;

    frBody *b1 = queryCtx->world->bodies[queryCtx->bodyIndex];
    frBody *b2 = queryCtx->world->bodies[otherBodyIndex];

    const frBodyPair key = { .first = b1, .second = b2 };

    frShape *s1 = frGetBodyShape(b1), *s2 = frGetBodyShape(b2);
    frTransform tx1 = frGetBodyTransform(b1), tx2 = frGetBodyTransform(b2);

    frCollision collision = { .count = 0 };

    if (!frComputeCollision(s1, tx1, s2, tx2, &collision)) {
        // NOTE: `hmdel()` returns `0` if `key` is not in `queryCtx->world->cache`!
        hmdel(queryCtx->world->cache, key);

        return;
    }

    collision.friction = 0.5f * (frGetShapeFriction(s1) + frGetShapeFriction(s2));
    collision.restitution = fminf(frGetShapeRestitution(s1), frGetShapeRestitution(s2));

    if (collision.friction <= 0.0f) collision.friction = 0.0f;
    if (collision.restitution <= 0.0f) collision.restitution = 0.0f;
    
    // TODO: ...
    hmputs(
        queryCtx->world->cache, 
        ((frContactCacheEntry) { 
            .key = key, 
            .value = collision 
        })
    );
}

/* Finds all pairs of bodies in `w` that are colliding. */
static void frPreStepWorld(frWorld *w) {
    for (int i = 0; i < arrlen(w->bodies); i++)
        frInsertToSpatialHash(w->hash, frGetBodyAABB(w->bodies[i]), i);

    for (int i = 0; i < arrlen(w->bodies); i++)
        frQuerySpatialHash(
            w->hash, 
            frGetBodyAABB(w->bodies[i]), 
            frPreStepQueryCallback, 
            &(frPreStepQueryContext) {
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