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

TEST utCircleVsCircle(void);
TEST utCircleVsPolygon(void);
TEST utPolygonVsPolygon(void);

/* Public Functions ======================================================== */

SUITE(collision) {
    RUN_TEST(utCircleVsCircle);
    RUN_TEST(utCircleVsPolygon);
    RUN_TEST(utPolygonVsPolygon);
}

/* Private Functions ======================================================= */

TEST utCircleVsCircle(void) {
    frShape *s1 = frCreateCircle(FR_API_STRUCT_ZERO(frMaterial), 1.0f);
    frShape *s2 = frCreateCircle(FR_API_STRUCT_ZERO(frMaterial), 2.0f);

    frBody *b1 = frCreateBodyFromShape(FR_BODY_STATIC,
                                       FR_API_STRUCT_ZERO(frVector2),
                                       s1);

    frBody *b2 = frCreateBodyFromShape(FR_BODY_STATIC,
                                       (frVector2) { .x = 4.0f, .y = 1.0f },
                                       s2);

    frCollision collision = { .count = 0 };

    {
        frComputeCollision(b1, b2, &collision);

        ASSERT_EQ(0, collision.count);
    }

    {
        frSetBodyPosition(b2, (frVector2) { .x = 3.0f });

        frComputeCollision(b1, b2, &collision);

        ASSERT_EQ(1, collision.count);

        ASSERT_IN_RANGE(1.0f, collision.direction.x, FLT_EPSILON);
        ASSERT_IN_RANGE(0.0f, collision.direction.y, FLT_EPSILON);

        ASSERT_IN_RANGE(0.0f, collision.contacts[0].depth, FLT_EPSILON);
    }

    {
        frSetBodyPosition(b2, (frVector2) { .x = 2.5f, .y = 1.0f });

        frComputeCollision(b1, b2, &collision);

        // TODO: ...
        // ASSERT_EQ(2, collision.count);
    }

    frReleaseBody(b2), frReleaseBody(b1);
    frReleaseShape(s2), frReleaseShape(s1);

    PASS();
}

TEST utCircleVsPolygon(void) {
    frShape *s1 = frCreateCircle(FR_API_STRUCT_ZERO(frMaterial), 1.0f);
    frShape *s2 = frCreateRectangle(FR_API_STRUCT_ZERO(frMaterial), 2.0f, 2.0f);

    frBody *b1 = frCreateBodyFromShape(FR_BODY_STATIC,
                                       FR_API_STRUCT_ZERO(frVector2),
                                       s1);

    frBody *b2 = frCreateBodyFromShape(FR_BODY_STATIC,
                                       (frVector2) { .x = 4.0f },
                                       s2);

    frCollision collision = { .count = 0 };

    {
        frComputeCollision(b1, b2, &collision);

        ASSERT_EQ(0, collision.count);
    }

    {
        frSetBodyPosition(b2, (frVector2) { .x = 2.0f });

        frComputeCollision(b1, b2, &collision);

        ASSERT_EQ(1, collision.count);
    }

    {
        frSetBodyPosition(b2, (frVector2) { .x = 1.5f });

        frComputeCollision(b1, b2, &collision);

        ASSERT_EQ(1, collision.count);
    }

    frReleaseBody(b2), frReleaseBody(b1);
    frReleaseShape(s2), frReleaseShape(s1);

    PASS();
}

TEST utPolygonVsPolygon(void) {
    frShape *s1 = frCreateRectangle(FR_API_STRUCT_ZERO(frMaterial), 1.0f, 1.0f);
    frShape *s2 = frCreateRectangle(FR_API_STRUCT_ZERO(frMaterial), 2.0f, 2.0f);

    frBody *b1 = frCreateBodyFromShape(FR_BODY_STATIC,
                                       FR_API_STRUCT_ZERO(frVector2),
                                       s1);

    frBody *b2 = frCreateBodyFromShape(FR_BODY_STATIC,
                                       (frVector2) { .x = 2.0f },
                                       s2);

    frCollision collision = { .count = 0 };

    {
        frComputeCollision(b1, b2, &collision);

        ASSERT_EQ(0, collision.count);
    }

    {
        // TODO: ...
    }

    {
        // TODO: ...
    }

    frReleaseBody(b2), frReleaseBody(b1);
    frReleaseShape(s2), frReleaseShape(s1);

    PASS();
}