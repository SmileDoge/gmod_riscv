/*
riscv-plic.c - RISC-V Platform-Level Interrupt Controller
Copyright (C) 2023  LekKit <github.com/LekKit>
              2021  cerg2010cerg2010 <github.com/cerg2010cerg2010>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_PLIC_H
#define RVVM_PLIC_H

#include "rvvmlib.h"

#define PLIC_ADDR_DEFAULT 0xC000000U

PUBLIC rvvm_intc_t* riscv_plic_init(rvvm_machine_t* machine, rvvm_addr_t addr);
PUBLIC rvvm_intc_t* riscv_plic_init_auto(rvvm_machine_t* machine);

#endif
