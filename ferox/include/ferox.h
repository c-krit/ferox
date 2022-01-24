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

#ifndef FEROX_H
#define FEROX_H

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
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

#ifdef __cplusplus
extern "C" {
#endif
    /* 직사각형 `rec1`과 `rec2`가 서로 충돌하는지 확인한다. */
    bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
        return ((rec1.x + rec1.width) - rec2.x) >= 0 && ((rec2.x + rec2.width) - rec1.x) >= 0
            && ((rec1.y + rec1.height) - rec2.y) >= 0 && ((rec2.y + rec2.height) - rec1.y) >= 0;
    }
#ifdef __cplusplus
}
#endif

#endif

/* | 매크로 정의... | */

#define FR_GLOBAL_PIXELS_PER_METER                16.0f

#define FR_BROADPHASE_CELL_SIZE                   3.0f

#define FR_DYNAMICS_CORRECTION_DEPTH_SCALE        0.25f
#define FR_DYNAMICS_CORRECTION_DEPTH_THRESHOLD    0.02f

#define FR_GEOMETRY_MAX_VERTEX_COUNT              10

#define FR_WORLD_ACCUMULATOR_LIMIT                200.0
#define FR_WORLD_DEFAULT_GRAVITY                  ((Vector2) { .y = 9.8f })
#define FR_WORLD_MAX_BODY_COUNT                   128
#define FR_WORLD_MAX_ITERATIONS                   16

#define FR_STRUCT_ZERO(T)                         ((T) { 0 })

#define FR_NUMBER_MIN(x, y)                       (((x) < (y)) ? (x) : (y))
#define FR_NUMBER_MAX(x, y)                       (((x) > (y)) ? (x) : (y))

/* | 자료형 정의... | */

/* 강체의 종류를 나타내는 구조체. */
typedef enum frBodyType {
    FR_BODY_UNKNOWN = -1,
    FR_BODY_STATIC,
    FR_BODY_KINEMATIC,
    FR_BODY_DYNAMIC
} frBodyType;

/* 강체의 비트 플래그를 나타내는 열거형. */
typedef enum frBodyFlag {
    FR_FLAG_NONE = 0x00,
    FR_FLAG_INFINITE_MASS = 0x01,
    FR_FLAG_INFINITE_INERTIA = 0x02
} frBodyFlag;

/* 강체의 비트 플래그 조합을 나타내는 자료형. */
typedef uint8_t frBodyFlags;

/* 도형의 종류를 나타내는 열거형. */
typedef enum frShapeType {
    FR_SHAPE_UNKNOWN,
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

/* 다각형의 꼭짓점 배열을 나타내는 구조체. */
typedef struct frVertices {
    Vector2 data[FR_GEOMETRY_MAX_VERTEX_COUNT];
    int count;
} frVertices;

/* 도형을 나타내는 구조체. */
typedef struct frShape frShape;

/* 강체를 나타내는 구조체. */
typedef struct frBody frBody;

/* 강체 사이의 충돌 정보를 나타내는 구조체. */
typedef struct frSolverCache {
    frBody *bodies[2];
    /* TODO: ... */
} frSolverCache;

/* 도형 사이의 충돌을 나타내는 구조체. */
typedef struct frCollision {
    bool check;
    frSolverCache cache;
    Vector2 direction;
    Vector2 points[2];
    float depths[2];
    int count;
} frCollision;

/* 강체 사이의 충돌 이벤트 발생 시에 호출되는 함수를 가리키는 포인터. */
typedef void (*frCollisionCallback)(frCollision *collision);

/* 강체 사이의 충돌 이벤트 처리에 사용되는 핸들러를 나타내는 구조체. */
typedef struct frCollisionHandler {
    frCollisionCallback pre_solve;
    frCollisionCallback post_solve;
} frCollisionHandler;

/* 광선을 나타내는 구조체. */
typedef struct frRay {
    Vector2 origin;
    Vector2 direction;
    float max_distance;
} frRay;

/* 광선 투사의 결과를 나타내는 구조체. */
typedef struct frRaycastHit {
    bool check;
    union {
        frShape *shape;
        frBody *body;
    };
    Vector2 point;
    Vector2 normal;
    float distance;
    bool inside;
} frRaycastHit;

/* 공간 해시맵을 나타내는 구조체. */
typedef struct frSpatialHash frSpatialHash;

/* 물리 법칙이 존재하는 세계를 나타내는 구조체. */
typedef struct frWorld frWorld;

#ifdef __cplusplus
extern "C" {
#endif

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

/* 공간 해시맵 `hash`의 각 셀의 크기를 반환한다. */
float frGetSpatialHashCellSize(frSpatialHash *hash);

/* 공간 해시맵 `hash`에서 키가 `key`인 값을 제거한다. */
void frRemoveFromSpatialHash(frSpatialHash *hash, int key);

/* 공간 해시맵 `hash`에서 직사각형 `rec`와 경계 범위가 겹치는 모든 도형의 인덱스를 반환한다. */
void frQuerySpatialHash(frSpatialHash *hash, Rectangle rec, int **queries);

/* 공간 해시맵 `hash`의 경계 범위를 `bounds`로 설정한다. */
void frSetSpatialHashBounds(frSpatialHash *hash, Rectangle bounds);

/* 공간 해시맵 `hash`의 각 셀의 크기를 `cell_size`로 설정한다. */
void frSetSpatialHashCellSize(frSpatialHash *hash, float cell_size);

/* 공간 해시맵 `hash`에서 벡터 `v`와 대응하는 키를 반환한다. */
int frComputeSpatialHashKey(frSpatialHash *hash, Vector2 v);

/* | `collision` 모듈 함수... | */

/* 도형 `s1`에서 `s2`로의 충돌을 계산한다. */
frCollision frComputeCollision(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 도형 `s`에 광선을 투사한다. */
frRaycastHit frComputeShapeRaycast(frShape *s, frTransform tx, frRay ray);

/* 강체 `b`에 광선을 투사한다. */
frRaycastHit frComputeBodyRaycast(frBody *b, frRay ray);

/* | `debug` 모듈 함수... | */

#ifndef FEROX_STANDALONE
    /* 게임 화면에 점 `p1`에서 `p2`로 향하는 화살표를 그린다. */
    void frDrawArrow(Vector2 p1, Vector2 p2, float thick, Color color);

    /* 게임 화면에 강체 `b`의 도형을 그린다. */
    void frDrawBody(frBody *b, Color color);

    /* 게임 화면에 강체 `b`의 도형 테두리를 그린다. */
    void frDrawBodyLines(frBody *b, float thick, Color color);

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
frBody *frCreateBody(frBodyType type, frBodyFlags flags, Vector2 p);

/* 종류가 `type`이고 위치가 `p`이며 충돌 처리용 도형이 `s`인 강체를 생성한다. */
frBody *frCreateBodyFromShape(frBodyType type, frBodyFlags flags, Vector2 p, frShape *s);

/* 강체 `b`에 할당된 메모리를 해제한다. */
void frReleaseBody(frBody *b);

/* 강체 `b에 충돌 처리용 도형 `s`를 추가한다. */
void frAttachShapeToBody(frBody *b, frShape *s);

/* 강체 `b`에서 충돌 처리용 도형을 제거한다. */ 
void frDetachShapeFromBody(frBody *b);

/* 구조체 `frBody`의 크기를 반환한다. */
size_t frGetBodyStructSize(void);

/* 강체 `b`의 종류를 반환한다. */
frBodyType frGetBodyType(frBody *b);

/* 강체 `b`의 비트 플래그 조합을 반환한다. */
frBodyFlags frGetBodyFlags(frBody *b);

/* 강체 `b`의 재질을 반환한다. */
frMaterial frGetBodyMaterial(frBody *b);

/* 강체 `b`의 질량을 반환한다. */
float frGetBodyMass(frBody *b);

/* 강체 `b`의 질량의 역수를 반환한다. */
float frGetBodyInverseMass(frBody *b);

/* 강체 `b`의 관성 모멘트를 반환한다. */
float frGetBodyInertia(frBody *b);

/* 강체 `b`의 관성 모멘트의 역수를 반환한다. */
float frGetBodyInverseInertia(frBody *b);

/* 강체 `b`의 속도를 반환한다. */
Vector2 frGetBodyVelocity(frBody *b);

/* 강체 `b`의 각속도를 반환한다. */
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

/* 강체 `b`의 사용자 데이터를 반환한다. */
void *frGetBodyUserData(frBody *b);

/* 세계 기준 좌표 `p`를 강체 `b`를 기준으로 한 좌표로 변환한다. */
Vector2 frGetLocalPoint(frBody *b, Vector2 p);

/* 강체 `b`를 기준으로 한 좌표 `p`를 세계 기준 좌표로 변환한다. */
Vector2 frGetWorldPoint(frBody *b, Vector2 p);

/* 강체 `b`의 종류를 `type`으로 설정한다. */
void frSetBodyType(frBody *b, frBodyType type);

/* 강체 `b`의 비트 플래그 조합을 `flags`으로 설정한다. */
void frSetBodyFlags(frBody *b, frBodyFlags flags);

/* 강체 `b`의 속도를 `v`로 설정한다. */
void frSetBodyVelocity(frBody *b, Vector2 v);

/* 강체 `b`의 각속도를 `a`로 설정한다. */
void frSetBodyAngularVelocity(frBody *b, double a);

/* 강체 `b`의 중력 가속률을 `scale`로 설정한다. */
void frSetBodyGravityScale(frBody *b, float scale);

/* 강체 `b`의 위치와 회전 각도를 `tx`의 값으로 설정한다. */ 
void frSetBodyTransform(frBody *b, frTransform tx);

/* 강체 `b`의 위치를 `p`로 설정한다. */
void frSetBodyPosition(frBody *b, Vector2 p);

/* 강체 `b`의 회전 각도를 `rotation`으로 설정한다. */
void frSetBodyRotation(frBody *b, float rotation);

/* 강체 `b`의 사용자 데이터를 `data`로 설정한다. */
void frSetBodyUserData(frBody *b, void *data);

/* 강체 `b`에 작용하는 모든 힘을 제거한다. */
void frClearBodyForces(frBody *b);

/* 강체 `b`에 중력 가속도 `gravity`를 적용한다. */
void frApplyGravity(frBody *b, Vector2 gravity);

/* 강체 `b`에 충격량 `impulse`를 적용한다. */
void frApplyImpulse(frBody *b, Vector2 impulse);

/* 강체 `b` 위의 점 `p`에 각운동량 `impulse`를 적용한다. */
void frApplyTorqueImpulse(frBody *b, Vector2 p, Vector2 impulse);

/* 강체 `b1`과 `b2`의 위치를 적절하게 보정한다. */
void frCorrectBodyPositions(frBody *b1, frBody *b2, frCollision collision);

/* 단위 시간 `dt` 이후의 강체 `b`의 위치를 계산한다. */
void frIntegrateForBodyPosition(frBody *b, double dt);

/* 단위 시간 `dt` 이후의 강체 `b`의 속도와 각속도를 계산한다. */
void frIntegrateForBodyVelocities(frBody *b, double dt);

/* 강체 `b1`과 `b2` 사이의 충돌을 해결한다. */
void frResolveCollision(frBody *b1, frBody *b2, frCollision collision);

/* | `geometry` 모듈 함수... | */

/* 반지름이 `radius`인 원을 나타내는 도형을 생성한다. */
frShape *frCreateCircle(frMaterial material, float radius);

/* 가로 길이가 `width`이고 세로 길이가 `height`인 직사각형을 나타내는 도형을 생성한다. */
frShape *frCreateRectangle(frMaterial material, float width, float height);

/* 꼭짓점 배열이 `vertices`인 다각형을 나타내는 도형을 생성한다. */
frShape *frCreatePolygon(frMaterial material, frVertices vertices);

/* 형태가 정해지지 않은 도형을 생성한다. */
frShape *frCreateShape(void);

/* 도형 `s`와 형태가 같은 새로운 도형을 반환한다. */
frShape *frCloneShape(frShape *s);

/* 도형 `s`에 할당된 메모리를 해제한다. */
void frReleaseShape(frShape *s);

/* 구조체 `frShape`의 크기를 반환한다. */
size_t frGetShapeStructSize(void);

/* 도형 `s`의 종류를 반환한다. */
frShapeType frGetShapeType(frShape *s);

/* 도형 `s`의 재질을 반환한다. */
frMaterial frGetShapeMaterial(frShape *s);

/* 도형 `s`의 넓이를 반환한다. */
float frGetShapeArea(frShape *s);

/* 도형 `s`의 질량을 반환한다. */
float frGetShapeMass(frShape *s);

/* 도형 `s`의 관성 모멘트를 반환한다. */
float frGetShapeInertia(frShape *s);

/* 도형 `s`의 AABB를 반환한다. */
Rectangle frGetShapeAABB(frShape *s, frTransform tx);

/* 원 `s`의 반지름을 반환한다. */
float frGetCircleRadius(frShape *s);

/* 다각형 `s`의 `index + 1`번째 꼭짓점을 반환한다. */
Vector2 frGetPolygonVertex(frShape *s, int index);

/* 다각형 `s`의 `index + 1`번째 법선 벡터를 반환한다. */
Vector2 frGetPolygonNormal(frShape *s, int index);

/* 다각형 `s`의 꼭짓점 배열을 반환한다. */
frVertices frGetPolygonVertices(frShape *s);

/* 다각형 `s`의 법선 벡터 배열을 반환한다. */
frVertices frGetPolygonNormals(frShape *s);

/* 원 `s`의 반지름을 `radius`로 변경한다. */
void frSetCircleRadius(frShape *s, float radius);

/* 다각형 `s`의 꼭짓점 배열을 `vertices`로 변경한다. */
void frSetPolygonVertices(frShape *s, frVertices vertices);

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
bool frNumberApproxEquals(float f1, float f2);

/* 각도 `angle` (단위: rad.)을 정규화하여, 구간 `[center - π/2, center + π/2]`에 포함되도록 한다. */
float frNormalizeAngle(float angle, float center);

/* 주어진 픽셀 단위 거리를 미터 단위 거리로 변환한다. */
float frNumberPixelsToMeters(float value);

/* 주어진 미터 단위 거리를 픽셀 단위 거리로 변환한다. */
float frNumberMetersToPixels(float value);

/* 주어진 픽셀 단위 `Rectangle` 구조체를 미터 단위 `Rectangle` 구조체로 변환한다. */
Rectangle frRecPixelsToMeters(Rectangle rec);

/* 주어진 미터 단위 `Rectangle` 구조체를 픽셀 단위 `Rectangle` 구조체로 변환한다. */
Rectangle frRecMetersToPixels(Rectangle rec);

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

/* 점 `p`와 점 `q`를 지나고 방향 벡터가 `v`인 직선 사이의 거리를 반환한다. */
float frVec2DistancePointLine(Vector2 p, Vector2 q, Vector2 v);

/* 영점을 기준으로 벡터 `v`를 `angle` (단위: rad.)만큼 회전시킨 벡터를 반환한다. */
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

/* 세계 `world`에 강체 `b`를 추가한다. */
bool frAddToWorld(frWorld *world, frBody *b);

/* 세계 `world`의 모든 강체를 제거한다. */
void frClearWorld(frWorld *world);

/* 세계 `world`에서 강체 `b`를 제거한다. */
bool frRemoveFromWorld(frWorld *world, frBody *b);

/* 구조체 `frWorld`의 크기를 반환한다. */
size_t frGetWorldStructSize(void);

/* 세계 `world`에서 인덱스가 `index`인 강체의 메모리 주소를 반환한다. */
frBody *frGetWorldBody(frWorld *world, int index);

/* 세계 `world`의 충돌 핸들러를 반환한다. */
frCollisionHandler frGetWorldCollisionHandler(frWorld *world);

/* 세계 `world`의 강체 배열의 크기를 반환한다. */
int frGetWorldBodyCount(frWorld *world);

/* 세계 `world`의 경계 범위를 반환한다. */
Rectangle frGetWorldBounds(frWorld *world);

/* 세계 `world`의 공간 해시맵을 반환한다. */
frSpatialHash *frGetWorldSpatialHash(frWorld *world);

/* 세계 `world`의 중력 가속도를 반환한다. */
Vector2 frGetWorldGravity(frWorld *world);

/* 강체 `b`가 세계 `world`의 경계 범위 안에 있는지 확인한다. */
bool frIsInWorldBounds(frWorld *world, frBody *b);

/* 세계 `world`의 경계 범위를 `bounds`로 설정한다. */
void frSetWorldBounds(frWorld *world, Rectangle bounds);

/* 세계 `world`의 충돌 핸들러를 `handler`로 설정한다. */
void frSetWorldCollisionHandler(frWorld *world, frCollisionHandler handler);

/* 세계 `world`의 중력 가속도를 `gravity`로 설정한다. */
void frSetWorldGravity(frWorld *world, Vector2 gravity);

/* 세계 `world`의 시간을 `dt` (단위: ms) 만큼 흐르게 한다. */
void frSimulateWorld(frWorld *world, double dt);

/* 세계 `world`의 모든 강체에 광선을 투사한다. */
int frComputeWorldRaycast(frWorld *world, frRay ray, frRaycastHit *hits);

#ifdef __cplusplus
}
#endif

#endif
