/*
riscv-aplic.h - RISC-V Advanced Platform-Level Interrupt Controller
Copyright (C) 2024  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_APLIC_H
#define RVVM_APLIC_H

#include "rvvmlib.h"

#define APLIC_M_ADDR_DEFAULT 0x0C000000U
#define APLIC_S_ADDR_DEFAULT 0x0D000000U

#undef s_addr

PUBLIC rvvm_intc_t* riscv_aplic_init(rvvm_machine_t* machine, rvvm_addr_t m_addr, rvvm_addr_t s_addr);

PUBLIC rvvm_intc_t* riscv_aplic_init_auto(rvvm_machine_t* machine);

#endif
