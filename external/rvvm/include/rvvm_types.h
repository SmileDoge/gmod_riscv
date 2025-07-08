/*
rvvm_types.c - RVVM integer types
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_TYPES_H
#define RVVM_TYPES_H

#include "compiler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if CHECK_INCLUDE(inttypes.h, 1)
#include <inttypes.h>
#endif

// Fix for MSVCRT printf specifier on MinGW
#if defined(_WIN32)
#undef PRIx32
#define PRIx32 "I32x"

#undef PRIx64
#define PRIx64 "I64x"

#undef PRId32
#define PRId32 "I32d"

#undef PRId64
#define PRId64 "I64d"

#undef PRIu32
#define PRIu32 "I32u"

#undef PRIu64
#define PRIu64 "I64u"
#endif

#ifndef PRIx32
#define PRIx32 "x"
#endif

#ifndef PRIx64
#define PRIx64 "llx"
#endif

#ifndef PRId32
#define PRId32 "d"
#endif

#ifndef PRId64
#define PRId64 "lld"
#endif

#ifndef PRIu32
#define PRIu32 "u"
#endif

#ifndef PRIu64
#define PRIu64 "llu"
#endif

#ifdef __SIZEOF_INT128__
#define INT128_SUPPORT 1
typedef unsigned __int128 uint128_t;
typedef __int128          int128_t;
#endif

typedef uint8_t regid_t;  // Register index
typedef uint8_t bitcnt_t; // Bits count

#endif
