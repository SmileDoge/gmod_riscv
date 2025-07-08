/*
ns16550a.h - NS16550A UART
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_NS16550A_H
#define RVVM_NS16550A_H

#include "rvvmlib.h"
#include "chardev.h"

#define NS16550A_ADDR_DEFAULT 0x10000000

PUBLIC rvvm_mmio_dev_t* ns16550a_init(rvvm_machine_t* machine, chardev_t* chardev,
                                      rvvm_addr_t addr, rvvm_intc_t* intc, rvvm_irq_t irq);

PUBLIC rvvm_mmio_dev_t* ns16550a_init_auto(rvvm_machine_t* machine, chardev_t* chardev);

PUBLIC rvvm_mmio_dev_t* ns16550a_init_term_auto(rvvm_machine_t* machine);

#endif
