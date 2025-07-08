/*
riscv-aclint.h - RISC-V Advanced Core Local Interruptor
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_ACLINT_H
#define RVVM_ACLINT_H

#include "rvvmlib.h"

#define CLINT_ADDR_DEFAULT 0x2000000U

PUBLIC void riscv_clint_init(rvvm_machine_t* machine, rvvm_addr_t addr);

PUBLIC void riscv_clint_init_auto(rvvm_machine_t* machine);

#endif
