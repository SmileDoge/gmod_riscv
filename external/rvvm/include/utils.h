/*
utils.h - Util functions
Copyright (C) 2021  LekKit <github.com/LekKit>
                    0xCatPKG <0xCatPKG@rvvm.dev>
                    0xCatPKG <github.com/0xCatPKG>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_UTILS_H
#define RVVM_UTILS_H

#include "compiler.h"
#include "atomics.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * String & numeric helpers
 */

// Evaluate max/min value
#define EVAL_MAX(a, b) ((a) > (b) ? (a) : (b))
#define EVAL_MIN(a, b) ((a) < (b) ? (a) : (b))

// Compute length of a static array
#define STATIC_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*(arr)))

// Align size up (To power of two!)
static inline size_t align_size_up(size_t x, size_t align)
{
    return (x + (align - 1)) & ~(align - 1);
}

// Align size down (To power of two!)
static inline size_t align_size_down(size_t x, size_t align)
{
    return x & ~(align - 1);
}

// Portable strtol/ltostr replacement
size_t   uint_to_str_base(char* str, size_t size, uint64_t val, uint8_t base);
uint64_t str_to_uint_base(const char* str, size_t* len, uint8_t base);
size_t   int_to_str_base(char* str, size_t size, int64_t val, uint8_t base);
int64_t  str_to_int_base(const char* str, size_t* len, uint8_t base);
size_t   int_to_str_dec(char* str, size_t size, int64_t val);
int64_t  str_to_int_dec(const char* str);

// Portable & safer string.h replacement
size_t      rvvm_strlen(const char* string);
size_t      rvvm_strnlen(const char* string, size_t size);
bool        rvvm_strcmp(const char* s1, const char* s2);
size_t      rvvm_strlcpy(char* dst, const char* src, size_t size);
const char* rvvm_strfind(const char* string, const char* pattern);

static inline size_t mem_suffix_shift(char suffix)
{
    switch (suffix) {
        case 'k': return 10;
        case 'K': return 10;
        case 'm': return 20;
        case 'M': return 20;
        case 'g': return 30;
        case 'G': return 30;
        default: return 0;
    }
}

// Generate random bytes
void rvvm_randombytes(void* buffer, size_t size);

// Generate random serial number (0-9, A-Z)
void rvvm_randomserial(char* serial, size_t size);

/*
 * Safe memory allocation
 */

#if GNU_ATTRIBUTE(__returns_nonnull__) && GNU_ATTRIBUTE(__warn_unused_result__) && GNU_ATTRIBUTE(__alloc_size__)
#define SAFE_MALLOC  __attribute__((__returns_nonnull__, __warn_unused_result__, __malloc__, __alloc_size__(1)))
#define SAFE_CALLOC  __attribute__((__returns_nonnull__, __warn_unused_result__, __malloc__, __alloc_size__(1, 2)))
#define SAFE_REALLOC __attribute__((__returns_nonnull__, __warn_unused_result__, __alloc_size__(2)))
#else
#define SAFE_MALLOC  GNU_DUMMY_ATTRIBUTE
#define SAFE_CALLOC  GNU_DUMMY_ATTRIBUTE
#define SAFE_REALLOC GNU_DUMMY_ATTRIBUTE
#endif

// These never return NULL
SAFE_MALLOC  void* safe_malloc(size_t size);
SAFE_CALLOC  void* safe_calloc(size_t size, size_t n);
SAFE_REALLOC void* safe_realloc(void* ptr, size_t size);

// Safe object allocation with type checking & zeroing
#define safe_new_arr(type, size) ((type*)safe_calloc(size, sizeof(type)))
#define safe_new_obj(type) safe_new_arr(type, 1)

#define default_free(ptr) (free)(ptr)

#define safe_free(ptr) \
do { \
    default_free(ptr); \
    ptr = NULL; \
} while(0)

// Implicitly NULL freed pointer to prevent use-after-free
#define free(ptr) safe_free(ptr)

/*
 * Command line & config parsing
 */

// Set command line arguments
void rvvm_set_args(int argc, char** argv);

// Load config file
bool rvvm_load_config(const char* path);

// Iterate over arguments in form of <-arg> [val], or <val> ("" is returned in such case)
const char* rvvm_next_arg(const char** val, int* iter);

// Check if argument is present on the command line or in the config
bool        rvvm_has_arg(const char* arg);

// Get argument value
const char* rvvm_getarg(const char* arg);
bool        rvvm_getarg_bool(const char* arg);
int         rvvm_getarg_int(const char* arg);
uint64_t    rvvm_getarg_size(const char* arg);

/*
 * Logger
 */

#define LOG_NONE  0
#define LOG_ERROR 1
#define LOG_WARN  2
#define LOG_INFO  3

void rvvm_set_loglevel(int loglevel);

#if GNU_ATTRIBUTE(__format__)
#define PRINT_FORMAT __attribute__((__format__(printf, 1, 2)))
#else
#define PRINT_FORMAT GNU_DUMMY_ATTRIBUTE
#endif

#ifdef USE_DEBUG
// Debug logger enabled at build time
PRINT_FORMAT void rvvm_debug(const char* format_str, ...);

#else
// Debug logs optimized out, but still performs compile-time format / unused arguments checking
static PRINT_FORMAT forceinline void rvvm_debug(const char* format_str, ...)
{
    UNUSED(format_str);
}

#endif

// Logging functions (controlled by loglevel)
PRINT_FORMAT void rvvm_info(const char* format_str, ...);
PRINT_FORMAT void rvvm_warn(const char* format_str, ...);
PRINT_FORMAT void rvvm_error(const char* format_str, ...);
PRINT_FORMAT void rvvm_fatal(const char* format_str, ...); // Aborts the process

/*
 * Initialization/deinitialization
 */

slow_path void do_once_finalize(uint32_t* ticket);

/*
 * Run scoped statement once per library lifetime (In a thread-safe way)
 *
 * DO_ONCE_SCOPED {
 *     if (!feature_avail()) {
 *         break;
 *     }
 *
 *     global_feature_init();
 * }
 */
#define DO_ONCE_SCOPED                                                                                                 \
    static uint32_t MACRO_IDENT(do_once_ticket) = 0;                                  /**/                             \
    POST_STMT_NAMED (unlikely(atomic_load_uint32(&MACRO_IDENT(do_once_ticket)) != 2), /**/                             \
                     do_once_finalize(&MACRO_IDENT(do_once_ticket)), ticket_stmt)     /**/                             \
        POST_STMT_NAMED (atomic_cas_uint32(&MACRO_IDENT(do_once_ticket), 0, 1),       /**/                             \
                         atomic_store_uint32(&MACRO_IDENT(do_once_ticket), 2), claim_stmt)

/*
 * Run an expression once per library lifetime (In a thread-safe way)
 *
 * DO_ONCE(rvvm_warn("This will only be printed once upon reaching!"));
 */
#define DO_ONCE(expr_once) \
do { \
    DO_ONCE_SCOPED { \
        expr_once; \
    } \
} while (0)

// Register a callback to be ran at deinitialization (exit or library unload)
void call_at_deinit(void (*function)(void));

// Perform manual deinitialization
GNU_DESTRUCTOR void full_deinit(void);

#endif
