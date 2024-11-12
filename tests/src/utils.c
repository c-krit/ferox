/*
    Copyright (c) 2021-2024 Jaedeok Kim <jdeokkim@protonmail.com>

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

/* Includes ===============================================================> */

#include "ferox.h"
#include "ferox_utils.h"

#include "greatest.h"

/* Macros =================================================================> */

#define BIT_ARRAY_LENGTH   ((1 << 6) + 1)
#define DYN_ARRAY_CAPACITY ((1 << 5) + 1)
#define RING_BUFFER_LENGTH ((1 << 4) + 1)

/* Private Function Prototypes ============================================> */

TEST utBitArrayOps(void);
TEST utDynArrayOps(void);
TEST utRingBufferOps(void);

/* Public Functions =======================================================> */

SUITE(utils) {
    RUN_TEST(utBitArrayOps);
    RUN_TEST(utDynArrayOps);
    RUN_TEST(utRingBufferOps);
}

/* Private Functions ======================================================> */

TEST utBitArrayOps(void) {
    frBitArray ba = frCreateBitArray(BIT_ARRAY_LENGTH);

    {
        const int numbers[] = { 42, 4, 15, 8, 15, 16, 4, 23, 15, 42, 16 };

        for (int i = 0, j = (sizeof numbers / sizeof *numbers); i < j; i++)
            frBitArraySet(ba, numbers[i]);

        int numberCount = 0;

        for (int i = 0; i < BIT_ARRAY_LENGTH; i++)
            if (frBitArrayGet(ba, i)) numberCount++;

        ASSERT_EQ(6, numberCount);
    }

    frReleaseBitArray(ba);

    PASS();
}

TEST utDynArrayOps(void) {
    frDynArray(int) values;

    frInitDynArray(values);

    {
        frSetDynArrayCapacity(values, DYN_ARRAY_CAPACITY);

        ASSERT_EQ(DYN_ARRAY_CAPACITY, frGetDynArrayCapacity(values));

        for (int i = 0; i < DYN_ARRAY_CAPACITY + 1; i++)
            frDynArrayPush(values, 2 * i);

        ASSERT_LT(DYN_ARRAY_CAPACITY, frGetDynArrayCapacity(values));

        ASSERT_EQ((DYN_ARRAY_CAPACITY << 1),
                  frGetDynArrayValue(values, DYN_ARRAY_CAPACITY));

        frSetDynArrayCapacity(values, (DYN_ARRAY_CAPACITY >> 1));

        ASSERT_EQ((DYN_ARRAY_CAPACITY >> 1), frGetDynArrayLength(values));
    }

    frReleaseDynArray(values);

    PASS();
}

TEST utRingBufferOps(void) {
    frRingBuffer(frContextNode) rbf;

    frInitRingBuffer(rbf, RING_BUFFER_LENGTH);

    {
        frContextNode node = { .id = -1 };

        for (int i = 0; i < RING_BUFFER_LENGTH; i++) {
            node.id = (i << 1);

            ASSERT_EQ(true, frAddToRingBuffer(rbf, node));
        }

        while (frRemoveFromRingBuffer(rbf, &node))
            ASSERT_EQ(0, node.id & 1);
    }

    frReleaseRingBuffer(rbf);

    PASS();
}
