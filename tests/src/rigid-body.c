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

#include "ferox.h"
#include "greatest.h"

/* Private Function Prototypes ============================================= */

TEST utPointInPolygon(void);

/* Public Functions ======================================================== */

SUITE(rigid_body) {
    RUN_TEST(utPointInPolygon);
}

/* Private Functions ======================================================= */

TEST utPointInPolygon(void) {
    frShape *s = frCreateRectangle(FR_API_STRUCT_ZERO(frMaterial), 8.0f, 8.0f);

    frBody *b = frCreateBodyFromShape(FR_BODY_STATIC,
                                      FR_API_STRUCT_ZERO(frVector2),
                                      s);

    {
        frSetBodyAngle(b, 0.25f * M_PI);

        ASSERT_EQ(false,
                  frBodyContainsPoint(b,
                                      (frVector2) { .x = -4.0f, .y = -4.0f }));

        // TODO: ...
    }

    frReleaseBody(b), frReleaseShape(s);

    PASS();
}
