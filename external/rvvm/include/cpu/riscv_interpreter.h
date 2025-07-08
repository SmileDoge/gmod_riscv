/*
riscv_interpreter.h - RISC-V Template Interpreter
Copyright (C) 2024  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RISCV_INTERPRETER_H
#define RISCV_INTERPRETER_H

#include "rvvm.h"
#include "riscv_cpu.h"
#include "riscv_hart.h"
#include "riscv_mmu.h"
#include "riscv_jit.h"
#include "compiler.h"
#include "bit_ops.h"

/*
 * Interpreter helpers
 */

#ifdef RV64

typedef uint64_t xlen_t;
typedef int64_t  sxlen_t;
typedef uint64_t xaddr_t;
#define SHAMT_BITS 6
#define DIV_OVERFLOW_RS1 ((sxlen_t)0x8000000000000000ULL)

#else

typedef uint32_t xlen_t;
typedef int32_t  sxlen_t;
typedef uint32_t xaddr_t;
#define SHAMT_BITS 5
#define DIV_OVERFLOW_RS1 ((sxlen_t)0x80000000U)

#endif

static forceinline xlen_t riscv_read_reg(rvvm_hart_t* vm, regid_t reg)
{
    return vm->registers[reg];
}

static forceinline sxlen_t riscv_read_reg_s(rvvm_hart_t* vm, regid_t reg)
{
    return vm->registers[reg];
}

static forceinline void riscv_write_reg(rvvm_hart_t* vm, regid_t reg, sxlen_t data)
{
    vm->registers[reg] = data;
}

// Provides entry point to riscv_emulate_insn()
#include "riscv_compressed.h"

/*
 * Optimized CPU interpreter dispatch loop.
 * Executes instructions until some event occurs (interrupt, trap).
 *
 * Calling this with vm->running == 0 allows single-stepping guest instructions.
 *
 * NOTE: Call riscv_restart_dispatch() after flushing TLB around PC,
 * otherwise the CPU loop will continue executing current page.
 */

TSAN_SUPPRESS void riscv_run_interpreter(rvvm_hart_t* vm)
{
    uint32_t insn = 0;

    // This is similar to TLB mechanism, but persists in local variables across instructions
    size_t insn_ptr = 0;
    xlen_t insn_page = vm->registers[RISCV_REG_PC] + RISCV_PAGE_SIZE;

    do {
        const xlen_t insn_addr = vm->registers[RISCV_REG_PC];
        if (likely(insn_addr - insn_page <= 0xFFC)) {
            // Direct instruction fetch by pointer
            insn = read_uint32_le_m((const void*)(size_t)(insn_ptr + insn_addr));
        } else {
            uint32_t tmp = 0;
            if (likely(riscv_fetch_insn(vm, insn_addr, &tmp))) {
                // Update pointer to the current page in real memory
                // If we are executing code from MMIO, direct memory fetch fails
                const xlen_t vpn = vm->registers[RISCV_REG_PC] >> RISCV_PAGE_SHIFT;
                const rvvm_tlb_entry_t* entry = &vm->tlb[vpn & RVVM_TLB_MASK];
                insn = tmp;
                insn_ptr = entry->ptr;
                insn_page = entry->e << RISCV_PAGE_SHIFT;
            } else {
                // Instruction fetch fault happened
                return;
            }
        }

#if defined(USE_JIT) && (defined(RVJIT_NATIVE_64BIT) || !defined(RV64))
        if (likely(vm->jit_compiling)) {
            // If we hit non-compilable instruction or cross page boundaries, current JIT block is finalized.
            if (vm->jit_block_ends || (vm->jit.virt_pc >> RISCV_PAGE_SHIFT) != (vm->registers[RISCV_REG_PC] >> RISCV_PAGE_SHIFT)) {
                riscv_jit_finalize(vm);
            }
            vm->jit_block_ends = true;
        }
#endif

        // Implicitly zero x0 register each time
        vm->registers[RISCV_REG_ZERO] = 0;

        riscv_emulate_insn(vm, insn);
    } while (likely(vm->running));
}

#endif
