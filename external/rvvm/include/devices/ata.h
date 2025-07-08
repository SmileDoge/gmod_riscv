/*
ata.h - IDE/ATA disk controller
Copyright (C) 2021  cerg2010cerg2010 <github.com/cerg2010cerg2010>
                    LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_ATA_H
#define RVVM_ATA_H

#include "rvvmlib.h"
#include "pci-bus.h"

#define ATA_DATA_ADDR_DEFAULT 0x00200000
#define ATA_CTL_ADDR_DEFAULT  0x00201000U

// Attach ATA PIO at specific address
PUBLIC bool ata_pio_init(rvvm_machine_t* machine, rvvm_addr_t ata_data_addr, rvvm_addr_t ata_ctl_addr, const char* image, bool rw);

// Automatically attach ATA PIO to a machine
PUBLIC bool ata_pio_init_auto(rvvm_machine_t* machine, const char* image, bool rw);

// Attach PCI ATA UDMA (IDE) to a PCI bus
PUBLIC pci_dev_t* ata_pci_init(pci_bus_t* pci_bus, const char* image, bool rw);

// Automatically attach PCI ATA UDMA (IDE) to a machine
PUBLIC pci_dev_t* ata_pci_init_auto(rvvm_machine_t* machine, const char* image, bool rw);

// Automatically attach ATA drive to a machine
PUBLIC bool ata_init_auto(rvvm_machine_t* machine, const char* image, bool rw);

#endif
