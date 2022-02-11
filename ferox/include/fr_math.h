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

#ifndef FR_MATH_H
#define FR_MATH_H

#include "ferox.h"

/* | 매크로 정의... | */

#ifdef _MSC_VER
    #define FR_MATH_INLINE __forceinline
#elif defined(__GNUC__)
    #if defined(__STRICT_ANSI__)
        #define FR_MATH_INLINE __inline__ __attribute__((always_inline))
    #else
        #define FR_MATH_INLINE inline __attribute__((always_inline))
    #endif
#else
    #define FR_MATH_INLINE
#endif

/* | 함수 정의... | */

/* 주어진 픽셀 단위 거리를 미터 단위 거리로 변환한다. */
FR_MATH_INLINE float frNumberPixelsToMeters(float value) {
    return (FR_GLOBAL_PIXELS_PER_METER > 0.0f)
        ? (value / FR_GLOBAL_PIXELS_PER_METER)
        : 0.0f;
}

/* 주어진 미터 단위 거리를 픽셀 단위 거리로 변환한다. */
FR_MATH_INLINE float frNumberMetersToPixels(float value) {
    return (FR_GLOBAL_PIXELS_PER_METER > 0.0f)
        ? (value * FR_GLOBAL_PIXELS_PER_METER)
        : 0.0f;
}

/* 주어진 픽셀 단위 `Rectangle` 구조체를 미터 단위 `Rectangle` 구조체로 변환한다. */
FR_MATH_INLINE Rectangle frRecPixelsToMeters(Rectangle rec) {
    return (Rectangle) {
        .x = frNumberPixelsToMeters(rec.x),
        .y = frNumberPixelsToMeters(rec.y),
        .width = frNumberPixelsToMeters(rec.width),
        .height = frNumberPixelsToMeters(rec.height)
    };
}

/* 주어진 미터 단위 `Rectangle` 구조체를 픽셀 단위 `Rectangle` 구조체로 변환한다. */
FR_MATH_INLINE Rectangle frRecMetersToPixels(Rectangle rec) {
    return (Rectangle) {
        .x = frNumberMetersToPixels(rec.x),
        .y = frNumberMetersToPixels(rec.y),
        .width = frNumberMetersToPixels(rec.width),
        .height = frNumberMetersToPixels(rec.height)
    };
}

/* 벡터 `v1`과 `v2`의 합을 반환한다. */
FR_MATH_INLINE Vector2 frVec2Add(Vector2 v1, Vector2 v2) {
    return (Vector2) { v1.x + v2.x, v1.y + v2.y };
}

/* 벡터 `v1`과 `v2`의 차를 반환한다. */
FR_MATH_INLINE Vector2 frVec2Subtract(Vector2 v1, Vector2 v2) {
    return (Vector2) { v1.x - v2.x, v1.y - v2.y };
}

/* 벡터 `v`의 각 성분에 `value`를 곱한 2차원 벡터를 반환한다. */
FR_MATH_INLINE Vector2 frVec2ScalarMultiply(Vector2 v, float value) {
    return (Vector2) { v.x * value, v.y * value };
}

/* 벡터 `v1`과 `v2`의 외적을 반환한다. */
FR_MATH_INLINE float frVec2CrossProduct(Vector2 v1, Vector2 v2) {
    // 평면 벡터의 외적은 스칼라 값이다.
    return (v1.x * v2.y) - (v1.y * v2.x);
}

/* 벡터 `v1`과 `v2`의 내적을 반환한다. */
FR_MATH_INLINE float frVec2DotProduct(Vector2 v1, Vector2 v2) {
    return (v1.x * v2.x) + (v1.y * v2.y);
}

/* 벡터 `v`의 크기의 제곱을 반환한다. */
FR_MATH_INLINE float frVec2MagnitudeSqr(Vector2 v) {
    return frVec2DotProduct(v, v);
}

/* 벡터 `v`의 크기를 반환한다. */
FR_MATH_INLINE float frVec2Magnitude(Vector2 v) {
    return sqrtf(frVec2MagnitudeSqr(v));
}

/* 벡터 `v`와 크기가 같고 방향이 반대인 2차원 벡터를 반환한다. */
FR_MATH_INLINE Vector2 frVec2Negate(Vector2 v) {
    return (Vector2) { -v.x, -v.y };
}

/* 벡터 `v`를 정규화한 2차원 벡터를 반환한다. */
FR_MATH_INLINE Vector2 frVec2Normalize(Vector2 v) {
    const float magnitude = frVec2Magnitude(v);
    
    return (magnitude > 0.0f)
        ? frVec2ScalarMultiply(v, 1.0f / magnitude)
        : FR_STRUCT_ZERO(Vector2);
}

/* 벡터 `v1`과 `v2`가 이루는 각도 (단위: rad.)를 반환한다. */
FR_MATH_INLINE float frVec2Angle(Vector2 v1, Vector2 v2) {
    return atan2f(v2.y, v2.x) - atan2f(v1.y, v1.x);
}

/* 벡터 `v1`의 모든 성분이 `v2`의 모든 성분과 근접한 값인지 확인한다. */
FR_MATH_INLINE bool frVec2ApproxEquals(Vector2 v1, Vector2 v2) {
    return frNumberApproxEquals(v1.x, v2.x) && frNumberApproxEquals(v1.y, v2.y);
}

/* 벡터 `v`의 왼쪽 방향에서 수직을 이루는 법선 벡터를 반환한다. */
FR_MATH_INLINE Vector2 frVec2LeftNormal(Vector2 v) {
    return frVec2Normalize((Vector2) { -v.y, v.x });
}

/* 벡터 `v`의 오른쪽 방향에서 수직을 이루는 법선 벡터를 반환한다. */
FR_MATH_INLINE Vector2 frVec2RightNormal(Vector2 v) {
    return frVec2Normalize((Vector2) { v.y, -v.x });
}

/* 영점을 기준으로 벡터 `v`를 `angle` (rad.)만큼 회전시킨 벡터를 반환한다. */
FR_MATH_INLINE Vector2 frVec2Rotate(Vector2 v, float angle) {
    const float sin_angle = sinf(angle);
    const float cos_angle = cosf(angle);
    
    return (Vector2) {
        (v.x * cos_angle - v.y * sin_angle),
        (v.x * sin_angle + v.y * cos_angle)
    };
}

/* 벡터 `v`를 `tx`의 값에 따라 회전시킨다. */
FR_MATH_INLINE Vector2 frVec2RotateTx(Vector2 v, frTransform tx) {
    Vector2 w = {
        (v.x * tx.cache.cos_a - v.y * tx.cache.sin_a),
        (v.x * tx.cache.sin_a + v.y * tx.cache.cos_a)
    };

    if (!tx.cache.valid) w = frVec2Rotate(v, tx.rotation);

    return w;
}

/* 벡터 `v`를 `tx`의 값에 따라 평행 이동하고 회전시킨다. */
FR_MATH_INLINE Vector2 frVec2Transform(Vector2 v, frTransform tx) {
    return frVec2Add(tx.position, frVec2RotateTx(v, tx));
}

/* 벡터 `v1`, `v2`와 `v3`가 반시계 방향으로 정렬되어 있는지 확인한다. */
FR_MATH_INLINE bool frVec2CCW(Vector2 v1, Vector2 v2, Vector2 v3) {
    return (v3.y - v1.y) * (v2.x - v1.x) < (v2.y - v1.y) * (v3.x - v1.x);
}

/* 픽셀 단위 벡터 `v`를 미터 단위 벡터로 변환한다. */
FR_MATH_INLINE Vector2 frVec2PixelsToMeters(Vector2 v) {
    return (FR_GLOBAL_PIXELS_PER_METER > 0.0f)
        ? frVec2ScalarMultiply(v, 1.0f / FR_GLOBAL_PIXELS_PER_METER)
        : FR_STRUCT_ZERO(Vector2);
}

/* 미터 단위 벡터 `v`를 픽셀 단위 벡터로 변환한다. */
FR_MATH_INLINE Vector2 frVec2MetersToPixels(Vector2 v) {
    return (FR_GLOBAL_PIXELS_PER_METER > 0.0f)
        ? frVec2ScalarMultiply(v, FR_GLOBAL_PIXELS_PER_METER)
        : FR_STRUCT_ZERO(Vector2);
}

#endif
