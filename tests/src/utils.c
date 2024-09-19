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

/* Macros ================================================================== */

#define RING_BUFFER_LENGTH  (1 << 3)

/* Private Function Prototypes ============================================= */

TEST utRingBufferOps(void);

/* Public Functions ======================================================== */

SUITE(utils) {
    RUN_TEST(utRingBufferOps);
}

/* Private Functions ======================================================= */

TEST utRingBufferOps(void) {
    frRingBuffer *rbf = frCreateRingBuffer(RING_BUFFER_LENGTH);

    {
        frIndexedData value = { .idx = 0 };

        for (int i = 0; i < RING_BUFFER_LENGTH; i++) {
            value.idx = i;

            bool result = (i < (RING_BUFFER_LENGTH - 1));

            ASSERT_EQ(result, frAddValueToRingBuffer(rbf, value));
        }

        ASSERT_EQ(false, frAddValueToRingBuffer(rbf, value));

        for (int i = 0; i < RING_BUFFER_LENGTH; i++) {
            bool result = (i < (RING_BUFFER_LENGTH - 1));

            ASSERT_EQ(result, frRemoveValueFromRingBuffer(rbf, &value));
            
            if (result == true) ASSERT_EQ(i, value.idx);
        }

        ASSERT_EQ(false, frRemoveValueFromRingBuffer(rbf, NULL));
    }

    frReleaseRingBuffer(rbf);

    PASS();
}
