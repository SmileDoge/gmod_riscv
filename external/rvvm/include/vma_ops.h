/*
vma_ops.h - Virtual memory area operations
Copyright (C) 2023  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef VMA_OPS_H
#define VMA_OPS_H

#include "blk_io.h"

#define VMA_NONE  0x0
#define VMA_EXEC  0x1
#define VMA_WRITE 0x2
#define VMA_READ  0x4
#define VMA_RDWR  (VMA_READ | VMA_WRITE)
#define VMA_RDEX  (VMA_READ | VMA_EXEC)
#define VMA_RWX   (VMA_READ | VMA_WRITE | VMA_EXEC)

#define VMA_SHARED 0x08 // Shared file mapping, private by default
#define VMA_FIXED  0x10 // Fixed mapping address (Non-destructive), pretty picky to use
#define VMA_THP    0x20 // Transparent hugepages
#define VMA_KSM    0x40 // Kernel same-page merging

/*
 * Misc memory helpers
 */

// Get host page size
size_t vma_page_size(void);

// Create anonymous memory-backed FD (POSIX only!)
int    vma_anon_memfd(size_t size);

// Broadcast a global memory barrier on all running threads. May fail on some host systems.
bool   vma_broadcast_membarrier(void);

/*
 * VMA allocations & file mapping
 */

// Allocate anonymous VMA, force needed address using VMA_FIXED
void* vma_alloc(void* addr, size_t size, uint32_t flags);

// Map file into memory, acts like vma_alloc() when file == NULL
void* vma_mmap(void* addr, size_t size, uint32_t flags, rvfile_t* file, uint64_t offset);

// Create separate RW/exec VMAs (For W^X JIT)
bool  vma_multi_mmap(void** rw, void** exec, size_t size);

// Resize anon VMA, pass VMA_FIXED to make sure it stays in place
void* vma_remap(void* addr, size_t old_size, size_t new_size, uint32_t flags);

/*
 * VMA operations
 */

// Change VMA protection flags
bool  vma_protect(void* addr, size_t size, uint32_t flags);

// Synchronise writes to shared file mapping
bool  vma_sync(void* addr, size_t size);

// Hint to free (zero-fill) underlying memory, VMA is still intact
bool  vma_clean(void* addr, size_t size, bool lazy);

// Hint to pageout memory, data is kept intact
bool  vma_pageout(void* addr, size_t size, bool lazy);

// Unmap the VMA
bool  vma_free(void* addr, size_t size);

#endif
