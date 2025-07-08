/*
hashmap.h - Open-addressing hashmap implementation
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef LEKKIT_HASHMAP_H
#define LEKKIT_HASHMAP_H

#include "compiler.h"

#include <stddef.h>
#include <stdint.h>

// This is the worst-case scenario lookup complexity, only 1/256 of entries may reach this point at all.
// Setting the value lower may improve worst-case scenario by a slight margin, but increases memory consumption by
// orders of magnitude.
#define HASHMAP_MAX_PROBES 256

typedef struct randomize_layout {
    size_t key;
    size_t val;
} hashmap_bucket_t;

// Bucket with val=0 is treated as unused bucket to reduce memory usage (no additional flag), size is actually a bitmask
// holding lowest 1s to represent size encoding space.
typedef struct randomize_layout {
    hashmap_bucket_t* buckets;
    size_t            size;
    size_t            entries;
    size_t            entry_balance;
} hashmap_t;

// Internal hashmap implementation paths
slow_path void hashmap_grow_internal(hashmap_t* map, size_t key, size_t val);
slow_path void hashmap_shrink_internal(hashmap_t* map);
slow_path void hashmap_rebalance_internal(hashmap_t* map, size_t index);

// Hint the expected amount of entries on map creation
void hashmap_init(hashmap_t* map, size_t size);
void hashmap_destroy(hashmap_t* map);
void hashmap_resize(hashmap_t* map, size_t size);
void hashmap_clear(hashmap_t* map);

static inline size_t hashmap_used_mem(hashmap_t* map)
{
    return (map->size + 1) * sizeof(hashmap_bucket_t);
}

#define hashmap_foreach(map, k, v)                                                                                     \
    for (size_t k, v, MACRO_IDENT(hashmap_iter) = 0;                        /**/                                       \
         k = ((map)->buckets[MACRO_IDENT(hashmap_iter) & (map)->size].key), /**/                                       \
         v = ((map)->buckets[MACRO_IDENT(hashmap_iter) & (map)->size].val), /**/                                       \
         MACRO_IDENT(hashmap_iter) <= (map)->size;                          /**/                                       \
         ++MACRO_IDENT(hashmap_iter))                                       /**/                                       \
        if (v)

static forceinline size_t hashmap_hash(size_t k)
{
    k ^= k << 21;
    k ^= k >> 17;
#if (SIZE_MAX > 0xFFFFFFFF)
    k ^= k >> 35;
    k ^= k >> 51;
#endif
    return k;
}

static inline void hashmap_put(hashmap_t* map, size_t key, size_t val)
{
    size_t hash = hashmap_hash(key);
    for (size_t i = 0; i < HASHMAP_MAX_PROBES; ++i) {
        size_t index = (hash + i) & map->size;

        if (likely(map->buckets[index].key == key)) {
            // The key is already used, change value
            map->buckets[index].val = val;

            if (!val) {
                // Value = 0 means we should clear a bucket
                // Rebalance colliding trailing entries
                hashmap_rebalance_internal(map, index);
                map->entries--;
            }
            return;
        } else if (likely(!map->buckets[index].val && val)) {
            // Empty bucket found, the key is unused
            map->entries++;
            map->buckets[index].key = key;
            map->buckets[index].val = val;
            return;
        }
    }
    // Near-key space is polluted with colliding entries, reallocate and rehash
    // Puts the new entry as well to simplify the inlined function
    if (unlikely(val)) {
        hashmap_grow_internal(map, key, val);
    }
}

static forceinline size_t hashmap_get(const hashmap_t* map, size_t key)
{
    size_t hash = hashmap_hash(key);
    for (size_t i = 0; i < HASHMAP_MAX_PROBES; ++i) {
        size_t index = (hash + i) & map->size;
        if (likely(map->buckets[index].key == key || !map->buckets[index].val)) {
            return map->buckets[index].val;
        }
    }
    return 0;
}

static inline void hashmap_remove(hashmap_t* map, size_t key)
{
    // Treat value zero as removed key
    hashmap_put(map, key, 0);
    if (unlikely(map->entries < map->entry_balance && map->entries > 256)) {
        hashmap_shrink_internal(map);
    }
}

#endif
