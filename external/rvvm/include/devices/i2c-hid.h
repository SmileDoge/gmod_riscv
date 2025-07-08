/*
i2c-hid.h - I2C HID Host Controller Interface
Copyright (C) 2022  X512 <github.com/X547>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef _I2C_HID_H_
#define _I2C_HID_H_

#include "hid_dev.h"
#include "rvvmlib.h"

PUBLIC void i2c_hid_init_auto(rvvm_machine_t* machine, hid_dev_t* hid_dev);

#endif  // _I2C_HID_H_
