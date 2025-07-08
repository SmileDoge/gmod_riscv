/*
framebuffer.h - Framebuffer context, RGB format handling
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_FRAMEBUFFER_H
#define RVVM_FRAMEBUFFER_H

#include "rvvmlib.h"

#define RGB_FMT_INVALID  0x0
#define RGB_FMT_R5G6B5   0x02
#define RGB_FMT_R8G8B8   0x03
#define RGB_FMT_A8R8G8B8 0x04 //!< Little-endian: BGRA, Big-endian: ARGB (Recommended)
#define RGB_FMT_A8B8G8R8 0x14 //!< Little-endian: RGBA, Big-endian: ABGR

//! Pixel RGB format
typedef uint8_t rgb_fmt_t;

//! Framebuffer context description
typedef struct {
    void*     buffer; //!< Buffer in host memory
    uint32_t  width;  //!< Width in pixels
    uint32_t  height; //!< Height in pixels
    uint32_t  stride; //!< Line alignment. Set to 0 if unsure.
    rgb_fmt_t format; //!< Pixel format
} fb_ctx_t;

/*
 * Pixel format handling
 */

//! Get bytes per pixel for a format
static inline size_t rgb_format_bytes(rgb_fmt_t format)
{
    switch (format) {
        case RGB_FMT_R5G6B5:   return 2;
        case RGB_FMT_R8G8B8:   return 3;
        case RGB_FMT_A8R8G8B8: return 4;
        case RGB_FMT_A8B8G8R8: return 4;
    }
    return 0;
}

//! Get bits per pixel (bpp) for a format
static inline size_t rgb_format_bpp(rgb_fmt_t format)
{
    return rgb_format_bytes(format) << 3;
}

//! Get pixel format from bpp
static inline rgb_fmt_t rgb_format_from_bpp(size_t bpp)
{
    switch (bpp) {
        case 16: return RGB_FMT_R5G6B5;
        case 24: return RGB_FMT_R8G8B8;
        // Default to ARGB when bpp = 32, this is what most guests and hosts expect
        case 32: return RGB_FMT_A8R8G8B8;
    }
    return RGB_FMT_INVALID;
}

/*
 * Framebuffer API
 */

//! Calculate effective framebuffer stride
static inline size_t framebuffer_stride(const fb_ctx_t* fb)
{
    return fb->stride ? fb->stride : fb->width * rgb_format_bytes(fb->format);
}

//! Calculate framebuffer region size
static inline size_t framebuffer_size(const fb_ctx_t* fb)
{
    return framebuffer_stride(fb) * fb->height;
}

//! \brief   Attach framebuffer context to the machine.
//! \warning The buffer is not freed automatically.
PUBLIC rvvm_mmio_dev_t* framebuffer_init(rvvm_machine_t* machine, rvvm_addr_t addr, const fb_ctx_t* fb);

PUBLIC rvvm_mmio_dev_t* framebuffer_init_auto(rvvm_machine_t* machine, const fb_ctx_t* fb);

#endif
