/*
    Copyright (c) 2021-2023 Jaedeok Kim <jdeokkim@protonmail.com>

    Permission is hereby granted, free of charge, to any person obtaining a 
    copyof this software and associated documentation files (the "Software"),
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

#include "ferox.h"
#include "greatest.h"

/* Macros ================================================================== */

GREATEST_MAIN_DEFS();

/* Private Function Prototypes ============================================= */

SUITE(CollisionSuite);

TEST CircleVsCircle(void);
TEST CircleVsPolygon(void);
TEST PolygonVsPolygon(void);

/* Public Functions ======================================================== */

int main(int argc, char *argv[]) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(CollisionSuite);

    GREATEST_MAIN_END();
}

/* Private Functions ======================================================= */

SUITE(CollisionSuite) {
    RUN_TEST(CircleVsCircle);
    RUN_TEST(CircleVsPolygon);
    RUN_TEST(PolygonVsPolygon);
}

TEST CircleVsCircle(void) {
    frShape *s1 = frCreateCircle(FR_API_STRUCT_ZERO(frMaterial), 1.0f);
    frShape *s2 = frCreateCircle(FR_API_STRUCT_ZERO(frMaterial), 2.0f);

    frBody *b1 = frCreateBodyFromShape(FR_BODY_DYNAMIC,
                                       (frVector2) { .x = -2.0f },
                                       s1);

    frBody *b2 = frCreateBodyFromShape(FR_BODY_DYNAMIC,
                                       (frVector2) { .x = 2.0f },
                                       s2);

    frCollision collision = { .count = 0 };

    ASSERT_EQ(frComputeCollision(b1, b2, &collision), false);

    frSetBodyPosition(b2, (frVector2) { .x = 1.0f });

    ASSERT_EQ(frComputeCollision(b1, b2, &collision), true);

    ASSERT_EQ(collision.count, 1);

    frReleaseBody(b2), frReleaseBody(b1);
    frReleaseShape(s2), frReleaseShape(s1);

    PASS();
}

TEST CircleVsPolygon(void) {
    frShape *s1 = frCreateCircle(FR_API_STRUCT_ZERO(frMaterial), 1.0f);
    frShape *s2 = frCreateRectangle(FR_API_STRUCT_ZERO(frMaterial), 4.0f, 3.0f);

    frBody *b1 = frCreateBodyFromShape(FR_BODY_DYNAMIC,
                                       (frVector2) { .x = -2.0f },
                                       s1);

    frBody *b2 = frCreateBodyFromShape(FR_BODY_DYNAMIC,
                                       (frVector2) { .x = 2.0f },
                                       s2);

    frCollision collision = { .count = 0 };

    ASSERT_EQ(frComputeCollision(b1, b2, &collision), false);

    frSetBodyPosition(b2, (frVector2) { .x = 1.0f });

    ASSERT_EQ(frComputeCollision(b1, b2, &collision), true);

    ASSERT_EQ(collision.count, 1);

    frReleaseBody(b2), frReleaseBody(b1);
    frReleaseShape(s2), frReleaseShape(s1);

    PASS();
}

TEST PolygonVsPolygon(void) {
    /* TODO: ... */

    PASS();
}