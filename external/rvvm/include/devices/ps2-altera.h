/*
ps2-altera.h - Altera PS2 Controller
Copyright (C) 2021  LekKit <githun.com/LekKit>
                    cerg2010cerg2010 <github.com/cerg2010cerg2010>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_PS2_ALTERA_H
#define RVVM_PS2_ALTERA_H

#include "rvvmlib.h"
#include "chardev.h"

#define PS2_ALTERA_ADDR_DEFAULT 0x20000000U

void ps2_altera_init(rvvm_machine_t* machine, chardev_t* chardev, rvvm_addr_t addr, rvvm_intc_t* intc, rvvm_irq_t irq);

void ps2_altera_init_auto(rvvm_machine_t* machine, chardev_t* chardev);

#endif
