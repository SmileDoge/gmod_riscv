/*
syscon.h - Poweroff/reset syscon device
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_SYSCON_H
#define RVVM_SYSCON_H

#include "rvvmlib.h"

#define SYSCON_DEFAULT_MMIO 0x100000

PUBLIC rvvm_mmio_dev_t* syscon_init(rvvm_machine_t* machine, rvvm_addr_t base_addr);
PUBLIC rvvm_mmio_dev_t* syscon_init_auto(rvvm_machine_t* machine);

#endif
