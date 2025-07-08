/*
gpio_api.h - General-Purpose IO API
Copyright (C) 2024  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_GPIO_API_H
#define RVVM_GPIO_API_H

typedef struct rvvm_gpio_dev rvvm_gpio_dev_t;

struct rvvm_gpio_dev {
    // IO Dev -> GPIO Dev calls
    bool (*pins_out)(rvvm_gpio_dev_t* dev, size_t off, uint32_t pins);

    // GPIO Dev -> IO Dev calls
    bool     (*pins_in)(rvvm_gpio_dev_t* dev, size_t off, uint32_t pins);
    uint32_t (*pins_read)(rvvm_gpio_dev_t* dev, size_t off);

    // Common RVVM API features
    void (*update)(rvvm_gpio_dev_t* dev);
    void (*remove)(rvvm_gpio_dev_t* dev);

    void* data;
    void* io_dev;
};

static inline bool gpio_pins_out(rvvm_gpio_dev_t* dev, size_t off, uint32_t pins)
{
    if (dev && dev->pins_out) return dev->pins_out(dev, off, pins);
    return false;
}

static inline bool gpio_write_pins(rvvm_gpio_dev_t* dev, size_t off, uint32_t pins)
{
    if (dev && dev->pins_in) return dev->pins_in(dev, off, pins);
    return false;
}

static inline uint32_t gpio_read_pins(rvvm_gpio_dev_t* dev, size_t off)
{
    if (dev && dev->pins_read) return dev->pins_read(dev, off);
    return 0;
}

static inline void gpio_free(rvvm_gpio_dev_t* dev)
{
    if (dev && dev->remove) dev->remove(dev);
}

static inline void gpio_update(rvvm_gpio_dev_t* dev)
{
    if (dev && dev->update) dev->update(dev);
}

#endif
