/*
rtc-goldfish.h - Goldfish Real-time Clock
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_RTC_GOLDFISH_H
#define RVVM_RTC_GOLDFISH_H

#include "rvvmlib.h"

#define RTC_GOLDFISH_ADDR_DEFAULT 0x101000

PUBLIC rvvm_mmio_dev_t* rtc_goldfish_init(rvvm_machine_t* machine, rvvm_addr_t addr,
                                          rvvm_intc_t* intc, rvvm_irq_t irq);

PUBLIC rvvm_mmio_dev_t* rtc_goldfish_init_auto(rvvm_machine_t* machine);

#endif

