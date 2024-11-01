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

#include <limits.h>

#include "ferox.h"

/* Macros =================================================================> */

#define INT_BIT  (sizeof(int) * CHAR_BIT)

/* Typedefs ===============================================================> */

/* A structure that represents a ring buffer for storing indexed data. */
struct frRingBuffer_ {
    frContextNode *buffer;
    int head, tail, length;
};

/* Public Functions =======================================================> */

/* Creates a bit array with `count` bits. */
frBitArray frCreateBitArray(int count) {
    int size = (count + (INT_BIT - 1) / INT_BIT);
    
    frBitArray ba = calloc(size, sizeof *ba);

    return ba;
}

/* Releases the memory allocated for `ba`. */
void frReleaseBitArray(frBitArray ba) {
    if (ba != NULL) free(ba);
}

/* Returns the `i`-th bit of `ba`. */
int frBitArrayGet(const frBitArray ba, int i) {
    return (ba != NULL) ? (ba[i / INT_BIT] & (1 << (i % INT_BIT))) : -1;
}

/* Sets the `i`-th bit of `ba`. */
void frBitArraySet(frBitArray ba, int i) {
    if (ba == NULL) return;
    
    ba[i / INT_BIT] |= (1 << (i % INT_BIT));
}

/* Resets the `i`-th bit of `ba`. */
void frBitArrayReset(frBitArray ba, int i) {
    if (ba == NULL) return;
    
    ba[i / INT_BIT] &= ~(1 << (i % INT_BIT));
}

/* ========================================================================> */

/* Creates a ring buffer with the size of `length`. */
frRingBuffer *frCreateRingBuffer(size_t length) {
    frRingBuffer *result = calloc(1, sizeof *result);

    {
        length--;

        for (size_t i = 1; i < (CHAR_BIT * sizeof(size_t)); i <<= 1)
            length |= (length >> i);

        length++;
    }

    result->buffer = calloc(length, sizeof *(result->buffer));
    result->length = length;

    return result;
}

/* Releases the memory allocated for `rbf`. */
void frReleaseRingBuffer(frRingBuffer *rbf) {
    if (rbf != NULL) free(rbf->buffer), free(rbf);
}

/* Adds a `node` to `rbf`. */
bool frAddNodeToRingBuffer(frRingBuffer *rbf, frContextNode node) {
    if (rbf == NULL || ((rbf->head + 1) & (rbf->length - 1)) == rbf->tail)
        return false;

    rbf->buffer[rbf->head] = node;

    rbf->head = (rbf->head + 1) & (rbf->length - 1);

    return true;
}

/* Removes a node from `rbf` and stores it to `node`. */
bool frRemoveNodeFromRingBuffer(frRingBuffer *rbf, frContextNode *node) {
    if (rbf == NULL || (rbf->head == rbf->tail)) return false;

    if (node != NULL) *node = rbf->buffer[rbf->tail];

    rbf->tail = (rbf->tail + 1) & (rbf->length - 1);

    return true;
}
