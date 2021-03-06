/*
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
#include "stb_ds.h"

/* | `world` 모듈 구조체... | */

/* 물리 법칙이 존재하는 세계를 나타내는 구조체. */
typedef struct frWorld {
    Vector2 gravity;
    frSpatialHash *hash;
    frBody **bodies;
    frCollision *collisions;
    int *queries;
    double last_time;
} frWorld;

/* | `world` 모듈 함수... | */

/* 세계 `world`를 시간 `dt` (단위: ms)만큼 업데이트한다. */
static void frUpdateWorld(frWorld *world, double dt);

/* 중력 가속도가 `gravity`이고 경계 범위가 `bounds`인 세계를 생성한다. */ 
frWorld *frCreateWorld(Vector2 gravity, Rectangle bounds) {
    frWorld *result = calloc(1, sizeof(frWorld));
    
    result->gravity = gravity;
    result->hash = frCreateSpatialHash(bounds, FR_BROADPHASE_CELL_SIZE);
    result->last_time = frGetCurrentTime();
    
    arrsetcap(result->bodies, FR_WORLD_MAX_OBJECT_COUNT);
    arrsetcap(result->collisions, FR_WORLD_MAX_OBJECT_COUNT);
    arrsetcap(result->queries, FR_WORLD_MAX_OBJECT_COUNT);
    
    return result;
}

/* 세계 `world`에 할당된 메모리를 해제한다. */
void frReleaseWorld(frWorld *world) {
    if (world == NULL) return;
    
    frClearWorld(world);
    
    frReleaseSpatialHash(world->hash);
    
    arrfree(world->bodies);
    arrfree(world->collisions);
    arrfree(world->queries);
    
    free(world);
}

/* 세계 `world`에 강체 `body`를 추가한다. */
bool frAddToWorld(frWorld *world, frBody *body) {
    if (world == NULL || arrlen(world->bodies) >= FR_WORLD_MAX_OBJECT_COUNT) 
        return false;
    
    arrput(world->bodies, body);
    
    return true;
}

/* 세계 `world`의 모든 강체를 제거한다. */
void frClearWorld(frWorld *world) {
    if (world == NULL) return;
    
    frClearSpatialHash(world->hash);
    
    arrdeln(world->bodies, 0, arrlen(world->bodies));
    arrdeln(world->collisions, 0, arrlen(world->collisions));
}

/* 세계 `world`에서 강체 `body`를 제거한다. */
bool frRemoveFromWorld(frWorld *world, frBody *body) {
    if (world == NULL || body == NULL) 
        return false;
    
    for (int i = 0; i < arrlen(world->bodies); i++) {
        if (world->bodies[i] == body) {
            arrdeln(world->bodies, i, 1);
            return true;
        }
    }
    
    return false;
}

/* 세계 `world`에서 인덱스가 `index`인 강체의 메모리 주소를 반환한다. */
frBody *frGetWorldBody(frWorld *world, int index) {
    return (world != NULL && index >= 0 && index < arrlen(world->bodies)) 
        ? world->bodies[index]
        : NULL;
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

/* 세계 `world`의 모든 강체와 충돌 처리용 도형에 할당된 메모리를 해제한다. */
void frReleaseWorldBodies(frWorld *world) {
    if (world == NULL || world->bodies == NULL) return;
    
    for (int i = 0; i < arrlen(world->bodies); i++) {
        frReleaseShape(frGetBodyShape(world->bodies[i]));
        frReleaseBody(world->bodies[i]);
    }
    
    frClearWorld(world);
}

/* 세계 `world`의 중력 가속도를 `gravity`로 설정한다. */
void frSetWorldGravity(frWorld *world, Vector2 gravity) {
    if (world != NULL) world->gravity = gravity;
}

/* 세계 `scene`의 시간을 `dt` (단위: ms)만큼 흐르게 한다. */
void frSimulateWorld(frWorld *world, double dt) {
    if (world == NULL) return;
    
    double current_time = frGetCurrentTime();
    double elapsed_time = frGetTimeDifference(current_time, world->last_time);
    
    double accumulator = elapsed_time;
    
    if (elapsed_time > FR_WORLD_ACCUMULATOR_LIMIT) 
        elapsed_time = FR_WORLD_ACCUMULATOR_LIMIT;
    
    for (; accumulator >= dt; accumulator -= dt)
        frUpdateWorld(world, dt);
    
    world->last_time = current_time;
}

/* 세계 `world`를 시간 `dt` (단위: ms)만큼 업데이트한다. */
static void frUpdateWorld(frWorld *world, double dt) {
    if (world == NULL || world->hash == NULL || world->bodies == NULL) return;
	
	double current_time = frGetCurrentTime();
	double start_time = current_time;

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
            
            // 두 강체의 질량의 역수의 합이 0이면 충돌 처리 과정에서 제외한다.
            if (frGetBodyInverseMass(b1) + frGetBodyInverseMass(b2) <= 0.0f)
                continue;
            
            frCollision collision = frComputeCollision(
                frGetBodyShape(b1),
                frGetBodyTransform(b1),
                frGetBodyShape(b2),
                frGetBodyTransform(b2)
            );
            
            if (collision.check) {
                collision.bodies[0] = b1;
                collision.bodies[1] = b2;
                
                arrput(world->collisions, collision);
            }
        }
        
        arrdeln(world->queries, 0, arrlen(world->queries));
    }
    
    for (int i = 0; i < arrlen(world->bodies); i++) {
        frApplyGravity(world->bodies[i], world->gravity);
        frIntegrateForBodyVelocities(world->bodies[i], dt);
    }
    
    // 순차적으로 충격량을 반복 적용하여, 두 강체 사이의 충돌을 해결한다.
    for (int i = 0; i < FR_WORLD_MAX_ITERATIONS; i++) {
        for (int j = 0; j < arrlen(world->collisions); j++) {
            frBody *b1 = world->collisions[j].bodies[0];
            frBody *b2 = world->collisions[j].bodies[1];
            
            frResolveCollision(b1, b2, world->collisions[j]);
        }
    }
    
    for (int i = 0; i < arrlen(world->bodies); i++)
        frIntegrateForBodyPosition(world->bodies[i], dt);
    
    for (int i = 0; i < arrlen(world->collisions); i++) {
        frBody *b1 = world->collisions[i].bodies[0];
        frBody *b2 = world->collisions[i].bodies[1];
        
        frCorrectBodyPositions(b1, b2, world->collisions[i]);
    }

    for (int i = 0; i < arrlen(world->bodies); i++)
        frClearBodyForces(world->bodies[i]);
    
    arrdeln(world->collisions, 0, arrlen(world->collisions));
    
    frClearSpatialHash(world->hash);
}