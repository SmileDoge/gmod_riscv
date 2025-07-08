/*
dlib.h - Dynamic library loader
Copyright (C) 2023  LekKit <github.com/LekKit>
                    0xCatPKG <0xCatPKG@rvvm.dev>
                    0xCatPKG <github.com/0xCatPKG>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_DLIB_H
#define RVVM_DLIB_H

#include <stdbool.h>
#include <stdint.h>

#define DLIB_NAME_PROBE 0x1 // Probe various A.so, libA.so variations
#define DLIB_MAY_UNLOAD 0x2 // Allow dlib_close() to actually unload the library

typedef struct dlib_ctx dlib_ctx_t;

// Load a library by name
dlib_ctx_t* dlib_open(const char* lib_name, uint32_t flags);

// Drop the library handle. unload the library if DLIB_MAY_UNLOAD was set
void dlib_close(dlib_ctx_t* lib);

// Resolve public library symbols
void* dlib_resolve(dlib_ctx_t* lib, const char* symbol_name);

// Get symbol from a specific lib, or a global symbol if lib_name == NULL
void* dlib_get_symbol(const char* lib_name, const char* symbol_name);

#endif
