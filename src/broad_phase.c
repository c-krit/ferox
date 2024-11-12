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

#include "external/ferox_utils.h"
#include "external/stb_ds.h"

#include "ferox.h"

/* Typedefs ===============================================================> */

/* 
    A structure that represents a two-dimensional vector 
    with `int` coordinates.
*/
typedef struct frVector2i_ {
    int x, y;
} frVector2i;

/* A structure that represents the key-value pair of a spatial hash.*/
typedef struct frSpatialHashEntry_ {
    frVector2i key;
    frDynArray(int) value;
} frSpatialHashEntry;

/* A struct that represents a spatial hash. */
struct frSpatialHash_ {
    frSpatialHashEntry *entries;
    float cellSize, inverseCellSize;
    frDynArray(int) queryResult;
    frBitArray indexSet;
};

/* Public Functions =======================================================> */

/* Creates a new spatial hash with the given `cellSize`. */
frSpatialHash *frCreateSpatialHash(float cellSize) {
    if (cellSize <= 0.0f) return NULL;

    frSpatialHash *sh = calloc(1, sizeof *sh);

    sh->cellSize = cellSize;
    sh->inverseCellSize = 1.0f / cellSize;

    sh->indexSet = frCreateBitArray(FR_WORLD_MAX_OBJECT_COUNT);

    frInitDynArray(sh->queryResult);

    return sh;
}

/* Releases the memory allocated by `sh`. */
void frReleaseSpatialHash(frSpatialHash *sh) {
    if (sh == NULL) return;

    for (int i = 0; i < hmlen(sh->entries); i++)
        frReleaseDynArray(sh->entries[i].value);

    frReleaseBitArray(sh->indexSet);
    frReleaseDynArray(sh->queryResult);

    hmfree(sh->entries), free(sh);
}

/* Erases all elements from `sh`. */
void frClearSpatialHash(frSpatialHash *sh) {
    if (sh == NULL) return;

    for (int i = 0; i < hmlen(sh->entries); i++)
        frSetDynArrayLength(sh->entries[i].value, 0);
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
            frVector2i key = { .x = x, .y = y };

            frSpatialHashEntry *entry = hmgetp_null(sh->entries, key);

            if (entry != NULL) {
                frDynArrayPush(entry->value, value);
            } else {
                frSpatialHashEntry newEntry = { .key = key };

                frInitDynArray(newEntry.value);
                frDynArrayPush(newEntry.value, value);

                hmputs(sh->entries, newEntry);
            }
        }
}

/* Query `sh` for any objects that are likely to overlap the given `aabb`. */
void frQuerySpatialHash(frSpatialHash *sh,
                        frAABB aabb,
                        frHashQueryFunc func,
                        void *userData) {
    if (sh == NULL) return;

    float inverseCellSize = sh->inverseCellSize;

    int minX = aabb.x * inverseCellSize;
    int minY = aabb.y * inverseCellSize;

    int maxX = (aabb.x + aabb.width) * inverseCellSize;
    int maxY = (aabb.y + aabb.height) * inverseCellSize;

    frSetDynArrayLength(sh->queryResult, 0);

    for (int y = minY; y <= maxY; y++)
        for (int x = minX; x <= maxX; x++) {
            frVector2i key = { .x = x, .y = y };

            frSpatialHashEntry *entry = hmgetp_null(sh->entries, key);

            if (entry == NULL) continue;

            for (int i = 0; i < frGetDynArrayLength(entry->value); i++)
                frDynArrayPush(sh->queryResult,
                               frGetDynArrayValue(entry->value, i));
        }

    {
        frBitArrayClear(sh->indexSet, FR_WORLD_MAX_OBJECT_COUNT);

        for (int i = 0; i < frGetDynArrayLength(sh->queryResult); i++)
            frBitArraySet(sh->indexSet, frGetDynArrayValue(sh->queryResult, i));

        frSetDynArrayLength(sh->queryResult, 0);

        for (int i = 0; i < FR_WORLD_MAX_OBJECT_COUNT; i++)
            if (frBitArrayGet(sh->indexSet, i))
                frDynArrayPush(sh->queryResult, i);
    }

    /*
        NOTE: For each object in the query result, the callback `func`tion
        will be called with the user data pointer `ctx`.
    */
    for (int i = 0; i < frGetDynArrayLength(sh->queryResult); i++)
        func((frContextNode) { .id = frGetDynArrayValue(sh->queryResult, i),
                               .ctx = userData });
}
