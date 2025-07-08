/*
blk_io.h - Cross-platform Block & File IO library
Copyright (C) 2022  0xCatPKG <0xCatPKG@rvvm.dev>
                    0xCatPKG <github.com/0xCatPKG>
                    LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef BLK_IO_H
#define BLK_IO_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/*
 * File API
 */

#define RVFILE_RW     0x1  // Open file in read/write mode
#define RVFILE_CREAT  0x2  // Create file if it doesn't exist (for RW only)
#define RVFILE_EXCL   0x4  // Prevent other processes from opening this file
#define RVFILE_TRUNC  0x8  // Truncate file conents upon opening (for RW only)
#define RVFILE_DIRECT 0x10 // Direct read/write DMA with the underlying storage
#define RVFILE_SYNC   0x20 // Disable writeback buffering

#define RVFILE_LEGAL_FLAGS 0x3F

#define RVFILE_SEEK_SET 0x0 // Set file cursor
#define RVFILE_SEEK_CUR 0x1 // Move file cursor
#define RVFILE_SEEK_END 0x2 // Set file cursor relative to end

// Use file cursor as offset for read/write
// Not suitable for async IO
#define RVFILE_CUR ((uint64_t)-1)

typedef struct blk_io_rvfile rvfile_t;

// Returns NULL on failure
rvfile_t* rvopen(const char* filepath, uint8_t filemode);
void      rvclose(rvfile_t* file);

// Get file size (Not synced across processes)
uint64_t  rvfilesize(rvfile_t* file);

// If offset == RVFILE_CURPOS, uses current file position as offset
// Otherwise is equialent to pread/pwrite, and is thread-safe
size_t    rvread(rvfile_t* file, void* dst, size_t size, uint64_t offset);
size_t    rvwrite(rvfile_t* file, const void* src, size_t size, uint64_t offset);

// Seek/tell for positioned IO
bool      rvseek(rvfile_t* file, int64_t offset, uint8_t startpos);
uint64_t  rvtell(rvfile_t* file);

// Trim (punch a hole) in file, leaving zeroes and releasing space on the host
bool      rvtrim(rvfile_t* file, uint64_t offset, uint64_t size);

// Set/grow file size
bool      rvtruncate(rvfile_t* file, uint64_t length);
bool      rvfallocate(rvfile_t* file, uint64_t length);

// Sync buffers to disk, or issue a write barrier (Depending on platform).
// NOTE: If this fails, do NOT perform further actions and GTFO!
bool      rvfsync(rvfile_t* file);

// Get native POSIX file descriptor, returns -1 on failure
int rvfile_get_posix_fd(rvfile_t* file);

// Get native Win32 file handle, returns NULL on failure
void* rvfile_get_win32_handle(rvfile_t* file);

/*
 * Block device API
 *
 * It's illegal to seek out of device bounds, resizing the device is also impossible.
 */

#define BLKDEV_RW  RVFILE_RW

#define BLKDEV_SEEK_SET RVFILE_SEEK_SET
#define BLKDEV_SEEK_CUR RVFILE_SEEK_CUR
#define BLKDEV_SEEK_END RVFILE_SEEK_END

#define BLKDEV_CUR RVFILE_CUR

typedef struct {
    const char* name;
    void     (*close)(void* dev);
    size_t   (*read)(void* dev, void* dst, size_t count, uint64_t offset);
    size_t   (*write)(void* dev, const void* src, size_t count, uint64_t offset);
    bool     (*trim)(void* dev, uint64_t offset, uint64_t count);
    bool     (*sync)(void* dev);
} blkdev_type_t;

typedef struct blkdev_t blkdev_t;

struct blkdev_t {
    const blkdev_type_t* type;
    void* data;
    uint64_t size;
    uint64_t pos;
};

// Open a block device image
blkdev_t* blk_open(const char* filename, uint8_t opts);

// Close a block device handle
void      blk_close(blkdev_t* dev);

// Get block device size in bytes
static inline uint64_t blk_getsize(blkdev_t* dev)
{
    if (dev) {
        return dev->size;
    }
    return 0;
}

// Read data from block device
static inline size_t blk_read(blkdev_t* dev, void* dst, size_t size, uint64_t offset)
{
    if (dev) {
        uint64_t real_pos = (offset == BLKDEV_CUR) ? dev->pos : offset;
        if (real_pos + size <= dev->size) {
            size_t ret = dev->type->read(dev->data, dst, size, real_pos);
            if (offset == BLKDEV_CUR) dev->pos += ret;
            return ret;
        }
    }
    return 0;
}

// Write data to block device
static inline size_t blk_write(blkdev_t* dev, const void* src, size_t size, uint64_t offset)
{
    if (dev) {
        uint64_t real_pos = (offset == BLKDEV_CUR) ? dev->pos : offset;
        if (real_pos + size <= dev->size) {
            size_t ret = dev->type->write(dev->data, src, size, real_pos);
            if (offset == BLKDEV_CUR) dev->pos += ret;
            return ret;
        }
    }
    return 0;
}

// Seek drive head (For seekable devices like ATA)
static inline bool blk_seek(blkdev_t* dev, int64_t offset, uint8_t startpos)
{
    if (!dev) {
        return false;
    }
    if (startpos == BLKDEV_SEEK_CUR) {
        offset = dev->pos + offset;
    } else if (startpos == BLKDEV_SEEK_END) {
        offset = dev->size - offset;
    } else if (startpos != BLKDEV_SEEK_SET) {
        return false;
    }
    if (((uint64_t)offset) >= dev->size) {
        // Illegal seek beyond device size
        return false;
    }
    dev->pos = offset;
    return true;
}

// Tell drive head position
static inline uint64_t blk_tell(blkdev_t* dev)
{
    if (dev) {
        return dev->pos;
    }
    return 0;
}

// Discard physical blocks, preferably zeroing them
static inline bool blk_trim(blkdev_t* dev, uint64_t offset, uint64_t size)
{
    if (dev && dev->type->trim) {
        uint64_t real_pos = (offset == BLKDEV_CUR) ? dev->pos : offset;
        if (real_pos + size > dev->size) {
            return dev->type->trim(dev->data, real_pos, size);
        }
    }
    return false;
}

// Flush write buffers
static inline bool blk_sync(blkdev_t* dev)
{
    if (dev && dev->type->sync) {
        return dev->type->sync(dev->data);
    }
    return false;
}

#endif
