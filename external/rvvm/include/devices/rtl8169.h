/*
rtl8169.h - Realtek RTL8169 NIC
Copyright (C) 2022  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_RTL8169_H
#define RVVM_RTL8169_H

#include "pci-bus.h"
#include "tap_api.h"

PUBLIC pci_dev_t* rtl8169_init(pci_bus_t* pci_bus, tap_dev_t* tap);
PUBLIC pci_dev_t* rtl8169_init_auto(rvvm_machine_t* machine);

#endif
