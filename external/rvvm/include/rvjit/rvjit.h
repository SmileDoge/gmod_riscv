/*
rvjit.h - Retargetable Versatile JIT Compiler
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVJIT_H
#define RVJIT_H

#include "hashmap.h"
#include "rvvm_types.h"
#include "utils.h"
#include "vector.h"

// RISC-V register allocator details
#define RVJIT_REGISTERS     32
#define RVJIT_REGISTER_ZERO 0

#if defined(__x86_64__) || defined(_M_AMD64)

// Target: x86_64, ABI: SystemV (preferred) / Win64
#define RVJIT_X86           1
#define RVJIT_NATIVE_64BIT  1
#define RVJIT_NATIVE_LINKER 1

#if GNU_ATTRIBUTE(__sysv_abi__)
// Force SystemV ABI whenever possible
#define RVJIT_CALL     __attribute__((__sysv_abi__))
#define RVJIT_ABI_SYSV 1
#elif defined(_WIN32)
// Win64 ABI will be used
#define RVJIT_ABI_WIN64 1
#else
// Non-GNU compiler on non-Windows target, let's assume SysV ABI
#define RVJIT_ABI_SYSV 1
#endif

#elif defined(__i386__) || defined(_M_IX86)

// Target: i386, ABI: fastcall
#define RVJIT_X86           1
#define RVJIT_ABI_FASTCALL  1
#define RVJIT_NATIVE_LINKER 1

#if GNU_ATTRIBUTE(__fastcall__)
#define RVJIT_CALL __attribute__((__fastcall__))
#elif defined(_WIN32)
#define RVJIT_CALL __fastcall
#endif

#elif defined(__riscv)

// Target: riscv64/riscv32, ABI: SystemV
#define RVJIT_RISCV         1
#define RVJIT_ABI_SYSV      1
#define RVJIT_NATIVE_LINKER 1

#if __riscv_xlen == 64
#define RVJIT_NATIVE_64BIT 1
#elif __riscv_xlen != 32
#error No JIT support for riscv128!
#endif

#elif defined(__aarch64__) || defined(_M_ARM64)

// Target: arm64, ABI: SystemV
#define RVJIT_ARM64         1
#define RVJIT_ABI_SYSV      1
#define RVJIT_NATIVE_64BIT  1
#define RVJIT_NATIVE_LINKER 1

#elif defined(__arm__) || defined(_M_ARM)

// Target: arm32, ABI: SystemV
#define RVJIT_ARM      1
#define RVJIT_ABI_SYSV 1

#else

#error No JIT support for the target platform!!!

#endif

// No specific calling convention requirements
#ifndef RVJIT_CALL
#define RVJIT_CALL GNU_DUMMY_ATTRIBUTE
#endif

typedef void (*RVJIT_CALL rvjit_func_t)(void*);

typedef uint8_t  regflags_t;
typedef uint32_t regmask_t;
typedef size_t   branch_t;
typedef size_t   rvjit_addr_t;

#define BRANCH_NEW    ((branch_t) - 1)
#define BRANCH_ENTRY  false
#define BRANCH_TARGET true

#define LINKAGE_NONE  0
#define LINKAGE_TAIL  1
#define LINKAGE_JMP   2

#define REG_ILL       0xFF // Register is not allocated

typedef struct {
    uint8_t*  data;
    uint8_t*  code;
    size_t    curr;
    size_t    size;
    hashmap_t blocks;
    hashmap_t block_links;

    // Dirty memory tracking
    uint32_t* jited_pages;
    uint32_t* dirty_pages;
    size_t    dirty_mask;
} rvjit_heap_t;

typedef struct {
    uint32_t   last_used; // Last usage of register for LRU reclaim
    int32_t    auipc_off;
    regid_t    hreg;  // Claimed host register, REG_ILL if not mapped
    regflags_t flags; // Register allocation details
} rvjit_reginfo_t;

typedef struct {
    rvjit_heap_t heap;

    uint8_t* code;
    size_t   size;
    size_t   space;

    // Block exit paths
    vector_t(struct {
        rvjit_addr_t dest;
        size_t       ptr;
    }) links;

    regmask_t       hreg_mask;       // Bitmask of available non-clobbered host registers
    regmask_t       abireclaim_mask; // Bitmask of reclaimed abi-clobbered host registers to restore
    rvjit_reginfo_t regs[RVJIT_REGISTERS];

#ifdef RVJIT_NATIVE_FPU
    regmask_t       fpu_reg_mask;
    rvjit_reginfo_t fpu_regs[RVJIT_REGISTERS];
#endif

    rvjit_addr_t virt_pc;
    rvjit_addr_t phys_pc;
    int32_t      pc_off;

    bool    rv64;
    bool    native_ptrs;
    uint8_t linkage;
} rvjit_block_t;

// Creates JIT context, sets upper limit on cache size
bool rvjit_ctx_init(rvjit_block_t* block, size_t heap_size);

// Frees the JIT context and block cache
// All functions generated by this context are invalid after freeing it!
void rvjit_ctx_free(rvjit_block_t* block);

// Set guest bitness
static inline void rvjit_set_rv64(rvjit_block_t* block, bool rv64)
{
#ifdef RVJIT_NATIVE_64BIT
    block->rv64 = rv64;
#else
    UNUSED(rv64);
    block->rv64 = false;
#endif
}

static inline void rvjit_set_native_ptrs(rvjit_block_t* block, bool native_ptrs)
{
    block->native_ptrs = native_ptrs;
}

// Creates a new block, prepares codegen
void rvjit_block_init(rvjit_block_t* block);

// Returns true if the block has some instructions emitted
static inline bool rvjit_block_nonempty(rvjit_block_t* block)
{
    return block->size != 0;
}

// Returns NULL when cache is full, otherwise returns a valid function pointer
// Inserts block into the lookup cache by phys_pc key
rvjit_func_t rvjit_block_finalize(rvjit_block_t* block);

// Looks up for compiled block by phys_pc, returns NULL when no block was found
rvjit_func_t rvjit_block_lookup(rvjit_block_t* block, rvjit_addr_t phys_pc);

// Track dirty memory to transparently invalidate JIT caches
void rvjit_init_memtracking(rvjit_block_t* block, size_t size);
void rvjit_mark_dirty_mem(rvjit_block_t* block, rvjit_addr_t addr, size_t size);

// Cleans up internal heap & lookup cache entirely
void rvjit_flush_cache(rvjit_block_t* block);

/*
 * Internal codegen APIs
 */

// Register bitmask
static inline regmask_t rvjit_hreg_mask(regid_t hreg)
{
    return (1ULL << hreg);
}

// Emit RVJIT prologue, set up codegen state
void rvjit_emit_init(rvjit_block_t* block);

// Emit RVJIT epilogue
void rvjit_emit_end(rvjit_block_t* block, uint8_t linkage);

// Emit instruction bytes into the block
static inline void rvjit_put_code(rvjit_block_t* block, const void* inst, size_t size)
{
    if (unlikely(block->space < block->size + size)) {
        block->space += 1024;
        block->code   = safe_realloc(block->code, block->space);
    }
    memcpy(block->code + block->size, inst, size);
    block->size += size;
}

// Claims any free hardware register, or reclaims mapped register preserving it's value in VM
regid_t rvjit_claim_hreg(rvjit_block_t* block);

// Frees explicitly claimed hardware register
static inline void rvjit_free_hreg(rvjit_block_t* block, regid_t hreg)
{
    block->hreg_mask |= rvjit_hreg_mask(hreg);
}

#ifdef RVJIT_NATIVE_FPU

// Claims any free hardware register, or reclaims mapped register preserving it's value in VM
regid_t rvjit_claim_fpu_reg(rvjit_block_t* block);

// Frees explicitly claimed hardware register
static inline void rvjit_free_fpu_reg(rvjit_block_t* block, regid_t hreg)
{
    block->fpu_reg_mask |= rvjit_hreg_mask(hreg);
}

#endif

#endif
