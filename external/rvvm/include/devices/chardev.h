/*
chardev.h - Character device backend for UART
Copyright (C) 2023  LekKit <github.com/LekKit>
                    宋文武 <iyzsong@envs.net>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_CHARDEV_H
#define RVVM_CHARDEV_H

#include "rvvmlib.h"

/*
 * IO Device - UART (or else) owning the chardev
 * Chardev - Terminal, emulated VT, socket
 */

typedef struct rvvm_chardev chardev_t;

struct rvvm_chardev {
    // IO Dev -> Chardev calls
    uint32_t (*poll)(chardev_t* dev);
    size_t (*read)(chardev_t* dev, void* buf, size_t nbytes);
    size_t (*write)(chardev_t* dev, const void* buf, size_t nbytes);

    // Chardev -> IO Device notifications (IRQ)
    void   (*notify)(void* io_dev, uint32_t flags);

    // Common RVVM API features
    void   (*update)(chardev_t* dev);
    void   (*remove)(chardev_t* dev);

    void* data;
    void* io_dev;
};

#define CHARDEV_RX 0x1
#define CHARDEV_TX 0x2

static inline uint32_t chardev_poll(chardev_t* dev)
{
    if (dev && dev->poll) {
        return dev->poll(dev);
    }
    return CHARDEV_TX;
}

static inline size_t chardev_read(chardev_t* dev, void* buf, size_t nbytes)
{
    if (dev && dev->read) {
        return dev->read(dev, buf, nbytes);
    }
    return 0;
}

static inline size_t chardev_write(chardev_t* dev, const void* buf, size_t nbytes)
{
    if (dev && dev->write) {
        return dev->write(dev, buf, nbytes);
    }
    return nbytes;
}

static inline void chardev_free(chardev_t* dev)
{
    if (dev && dev->remove) {
        dev->remove(dev);
    }
}

static inline void chardev_update(chardev_t* dev)
{
    if (dev && dev->update) {
        dev->update(dev);
    }
}

static inline void chardev_notify(chardev_t* dev, uint32_t flags)
{
    if (dev && dev->notify) {
        dev->notify(dev->io_dev, flags);
    }
}

// Built-in chardev implementations

PUBLIC chardev_t* chardev_term_create(void); // stdio
PUBLIC chardev_t* chardev_fd_create(int rfd, int wfd); // POSIX fd
PUBLIC chardev_t* chardev_pty_create(const char* path); // POSIX pipe/pty

#endif
