/*
vector.h - Vector Container
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef LEKKIT_VECTOR_H
#define LEKKIT_VECTOR_H

#include "compiler.h"
#include <stddef.h>
#include <string.h>

#define vector_t(vec_elem_type) struct { vec_elem_type* data; size_t size; size_t count; }

// Grow the internal vector buffer to fit element at pos, does not initialize memory
slow_path void vector_grow_internal(void* vec, size_t elem_size, size_t pos);

// Emplace new element at pos, zeroing the new element and every preceeding one
void vector_emplace_internal(void* vec, size_t elem_size, size_t pos);

// Erase element at pos, moving the trailing elements into it's place
void vector_erase_internal(void* vec, size_t elem_size, size_t pos);

// Initialize vector by zeroing it's fields
#define vector_init(vec) \
do { \
    (vec).data = NULL; \
    (vec).size = 0; \
    (vec).count = 0; \
} while(0)

// Free vector buffer
// May be called multiple times, the vector is empty yet reusable afterwards
// Semantically identical to clear(), but also frees memory
#define vector_free(vec) \
do { \
    free((vec).data); \
    (vec).data = NULL; \
    (vec).size = 0; \
    (vec).count = 0; \
} while(0)

// Empty the vector
#define vector_clear(vec) do { (vec).count = 0; } while(0)

// Get element count
#define vector_size(vec) (vec).count

// Get underlying buffer capacity
#define vector_capacity(vec) (vec).size

// Dereference element at specific position
#define vector_at(vec, pos) (vec).data[pos]

// Get vector elements buffer
#define vector_buffer(vec) (vec).data

// Resize the vector, zeroing any newly alocated elements
#define vector_resize(vec, size) \
do { \
    if (unlikely((size) > (vec).count)) { \
        vector_emplace_internal(&(vec), sizeof(*(vec).data), (size) - 1); \
    } \
    (vec).count = (size); \
} while(0)

// Put element at specific position, overwriting previous element there if any
#define vector_put(vec, pos, val) \
do { \
    if (unlikely(pos >= (vec).count)) { \
        vector_emplace_internal(&(vec), sizeof(*(vec).data), pos); \
    } \
    (vec).data[pos] = val; \
} while(0)

// Insert new element at the end of the vector
#define vector_push_back(vec, val) \
do { \
    if (unlikely((vec).count >= (vec).size)) { \
        vector_grow_internal(&(vec), sizeof(*(vec).data), (vec).count); \
    } \
    (vec).data[(vec).count++] = val; \
} while(0)

// Insert element at specific position, move trailing elements forward
#define vector_insert(vec, pos, val) \
do { \
    vector_emplace_internal(&(vec), sizeof(*(vec).data), pos); \
    (vec).data[pos] = val; \
} while(0)

// Emplace new element at the end of the vector
#define vector_emplace_back(vec) \
do { \
    if (unlikely((vec).count >= (vec).size)) { \
        vector_grow_internal(&(vec), sizeof(*(vec).data), (vec).count); \
    } \
    memset(&(vec).data[(vec).count++], 0, sizeof(*(vec).data)); \
} while(0)

// Emplace new element at specific position, move trailing elements forward
#define vector_emplace(vec, pos) vector_emplace_internal(&(vec), sizeof(*(vec).data), pos)

// Erase element at specific posision, move trailing elements backward
#define vector_erase(vec, pos) vector_erase_internal(&(vec), sizeof(*(vec).data), pos)

// Iterates the vector in forward order
// Be sure to break loop after vector_erase(), since it invalidates forward iterators
#define vector_foreach(vec, iter) \
    for (size_t iter=0; iter<(vec).count; ++iter)

// Iterates the vector in reversed order, which is safe for vector_erase()
#define vector_foreach_back(vec, iter) \
    for (size_t iter=(vec).count; iter--;)

#endif
