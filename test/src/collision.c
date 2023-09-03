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

#include "ferox.h"
#include "greatest.h"

/* Macros =============================================================================== */

#define FR_TEST_EPSILON 0.000001f

/* Public Function Prototypes =========================================================== */

TEST BoxToBox1(void);
TEST BoxToBox2(void);
TEST BoxToBox3(void);
TEST BoxToBox4(void);
TEST BoxToBox5(void);
TEST BoxToBox6(void);
TEST BoxToBox7(void);

/* Public Functions ===================================================================== */

GREATEST_MAIN_DEFS();

SUITE(BoxToBox) {
    RUN_TEST(BoxToBox1);
    RUN_TEST(BoxToBox2);
    RUN_TEST(BoxToBox3);
    RUN_TEST(BoxToBox4);
    RUN_TEST(BoxToBox5);
    RUN_TEST(BoxToBox6);
    RUN_TEST(BoxToBox7);
}

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(BoxToBox);

    /* TODO: ... */

    GREATEST_MAIN_END();
}

TEST BoxToBox1(void) {
    frShape *s1 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(150.0f),
        frPixelsToUnits(100.0f)
    );

    frBody *b1 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = -50.0f,
                .y = 0.0f
            }
        ),
        s1
    );

    frShape *s2 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(150.0f),
        frPixelsToUnits(50.0f)
    );

    frBody *b2 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = 50.0f,
                .y = 0.0f
            }
        ),
        s2
    );

    frTransform tx1 = frGetBodyTransform(b1);
    frTransform tx2 = frGetBodyTransform(b2);

    frCollision collision = { .count = 0 };

    frComputeCollision(s1, tx1, s2, tx2, &collision);

    ASSERT_EQ(collision.count, 2);

    ASSERT_IN_RANGE(1.000000f, collision.direction.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(0.000000f, collision.direction.y, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(-1.562500f, collision.contacts[0].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(-1.562500f, collision.contacts[0].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(3.125000f, collision.contacts[0].depth, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(-1.562500f, collision.contacts[1].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(1.562500f, collision.contacts[1].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(3.125000f, collision.contacts[1].depth, FR_TEST_EPSILON);

    frReleaseShape(s2), frReleaseShape(s1);
    frReleaseBody(b2), frReleaseBody(b1);

    PASS();
}

TEST BoxToBox2(void) {
    frShape *s1 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(150.0f),
        frPixelsToUnits(100.0f)
    );

    frBody *b1 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = -50.0f,
                .y = 0.0f
            }
        ),
        s1
    );

    frShape *s2 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(150.0f),
        frPixelsToUnits(200.0f)
    );

    frBody *b2 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = 40.0f,
                .y = 20.0f
            }
        ),
        s2
    );

    frTransform tx1 = frGetBodyTransform(b1);
    frTransform tx2 = frGetBodyTransform(b2);

    frCollision collision = { .count = 0 };

    frComputeCollision(s1, tx1, s2, tx2, &collision);

    ASSERT_EQ(collision.count, 2);

    ASSERT_IN_RANGE(1.000000f, collision.direction.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(0.000000f, collision.direction.y, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(-2.187500f, collision.contacts[0].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(3.125000f, collision.contacts[0].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(3.750000f, collision.contacts[0].depth, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(-2.187500f, collision.contacts[1].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(-3.125000f, collision.contacts[1].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(3.750000f, collision.contacts[1].depth, FR_TEST_EPSILON);

    frReleaseShape(s2), frReleaseShape(s1);
    frReleaseBody(b2), frReleaseBody(b1);

    PASS();
}

TEST BoxToBox3(void) {
    frShape *s1 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(150.0f),
        frPixelsToUnits(100.0f)
    );

    frBody *b1 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = -50.0f,
                .y = 0.0f
            }
        ),
        s1
    );

    frSetBodyAngle(b1, (M_PI / 180.0f) * 15.0f);

    frShape *s2 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(150.0f),
        frPixelsToUnits(200.0f)
    );

    frBody *b2 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = 40.0f,
                .y = 80.0f
            }
        ),
        s2
    );

    frTransform tx1 = frGetBodyTransform(b1);
    frTransform tx2 = frGetBodyTransform(b2);

    frCollision collision = { .count = 0 };

    frComputeCollision(s1, tx1, s2, tx2, &collision);

    ASSERT_EQ(collision.count, 2);

    ASSERT_IN_RANGE(0.965926f, collision.direction.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(0.258819f, collision.direction.y, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(-2.187500f, collision.contacts[0].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(-1.250000f, collision.contacts[0].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(4.105468f, collision.contacts[0].depth, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(-2.187500f, collision.contacts[1].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(3.486440f, collision.contacts[1].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(2.879587f, collision.contacts[1].depth, FR_TEST_EPSILON);

    frReleaseShape(s2), frReleaseShape(s1);
    frReleaseBody(b2), frReleaseBody(b1);

    PASS();
}

TEST BoxToBox4(void) {
    frShape *s1 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(150.0f),
        frPixelsToUnits(100.0f)
    );

    frBody *b1 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = -50.0f,
                .y = 0.0f
            }
        ),
        s1
    );

    frSetBodyAngle(b1, (M_PI / 180.0f) * 15.0f);

    frShape *s2 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(150.0f),
        frPixelsToUnits(200.0f)
    );

    frBody *b2 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = 40.0f,
                .y = -80.0f
            }
        ),
        s2
    );

    frTransform tx1 = frGetBodyTransform(b1);
    frTransform tx2 = frGetBodyTransform(b2);

    frCollision collision = { .count = 0 };

    frComputeCollision(s1, tx1, s2, tx2, &collision);

    ASSERT_EQ(collision.count, 2);

    ASSERT_IN_RANGE(0.258819f, collision.direction.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(-0.965926f, collision.direction.y, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(-2.187500f, collision.contacts[0].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(1.250000f, collision.contacts[0].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(4.089765f, collision.contacts[0].depth, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(1.392921f, collision.contacts[1].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(1.250000f, collision.contacts[1].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(3.163084f, collision.contacts[1].depth, FR_TEST_EPSILON);

    frReleaseShape(s2), frReleaseShape(s1);
    frReleaseBody(b2), frReleaseBody(b1);

    PASS();
}

TEST BoxToBox5(void) {
    frShape *s1 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(150.0f),
        frPixelsToUnits(100.0f)
    );

    frBody *b1 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = -50.0f,
                .y = 0.0f
            }
        ),
        s1
    );

    frSetBodyAngle(b1, (M_PI / 180.0f) * 15.0f);

    frShape *s2 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(150.0f),
        frPixelsToUnits(200.0f)
    );

    frBody *b2 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = 40.0f,
                .y = 0.0f
            }
        ),
        s2
    );

    frTransform tx1 = frGetBodyTransform(b1);
    frTransform tx2 = frGetBodyTransform(b2);

    frCollision collision = { .count = 0 };

    frComputeCollision(s1, tx1, s2, tx2, &collision);

    ASSERT_EQ(collision.count, 2);

    ASSERT_IN_RANGE(1.000000f, collision.direction.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(0.000000f, collision.direction.y, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(0.593968f, collision.contacts[0].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(4.231732f, collision.contacts[0].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(2.781468f, collision.contacts[0].depth, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(2.211587f, collision.contacts[1].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(-1.805304f, collision.contacts[1].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(4.399087f, collision.contacts[1].depth, FR_TEST_EPSILON);

    frReleaseShape(s2), frReleaseShape(s1);
    frReleaseBody(b2), frReleaseBody(b1);

    PASS();
}

TEST BoxToBox6(void) {
    frShape *s1 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(450.0f),
        frPixelsToUnits(50.0f)
    );

    frBody *b1 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = 0.0f,
                .y = 80.0f
            }
        ),
        s1
    );

    frShape *s2 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(50.0f),
        frPixelsToUnits(50.0f)
    );

    frBody *b2 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = 40.0f,
                .y = 32.0f
            }
        ),
        s2
    );

    frSetBodyAngle(b2, (M_PI / 180.0f) * 15.0f);

    frTransform tx1 = frGetBodyTransform(b1);
    frTransform tx2 = frGetBodyTransform(b2);

    frCollision collision = { .count = 0 };

    frComputeCollision(s1, tx1, s2, tx2, &collision);

    ASSERT_EQ(collision.count, 1);

    ASSERT_IN_RANGE(0.000000f, collision.direction.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(-1.000000f, collision.direction.y, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(3.604854f, collision.contacts[0].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(3.913664f, collision.contacts[0].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(0.476164f, collision.contacts[0].depth, FR_TEST_EPSILON);

    frReleaseShape(s2), frReleaseShape(s1);
    frReleaseBody(b2), frReleaseBody(b1);

    PASS();
}

TEST BoxToBox7(void) {
    frShape *s1 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(450.0f),
        frPixelsToUnits(50.0f)
    );

    frBody *b1 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = 0.0f,
                .y = 80.0f
            }
        ),
        s1
    );

    frShape *s2 = frCreateRectangle(
        FR_API_STRUCT_ZERO(frMaterial),
        frPixelsToUnits(50.0f),
        frPixelsToUnits(50.0f)
    );

    frBody *b2 = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        frVector2PixelsToUnits(
            (frVector2) { 
                .x = 40.0f,
                .y = 40.0f
            }
        ),
        s2
    );

    frSetBodyAngle(b2, (M_PI / 180.0f) * 15.0f);

    frTransform tx1 = frGetBodyTransform(b1);
    frTransform tx2 = frGetBodyTransform(b2);

    frCollision collision = { .count = 0 };

    frComputeCollision(s1, tx1, s2, tx2, &collision);

    ASSERT_EQ(collision.count, 2);

    ASSERT_IN_RANGE(0.000000f, collision.direction.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(-1.000000f, collision.direction.y, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(0.586336f, collision.contacts[0].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(3.604854f, collision.contacts[0].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(0.167354f, collision.contacts[0].depth, FR_TEST_EPSILON);

    ASSERT_IN_RANGE(3.604854f, collision.contacts[1].point.x, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(4.413664f, collision.contacts[1].point.y, FR_TEST_EPSILON);
    ASSERT_IN_RANGE(0.976164f, collision.contacts[1].depth, FR_TEST_EPSILON);

    frReleaseShape(s2), frReleaseShape(s1);
    frReleaseBody(b2), frReleaseBody(b1);

    PASS();
}