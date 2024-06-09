/*
    Copyright (c) 2021-2023 Jaedeok Kim <jdeokkim@protonmail.com>

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

#include <float.h>

#include "ferox.h"
#include "greatest.h"

/* Private Function Prototypes ============================================= */

TEST utComputeAABBs(void);
TEST utConvexHull(void);

/* Public Functions ======================================================== */

SUITE(geometry) {
    RUN_TEST(utComputeAABBs);
    RUN_TEST(utConvexHull);
}

/* Private Functions ======================================================= */

TEST utComputeAABBs(void) {
    frShape *s1 = frCreateCircle(FR_API_STRUCT_ZERO(frMaterial), 1.0f);
    frShape *s2 = frCreateRectangle(FR_API_STRUCT_ZERO(frMaterial), 1.0f, 1.0f);

    frBody *b = frCreateBodyFromShape(FR_BODY_STATIC,
                                      FR_API_STRUCT_ZERO(frVector2),
                                      s1);

    frAABB aabb = frGetBodyAABB(b);

    {
        ASSERT_IN_RANGE(-1.0f, aabb.x, FLT_EPSILON);
        ASSERT_IN_RANGE(-1.0f, aabb.y, FLT_EPSILON);

        ASSERT_IN_RANGE(2.0f, aabb.width, FLT_EPSILON);
        ASSERT_IN_RANGE(2.0f, aabb.height, FLT_EPSILON);
    }

    {
        // TODO: ...
    }

    frReleaseBody(b), frReleaseShape(s2), frReleaseShape(s1);

    PASS();
}

TEST utConvexHull(void) {
    frShape *s = frCreatePolygon(FR_API_STRUCT_ZERO(frMaterial),
                                 &(const frVertices) {
                                    .data = {
                                        (frVector2) { .x = 1.0f, .y = 1.0f },
                                        (frVector2) { .x = 0.75f, .y = 1.5f },
                                        (frVector2) { .x = -1.5f, .y = 1.5f },
                                        (frVector2) { .x = -1.25f, .y = 1.0f },
                                        (frVector2) { .x = -1.75f, .y = 0.5f },
                                        (frVector2) { .x = -1.0f, .y = -0.75f },
                                        (frVector2) { .x = -0.1f, .y = 0.5f },
                                    },
                                    .count = 7
                                 });

    ASSERT_EQ(5, frGetPolygonVertices(s)->count);

    PASS();
}