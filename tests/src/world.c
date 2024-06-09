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

/* Constants =============================================================== */

static const float CELL_SIZE = 2.0f, DELTA_TIME = 1.0f / 60.0f;

/* Private Function Prototypes ============================================= */

TEST utStepWorld(void);

/* Public Functions ======================================================== */

SUITE(world) {
    RUN_TEST(utStepWorld);
}

/* Private Functions ======================================================= */

TEST utStepWorld(void) {
    frWorld *w = frCreateWorld(frVector2ScalarMultiply(FR_WORLD_DEFAULT_GRAVITY,
                                                       1.0f),
                               CELL_SIZE);

    frBody *ground = frCreateBodyFromShape(
        FR_BODY_STATIC,
        (frVector2) { .y = 4.0f },
        frCreateRectangle((frMaterial) { .density = 1.0f, .friction = 0.5f },
                          16.0f,
                          2.0f));

    frAddBodyToWorld(w, ground);

    frBody *box = frCreateBodyFromShape(
        FR_BODY_DYNAMIC,
        FR_API_STRUCT_ZERO(frVector2),
        frCreateRectangle((frMaterial) { .density = 1.0f, .friction = 0.5f },
                          1.0f,
                          1.0f));

    frAddBodyToWorld(w, box);

    {
        frStepWorld(w, DELTA_TIME);

        ASSERT_EQ(false, frComputeCollision(box, ground, NULL));

        // TODO: ...
    }

    frReleaseShape(frGetBodyShape(ground));
    frReleaseShape(frGetBodyShape(box));

    frReleaseWorld(w);

    PASS();
}
