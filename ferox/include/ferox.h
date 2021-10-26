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

#ifndef FEROX_H
#define FEROX_H

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef FEROX_STANDALONE
    #include "raylib.h"
#else
    #ifndef PI
        #define PI 3.14159265358979323846f
    #endif

    #define DEG2RAD (PI / 180.0f)
    #define RAD2DEG (180.0f / PI)

    /* 2차원 벡터를 나타내는 구조체. */
    typedef struct Vector2 {
        float x;
        float y;
    } Vector2;

    /* 직사각형을 나타내는 구조체. */
    typedef struct Rectangle {
        float x;
        float y;
        float width;
        float height;
    } Rectangle;

    /* 직사각형 `rec1`과 `rec2`가 서로 충돌하는지 확인한다. */
    bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
        return ((rec1.x + rec1.width) - rec2.x) >= 0 && ((rec2.x + rec2.width) - rec1.x) >= 0
            && ((rec1.y + rec1.height) - rec2.y) >= 0 && ((rec2.y + rec2.height) - rec1.y) >= 0;
    }
#endif

/* | 매크로 함수... | */

#define FR_STRUCT_ZERO(T) ((T) { 0 })

#define FR_NUMBER_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define FR_NUMBER_MAX(x, y) (((x) > (y)) ? (x) : (y))

/* | 매크로 값... | */

#ifndef FR_GLOBAL_PIXELS_PER_METER
    #define FR_GLOBAL_PIXELS_PER_METER 16.0f
#endif

#define FR_BROADPHASE_CELL_SIZE 8

#ifndef FEROX_STANDALONE
    #define FR_DEBUG_BACKGROUND_COLOR (GetColor(0x111111FF))
#endif

#define FR_DYNAMICS_CORRECTION_DEPTH_SCALE 0.25f
#define FR_DYNAMICS_CORRECTION_DEPTH_THRESHOLD 0.02f
#define FR_DYNAMICS_DEFAULT_MATERIAL ((frMaterial) { 1.0f, 0.0f, 0.5f, 0.25f })

#define FR_GEOMETRY_MAX_VERTEX_COUNT 8

#define FR_WORLD_ACCUMULATOR_LIMIT 200.0
#define FR_WORLD_DEFAULT_GRAVITY ((Vector2) { 0.0f, 9.8f })
#define FR_WORLD_MAX_OBJECT_COUNT 128
#define FR_WORLD_MAX_ITERATIONS 16

/* | 전역 구조체... | */

/* 강체의 종류를 나타내는 구조체. */
typedef enum frBodyType {
    FR_BODY_UNKNOWN = -1,
    FR_BODY_STATIC,
    FR_BODY_KINEMATIC,
    FR_BODY_DYNAMIC
} frBodyType;

/* 강체의 상태를 나타내는 구조체. */
typedef enum frBodyState {
    FR_STATE_UNKNOWN = -1,
    FR_STATE_AWAKE,
    FR_STATE_SLEEPING
} frBodyState;

/* 도형의 종류를 나타내는 구조체. */
typedef enum frShapeType {
    FR_SHAPE_UNKNOWN = -1,
    FR_SHAPE_CIRCLE,
    FR_SHAPE_POLYGON
} frShapeType;

/* 도형의 재질을 나타내는 구조체. */
typedef struct frMaterial {
    float density;
    float restitution;
    float static_friction;
    float dynamic_friction;
} frMaterial;

/* 도형 또는 강체의 위치와 회전 각도 (단위: rad.)를 나타내는 구조체. */
typedef struct frTransform {
    Vector2 position;
    float rotation;
} frTransform;

/* 도형을 나타내는 구조체. */
typedef struct frShape frShape;

/* 강체를 나타내는 구조체. */
typedef struct frBody frBody;

/* 도형 사이의 충돌을 나타내는 구조체. */
typedef struct frCollision {
    bool check;
    frBody *_bodies[2];
    Vector2 direction;
    Vector2 points[2];
    float depths[2];
    int count;
} frCollision;

/* 도형에 광선을 투사했을 때의 결과를 나타내는 구조체. */
typedef struct frRaycastHit {
    bool check;
    frShape *shape;
    Vector2 point;
    Vector2 normal;
    float distance;
    bool inside;
} frRaycastHit;

/* 공간 해시맵을 나타내는 구조체. */
typedef struct frSpatialHash frSpatialHash;

/* 물리 법칙이 존재하는 세계를 나타내는 구조체. */
typedef struct frWorld frWorld;

/* | `broadphase` 모듈 함수... | */

/* 경계 범위가 `bounds`이고 각 셀의 크기가 `cell_size`인 공간 해시맵의 메모리 주소를 반환한다. */
frSpatialHash *frCreateSpatialHash(Rectangle bounds, float cell_size);

/* 공간 해시맵 `hash`에 할당된 메모리를 해제한다. */
void frReleaseSpatialHash(frSpatialHash *hash);

/* 공간 해시맵 `hash`에 직사각형 `rec`로 생성한 키와 `value`를 추가한다. */
void frAddToSpatialHash(frSpatialHash *hash, Rectangle rec, int value);

/* 공간 해시맵 `hash`의 모든 키와 값을 제거한다. */
void frClearSpatialHash(frSpatialHash *hash);

/* 공간 해시맵 `hash`의 경계 범위를 반환한다. */
Rectangle frGetSpatialHashBounds(frSpatialHash *hash);

/* 공간 해시맵 `hash`에서 키가 `key`인 값을 제거한다. */
void frRemoveFromSpatialHash(frSpatialHash *hash, int key);

/* 공간 해시맵 `hash`에서 직사각형 `rec`와 경계 범위가 겹치는 모든 도형의 인덱스를 반환한다. */
void frQuerySpatialHash(frSpatialHash *hash, Rectangle rec, int **result);

/* 공간 해시맵 `hash`에서 벡터 `v`와 대응하는 키를 반환한다. */
int frComputeSpatialHashKey(frSpatialHash *hash, Vector2 v);

/* | `collision` 모듈 함수... | */

/* 도형 `s1`에서 `s2`로의 충돌을 계산한다. */
frCollision frComputeCollision(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* `p`에서 `v` 방향으로 최대 `max_distance`의 거리까지 진행하는 광선을 도형 `s`에 투사한다. */
frRaycastHit frComputeRaycast(frShape *s, frTransform tx, Vector2 p, Vector2 v, float max_distance);

/* Sutherland-Hodgman 다각형 절단 알고리즘을 이용하여, 다각형 `s1`을 `s2`에 맞게 절단한다. */
frShape *frSutherlandHodgman(frShape *s1, frShape *s2);

/* | `debug` 모듈 함수... | */

#ifndef FEROX_STANDALONE
    /* 게임 화면에 강체 `b`의 도형을 그린다. */
    void frDrawBody(frBody *b, Color color);

    /* 게임 화면에 강체 `b`의 도형 테두리를 그린다. */
    void frDrawBodyLines(frBody *b, Color color);

    /* 게임 화면에 강체 `b`의 AABB와 질량 중심을 그린다. */
    void frDrawBodyAABB(frBody *b, Color color);

    /* 게임 화면에 강체 `b`의 물리량 정보를 그린다. */
    void frDrawBodyProperties(frBody *b, Color color);

    /* 게임 화면에 공간 해시맵 `hm`을 그린다. */
    void frDrawSpatialHash(frSpatialHash *hm);

    /* 무작위 색상을 반환한다. */
    Color frGetRandomColor(void);
#endif

/* | `dynamics` 모듈 함수... | */

/* 종류가 `type`이고 위치가 `p`인 강체를 생성한다. */
frBody *frCreateBody(frBodyType type, Vector2 p);

/* 종류가 `type`이고 위치가 `p`이며 충돌 처리용 도형이 `shape`인 강체를 생성한다. */
frBody *frCreateBodyFromShape(frBodyType type, Vector2 p, frShape *s);

/* 강체 `b`에 할당된 메모리를 해제한다. */
void frReleaseBody(frBody *b);

/* 강체 `b에 충돌 처리용 도형 `s`를 추가한다. */
void frAttachShapeToBody(frBody *b, frShape *s);

/* 강체 `b`에서 충돌 처리용 도형을 제거한다. */ 
void frDetachShapeFromBody(frBody *b);

/* 강체 `b`의 종류를 반환한다. */
frBodyType frGetBodyType(frBody *b);

/* 강체 `b`의 상태를 반환한다. */
frBodyType frGetBodyState(frBody *b);

/* 강체 `b`의 질량을 반환한다. */
float frGetBodyMass(frBody *b);

/* 강체 `b`의 질량의 역수를 반환한다. */
float frGetBodyInverseMass(frBody *b);

/* 강체 `b`의 Z축을 기준으로 한 관성 모멘트를 반환한다. */
float frGetBodyInertia(frBody *b);

/* 강체 `b`의 Z축을 기준으로 한 관성 모멘트의 역수를 반환한다. */
float frGetBodyInverseInertia(frBody *b);

/* 강체 `b`의 속도를 반환한다. */
Vector2 frGetBodyVelocity(frBody *b);

/* 강체 `b`의 각속를 반환한다. */
float frGetBodyAngularVelocity(frBody *b);

/* 강체 `b`의 중력 가속률을 반환한다. */
float frGetBodyGravityScale(frBody *b);

/* 강체 `b`의 위치와 회전 각도 (단위: rad.)를 반환한다. */
frTransform frGetBodyTransform(frBody *b);

/* 강체 `b`의 위치를 반환한다. */
Vector2 frGetBodyPosition(frBody *b);

/* 강체 `b`의 회전 각도 (단위: rad.)를 반환한다. */
float frGetBodyRotation(frBody *b);

/* 강체 `b`의 충돌 처리용 도형을 반환한다. */
frShape *frGetBodyShape(frBody *b);

/* 강체 `b`의 AABB를 반환한다. */
Rectangle frGetBodyAABB(frBody *b);

/* 세계 기준 좌표 `p`를 강체 `b`를 기준으로 한 좌표로 변환한다. */
Vector2 frGetLocalPoint(frBody *b, Vector2 p);

/* 강체 `b`를 기준으로 한 좌표 `p`를 세계 기준 좌표로 변환한다. */
Vector2 frGetWorldPoint(frBody *b, Vector2 p);

/* 강체 `b`의 상태를 반환한다. */
frBodyType frGetBodyState(frBody *b);

/* 강체 `b`의 종류를 `type`으로 설정한다. */
void frSetBodyType(frBody *b, frBodyType type);

/* 강체 `b`의 상태를 `state`으로 설정한다. */
void frSetBodyState(frBody *b, frBodyState state);

/* 강체 `b`의 위치를 `p`로 설정한다. */
void frSetBodyPosition(frBody *b, Vector2 p);

/* 강체 `b`의 회전 각도를 `rotation`으로 설정한다. */
void frSetBodyRotation(frBody *b, float rotation);

/* 강체 `b`의 위치와 회전 각도를 `tx`의 값으로 설정한다. */ 
void frSetBodyTransform(frBody *b, frTransform tx);

/* 강체 `b`의 속도를 `v`로 설정한다. */
void frSetBodyVelocity(frBody *b, Vector2 v);

/* 강체 `b`의 각속도를 `a`로 설정한다. */
void frSetBodyAngularVelocity(frBody *b, double a);

/* 강체 `b`의 중력 가속률을 `gravity_scale`로 설정한다. */
void frSetBodyGravityScale(frBody *b, float gravity_scale);

/* 강체 `b`에 작용하는 모든 힘을 제거한다. */
void frClearBodyForces(frBody *b);

/* 강체 `b`에 중력 가속도 `gravity`를 적용한다. */
void frApplyGravity(frBody *b, Vector2 gravity);

/* 강체 `b`에 충격량 `impulse`를 적용한다. */
void frApplyImpulse(frBody *b, Vector2 impulse);

/* 강체 `b` 위의 점 `point`에 각운동량 `impulse`를 적용한다. */
void frApplyTorqueImpulse(frBody *b, Vector2 point, Vector2 impulse);

/* 강체 `b1`과 `b2`의 위치를 적절하게 보정한다. */
void frCorrectBodyPositions(frBody *b1, frBody *b2, frCollision collision);

/* 단위 시간 `dt` 이후의 강체 `b`의 위치를 계산한다. */
void frIntegrateForBodyPosition(frBody *b, double dt);

/* 단위 시간 `dt` 이후의 강체 `b`의 속도와 각속도를 계산한다. */
void frIntegrateForBodyVelocities(frBody *b, double dt);

/* 강체 `b1`과 `b2` 사이의 충돌을 해결한다. */
void frResolveCollision(frBody *b1, frBody *b2, frCollision collision);

/* | `geometry` 모듈 함수... | */

/* 반지름이 `radius`인 원을 나타내는 도형 구조체의 메모리 주소를 반환한다. */
frShape *frCreateCircle(frMaterial material, float radius);

/* 꼭짓점 배열이 `vertices`이고 꼭짓점 개수가 `count`인 다각형을 나타내는 도형 구조체의 메모리 주소를 반환한다. */
frShape *frCreatePolygon(frMaterial material, Vector2 *vertices, int count);

/* 형태가 정해지지 않은 도형 구조체의 메모리 주소를 반환한다. */
frShape *frCreateShape(void);

/* 도형 `s`와 형태가 같은 새로운 도형을 반환한다. */
frShape *frCloneShape(frShape *s);

/* 도형 `s`에 할당된 메모리를 해제한다. */
void frReleaseShape(frShape *s);

/* 원 `s`의 반지름을 반환한다. */
float frGetCircleRadius(frShape *s);

/* 다각형 `s`의 꼭짓점 배열의 메모리 주소를 반환한다. */
Vector2 *frGetPolygonVertices(frShape *s, int *vertex_count);

/* 다각형 `s`의 각 변과 수직인 모든 변이 저장된 배열의 메모리 주소를 반환한다. */
Vector2 *frGetPolygonNormals(frShape *s, int *normal_count);

/* 도형 `s`의 AABB를 반환한다. */
Rectangle frGetShapeAABB(frShape *s, frTransform tx);

/* 도형 `s`의 넓이를 반환한다. */
float frGetShapeArea(frShape *s);

/* 도형 `s`의 무게중심을 반환한다. */
Vector2 frGetShapeCentroid(frShape *s);

/* 도형 `s`의 Z축을 기준으로 한 관성 모멘트를 반환한다. */
float frGetShapeInertia(frShape *s);

/* 도형 `s`의 질량을 반환한다. */
float frGetShapeMass(frShape *s);

/* 도형 `s`의 재질을 반환한다. */
frMaterial frGetShapeMaterial(frShape *s);

/* 도형 `s`의 종류를 반환한다. */
frShapeType frGetShapeType(frShape *s);

/* 원 `s`의 반지름을 `radius`로 변경한다. */
void frSetCircleRadius(frShape *s, float radius);

/* 다각형 `s`의 꼭짓점 배열을 꼭짓점 개수 `count`개의 배열 `vertices`로 변경한다. */
void frSetPolygonVertices(frShape *s, Vector2 *vertices, int count);

/* 도형 `s`의 재질을 `material`로 설정한다. */
void frSetShapeMaterial(frShape *s, frMaterial material);

/* 도형 `s`의 종류를 `type`으로 변경한다. */
void frSetShapeType(frShape *s, frShapeType type);

/* 점 `p`가 도형 `s`의 내부에 있는지 확인한다. */
bool frShapeContainsPoint(frShape *s, frTransform tx, Vector2 p);

/* | `timer` 모듈 함수... | */

/* 단조 시계를 초기화한다. */
void frInitClock(void);

/* 단조 시계의 현재 시각 (단위: ms)을 반환한다. */
double frGetCurrentTime(void);

/* 단조 시계의 시각 `new_time`과 `old_time`의 차이를 반환한다. */
double frGetTimeDifference(double new_time, double old_time);

/* 단조 시계의 현재 시각과 `old_time`과의 차이를 반환한다. */
double frGetTimeSince(double old_time);

/* | `utils` 모듈 함수... | */

/* 부동 소수점 값 `f1`이 `f2`와 근접한 값인지 확인한다. */
bool frApproxEquals(float f1, float f2);

/* 각도 `angle` (단위: rad.)을 정규화하여, 구간 `[center - π/2, center + π/2]`에 포함되도록 한다. */
float frNormalizeAngle(float angle, float center);

/* 주어진 픽셀 단위 거리를 미터 단위 거리로 변환한다. */
float frNumberPixelsToMeters(float value);

/* 주어진 미터 단위 거리를 픽셀 단위 거리로 변환한다. */
float frNumberMetersToPixels(float value);

/* | `vectors` 모듈 함수... | */

/* 벡터 `v1`과 `v2`의 합을 반환한다. */
Vector2 frVec2Add(Vector2 v1, Vector2 v2);

/* 벡터 `v1`과 `v2`의 차를 반환한다. */
Vector2 frVec2Subtract(Vector2 v1, Vector2 v2);

/* 벡터 `v`의 각 성분에 `value`를 곱한 2차원 벡터를 반환한다. */
Vector2 frVec2ScalarMultiply(Vector2 v, float value);

/* 벡터 `v`의 크기를 반환한다. */
float frVec2Magnitude(Vector2 v);

/* 벡터 `v`의 크기의 제곱을 반환한다. */
float frVec2MagnitudeSqr(Vector2 v);

/* 벡터 `v`와 크기가 같고 방향이 반대인 2차원 벡터를 반환한다. */
Vector2 frVec2Negate(Vector2 v);

/* 벡터 `v`를 정규화한 2차원 벡터를 반환한다. */
Vector2 frVec2Normalize(Vector2 v);

/* 벡터 `v1`과 `v2`가 이루는 각도 (단위: rad.)를 반환한다. */
float frVec2Angle(Vector2 v1, Vector2 v2);

/* 벡터 `v1`의 모든 성분이 `v2`의 모든 성분과 근접한 값인지 확인한다. */
bool frVec2ApproxEquals(Vector2 v1, Vector2 v2);

/* 벡터 `v1`과 `v2`의 외적을 반환한다. */
float frVec2CrossProduct(Vector2 v1, Vector2 v2);

/* 벡터 `v1`과 `v2`의 내적을 반환한다. */
float frVec2DotProduct(Vector2 v1, Vector2 v2);

/* 벡터 `v`의 왼쪽 방향에서 수직을 이루는 법선 벡터를 반환한다. */
Vector2 frVec2LeftNormal(Vector2 v);

/* 벡터 `v`의 오른쪽 방향에서 수직을 이루는 법선 벡터를 반환한다. */
Vector2 frVec2RightNormal(Vector2 v);

/* 벡터 `v1`과 `v2` 사이의 거리를 반환한다. */
float frVec2Distance(Vector2 v1, Vector2 v2);

/* 벡터 `v1`과 `v2` 사이의 거리의 제곱을 반환한다. */
float frVec2DistanceSqr(Vector2 v1, Vector2 v2);

/* 평면 위의 점 `p`와 점 `q`를 지나고 방향 벡터가 `v`인 직선 사이의 거리를 반환한다. */
float frVec2DistancePointLine(Vector2 p, Vector2 q, Vector2 v);

/* 영점을 기준으로 벡터 `v2`를 `angle` (단위: rad.)만큼 회전시킨 벡터를 반환한다. */
Vector2 frVec2Rotate(Vector2 v, float angle);

/* 벡터 `v`를 `tx`의 값에 따라 평행 이동하고 회전시킨다. */
Vector2 frVec2Transform(Vector2 v, frTransform tx);

/* 벡터 `v1`, `v2`와 `v3`가 반시계 방향으로 정렬되어 있는지 확인한다. */
bool frVec2CCW(Vector2 v1, Vector2 v2, Vector2 v3);

/* 픽셀 단위 벡터 `v`를 미터 단위 벡터로 변환한다. */
Vector2 frVec2PixelsToMeters(Vector2 v);

/* 미터 단위 벡터 `v`를 픽셀 단위 벡터로 변환한다. */
Vector2 frVec2MetersToPixels(Vector2 v);

/* | `world` 모듈 함수... | */

/* 중력 가속도가 `gravity`이고 경계 범위가 `bounds`인 세계를 생성한다. */ 
frWorld *frCreateWorld(Vector2 gravity, Rectangle bounds);

/* 세계 `world`에 할당된 메모리를 해제한다. */
void frReleaseWorld(frWorld *world);

/* 세계 `world`에 강체 `body`를 추가한다. */
bool frAddToWorld(frWorld *world, frBody *body);

/* 세계 `world`의 모든 강체를 제거한다. */
void frClearWorld(frWorld *world);

/* 세계 `world`에서 강체 `body`를 제거한다. */
bool frRemoveFromWorld(frWorld *world, frBody *body);

/* 세계 `world`에서 인덱스가 `index`인 강체의 메모리 주소를 반환한다. */
frBody *frGetWorldBody(frWorld *world, int index);

/* 세계 `world`의 강체 배열의 크기를 반환한다. */
int frGetWorldBodyCount(frWorld *world);

/* 세계 `world`의 경계 범위를 반환한다. */
Rectangle frGetWorldBounds(frWorld *world);

/* 세계 `world`의 공간 해시맵을 반환한다. */
frSpatialHash *frGetWorldSpatialHash(frWorld *world);

/* 세계 `world`의 중력 가속도를 반환한다. */
Vector2 frGetWorldGravity(frWorld *world);

/* 세계 `world`의 모든 강체와 충돌 처리용 도형에 할당된 메모리를 해제한다. */
void frReleaseWorldBodies(frWorld *world);

/* 세계 `world`의 중력 가속도를 `gravity`로 설정한다. */
void frSetWorldGravity(frWorld *world, Vector2 gravity);

/* 세계 `scene`의 시간을 `dt` (단위: ms) 만큼 흐르게 한다. */
void frSimulateWorld(frWorld *world, double dt);

#endif