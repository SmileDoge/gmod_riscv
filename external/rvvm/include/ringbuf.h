/*
ringbuf.h - FIFO Ring buffer
Copyright (C) 2021  cerg2010cerg2010 <github.com/cerg2010cerg2010>
                    LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_RINGBUF_H
#define RVVM_RINGBUF_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct ringbuf {
    void*  data;
    size_t size;
    size_t start;
    size_t consumed;
} ringbuf_t;

void ringbuf_create(ringbuf_t* rb, size_t size);
void ringbuf_destroy(ringbuf_t* rb);

size_t ringbuf_space(ringbuf_t* rb);
size_t ringbuf_avail(ringbuf_t* rb);

// Serial operation (Returns actual amount of read/written bytes)
size_t ringbuf_read(ringbuf_t* rb, void* data, size_t len);
size_t ringbuf_peek(ringbuf_t* rb, void* data, size_t len);
size_t ringbuf_skip(ringbuf_t* rb, size_t len);
size_t ringbuf_write(ringbuf_t* rb, const void* data, size_t len);

// Error out instead of partial operation
bool ringbuf_get(ringbuf_t* rb, void* data, size_t len);
bool ringbuf_put(ringbuf_t* rb, const void* data, size_t len);

static inline bool ringbuf_put_u8(ringbuf_t* rb, uint8_t x) { return ringbuf_put(rb, &x, sizeof(x)); }
static inline bool ringbuf_put_u16(ringbuf_t* rb, uint16_t x) { return ringbuf_put(rb, &x, sizeof(x)); }
static inline bool ringbuf_put_u32(ringbuf_t* rb, uint32_t x) { return ringbuf_put(rb, &x, sizeof(x)); }
static inline bool ringbuf_put_u64(ringbuf_t* rb, uint64_t x) { return ringbuf_put(rb, &x, sizeof(x)); }

static inline bool ringbuf_get_u8(ringbuf_t* rb, uint8_t* x) { return ringbuf_get(rb, x, sizeof(*x)); }
static inline bool ringbuf_get_u16(ringbuf_t* rb, uint16_t* x) { return ringbuf_get(rb, x, sizeof(*x)); }
static inline bool ringbuf_get_u32(ringbuf_t* rb, uint32_t* x) { return ringbuf_get(rb, x, sizeof(*x)); }
static inline bool ringbuf_get_u64(ringbuf_t* rb, uint64_t* x) { return ringbuf_get(rb, x, sizeof(*x)); }

#endif
