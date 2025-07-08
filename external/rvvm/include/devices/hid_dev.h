/*
hid_dev.h - Generic HID Device API
Copyright (C) 2022  X512 <github.com/X547>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_HID_DEV_H
#define RVVM_HID_DEV_H

#include <stdint.h>
#include <stdbool.h>

#define REPORT_TYPE_INPUT   1
#define REPORT_TYPE_OUTPUT  2
#define REPORT_TYPE_FEATURE 3

#define HID_PROTOCOL_BOOT   0
#define HID_PROTOCOL_REPORT 1

#define HID_POWER_ON    0
#define HID_POWER_SLEEP 1

typedef struct {
    void* host;
    void* dev;

    // static information
    const uint8_t* report_desc;
    uint16_t report_desc_size;
    uint16_t max_input_size;
    uint16_t max_output_size;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t version_id;

    // device -> host calls
    void (*input_available)(void* host, uint8_t report_id);

    void (*reset)(void* dev);
    void (*read_report )(void* dev, uint8_t report_type, uint8_t report_id, uint32_t offset, uint8_t *val);
    void (*write_report)(void* dev, uint8_t report_type, uint8_t report_id, uint32_t offset, uint8_t val);
    void (*get_idle)(void* dev, uint8_t report_id, uint16_t* idle /* milliseconds */);
    void (*set_idle)(void* dev, uint8_t report_id, uint16_t idle);
    void (*get_protocol)(void* dev, uint16_t* protocol);
    void (*set_protocol)(void* dev, uint16_t protocol);
    void (*set_power)(void* dev, uint16_t power);
    void (*remove)(void* dev);
} hid_dev_t;

#endif    // RVVM_HID_DEV_H
