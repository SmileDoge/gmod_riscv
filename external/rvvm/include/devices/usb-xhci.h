/*
usb-xhci.h - USB Extensible Host Controller Interface
Copyright (C) 2024  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_USB_XHCI_H
#define RVVM_USB_XHCI_H

#include "pci-bus.h"

PUBLIC pci_dev_t* usb_xhci_init(pci_bus_t* pci_bus);

#endif
