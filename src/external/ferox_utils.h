/*
    Copyright (c) 2021-2024 Jaedeok Kim <jdeokkim@protonmail.com>

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

#ifndef FEROX_UTILS_H
#define FEROX_UTILS_H

/* Includes ===============================================================> */

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Macros =================================================================> */

/* Creates a bit array with `n` bits. */
#define frCreateBitArray(n)  \
    calloc((n), sizeof(char))

/* Releases the memory allocated for `ba`. */
#define frReleaseBitArray(ba)  \
    free((ba))

/* Clears all bits of `ba`. */
#define frBitArrayClear(ba, n)  \
    memset((ba), 0, (n))

/* Returns the `i`-th bit of `ba`. */
#define frBitArrayGet(ba, i)  \
    ((ba)[i])

/* Sets the `i`-th bit of `ba`. */
#define frBitArraySet(ba, i)  \
    ((ba)[(i)] = 1)

/* ========================================================================> */

#define DSA_INIT_CAPACITY  8

/* ========================================================================> */

/* A structure that represents a dynamically sized array. */
#define frDynArray(type)         \
    struct {                     \
        type *buffer;            \
        size_t length;           \
        size_t capacity;         \
    }

/* Initializes a dynamically sized array. */
#define frInitDynArray(arr)                                     \
    do {                                                        \
        (arr).length = 0, (arr).capacity = DSA_INIT_CAPACITY;   \
                                                                \
        (arr).buffer = malloc((arr).capacity                    \
            * sizeof *((arr).buffer));                          \
    } while (0)

/* Releases the memory allocated for `arr`. */
#define frReleaseDynArray(arr)  \
    do {                        \
       free((arr).buffer);      \
    } while (0)

/* Returns the capacity of `arr`. */
#define frGetDynArrayCapacity(arr)  \
    ((arr).capacity)

/* Returns the length of `arr`. */
#define frGetDynArrayLength(arr)  \
    ((arr).length)

/* Returns the `i`-th value of `arr`. */
#define frGetDynArrayValue(arr, i)  \
    ((arr).buffer[(i)])

/* Sets the capacity of `arr` to `newCapacity`. */
#define frSetDynArrayCapacity(arr, newCapacity)     \
    do {                                            \
        void *newBuffer = realloc(                  \
            (arr).buffer,                           \
            (newCapacity) * sizeof *((arr).buffer)  \
        );                                          \
                                                    \
        if (newBuffer != NULL) {                    \
            (arr).buffer = newBuffer;               \
                                                    \
            if ((arr).length > newCapacity)         \
                (arr).length = newCapacity;         \
                                                    \
            (arr).capacity = newCapacity;           \
        }                                           \
    } while (0)

/* Sets the length of `arr` to `newLength`. */
#define frSetDynArrayLength(arr, newLength)  \
    ((arr).length = newLength)

/* Appends `newValue` at the end of `arr`. */
#define frDynArrayPush(arr, newValue)                               \
    do {                                                            \
        if ((arr).length >= (arr).capacity)                         \
            frSetDynArrayCapacity((arr), ((arr).capacity << 1));    \
                                                                    \
        (arr).buffer[(arr).length] = (newValue), ++((arr).length);  \
    } while (0)

/* Swaps the `i`-th value and the `j`-th value of `arr`. */
#define frDynArraySwap(type, arr, i, j)         \
    do {                                        \
        type tmp = (arr).buffer[(j)];           \
                                                \
        (arr).buffer[(j)] = (arr).buffer[(i)],  \
        (arr).buffer[(i)] = tmp;                \
    } while (0)

/* ========================================================================> */

/*
    NOTE: https://graphics.stanford.edu/%7Eseander/bithacks.html
    
    Example #1: `x = 0b00010101`
    
    => `x = 0b00010100`
    => `x = 0b00011111`
    => `x = 0b00100000`

    Example #2: `x = 0b00010100`
    
    => `x = 0b00010011`
    => `x = 0b00011111`
    => `x = 0b00100000`
*/

/* Rounds up `x` to the next highest power of 2. */
#define frRoundUp32(x)  \
     (--(x),            \
     (x) |= (x) >> 1,   \
     (x) |= (x) >> 2,   \
     (x) |= (x) >> 4,   \
     (x) |= (x) >> 8,   \
     (x) |= (x) >> 16,  \
     ++(x))

/* ========================================================================> */

/* A structure that represents a ring buffer. */
#define frRingBuffer(type)   \
    struct {                 \
        type *buffer;        \
        size_t length;       \
        int head; int tail;  \
    }

/* Initializes a ring buffer with the given `size`. */
#define frInitRingBuffer(rbf, size)                                   \
    do {                                                              \
        size_t newSize = size;                                        \
                                                                      \
        (rbf).length = frRoundUp32(newSize);                          \
        (rbf).head = (rbf).tail = 0;                                  \
                                                                      \
        (rbf).buffer = calloc((rbf).length, sizeof *((rbf).buffer));  \
    } while (0)

/* Releases the memory allocated for `rbf`. */
#define frReleaseRingBuffer(rbf)  \
    do {                          \
       free((rbf).buffer);        \
    } while (0)

/* Adds a `value` to `rbf`. */
#define frAddToRingBuffer(rbf, value)                            \
    ((((rbf).head + 1) & ((rbf).length - 1)) != (rbf).tail       \
        ? (                                                      \
            (rbf).buffer[(rbf).head] = (value),                  \
            (rbf).head = ((rbf).head + 1) & ((rbf).length - 1),  \
            !0                                                   \
        )                                                        \
        : 0                                                      \
    )

/* Removes a node from `rbf` and stores it to `valuePtr`. */
#define frRemoveFromRingBuffer(rbf, valuePtr)                        \
    (((rbf).head != (rbf).tail)                                      \
        ? (((valuePtr) != NULL)                                      \
            ? (                                                      \
                *((valuePtr)) = (rbf).buffer[(rbf).tail],            \
                (rbf).tail = ((rbf).tail + 1) & ((rbf).length - 1),  \
                !0                                                   \
            )                                                        \
            : 0                                                      \
        )                                                            \
        : 0                                                          \
    )

/* Typedefs ===============================================================> */

/* A data type that represents a bit array.*/
typedef char *frBitArray;

#endif  // `FEROX_UTILS_H`
