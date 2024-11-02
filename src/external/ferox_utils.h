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

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

/* Macros =================================================================> */

/* A structure that represents a ring buffer for storing indexed data. */
#define frRingBuffer(type)  \
    struct {                \
        type *buffer;       \
        size_t size;        \
        int head, tail;     \
    }

/*
    NOTE: https://graphics.stanford.edu/%7Eseander/bithacks.html
    
    Example #1: `v = 0b00010101`
    
    => `v = 0b00010100`
    => `v = 0b00011111`
    => `v = 0b00100000`

    Example #2: `v = 0b00010100`
    
    => `v = 0b00010011`
    => `v = 0b00011111`
    => `v = 0b00100000`
*/

/* Rounds up `v` to the next highest power of 2. */
#define frRoundUp32(v)  \
    (--(v),             \
     (v) |= (v) >> 1,   \
     (v) |= (v) >> 2,   \
     (v) |= (v) >> 4,   \
     (v) |= (v) >> 8,   \
     (v) |= (v) >> 16,  \
     ++(v))

/* ========================================================================> */

/* Initializes a ring buffer with the size of `length`. */
#define frInitRingBuffer(rbf, length)                               \
    do {                                                            \
        size_t newLength = length;                                  \
                                                                    \
        (rbf).size = frRoundUp32(newLength);                        \
        (rbf).head = (rbf).tail = 0;                                \
                                                                    \
        (rbf).buffer = calloc((rbf).size, sizeof *((rbf).buffer));  \
    } while (0)

/* Releases the memory allocated for `rbf`. */
#define frReleaseRingBuffer(rbf)  \
    do {                          \
       free((rbf).buffer);        \
    } while (0)

/* Adds a `value` to `rbf`. */
#define frAddToRingBuffer(rbf, value)                          \
    ((((rbf).head + 1) & ((rbf).size - 1)) != (rbf).tail       \
        ? (                                                    \
            (rbf).buffer[(rbf).head] = (value),                \
            (rbf).head = ((rbf).head + 1) & ((rbf).size - 1),  \
            true                                               \
        )                                                      \
        : false                                                \
    )

/* Removes a node from `rbf` and stores it to `valuePtr`. */
#define frRemoveFromRingBuffer(rbf, valuePtr)                      \
    (((rbf).head != (rbf).tail)                                    \
        ? (((valuePtr) != NULL)                                    \
            ? (                                                    \
                *((valuePtr)) = (rbf).buffer[(rbf).tail],          \
                (rbf).tail = ((rbf).tail + 1) & ((rbf).size - 1),  \
                true                                               \
            )                                                      \
            : false                                                \
        )                                                          \
        : false                                                    \
    )

/* ========================================================================> */

/* Creates a bit array with `count` bits. */
#define frCreateBitArray(count)               \
    calloc(                                   \
        ((count) + (INT_BIT - 1) / INT_BIT),  \
        sizeof(frBitArray)                    \
    )

/* Releases the memory allocated for `ba`. */
#define frReleaseBitArray(ba)  \
    free(ba)

/* Returns the `i`-th bit of `ba`. */
#define frBitArrayGet(ba, i)                              \
    (((ba) != NULL)                                       \
        ? ((ba)[(i) / INT_BIT] & (1 << ((i) % INT_BIT)))  \
        : -1                                              \
    )

/* Sets the `i`-th bit of `ba`. */
#define frBitArraySet(ba, i)                         \
    ((ba)[(i) / INT_BIT] |= (1 << ((i) % INT_BIT)))

/* Resets the `i`-th bit of `ba`. */
#define frBitArrayReset(ba, i)                        \
    ((ba)[(i) / INT_BIT] &= ~(1 << ((i) % INT_BIT)))

/* Typedefs ===============================================================> */

/* A data type that represents a bit array.*/
typedef int *frBitArray;

#endif // `FEROX_UTILS_H`