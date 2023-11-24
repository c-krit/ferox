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

SUITE(SpatialHashSuite);

TEST QuerySpatialHash(void);

/* Public Functions ======================================================== */

int main(int argc, char *argv[]) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(SpatialHashSuite);

    GREATEST_MAIN_END();
}

/* Private Functions ======================================================= */

SUITE(SpatialHashSuite) {
    RUN_TEST(QuerySpatialHash);
}
bool HashQueryCallback(int index, void *ctx) {
    int *count = (int *) ctx;

    (*count)++;

    return true;
};

TEST QuerySpatialHash(void) {
    const float CELL_SIZE = 4.0f;

    frSpatialHash *sh = frCreateSpatialHash(CELL_SIZE);

    ASSERT_EQ(frGetSpatialHashCellSize(sh), CELL_SIZE);

    frAABB keys[5] = {
        { .x = 3.0f, .y = 3.0f, .width = 3.0f, .height = 3.0f },
        { .x = 5.0f, .y = 4.0f, .width = 2.0f, .height = 2.0f },
        { .x = 5.0f, .y = 5.0f, .width = 2.0f, .height = 2.0f },
        { .x = 7.0f, .y = 7.0f, .width = 2.5f, .height = 2.5f },
        { .x = 6.0f, .y = 6.0f, .width = 2.7f, .height = 2.7f }
    };

    for (int i = 0; i < 5; i++) {
        frInsertToSpatialHash(sh, keys[i], i);
    }

    int count = 0;

    frQuerySpatialHash(sh,
                       (frAABB) { .x = 5, .y = 5, .width = 3, .height = 3 },
                       HashQueryCallback,
                       &count);

    ASSERT_EQ(count, 5);

    frReleaseSpatialHash(sh);

    PASS();
}