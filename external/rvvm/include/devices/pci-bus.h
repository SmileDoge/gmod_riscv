/*
pci-bus.h - Peripheral Component Interconnect Bus
Copyright (C) 2021  LekKit <github.com/LekKit>
                    cerg2010cerg2010 <github.com/cerg2010cerg2010>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_PCI_BUS_H
#define RVVM_PCI_BUS_H

#include "rvvmlib.h"

// PCI constants
#define PCI_BUS_IRQS          0x4
#define PCI_BUS_DEVS          0x20
#define PCI_DEV_FUNCS         0x8
#define PCI_FUNC_BARS         0x6

// PCI INTx pins
#define PCI_IRQ_PIN_INTA      0x1
#define PCI_IRQ_PIN_INTB      0x2
#define PCI_IRQ_PIN_INTC      0x3
#define PCI_IRQ_PIN_INTD      0x4

// Default PCI bus configuration
#define PCI_ECAM_ADDR_DEFAULT 0x30000000U
#define PCI_IO_ADDR_DEFAULT   0x03000000U
#define PCI_IO_SIZE_DEFAULT   0x00010000U
#define PCI_MEM_ADDR_DEFAULT  0x40000000U
#define PCI_MEM_SIZE_DEFAULT  0x40000000U

// PCI function address in the form of [bus:8] | [dev:5] | [func:3]
typedef uint32_t pci_bus_addr_t;

// PCI function handle
typedef struct rvvm_pci_function pci_func_t;

// PCI device handle
typedef struct rvvm_pci_device pci_dev_t;

// PCI single function description (For multi-function devices, see pci_dev_desc_t)
typedef struct {
    uint16_t        vendor_id;
    uint16_t        device_id;
    uint16_t        class_code;
    uint8_t         prog_if;
    uint8_t         rev;
    uint8_t         irq_pin;
    rvvm_mmio_dev_t bar[PCI_FUNC_BARS];
    rvvm_mmio_dev_t expansion_rom;
} pci_func_desc_t;

// PCI multi-function device description
typedef struct {
    const pci_func_desc_t* func[PCI_DEV_FUNCS];
} pci_dev_desc_t;

/**
 * @defgroup rvvm_pci_api PCI Management API
 * @addtogroup rvvm_pci_api
 * @{
 */

//! \brief  Attach a manually configured PCIe host ECAM controller to a powered off machine
//! \param  machine   RVVM Machine handle
//! \param  intc      Wired interrupt controller handle
//! \param  irqs      Vector of 4 wire IRQs for legacy INTx interrupts
//! \param  ecam_addr Base address of ECAM space
//! \param  bus_count Amount of PCI bus addressing units
//! \param  io_addr   Start of PCI IO Port Space (Unused, but neded for proper guest initialization)
//! \param  io_len    Length of PCI IO Port Space (Unused, but neded for proper guest initialization)
//! \param  mem_addr  Start of PCI MMIO Space
//! \param  mem_len   Length of PCI MMIO Space
//! \return PCI bus handle, or NULL on failure
PUBLIC pci_bus_t* pci_bus_init(rvvm_machine_t* machine,                   // Owner machine
                               rvvm_intc_t* intc, const rvvm_irq_t* irqs, // Wired IRQ lines (4 entries)
                               rvvm_addr_t ecam_addr, size_t bus_count,   // ECAM region (Size calculated via bus_count)
                               rvvm_addr_t io_addr, size_t io_len,        // IO region
                               rvvm_addr_t mem_addr, size_t mem_len);     // MEM region

//! \brief  Attach an automatically configured PCIe host ECAM controller to a powered off machine
//! \param  machine RVVM Machine handle with a working IRQ controller
//! \return PCI bus handle, or NULL on failure
PUBLIC pci_bus_t* pci_bus_init_auto(rvvm_machine_t* machine);

//! \brief  Attach a single-function PCI device to a PCI bus (Hot-plug allowed)
//! \param  bus  Valid PCI bus handle
//! \param  desc PCI function description, device data is always cleaned up automatically
//! \return PCI device handle, or NULL on failure
PUBLIC pci_dev_t* pci_attach_func(pci_bus_t* bus, const pci_func_desc_t* desc);

//! \brief  Attach a multi-function PCI device to a PCI bus (Hot-plug allowed)
//! \param  bus  Valid PCI bus handle
//! \param  desc PCI multi-function description, device data is always cleaned up automatically
//! \return PCI device handle, or NULL on failure
PUBLIC pci_dev_t* pci_attach_multifunc(pci_bus_t* bus, const pci_dev_desc_t* desc);

//! \brief  Attach a single-function PCI device at specific PCI bus address (Hot-plug allowed)
//! \param  bus      Valid PCI bus handle
//! \param  desc     PCI function description, device data is always cleaned up automatically
//! \param  bus_addr PCI function address on a PCI bus
//! \return PCI device handle, or NULL on failure
PUBLIC pci_dev_t* pci_attach_func_at(pci_bus_t* bus, const pci_func_desc_t* desc, pci_bus_addr_t bus_addr);

//! \brief  Attach a multi-function PCI device at specific PCI bus address (Hot-plug allowed)
//! \param  bus      Valid PCI bus handle
//! \param  desc     PCI multi-function description, device data is always cleaned up automatically
//! \param  bus_addr PCI device address on a PCI bus
//! \return PCI device handle, or NULL on failure
PUBLIC pci_dev_t* pci_attach_multifunc_at(pci_bus_t* bus, const pci_dev_desc_t* desc, pci_bus_addr_t bus_addr);

//! \brief  Get a PCI device handle from a PCI bus address
//! \param  bus      Valid PCI bus handle
//! \param  bus_addr PCI bus address
//! \return PCI device handle, or NULL on failure
PUBLIC pci_dev_t* pci_get_bus_device(pci_bus_t* bus, pci_bus_addr_t bus_addr);

//! \brief  Get a PCI function handle from a PCI bus address
//! \param  bus      Valid PCI bus handle
//! \param  bus_addr PCI bus address
//! \return PCI function handle, or NULL on failure
PUBLIC pci_func_t* pci_get_bus_func(pci_bus_t* bus, pci_bus_addr_t bus_addr);

//! \brief  Get a PCI bus address of a PCI device
//! \param  dev Valid PCI device handle
//! \return PCI bus address
PUBLIC pci_bus_addr_t pci_get_device_bus_addr(pci_dev_t* dev);

//! \brief  Get a PCI bus address of a PCI function
//! \param  func Valid PCI function handle
//! \return PCI bus address
PUBLIC pci_bus_addr_t pci_get_func_bus_addr(pci_func_t* func);

//! \brief  Unplug a PCI device from the slot (Hot-unplug allowed)
//! \param  dev Valid PCI device handle, device data is always cleaned up automatically
PUBLIC void pci_remove_device(pci_dev_t* dev);

/** @}*/

/**
 * @defgroup rvvm_pci_device_api PCI Device API
 * @addtogroup rvvm_pci_device_api
 * @{
 */

//! \brief  Get a PCI function handle from a PCI device handle
//! \param  dev     Valid PCI device handle
//! \param  func_id PCI function ID in the range of 0-7
//! \return PCI function handle, or NULL on failure
PUBLIC pci_func_t* pci_get_device_func(pci_dev_t* dev, size_t func_id);

//! \brief Send INTx/MSI/MSI-X interrupt edge to the PCI host
//! \param func   Valid handle to a PCI function which sent the IRQ
//! \param msi_id MSI/MSI-X IRQ Vector ID (Ignored with INTx emulation)
PUBLIC void pci_send_irq(pci_func_t* func, uint32_t msi_id);

//! \brief Raise INTx/MSI/MSI-X interrupt vector
//! \param func   Valid handle to a PCI function which raised the IRQ
//! \param msi_id MSI/MSI-X IRQ Vector ID (Ignored with INTx emulation)
PUBLIC void pci_raise_irq(pci_func_t* func, uint32_t msi_id);

//! \brief Lower INTx/MSI/MSI-X interrupt vector
//! \param func   Valid handle to a PCI function which lowered the IRQ
//! \param msi_id MSI/MSI-X IRQ Vector ID (Ignored with INTx emulation)
PUBLIC void pci_lower_irq(pci_func_t* func, uint32_t msi_id);

//! \brief Perform direct memory access to the PCI host
//! \param func Valid handle to a PCI function which performs DMA access
//! \param addr Physical memory address
//! \param size Memory region size
//! \return Direct pointer to PCI host memory, or NULL on failure
PUBLIC void* pci_get_dma_ptr(pci_func_t* func, rvvm_addr_t addr, size_t size);

/** @}*/

#endif
