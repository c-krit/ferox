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

#define STB_DS_IMPLEMENTATION
#include "external/stb_ds.h"

#include "ferox.h"

/* Typedefs ================================================================ */

/* A structure that represents the key of a spatial hash entry. */
typedef struct _frSpatialHashKey {
    int x, y;
} frSpatialHashKey;

/* A structure that represents the value of a spatial hash entry. */
typedef int *frSpatialHashValue;

/* A structure that represents the key-value pair of a spatial hash.*/
typedef struct _frSpatialHashEntry {
    frSpatialHashKey key;
    frSpatialHashValue value;
} frSpatialHashEntry;

/* A struct that represents a spatial hash. */
struct _frSpatialHash {
    float cellSize, inverseCellSize;
    frSpatialHashValue queryResult;
    frSpatialHashEntry *entries;
};

/* Private Function Prototypes ============================================= */

/* 
    Returns ​a negative integer value if `a` is less than `b`, ​a positive 
    integer value if `a` is greater than `b` and zero if `a` and `b` 
    are equivalent.
*/
static int frQSortCompare(const void *a, const void *b);

/* Public Functions ======================================================== */

/* Creates a new spatial hash with the given `cellSize`. */
frSpatialHash *frCreateSpatialHash(float cellSize) {
    if (cellSize <= 0.0f) return NULL;

    // NOTE: `sh->queryResult` and `sh->entries` must be initialized to `NULL`
    frSpatialHash *sh = calloc(1, sizeof *sh);

    sh->cellSize = cellSize;
    sh->inverseCellSize = 1.0f / cellSize;

    return sh;
}

/* Releases the memory allocated by `sh`. */
void frReleaseSpatialHash(frSpatialHash *sh) {
    if (sh == NULL) return;

    for (int i = 0; i < hmlen(sh->entries); i++)
        arrfree(sh->entries[i].value);

    hmfree(sh->entries), arrfree(sh->queryResult), free(sh);
}

/* Erases all elements from `sh`. */
void frClearSpatialHash(frSpatialHash *sh) {
    if (sh == NULL) return;

    arrsetlen(sh->queryResult, 0);

    for (int i = 0; i < hmlen(sh->entries); i++)
        arrsetlen(sh->entries[i].value, 0);
}

/* Returns the cell size of `sh`. */
float frGetSpatialHashCellSize(const frSpatialHash *sh) {
    return (sh != NULL) ? sh->cellSize : 0.0f;
}

/* Inserts a `key`-`value` pair into `sh`. */
void frInsertIntoSpatialHash(frSpatialHash *sh, frAABB key, int value) {
    if (sh == NULL) return;

    float inverseCellSize = sh->inverseCellSize;

    int minX = key.x * inverseCellSize;
    int minY = key.y * inverseCellSize;

    int maxX = (key.x + key.width) * inverseCellSize;
    int maxY = (key.y + key.height) * inverseCellSize;

    for (int y = minY; y <= maxY; y++)
        for (int x = minX; x <= maxX; x++) {
            const frSpatialHashKey key = { .x = x, .y = y };

            frSpatialHashEntry *entry = hmgetp_null(sh->entries, key);

            if (entry != NULL) {
                arrput(entry->value, value);
            } else {
                frSpatialHashEntry newEntry = { .key = key };

                arrput(newEntry.value, value);

                hmputs(sh->entries, newEntry);
            }
        }
}

/* Query `sh` for any objects that are likely to overlap the given `aabb`. */
void frQuerySpatialHash(frSpatialHash *sh,
                        frAABB aabb,
                        frHashQueryFunc func,
                        void *ctx) {
    if (sh == NULL) return;

    float inverseCellSize = sh->inverseCellSize;

    int minX = aabb.x * inverseCellSize;
    int minY = aabb.y * inverseCellSize;

    int maxX = (aabb.x + aabb.width) * inverseCellSize;
    int maxY = (aabb.y + aabb.height) * inverseCellSize;

    arrsetlen(sh->queryResult, 0);

    for (int y = minY; y <= maxY; y++)
        for (int x = minX; x <= maxX; x++) {
            const frSpatialHashKey key = { .x = x, .y = y };

            frSpatialHashEntry *entry = hmgetp_null(sh->entries, key);

            if (entry == NULL) continue;

            for (int i = 0; i < arrlen(entry->value); i++)
                arrput(sh->queryResult, entry->value[i]);
        }

    size_t oldLength = arrlen(sh->queryResult);

    if (oldLength > 1) {
        // NOTE: Sort the array first, then remove duplicates!
        qsort(sh->queryResult,
              oldLength,
              sizeof *(sh->queryResult),
              frQSortCompare);

        size_t newLength = 0;

        for (int i = 0; i < oldLength; i++)
            if (sh->queryResult[i] != sh->queryResult[i + 1])
                sh->queryResult[newLength++] = sh->queryResult[i];

        if (sh->queryResult[newLength - 1] != sh->queryResult[oldLength - 1])
            sh->queryResult[newLength++] = sh->queryResult[oldLength - 1];

        arrsetlen(sh->queryResult, newLength);
    }

    /*
        NOTE: For each object in the query result, the callback `func`tion
        will be called with the user data pointer `ctx`.
    */
    for (int i = 0; i < arrlen(sh->queryResult); i++)
        func(sh->queryResult[i], ctx);
}

/* Private Functions ======================================================= */

/* 
    Returns ​a negative integer value if `a` is less than `b`, ​a positive 
    integer value if `a` is greater than `b` and zero if `a` and `b` 
    are equivalent.
*/
static int frQSortCompare(const void *a, const void *b) {
    const int x = *(const int *) a;
    const int y = *(const int *) b;

    return (x > y) - (x < y);
}
