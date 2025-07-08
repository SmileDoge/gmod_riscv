/*
riscv_mmu.h - RISC-V Memory Mapping Unit
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RISCV_MMU_H
#define RISCV_MMU_H

#include "rvvm.h"
#include "compiler.h"
#include "mem_ops.h"
#include "riscv_csr.h"

// MMU page access bits
#define RISCV_MMU_READ     0x2
#define RISCV_MMU_WRITE    0x4
#define RISCV_MMU_EXEC     0x8

#define RISCV_PTE_VALID    0x1
#define RISCV_PTE_LEAF     0xA
#define RISCV_PTE_USER     0x10
#define RISCV_PTE_GLOBAL   0x20
#define RISCV_PTE_ACCESSED 0x40
#define RISCV_PTE_DIRTY    0x80

#define RISCV_PAGE_SHIFT   12
#define RISCV_PAGE_MASK    0xFFF
#define RISCV_PAGE_SIZE    0x1000
#define RISCV_PAGE_PNMASK  (~0xFFFULL)

// Init physical memory (be careful to not overlap MMIO regions!)
bool riscv_init_ram(rvvm_ram_t* mem, rvvm_addr_t base_addr, size_t size);
void riscv_free_ram(rvvm_ram_t* mem);

// Flush the TLB (For SFENCE.VMA, etc)
void riscv_tlb_flush(rvvm_hart_t* vm);
void riscv_tlb_flush_page(rvvm_hart_t* vm, rvvm_addr_t addr);

#ifdef USE_JIT
void riscv_jit_tlb_flush(rvvm_hart_t* vm);
#endif

/*
 * Slow memory access path which walks the MMU
 */

// Never trap the CPU
#define RISCV_MMU_ATTR_NO_TRAP 0x100

// Ignore MMU protection (Perform operation on any valid page)
#define RISCV_MMU_ATTR_NO_PROT 0x200

// Translate into physical address and write to *(rvvm_addr_t*)data
#define RISCV_MMU_ATTR_PHYS    0x400

// Return direct pointer to memory, data is used as MMIO bounce buffer if non-NULL
#define RISCV_MMU_ATTR_PTR     0x800

// Used for GDB stub memory accesses
#define RISCV_MMU_ATTR_DEBUG (RISCV_MMU_ATTR_NO_TRAP | RISCV_MMU_ATTR_NO_PROT)

no_inline void* riscv_mmu_op_internal(rvvm_hart_t* vm, rvvm_addr_t vaddr, void* data, uint32_t attr);

static forceinline void* riscv_mmu_op_helper(rvvm_hart_t* vm, rvvm_addr_t vaddr, void* data, uint32_t attr, uint8_t size, uint8_t access)
{
    return riscv_mmu_op_internal(vm, vaddr, data, (((uint32_t)size) << 16) | (attr & 0xFF00) | access);
}

/*
 * Slow path MMU helpers
 */

// Load/store/fetch helper
// Traps on pagefault
static forceinline bool riscv_mmu_op(rvvm_hart_t* vm, rvvm_addr_t vaddr, void* data, uint8_t size, uint8_t access)
{
    return !!riscv_mmu_op_helper(vm, vaddr, data, 0, size, access);
}

// Translate virtual address to physical with respect to current CPU mode and access type
// Should not have visible side effects to the guest
static forceinline bool riscv_mmu_virt_translate(rvvm_hart_t* vm, rvvm_addr_t vaddr, rvvm_addr_t* paddr, uint8_t access)
{
    return !!riscv_mmu_op_helper(vm, vaddr, paddr, RISCV_MMU_ATTR_PHYS | RISCV_MMU_ATTR_NO_TRAP, 0, access);
}

// Translate virtual address into a pointer (For atomics)
// When non-null data is passed, it is used as a bounce buffer for MMIO atomics
static forceinline void* riscv_mmu_ptr_translate(rvvm_hart_t* vm, rvvm_addr_t vaddr, void* data, size_t size, uint8_t access)
{
    return riscv_mmu_op_helper(vm, vaddr, data, RISCV_MMU_ATTR_PTR, size, access);
}

// Commit RMW changes back to MMIO
// Does not trap - Should already trap in riscv_mmu_ptr_translate()
static forceinline void riscv_rmw_mmio_commit(rvvm_hart_t* vm, rvvm_addr_t vaddr, void* data, size_t size)
{
    riscv_mmu_op_helper(vm, vaddr, data, RISCV_MMU_ATTR_NO_TRAP, size, RISCV_MMU_WRITE);
}

/*
 * Inlined TLB-cached memory operations (Used for performance)
 * Fall back to MMU functions if:
 * - Address is not TLB-cached (TLB miss / possible pagefault)
 * - Address misaligned
 * - MMIO is accessed (Since MMIO regions aren't memory)
 */

// Instruction fetch

static forceinline bool riscv_fetch_insn(rvvm_hart_t* vm, rvvm_addr_t vaddr, uint32_t* insn)
{
    rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    if (likely(vm->tlb[vpn & RVVM_TLB_MASK].e == vpn)) {
        uint32_t tmp = read_uint16_le_m((void*)(size_t)(vm->tlb[vpn & RVVM_TLB_MASK].ptr + vaddr));
        if ((tmp & 0x3) == 0x3) {
            // This is a 4-byte instruction, fetch the next half
            vpn = (vaddr + 2) >> RISCV_PAGE_SHIFT;
            if (likely(vm->tlb[vpn & RVVM_TLB_MASK].e == vpn)) {
                tmp |= ((uint32_t)read_uint16_le_m((void*)(size_t)(vm->tlb[vpn & RVVM_TLB_MASK].ptr + vaddr + 2))) << 16;
                *insn = tmp;
                return true;
            }
        } else {
            *insn = tmp;
            return true;
        }
    }
    uint8_t buff[4] = {0};
    if (unlikely(!riscv_mmu_op(vm, vaddr, buff, 2, RISCV_MMU_EXEC))) {
        return false;
    }
    if ((buff[0] & 0x3) == 0x3) {
        // This is a 4-byte instruction scattered between pages
        // Fetch second part (may trigger a pagefault, that's the point)
        if (unlikely(!riscv_mmu_op(vm, vaddr + 2, buff + 2, 2, RISCV_MMU_EXEC))) {
            return false;
        }
    }
    *insn = read_uint32_le_m(buff);
    return true;
}

// VM Address translation (May not span over page bounds!)

static forceinline bool riscv_virt_translate_e(rvvm_hart_t* vm, rvvm_addr_t vaddr, rvvm_addr_t* paddr)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->e == vpn)) {
        *paddr = entry->ptr + vaddr - (size_t)vm->mem.data + vm->mem.addr;
        return true;
    }
    return riscv_mmu_virt_translate(vm, vaddr, paddr, RISCV_MMU_EXEC);
}

static forceinline void* riscv_ro_translate(rvvm_hart_t* vm, rvvm_addr_t vaddr, void* buff, size_t size)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->r == vpn)) {
        return (void*)(size_t)(entry->ptr + vaddr);
    }
    return riscv_mmu_ptr_translate(vm, vaddr, buff, size, RISCV_MMU_READ);
}

static forceinline void* riscv_rmw_translate(rvvm_hart_t* vm, rvvm_addr_t vaddr, void* buff, size_t size)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->w == vpn)) {
        return (void*)(size_t)(entry->ptr + vaddr);
    }
    return riscv_mmu_ptr_translate(vm, vaddr, buff, size, RISCV_MMU_WRITE);
}

// Integer load operations

static forceinline void riscv_load_u64(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->r == vpn && !(vaddr & 7))) {
        vm->registers[reg] = read_uint64_le((void*)(size_t)(entry->ptr + vaddr));
    } else {
        uint64_t tmp;
        if (riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_READ)) {
            vm->registers[reg] = read_uint64_le(&tmp);
        }
    }
}

static forceinline void riscv_load_u32(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->r == vpn && !(vaddr & 3))) {
        vm->registers[reg] = read_uint32_le((void*)(size_t)(entry->ptr + vaddr));
    } else {
        uint32_t tmp;
        if (riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_READ)) {
            vm->registers[reg] = read_uint32_le(&tmp);
        }
    }
}

static forceinline void riscv_load_s32(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->r == vpn && !(vaddr & 3))) {
        vm->registers[reg] = (int32_t)read_uint32_le((void*)(size_t)(entry->ptr + vaddr));
    } else {
        uint32_t tmp;
        if (riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_READ)) {
            vm->registers[reg] = (int32_t)read_uint32_le(&tmp);
        }
    }
}

static forceinline void riscv_load_u16(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->r == vpn && !(vaddr & 1))) {
        vm->registers[reg] = read_uint16_le((void*)(size_t)(entry->ptr + vaddr));
    } else {
        uint16_t tmp;
        if (riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_READ)) {
            vm->registers[reg] = read_uint16_le(&tmp);
        }
    }
}

static forceinline void riscv_load_s16(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->r == vpn && !(vaddr & 1))) {
        vm->registers[reg] = (int16_t)read_uint16_le((void*)(size_t)(entry->ptr + vaddr));
    } else {
        uint16_t tmp;
        if (riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_READ)) {
            vm->registers[reg] = (int16_t)read_uint16_le(&tmp);
        }
    }
}

static forceinline void riscv_load_u8(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->r == vpn)) {
        vm->registers[reg] = read_uint8((void*)(size_t)(entry->ptr + vaddr));
    } else {
        uint8_t tmp;
        if (riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_READ)) {
            vm->registers[reg] = tmp;
        }
    }
}

static forceinline void riscv_load_s8(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->r == vpn)) {
        vm->registers[reg] = (int8_t)read_uint8((void*)(size_t)(entry->ptr + vaddr));
    } else {
        uint8_t tmp;
        if (riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_READ)) {
            vm->registers[reg] = (int8_t)tmp;
        }
    }
}

// Integer store operations

static forceinline void riscv_store_u64(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->w == vpn && !(vaddr & 7))) {
        write_uint64_le((void*)(size_t)(entry->ptr + vaddr), vm->registers[reg]);
    } else {
        uint64_t tmp;
        write_uint64_le_m(&tmp, vm->registers[reg]);
        riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_WRITE);
    }
}

static forceinline void riscv_store_u32(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->w == vpn && !(vaddr & 3))) {
        write_uint32_le((void*)(size_t)(entry->ptr + vaddr), vm->registers[reg]);
    } else {
        uint32_t tmp;
        write_uint32_le_m(&tmp, vm->registers[reg]);
        riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_WRITE);
    }
}

static forceinline void riscv_store_u16(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->w == vpn && !(vaddr & 1))) {
        write_uint16_le((void*)(size_t)(entry->ptr + vaddr), vm->registers[reg]);
    } else {
        uint16_t tmp;
        write_uint16_le_m(&tmp, vm->registers[reg]);
        riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_WRITE);
    }
}

static forceinline void riscv_store_u8(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->w == vpn)) {
        write_uint8((void*)(size_t)(entry->ptr + vaddr), vm->registers[reg]);
    } else {
        uint8_t tmp = vm->registers[reg];
        riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_WRITE);
    }
}

#ifdef USE_FPU

// FPU load operations

static forceinline void riscv_load_double(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->r == vpn && !(vaddr & 7))) {
        vm->fpu_registers[reg] = read_double_le((void*)(size_t)(entry->ptr + vaddr));
        riscv_fpu_set_dirty(vm);
    } else {
        double tmp;
        if (riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_READ)) {
            vm->fpu_registers[reg] = read_double_le_m(&tmp);
            riscv_fpu_set_dirty(vm);
        }
    }
}

static forceinline void riscv_load_float(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->r == vpn && !(vaddr & 3))) {
        write_float_nanbox(&vm->fpu_registers[reg], read_float_le((void*)(size_t)(entry->ptr + vaddr)));
        riscv_fpu_set_dirty(vm);
    } else {
        float tmp;
        if (riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_READ)) {
            write_float_nanbox(&vm->fpu_registers[reg], read_float_le_m(&tmp));
            riscv_fpu_set_dirty(vm);
        }
    }
}

// FPU store operations

static forceinline void riscv_store_double(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->w == vpn && !(vaddr & 7))) {
        write_double_le((void*)(size_t)(entry->ptr + vaddr), vm->fpu_registers[reg]);
    } else {
        double tmp;
        write_double_le_m(&tmp, vm->fpu_registers[reg]);
        riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_WRITE);
    }
}

static forceinline void riscv_store_float(rvvm_hart_t* vm, rvvm_addr_t vaddr, regid_t reg)
{
    const rvvm_addr_t vpn = vaddr >> RISCV_PAGE_SHIFT;
    const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
    if (likely(entry->w == vpn && !(vaddr & 3))) {
        write_float_le((void*)(size_t)(entry->ptr + vaddr), read_float_nanbox(&vm->fpu_registers[reg]));
    } else {
        float tmp;
        write_float_le_m(&tmp, read_float_nanbox(&vm->fpu_registers[reg]));
        riscv_mmu_op(vm, vaddr, &tmp, sizeof(tmp), RISCV_MMU_WRITE);
    }
}

#endif

#endif
