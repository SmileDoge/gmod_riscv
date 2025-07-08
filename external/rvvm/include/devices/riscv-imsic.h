/*
riscv-imsic.h - RISC-V Incoming Message-Signaled Interrupt Controller
Copyright (C) 2024  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_IMSIC_H
#define RVVM_IMSIC_H

#include "rvvmlib.h"

#define IMSIC_M_ADDR_DEFAULT 0x24000000U
#define IMSIC_S_ADDR_DEFAULT 0x28000000U

PUBLIC void riscv_imsic_init(rvvm_machine_t* machine, rvvm_addr_t addr, bool smode);

PUBLIC void riscv_imsic_init_auto(rvvm_machine_t* machine);

#endif
