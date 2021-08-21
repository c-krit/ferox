/*
    Copyright (c) 2021 jdeokkim

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

#include "ferox.h"

/* | `broadphase` 모듈 구조체... | */

/* 공간 해시맵의 키와 값을 나타내는 구조체. */
typedef struct frSpatialHashEntry {
    int key;
    int *values;
} frSpatialHashEntry;

/* 공간 해시맵을 나타내는 구조체. */
typedef struct frSpatialHash {
    Rectangle bounds;
    float cell_size;
    float inverse_cell_size;
    frSpatialHashEntry *map;
} frSpatialHash;

/* | `broadphase` 모듈 함수... | */

/* C 표준 라이브러리의 `qsort()` 함수 호출에 사용되는 비교 함수이다. */
static int frQuickSortCallback(const void *x, const void *y);

/* 경계 범위가 `bounds`이고 각 셀의 크기가 `cell_size`인 공간 해시맵의 메모리 주소를 반환한다. */
frSpatialHash *frCreateSpatialHash(Rectangle bounds, float cell_size) {
    if (cell_size <= 0.0f) return NULL;
    
    frSpatialHash *result = calloc(1, sizeof(frSpatialHash));
    
    result->bounds = bounds;
    result->cell_size = cell_size;
    result->inverse_cell_size = 1.0f / cell_size;
    
    return result;
}

/* 공간 해시맵 `hash`에 할당된 메모리를 해제한다. */
void frReleaseSpatialHash(frSpatialHash *hash) {
    if (hash == NULL) return; 
    
    for (int i = 0; i < hmlen(hash->map); i++)
        frReleaseArray(hash->map[i].values);
    
    hmfree(hash->map);
    free(hash);
}

/* 공간 해시맵 `hash`에 직사각형 `rect`로 생성한 키와 `value`를 추가한다. */
void frAddToSpatialHash(frSpatialHash *hash, Rectangle rect, int value) {
    if (hash == NULL) return;
    
    int x0 = frComputeSpatialHashKey(hash, (Vector2) { .x = rect.x });    
    int x1 = frComputeSpatialHashKey(hash, (Vector2) { .x = rect.x + rect.width });
    
    int y0 = frComputeSpatialHashKey(hash, (Vector2) { .y = rect.y });
    int y1 = frComputeSpatialHashKey(hash, (Vector2) { .y = rect.y + rect.height });
    
    for (int y = y0; y <= y1; y += hash->bounds.width) {
        for (int x = x0; x <= x1; x++) {
            frSpatialHashEntry *entry = hmgetp_null(hash->map, x + y);
            
            if (entry != NULL) {
                frAddToArray(entry->values, value);
            } else {
                frSpatialHashEntry entry = (frSpatialHashEntry) { x + y, NULL };
                frAddToArray(entry.values, value);

                hmputs(hash->map, entry);
            }
        }
    }
}

/* 공간 해시맵 `hash`의 모든 키와 값을 제거한다. */
void frClearSpatialHash(frSpatialHash *hash) {
    if (hash == NULL) return;
    
    for (int i = 0; i < hmlen(hash->map); i++)
        frClearArray(hash->map[i].values);
}

/* 공간 해시맵 `hash`의 경계 범위를 반환한다. */
Rectangle frGetSpatialHashBounds(frSpatialHash *hash) {
    return (hash != NULL) ? hash->bounds : FR_STRUCT_ZERO(Rectangle);
}

/* 공간 해시맵 `hash`에서 키가 `key`인 값을 제거한다. */
void frRemoveFromSpatialHash(frSpatialHash *hash, int key) {
    if (hash == NULL) return;
    
    frSpatialHashEntry entry = hmgets(hash->map, key);
    frReleaseArray(entry.values);
    
    hmdel(hash->map, key);
}

/* 공간 해시맵 `hash`에서 직사각형 `rect`와 경계 범위가 겹치는 모든 도형의 인덱스를 반환한다. */
void frQuerySpatialHash(frSpatialHash *hash, Rectangle rect, int **result) {
    if (hash == NULL || result == NULL) return;
    
    int x0 = frComputeSpatialHashKey(hash, (Vector2) { .x = rect.x });
    int x1 = frComputeSpatialHashKey(hash, (Vector2) { .x = rect.x + rect.width });
    
    int y0 = frComputeSpatialHashKey(hash, (Vector2) { .y = rect.y });
    int y1 = frComputeSpatialHashKey(hash, (Vector2) { .y = rect.y + rect.height });
    
    for (int y = y0; y <= y1; y += hash->bounds.width) {
        for (int x = x0; x <= x1; x++) {
            frSpatialHashEntry *entry = hmgetp_null(hash->map, x + y);

            if (entry != NULL) 
                for (int j = 0; j < frGetArrayLength(entry->values); j++)
                    frAddToArray(*result, entry->values[j]);
        }
    }
    
    qsort(*result, arrlen(*result), sizeof(int), frQuickSortCallback);
    
    for (int i = 0; i < arrlen(*result); i++)
        while ((i + 1) < arrlen(*result) && (*result)[i + 1] == (*result)[i])
            arrdel(*result, i + 1);
}

/* 공간 해시맵 `hash`에서 벡터 `v`와 대응하는 키를 반환한다. */
int frComputeSpatialHashKey(frSpatialHash *hash, Vector2 v) {
    return (hash != NULL) 
        ? (int) (v.y * hash->inverse_cell_size) * hash->bounds.width + (int) (v.x * hash->inverse_cell_size)
        : -1;
}

/* C 표준 라이브러리의 `qsort()` 함수 호출에 사용되는 비교 함수이다. */
static int frQuickSortCallback(const void *x, const void *y) {
    int nx = *(const int *) x, ny = *(const int *) y;
    
    return (nx > ny) - (nx < ny);
}