/*
i2c-oc.h - OpenCores I2C Controller
Copyright (C) 2022  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_I2C_OC_H
#define RVVM_I2C_OC_H

#include "rvvmlib.h"

#define I2C_OC_ADDR_DEFAULT 0x10030000

#define I2C_AUTO_ADDR 0x0 // Auto-pick I2C device address

typedef struct {
    // I2C bus address
    uint16_t addr;
    // Device-specific data
    void*    data;

    // Start transaction, return device availability
    bool (*start)(void* dev, bool is_write);
    // Return false on NACK or no data to read
    bool (*write)(void* dev, uint8_t byte);
    bool (*read)(void* dev, uint8_t* byte);
    // Stop the current transaction
    void (*stop)(void* dev);
    // Device cleanup
    void (*remove)(void* dev);
} i2c_dev_t;

PUBLIC i2c_bus_t* i2c_oc_init(rvvm_machine_t* machine, rvvm_addr_t addr, rvvm_intc_t* intc, rvvm_irq_t irq);
PUBLIC i2c_bus_t* i2c_oc_init_auto(rvvm_machine_t* machine);

// Returns assigned device address or zero on error
PUBLIC uint16_t   i2c_attach_dev(i2c_bus_t* bus, const i2c_dev_t* dev_desc);

// Get I2C controller FDT node for nested device nodes
PUBLIC struct fdt_node* i2c_bus_fdt_node(i2c_bus_t* bus);

#endif
