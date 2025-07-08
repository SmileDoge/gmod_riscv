/*
riscv_priv.h - RISC-V Privileged
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RISCV_PRIV_H
#define RISCV_PRIV_H

#include "rvvm.h"

slow_path void riscv_emulate_opc_system(rvvm_hart_t* vm, const uint32_t insn);
slow_path void riscv_emulate_opc_misc_mem(rvvm_hart_t* vm, const uint32_t insn);

#endif
