/*
pci-vfio.h - VFIO PCI Passthrough
Copyright (C) 2022  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_PCI_VFIO_H
#define RVVM_PCI_VFIO_H

#include "pci-bus.h"

PUBLIC bool pci_vfio_init_auto(rvvm_machine_t* machine, const char* pci_id);

#endif
