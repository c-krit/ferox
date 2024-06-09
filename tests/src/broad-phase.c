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

#define MAX_OBJECT_COUNT  1024
#define SIZE_IN_CELLS     32

/* Constants =============================================================== */

static const int CELL_SIZE = 2;

/* Private Function Prototypes ============================================= */

static bool utHashQuery(int index, void *ctx);

TEST utProximityQueries(void);

/* Public Functions ======================================================== */

SUITE(broad_phase) {
    RUN_TEST(utProximityQueries);
}

/* Private Functions ======================================================= */

static bool utHashQuery(int index, void *ctx) {
    int *queryResult = ctx;

    (*queryResult)++;

    return true;
}

TEST utProximityQueries(void) {
    frSpatialHash *sh = frCreateSpatialHash(CELL_SIZE);

    frAABB cursorBounds = { .width = 1.0f,
                            .height = 1.0f };

    int queryResult = 0;

    {
        for (int i = 0; i < MAX_OBJECT_COUNT; i++) {
            frVector2 offset = { .x = (CELL_SIZE >> 1), .y = (CELL_SIZE >> 1) };

            frAABB key = { .x = offset.x + ((i % SIZE_IN_CELLS) * CELL_SIZE),
                           .y = offset.y + ((i / SIZE_IN_CELLS) * CELL_SIZE),
                           .width = offset.x,
                           .height = offset.y };

            frInsertToSpatialHash(sh, key, i);
        }

        {
            cursorBounds.x = cursorBounds.y = CELL_SIZE / 4.0f;

            frQuerySpatialHash(sh, cursorBounds, utHashQuery, &queryResult);

            ASSERT_EQ(1, queryResult);
        }

        queryResult = 0;

        {
            cursorBounds.x = cursorBounds.y = CELL_SIZE;

            frQuerySpatialHash(sh, cursorBounds, utHashQuery, &queryResult);

            ASSERT_EQ(4, queryResult);
        }

        frClearSpatialHash(sh);
    }

    frReleaseSpatialHash(sh);

    PASS();
}