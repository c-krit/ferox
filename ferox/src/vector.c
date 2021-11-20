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

/* | `vectors` 모듈 함수... | */

/* 벡터 `v1`과 `v2`의 합을 반환한다. */
Vector2 frVec2Add(Vector2 v1, Vector2 v2) {
    return (Vector2) { v1.x + v2.x, v1.y + v2.y };
}

/* 벡터 `v1`과 `v2`의 차를 반환한다. */
Vector2 frVec2Subtract(Vector2 v1, Vector2 v2) {
    return (Vector2) { v1.x - v2.x, v1.y - v2.y };
}

/* 벡터 `v`의 각 성분에 `value`를 곱한 2차원 벡터를 반환한다. */
Vector2 frVec2ScalarMultiply(Vector2 v, float value) {
    return (Vector2) { v.x * value, v.y * value };
}

/* 벡터 `v`의 크기를 반환한다. */
float frVec2Magnitude(Vector2 v) {
    return sqrtf(frVec2MagnitudeSqr(v));
}

/* 벡터 `v`의 크기의 제곱을 반환한다. */
float frVec2MagnitudeSqr(Vector2 v) {
    return frVec2DotProduct(v, v);
}

/* 벡터 `v`와 크기가 같고 방향이 반대인 2차원 벡터를 반환한다. */
Vector2 frVec2Negate(Vector2 v) {
    return (Vector2) { -v.x, -v.y };
}

/* 벡터 `v`를 정규화한 2차원 벡터를 반환한다. */
Vector2 frVec2Normalize(Vector2 v) {
    float magnitude = frVec2Magnitude(v);
    
    return (magnitude > 0.0f)
        ? frVec2ScalarMultiply(v, 1.0f / magnitude)
        : FR_STRUCT_ZERO(Vector2);
}

/* 벡터 `v1`과 `v2`가 이루는 각도 (단위: rad.)를 반환한다. */
float frVec2Angle(Vector2 v1, Vector2 v2) {
    return atan2f(v2.y, v2.x) - atan2f(v1.y, v1.x);
}

/* 벡터 `v1`의 모든 성분이 `v2`의 모든 성분과 근접한 값인지 확인한다. */
bool frVec2ApproxEquals(Vector2 v1, Vector2 v2) {
    return frApproxEquals(v1.x, v2.x) && frApproxEquals(v1.y, v2.y);
}

/* 벡터 `v1`과 `v2`의 외적을 반환한다. */
float frVec2CrossProduct(Vector2 v1, Vector2 v2) {
    // 평면 벡터의 외적은 스칼라 값이다.
    return (v1.x * v2.y) - (v1.y * v2.x);
}

/* 벡터 `v1`과 `v2`의 내적을 반환한다. */
float frVec2DotProduct(Vector2 v1, Vector2 v2) {
    return (v1.x * v2.x) + (v1.y * v2.y);
}

/* 벡터 `v`의 왼쪽 방향에서 수직을 이루는 법선 벡터를 반환한다. */
Vector2 frVec2LeftNormal(Vector2 v) {
    return frVec2Normalize((Vector2) { -v.y, v.x });
}

/* 벡터 `v`의 오른쪽 방향에서 수직을 이루는 법선 벡터를 반환한다. */
Vector2 frVec2RightNormal(Vector2 v) {
    return frVec2Normalize((Vector2) { v.y, -v.x });
}

/* 벡터 `v1`과 `v2` 사이의 거리를 반환한다. */
float frVec2Distance(Vector2 v1, Vector2 v2) {
    return frVec2Magnitude(frVec2Subtract(v1, v2));
}

/* 벡터 `v1`과 `v2` 사이의 거리의 제곱을 반환한다. */
float frVec2DistanceSqr(Vector2 v1, Vector2 v2) {
    return frVec2MagnitudeSqr(frVec2Subtract(v1, v2));
}

/* 평면 위의 점 `p`와 점 `q`를 지나고 방향 벡터가 `v`인 직선 사이의 거리를 반환한다. */
float frVec2DistancePointLine(Vector2 p, Vector2 q, Vector2 v) {
    return frVec2DotProduct(frVec2Subtract(q, p), frVec2LeftNormal(v));
}

/* 영점을 기준으로 벡터 `v`를 `angle` (rad.)만큼 회전시킨 벡터를 반환한다. */
Vector2 frVec2Rotate(Vector2 v, float angle) {
    float sin_angle = sinf(angle);
    float cos_angle = cosf(angle);
    
    return (Vector2) {
        (v.x * cos_angle - v.y * sin_angle),
        (v.x * sin_angle + v.y * cos_angle)
    };
}

/* 벡터 `v`를 `tx`의 값에 따라 평행 이동하고 회전시킨다. */
Vector2 frVec2Transform(Vector2 v, frTransform tx) {
    return frVec2Add(tx.position, frVec2Rotate(v, tx.rotation));
}

/* 벡터 `v1`, `v2`와 `v3`가 반시계 방향으로 정렬되어 있는지 확인한다. */
bool frVec2CCW(Vector2 v1, Vector2 v2, Vector2 v3) {
    return (v3.y - v1.y) * (v2.x - v1.x) < (v2.y - v1.y) * (v3.x - v1.x);
};

/* 픽셀 단위 벡터 `v`를 미터 단위 벡터로 변환한다. */
Vector2 frVec2PixelsToMeters(Vector2 v) {
    return (FR_GLOBAL_PIXELS_PER_METER > 0.0f)
        ? frVec2ScalarMultiply(v, 1.0f / FR_GLOBAL_PIXELS_PER_METER)
        : FR_STRUCT_ZERO(Vector2);
}

/* 미터 단위 벡터 `v`를 픽셀 단위 벡터로 변환한다. */
Vector2 frVec2MetersToPixels(Vector2 v) {
    return (FR_GLOBAL_PIXELS_PER_METER > 0.0f)
        ? frVec2ScalarMultiply(v, FR_GLOBAL_PIXELS_PER_METER)
        : FR_STRUCT_ZERO(Vector2);
}