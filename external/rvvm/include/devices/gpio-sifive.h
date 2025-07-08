/*
gpio-sifive.h - SiFive GPIO Controller
Copyright (C) 2024  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_GPIO_SIFIVE_H
#define RVVM_GPIO_SIFIVE_H

#include "rvvmlib.h"
#include "gpio_api.h"

#define GPIO_SIFIVE_PINS 32

#define GPIO_SIFIVE_ADDR_DEFAULT 0x10060000

PUBLIC rvvm_mmio_dev_t* gpio_sifive_init(rvvm_machine_t* machine, rvvm_gpio_dev_t* gpio,
                                         rvvm_addr_t base_addr, rvvm_intc_t* intc, const rvvm_irq_t* irqs);

PUBLIC rvvm_mmio_dev_t* gpio_sifive_init_auto(rvvm_machine_t* machine, rvvm_gpio_dev_t* gpio);

#endif
