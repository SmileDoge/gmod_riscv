/*
riscv_cpu.h - RISC-V CPU Interfaces
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RISCV_CPU_H
#define RISCV_CPU_H

#include "rvvm.h"
#include "riscv_mmu.h"

// Run the vCPU in current thread
void riscv_run_till_event(rvvm_hart_t* vm);

// Trap the vCPU on illegal instruction
slow_path void riscv_illegal_insn(rvvm_hart_t* vm, const uint32_t insn);

// Internal interpreter ISA switching
void riscv32_run_interpreter(rvvm_hart_t* vm);
void riscv64_run_interpreter(rvvm_hart_t* vm);

/*
 * JIT infrastructure
 */

// Flush the JIT cache (Kinda like instruction cache)
void riscv_jit_flush_cache(rvvm_hart_t* vm);

// Discard the currently JITed block
static forceinline void riscv_jit_discard(rvvm_hart_t* vm)
{
#ifdef USE_JIT
    vm->jit_compiling = false;
#else
    UNUSED(vm);
#endif
}

// Finish the currently JITed block
static forceinline void riscv_jit_compile(rvvm_hart_t* vm)
{
#ifdef USE_JIT
    vm->jit_block_ends = true;
#else
    UNUSED(vm);
#endif
}

// Mark the physical memory as dirty (Overwritten)
#ifdef USE_JIT
void riscv_jit_mark_dirty_mem(rvvm_machine_t* machine, rvvm_addr_t addr, size_t size);
#else
static forceinline void riscv_jit_mark_dirty_mem(rvvm_machine_t* machine, rvvm_addr_t addr, size_t size) {
    UNUSED(machine);
    UNUSED(addr);
    UNUSED(size);
}
#endif

/*
 * Interpreter JIT glue
 */

slow_path bool riscv_jit_tlb_lookup(rvvm_hart_t* vm);

slow_path void riscv_jit_finalize(rvvm_hart_t* vm);

#endif
