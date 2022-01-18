/*
    Copyright (c) 2021-2022 jdeokkim

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
#include "stb_ds.h"

/* | `world` 모듈 자료형 정의... | */

/* 물리 법칙이 존재하는 세계를 나타내는 구조체. */
typedef struct frWorld {
    Vector2 gravity;
    frBody **bodies;
    frSpatialHash *hash;
    frCollision *collisions;
    frCollisionHandler handler;
    double accumulator;
    double timestamp;
    int *queries;
} frWorld;

/* | `world` 모듈 함수... | */

/* 세계 `world`의 업데이트 이전 작업을 실행한다. */
static void frPreUpdateWorld(frWorld *world);

/* 세계 `world`의 업데이트 이후 작업을 실행한다. */
static void frPostUpdateWorld(frWorld *world);

/* 세계 `world`를 시간 `dt` (단위: ms)만큼 업데이트한다. */
static void frUpdateWorld(frWorld *world, double dt);

/* 중력 가속도가 `gravity`이고 경계 범위가 `bounds`인 세계를 생성한다. */ 
frWorld *frCreateWorld(Vector2 gravity, Rectangle bounds) {
    frWorld *result = calloc(1, sizeof(frWorld));
    
    result->gravity = gravity;
    result->hash = frCreateSpatialHash(bounds, FR_BROADPHASE_CELL_SIZE);
    result->timestamp = frGetCurrentTime();
    
    arrsetcap(result->bodies, FR_WORLD_MAX_BODY_COUNT);
    arrsetcap(result->collisions, FR_WORLD_MAX_BODY_COUNT * FR_WORLD_MAX_BODY_COUNT);
    arrsetcap(result->queries, FR_WORLD_MAX_BODY_COUNT);
    
    return result;
}

/* 세계 `world`에 할당된 메모리를 해제한다. */
void frReleaseWorld(frWorld *world) {
    if (world == NULL) return;
    
    for (int i = 0; i < arrlen(world->bodies); i++) {
        frBody *b = world->bodies[i];
        frShape *s = frGetBodyShape(b);

        frReleaseShape(s);
        frReleaseBody(b);
    }
    
    frReleaseSpatialHash(world->hash);
    
    arrfree(world->bodies);
    arrfree(world->collisions);
    arrfree(world->queries);
    
    free(world);
}

/* 세계 `world`에 강체 `b`를 추가한다. */
bool frAddToWorld(frWorld *world, frBody *b) {
    if (world == NULL || arrlen(world->bodies) >= FR_WORLD_MAX_BODY_COUNT) 
        return false;
    
    arrput(world->bodies, b);
    
    return true;
}

/* 세계 `world`의 모든 강체를 제거한다. */
void frClearWorld(frWorld *world) {
    if (world == NULL) return;
    
    frClearSpatialHash(world->hash);
    
    arrdeln(world->bodies, 0, arrlen(world->bodies));
    arrdeln(world->collisions, 0, arrlen(world->collisions));
}

/* 세계 `world`에서 강체 `b`를 제거한다. */
bool frRemoveFromWorld(frWorld *world, frBody *b) {
    if (world == NULL || b == NULL) return false;
    
    for (int i = 0; i < arrlen(world->bodies); i++) {
        if (world->bodies[i] == b) {
            arrdeln(world->bodies, i, 1);
            return true;
        }
    }
    
    return false;
}

/* 구조체 `frWorld`의 크기를 반환한다. */
size_t frGetWorldStructSize(void) {
    return sizeof(frWorld);
}

/* 세계 `world`에서 인덱스가 `index`인 강체의 메모리 주소를 반환한다. */
frBody *frGetWorldBody(frWorld *world, int index) {
    return (world != NULL && index >= 0 && index < arrlen(world->bodies)) 
        ? world->bodies[index]
        : NULL;
}

/* 세계 `world`의 충돌 핸들러를 반환한다. */
frCollisionHandler frGetWorldCollisionHandler(frWorld *world) {
    return (world != NULL) ? world->handler : FR_STRUCT_ZERO(frCollisionHandler);
}

/* 세계 `world`의 강체 배열의 크기를 반환한다. */
int frGetWorldBodyCount(frWorld *world) {
    return (world != NULL) ? arrlen(world->bodies) : 0;
}

/* 세계 `world`의 경계 범위를 반환한다. */
Rectangle frGetWorldBounds(frWorld *world) {
    return (world != NULL) 
        ? frGetSpatialHashBounds(world->hash) 
        : FR_STRUCT_ZERO(Rectangle);
}

/* 세계 `world`의 공간 해시맵을 반환한다. */
frSpatialHash *frGetWorldSpatialHash(frWorld *world){
    return (world != NULL) ? world->hash : NULL;
}

/* 세계 `world`의 중력 가속도를 반환한다. */
Vector2 frGetWorldGravity(frWorld *world) {
    return (world != NULL) ? world->gravity : FR_STRUCT_ZERO(Vector2);
}

/* 강체 `b`가 세계 `world`의 경계 범위 안에 있는지 확인한다. */
bool frIsInWorldBounds(frWorld *world, frBody *b) {
    if (world == NULL || b == NULL) return false;

    return CheckCollisionRecs(frGetBodyAABB(b), frGetWorldBounds(world));
}

/* 세계 `world`의 경계 범위를 `bounds`로 설정한다. */
void frSetWorldBounds(frWorld *world, Rectangle bounds) {
    if (world != NULL) frSetSpatialHashBounds(world->hash, bounds);
}

/* 세계 `world`의 충돌 핸들러를 `handler`로 설정한다. */
void frSetWorldCollisionHandler(frWorld *world, frCollisionHandler handler) {
    if (world != NULL) world->handler = handler;
}

/* 세계 `world`의 중력 가속도를 `gravity`로 설정한다. */
void frSetWorldGravity(frWorld *world, Vector2 gravity) {
    if (world != NULL) world->gravity = gravity;
}

/* 세계 `world`의 시간을 `dt` (단위: ms)만큼 흐르게 한다. */
void frSimulateWorld(frWorld *world, double dt) {
    if (world == NULL || dt == 0.0f) return;
    
    double current_time = frGetCurrentTime();
    double elapsed_time = frGetTimeDifference(current_time, world->timestamp);

    world->accumulator += elapsed_time;
    world->timestamp = current_time;
    
    if (world->accumulator > FR_WORLD_ACCUMULATOR_LIMIT)
        world->accumulator = FR_WORLD_ACCUMULATOR_LIMIT;
    
    for (; world->accumulator >= dt; world->accumulator -= dt)
        frUpdateWorld(world, dt);
}

/* 세계 `world`의 모든 강체에 광선을 투사한다. */
int frComputeWorldRaycast(frWorld *world, frRay ray, frRaycastHit *hits) {
    int result = 0;

    if (world == NULL || hits == NULL) return result;

    for (int i = 0; i < arrlen(world->bodies); i++) 
        frAddToSpatialHash(world->hash, frGetBodyAABB(world->bodies[i]), i);

    Vector2 p1 = ray.origin, p2 = frVec2Add(
        ray.origin, 
        frVec2ScalarMultiply(
            frVec2Normalize(ray.direction), 
            ray.max_distance
        )
    );

    Rectangle rec = (Rectangle) {
        .x = FR_NUMBER_MIN(p1.x, p2.x),
        .y = FR_NUMBER_MIN(p1.y, p2.y),
        .width = FR_NUMBER_MAX(p2.x - p1.x, p1.x - p2.x),
        .height = FR_NUMBER_MAX(p2.y - p1.y, p1.y - p2.y)
    };

    frQuerySpatialHash(world->hash, rec, &world->queries);

    for (int i = 0; i < arrlen(world->queries); i++) {
        frRaycastHit raycast = frComputeBodyRaycast(world->bodies[world->queries[i]], ray);
        
        if (raycast.check) 
            hits[result++] = raycast;
    }

    frClearSpatialHash(world->hash);

    return result;
}

/* 세계 `world`의 업데이트 이전 작업을 실행한다. */
static void frPreUpdateWorld(frWorld *world) {
    for (int i = 0; i < arrlen(world->bodies); i++)
        frAddToSpatialHash(world->hash, frGetBodyAABB(world->bodies[i]), i);
    
    for (int i = 0; i < arrlen(world->bodies); i++) {
        frQuerySpatialHash(
            world->hash, 
            frGetBodyAABB(world->bodies[i]), 
            &world->queries
        );
        
        for (int k = 0; k < arrlen(world->queries); k++) {
            int j = world->queries[k];
            
            if (j <= i) continue;
            
            frBody *b1 = world->bodies[i];
            frBody *b2 = world->bodies[j];
            
            frCollision collision = frComputeCollision(
                frGetBodyShape(b1),
                frGetBodyTransform(b1),
                frGetBodyShape(b2),
                frGetBodyTransform(b2)
            );
            
            if (collision.check) {
                collision._bodies[0] = b1;
                collision._bodies[1] = b2;
                
                arrput(world->collisions, collision);
            }
        }
        
        arrdeln(world->queries, 0, arrlen(world->queries));
    }
}

/* 세계 `world`의 업데이트 이후 작업을 실행한다. */
static void frPostUpdateWorld(frWorld *world) {
    for (int i = 0; i < arrlen(world->bodies); i++)
        frClearBodyForces(world->bodies[i]);
    
    arrdeln(world->collisions, 0, arrlen(world->collisions));
    
    frClearSpatialHash(world->hash);
}

/* 세계 `world`를 시간 `dt` (단위: ms)만큼 업데이트한다. */
static void frUpdateWorld(frWorld *world, double dt) {
    if (world == NULL || world->hash == NULL || world->bodies == NULL) return;

    frPreUpdateWorld(world);
    
    // 강체 사이의 충돌 해결 직전에 사전 정의된 함수를 호출한다. 
    for (int i = 0; i < arrlen(world->collisions); i++) {
        frCollisionCallback pre_solve_cb = world->handler.pre_solve;
        
        if (pre_solve_cb != NULL) pre_solve_cb(&world->collisions[i]);
    }
    
    // 강체에 중력을 적용하고, 강체의 속도와 각속도를 계산한다.
    for (int i = 0; i < arrlen(world->bodies); i++) {
        frApplyGravity(world->bodies[i], world->gravity);
        frIntegrateForBodyVelocities(world->bodies[i], dt);
    }
    
    // 순차적으로 충격량을 반복 적용하여, 두 강체 사이의 충돌을 해결한다.
    for (int i = 0; i < FR_WORLD_MAX_ITERATIONS; i++) {
        for (int j = 0; j < arrlen(world->collisions); j++) {
            frBody *b1 = world->collisions[j]._bodies[0];
            frBody *b2 = world->collisions[j]._bodies[1];
            
            frResolveCollision(b1, b2, world->collisions[j]);
        }
    }
    
    // 강체의 현재 위치를 계산한다.
    for (int i = 0; i < arrlen(world->bodies); i++)
        frIntegrateForBodyPosition(world->bodies[i], dt);
    
    // 강체의 위치를 적절하게 보정한다.
    for (int i = 0; i < arrlen(world->collisions); i++) {
        frBody *b1 = world->collisions[i]._bodies[0];
        frBody *b2 = world->collisions[i]._bodies[1];
        
        frCorrectBodyPositions(b1, b2, world->collisions[i]);
    }
    
    // 강체 사이의 충돌 해결 직후에 사전 정의된 함수를 호출한다. 
    for (int i = 0; i < arrlen(world->collisions); i++) {
        frCollisionCallback post_solve_cb = world->handler.post_solve;
        
        if (post_solve_cb != NULL) post_solve_cb(&world->collisions[i]);
    }
    
    frPostUpdateWorld(world);
}