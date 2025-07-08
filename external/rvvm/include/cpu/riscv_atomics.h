/*
riscv_atomics.h - RISC-V Atomics interpreter
Copyright (C) 2024  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RISCV_ATOMICS_H
#define RISCV_ATOMICS_H

#define RISCV_AMO_LR   0x2
#define RISCV_AMO_SC   0x3
#define RISCV_AMO_SWAP 0x1
#define RISCV_AMO_ADD  0x0
#define RISCV_AMO_XOR  0x4
#define RISCV_AMO_AND  0xC
#define RISCV_AMO_OR   0x8
#define RISCV_AMO_MIN  0x10
#define RISCV_AMO_MAX  0x14
#define RISCV_AMO_MINU 0x18
#define RISCV_AMO_MAXU 0x1C

static forceinline void riscv_emulate_atomic_w(rvvm_hart_t* vm, const uint32_t insn)
{
    const uint32_t op = insn >> 27;
    const regid_t rds = bit_cut(insn, 7, 5);
    const regid_t rs1 = bit_cut(insn, 15, 5);
    const regid_t rs2 = bit_cut(insn, 20, 5);
    const xaddr_t vaddr = riscv_read_reg(vm, rs1);
    const uint32_t val = riscv_read_reg(vm, rs2);
    // MMIO atomics bounce buffer
    uint32_t bounce = 0;
    void* ptr = NULL;

    if (unlikely(vaddr & 3)) {
        // Misaligned atomic
        uint32_t cause = (op == RISCV_AMO_LR) ? RISCV_TRAP_LOAD_MISALIGN : RISCV_TRAP_STORE_MISALIGN;
        riscv_trap(vm, cause, vaddr);
        return;
    }

    if (op == RISCV_AMO_LR) {
        ptr = riscv_ro_translate(vm, vaddr, &bounce, sizeof(bounce));
    } else {
        ptr = riscv_rmw_translate(vm, vaddr, &bounce, sizeof(bounce));
    }

    if (unlikely(!ptr)) {
        // Pagefault
        return;
    }

    switch (op) {
        case RISCV_AMO_LR:
            // Mark our reservation
            vm->lrsc = true;
            vm->lrsc_addr = vaddr;
            vm->lrsc_cas = atomic_load_uint32_le(ptr);
            riscv_write_reg(vm, rds, (int32_t)vm->lrsc_cas);

            // Return to skip RMW MMIO commit
            return;
        case RISCV_AMO_SC:
            // If our reservation is valid, perform a CAS
            if (vm->lrsc && vm->lrsc_addr == vaddr && atomic_cas_uint32_le(ptr, vm->lrsc_cas, val)) {
                // SC success
                riscv_write_reg(vm, rds, 0);
                vm->lrsc = false;
            } else {
                // SC failed, skip RMW MMIO commit
                riscv_write_reg(vm, rds, 1);
                vm->lrsc = false;
                return;
            }
            break;
        case RISCV_AMO_SWAP:
            riscv_write_reg(vm, rds, (int32_t)atomic_swap_uint32_le(ptr, val));
            break;
        case RISCV_AMO_ADD:
            riscv_write_reg(vm, rds, (int32_t)atomic_add_uint32_le(ptr, val));
            break;
        case RISCV_AMO_XOR:
            riscv_write_reg(vm, rds, (int32_t)atomic_xor_uint32_le(ptr, val));
            break;
        case RISCV_AMO_AND:
            riscv_write_reg(vm, rds, (int32_t)atomic_and_uint32_le(ptr, val));
            break;
        case RISCV_AMO_OR:
            riscv_write_reg(vm, rds, (int32_t)atomic_or_uint32_le(ptr, val));
            break;
        case RISCV_AMO_MIN:
            riscv_write_reg(vm, rds, (int32_t)atomic_min_int32_le(ptr, val));
            break;
        case RISCV_AMO_MAX:
            riscv_write_reg(vm, rds, (int32_t)atomic_max_int32_le(ptr, val));
            break;
        case RISCV_AMO_MINU:
            riscv_write_reg(vm, rds, (int32_t)atomic_minu_uint32_le(ptr, val));
            break;
        case RISCV_AMO_MAXU:
            riscv_write_reg(vm, rds, (int32_t)atomic_maxu_uint32_le(ptr, val));
            break;
        default:
            riscv_illegal_insn(vm, insn);
            return;
    }

    if (unlikely(ptr == &bounce)) {
        riscv_rmw_mmio_commit(vm, vaddr, &bounce, sizeof(bounce));
    }
}

#ifdef RV64

static forceinline void riscv_emulate_atomic_d(rvvm_hart_t* vm, const uint32_t insn)
{
    const uint32_t op = insn >> 27;
    const regid_t rds = bit_cut(insn, 7, 5);
    const regid_t rs1 = bit_cut(insn, 15, 5);
    const regid_t rs2 = bit_cut(insn, 20, 5);
    const xaddr_t vaddr = riscv_read_reg(vm, rs1);
    const uint64_t val = riscv_read_reg(vm, rs2);
    // MMIO atomics bounce buffer
    uint64_t bounce;
    void* ptr = NULL;

    if (unlikely(vaddr & 7)) {
        uint32_t cause = (op == RISCV_AMO_LR) ? RISCV_TRAP_LOAD_MISALIGN : RISCV_TRAP_STORE_MISALIGN;
        riscv_trap(vm, cause, vaddr);
        return;
    }

    if (op == RISCV_AMO_LR) {
        ptr = riscv_ro_translate(vm, vaddr, &bounce, sizeof(bounce));
    } else {
        ptr = riscv_rmw_translate(vm, vaddr, &bounce, sizeof(bounce));
    }

    if (unlikely(!ptr)) {
        // Pagefault
        return;
    }

    switch (op) {
        case RISCV_AMO_LR:
            // Mark our reservation
            vm->lrsc = true;
            vm->lrsc_addr = vaddr;
            vm->lrsc_cas = atomic_load_uint64_le(ptr);
            riscv_write_reg(vm, rds, vm->lrsc_cas);

            // Return to skip RMW MMIO commit
            return;
        case RISCV_AMO_SC:
            // If our reservation is valid, perform a CAS
            if (vm->lrsc && vm->lrsc_addr == vaddr && atomic_cas_uint64_le(ptr, vm->lrsc_cas, val)) {
                // SC success
                riscv_write_reg(vm, rds, 0);
                vm->lrsc = false;
            } else {
                // SC failed, skip RMW MMIO commit
                riscv_write_reg(vm, rds, 1);
                vm->lrsc = false;
                return;
            }
            break;
        case RISCV_AMO_SWAP:
            riscv_write_reg(vm, rds, atomic_swap_uint64_le(ptr, val));
            break;
        case RISCV_AMO_ADD:
            riscv_write_reg(vm, rds, atomic_add_uint64_le(ptr, val));
            break;
        case RISCV_AMO_XOR:
            riscv_write_reg(vm, rds, atomic_xor_uint64_le(ptr, val));
            break;
        case RISCV_AMO_AND:
            riscv_write_reg(vm, rds, atomic_and_uint64_le(ptr, val));
            break;
        case RISCV_AMO_OR:
            riscv_write_reg(vm, rds, atomic_or_uint64_le(ptr, val));
            break;
        case RISCV_AMO_MIN:
            riscv_write_reg(vm, rds, atomic_min_int64_le(ptr, val));
            break;
        case RISCV_AMO_MAX:
            riscv_write_reg(vm, rds, atomic_max_int64_le(ptr, val));
            break;
        case RISCV_AMO_MINU:
            riscv_write_reg(vm, rds, atomic_minu_uint64_le(ptr, val));
            break;
        case RISCV_AMO_MAXU:
            riscv_write_reg(vm, rds, atomic_maxu_uint64_le(ptr, val));
            break;
        default:
            riscv_illegal_insn(vm, insn);
            break;
    }

    if (unlikely(ptr == &bounce)) {
        riscv_rmw_mmio_commit(vm, vaddr, &bounce, sizeof(bounce));
    }
}

#endif

static no_inline void riscv_emulate_a_opc_amo(rvvm_hart_t* vm, const uint32_t insn)
{
    const uint32_t funct3 = bit_cut(insn, 12, 3);
    switch (funct3) {
        case 0x2:
            riscv_emulate_atomic_w(vm, insn);
            return;
#ifdef RV64
        case 0x3:
            riscv_emulate_atomic_d(vm, insn);
            return;
#endif
    }
    riscv_illegal_insn(vm, insn);
}

#endif
