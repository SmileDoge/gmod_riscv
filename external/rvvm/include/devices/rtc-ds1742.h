/*
rtc-ds7142.h - Dallas DS1742 Real-time Clock
Copyright (C) 2023  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_RTC_DS1742_H
#define RVVM_RTC_DS1742_H

#include "rvvmlib.h"

#define RTC_DS1742_DEFAULT_MMIO 0x101000

PUBLIC rvvm_mmio_dev_t* rtc_ds1742_init(rvvm_machine_t* machine, rvvm_addr_t base_addr);
PUBLIC rvvm_mmio_dev_t* rtc_ds1742_init_auto(rvvm_machine_t* machine);

#endif

