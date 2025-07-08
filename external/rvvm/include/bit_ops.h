/*
bit_ops.h - Bit operations
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef LEKKIT_BIT_OPS_H
#define LEKKIT_BIT_OPS_H

#include "compiler.h"
#include "rvvm_types.h"

#if defined(_MSC_VER)
#include <intrin.h> // For __mulh(), __umulh()
#include <stdlib.h> // For _byteswap_ulong(), _byteswap_uint64()
#endif

/*
 * Simple bit operations (sign-extend, mask, bit check/set)
 */

/*
 * Sign-extend bits in the lower part of val into signed i64
 * Usage:
 *     int ext = sign_extend(val, 20);
 *
 *     [ext is now equal to signed lower 20 bits of val]
 */
static forceinline int64_t sign_extend(uint64_t val, bitcnt_t bits)
{
    return ((int64_t)(val << (64 - bits))) >> (64 - bits);
}

// Generate bitmask of given bitcount
static forceinline uint64_t bit_mask(bitcnt_t count)
{
    return (1ULL << count) - 1;
}

// Cut bits from val at given position (from lower bit)
static forceinline uint64_t bit_cut(uint64_t val, bitcnt_t pos, bitcnt_t bits)
{
    return (val >> pos) & bit_mask(bits);
}

// Replace bits in val at given position (from lower bit) by rep
static inline uint64_t bit_replace(uint64_t val, bitcnt_t pos, bitcnt_t bits, uint64_t rep)
{
    return (val & (~(bit_mask(bits) << pos))) | ((rep & bit_mask(bits)) << pos);
}

// Check if Nth bit of a value is set
static forceinline bool bit_check(uint64_t val, bitcnt_t pos)
{
    return (val >> pos) & 0x1;
}

// Return a bitmask with Nth bit set, clamp to 32 bits
static forceinline uint32_t bit_set32(bitcnt_t pos)
{
    return (1U << (pos & 31));
}

// Return a bitmask with Nth bit set, clamp to 64 bits
static forceinline uint64_t bit_set64(bitcnt_t pos)
{
    return (1ULL << (pos & 63));
}

/*
 * Bit rotations
 */

// Rotate u32 left
static forceinline uint32_t bit_rotl32(uint32_t val, bitcnt_t bits)
{
    return (val << (bits & 0x1F)) | (val >> ((32 - bits) & 0x1F));
}

// Rotate u64 left
static forceinline uint64_t bit_rotl64(uint64_t val, bitcnt_t bits)
{
    return (val << (bits & 0x3F)) | (val >> ((64 - bits) & 0x3F));
}

// Rotate u32 right
static forceinline uint32_t bit_rotr32(uint32_t val, bitcnt_t bits)
{
    return (val >> (bits & 0x1F)) | (val << ((32 - bits) & 0x1F));
}

// Rotate u64 right
static forceinline uint64_t bit_rotr64(uint64_t val, bitcnt_t bits)
{
    return (val >> (bits & 0x3F)) | (val << ((64 - bits) & 0x3F));
}

/*
 * Accelerated bit operations which codegen into specialized instructions
 */

// Count leading zeroes (from highest bit position) in u32
static inline bitcnt_t bit_clz32(uint32_t val)
{
    if (unlikely(!val)) {
        return 32;
    }
#if GCC_CHECK_VER(3, 4) || GNU_BUILTIN(__builtin_clz)
    return __builtin_clz(val);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse(&index, val);
    return 31 ^ index;
#else
    // de Brujin hashmap, see https://en.wikipedia.org/wiki/De_Bruijn_sequence
    const uint8_t lut[32] = {
        0, 31, 4,  30, 3,  18, 8,  29, 2, 10, 12, 17, 7,  15, 28, 24,
        1, 5,  19, 9,  11, 13, 16, 25, 6, 20, 14, 26, 21, 27, 22, 23,
    };
    val |= (val >> 1);
    val |= (val >> 2);
    val |= (val >> 4);
    val |= (val >> 8);
    val |= (val >> 16);
    return lut[((val + 1) * 0x077CB531U) >> 27];
#endif
}

// Count leading zeroes (from highest bit position) in u64
static inline bitcnt_t bit_clz64(uint64_t val)
{
    if (unlikely(!val)) {
        return 64;
    }
#if defined(HOST_64BIT) && (GCC_CHECK_VER(3, 4) || GNU_BUILTIN(__builtin_clzll))
    return __builtin_clzll(val);
#elif defined(HOST_64BIT) && defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse64(&index, val);
    return 63 ^ index;
#else
    return (val >> 32) ? bit_clz32(val >> 32) : (32 + bit_clz32(val));
#endif
}

// Count trailing zeroes (Least significant set bit position) in u32
static inline bitcnt_t bit_ctz32(uint32_t val)
{
    if (unlikely(!val)) {
        return 32;
    }
#if GCC_CHECK_VER(3, 4) || GNU_BUILTIN(__builtin_ctz)
    return __builtin_ctz(val);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanForward(&index, val);
    return index;
#else
    // de Brujin hashmap, see https://en.wikipedia.org/wiki/De_Bruijn_sequence
    const uint8_t lut[32] = {
        0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9,
    };
    return lut[((val & -val) * 0x077CB531U) >> 27];
#endif
}

// Count trailing zeroes (Least significant set bit position) in u64
static inline bitcnt_t bit_ctz64(uint64_t val)
{
    if (unlikely(!val)) {
        return 64;
    }
#if defined(HOST_64BIT) && (GCC_CHECK_VER(3, 4) || GNU_BUILTIN(__builtin_ctzll))
    return __builtin_ctzll(val);
#elif defined(HOST_64BIT) && defined(_MSC_VER)
    unsigned long index;
    _BitScanForward64(&index, val);
    return index;
#else
    bitcnt_t tmp = (!((uint32_t)val)) << 5;
    return bit_ctz32(val >> tmp) + tmp;
#endif
}

// Get most significant set bit position in u32
static inline bitcnt_t bit_bsr32(uint32_t val)
{
    if (unlikely(!val)) {
        return 32;
    }
#if GCC_CHECK_VER(3, 4) || GNU_BUILTIN(__builtin_clz)
    return 31 ^ __builtin_clz(val);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse(&index, val);
    return index;
#else
    return 31 ^ bit_clz32(val);
#endif
}

// Get most significant set bit position in u64
static inline bitcnt_t bit_bsr64(uint64_t val)
{
    if (unlikely(!val)) {
        return 64;
    }
#if defined(HOST_64BIT) && (GCC_CHECK_VER(3, 4) || GNU_BUILTIN(__builtin_clzll))
    return 63 ^ __builtin_clzll(val);
#elif defined(HOST_64BIT) && defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse64(&index, val);
    return index;
#else
    return 63 ^ bit_clz64(val);
#endif
}

// Normalize the value to nearest next power of two
static inline uint64_t bit_next_pow2(uint64_t val)
{
    // Fast path for proper pow2 values
    if (likely(!(val & (val - 1)))) {
        return val;
    }
#if defined(HOST_64BIT) && (GCC_CHECK_VER(3, 4) || GNU_BUILTIN(__builtin_clzll) || defined(_MSC_VER))
    // Computing this on zero or pow2 would be invalid,
    // but the fast path check already covers this
    return (1ULL << bit_bsr64(val));
#else
    // Bit twiddling hacks
    val -= 1;
    val |= (val >> 1);
    val |= (val >> 2);
    val |= (val >> 4);
    val |= (val >> 8);
    val |= (val >> 16);
    val |= (val >> 32);
    return val + 1;
#endif
}

// Count raised bits in u32
static inline bitcnt_t bit_popcnt32(uint32_t val)
{
#if GCC_CHECK_VER(3, 4) || GNU_BUILTIN(__builtin_popcount)
    return __builtin_popcount(val);
#else
    val -= (val >> 1) & 0x55555555;
    val  = (val & 0x33333333) + ((val >> 2) & 0x33333333);
    val  = (val + (val >> 4)) & 0x0F0F0F0F;
    val += val >> 8;
    return (val + (val >> 16)) & 0x3F;
#endif
}

// Count raised bits in u64
static inline bitcnt_t bit_popcnt64(uint64_t val)
{
#if defined(HOST_64BIT) && (GCC_CHECK_VER(3, 4) || GNU_BUILTIN(__builtin_popcountll))
    return __builtin_popcountll(val);
#else
    return bit_popcnt32(val) + bit_popcnt32(val >> 32);
#endif
}

// Bitwise OR-combine, byte granule for orc.b instruction emulation
static inline uint64_t bit_orc_b(uint64_t val)
{
#if defined(GNU_EXTS) && defined(__x86_64__)
    uint64_t tmp = 0;
    __asm__("pcmpeqb %1, %0\n\t"
            "pcmpeqb %1, %0\n\t"
            : "+x"(val)
            : "x"(tmp));
#elif defined(GNU_EXTS) && defined(__aarch64__)
    __asm__("cmtst %0.8b, %0.8b, %0.8b" : "+w"(val));
#else
    const uint64_t bytes_hi = 0x8080808080808080ULL;
    const uint64_t bytes_lo = 0x0101010101010101ULL;
    // Only non-zero bytes will hold 0x80 pattern afterwards
    val = (((val | bytes_hi) - bytes_lo) | val) & bytes_hi;
    // Spill 0x80 pattern into 0xFF via shift-subtract
    val >>= 7;
    return (val << 8) - val;
#endif
    return val;
}

/*
 * clmul using SSE intrins for future investigation:
    __m128i a_vec = _mm_cvtsi64_si128(a);
    __m128i b_vec = _mm_cvtsi64_si128(b);
    return _mm_cvtsi128_si64(_mm_clmulepi64_si128(a_vec, b_vec, 0));
 *
 * clmulh using SSE intrins:
    __m128i a_vec = _mm_cvtsi64_si128(a);
    __m128i b_vec = _mm_cvtsi64_si128(b);
    return _mm_extract_epi64(_mm_clmulepi64_si128(a_vec, b_vec, 0), 1);
 */

// Carry-less multiply
static inline uint32_t bit_clmul32(uint32_t a, uint32_t b)
{
    uint32_t ret = 0;
    do {
        if (b & 1) {
            ret ^= a;
        }
        b >>= 1;
    } while ((a <<= 1));
    return ret;
}

static inline uint64_t bit_clmul64(uint64_t a, uint64_t b)
{
    uint64_t ret = 0;
    do {
        if (b & 1) {
            ret ^= a;
        }
        b >>= 1;
    } while ((a <<= 1));
    return ret;
}

static inline uint32_t bit_clmulh32(uint32_t a, uint32_t b)
{
    uint32_t ret = 0;
    bitcnt_t i   = 31;
    do {
        b >>= 1;
        if (b & 1) {
            ret ^= (a >> i);
        }
        i--;
    } while (b);
    return ret;
}

static inline uint64_t bit_clmulh64(uint64_t a, uint64_t b)
{
    uint64_t ret = 0;
    bitcnt_t i   = 63;
    do {
        b >>= 1;
        if (b & 1) {
            ret ^= (a >> i);
        }
        i--;
    } while (b);
    return ret;
}

static inline uint32_t bit_clmulr32(uint32_t a, uint32_t b)
{
    uint32_t ret = 0;
    bitcnt_t i   = 31;
    do {
        if (b & 1) {
            ret ^= (a >> i);
        }
        b >>= 1;
        i--;
    } while (b);
    return ret;
}

static inline uint64_t bit_clmulr64(uint64_t a, uint64_t b)
{
    uint64_t ret = 0;
    bitcnt_t i   = 63;
    do {
        if (b & 1) {
            ret ^= (a >> i);
        }
        b >>= 1;
        i--;
    } while (b);
    return ret;
}

// Bswap 32-bit value (From BE to LE or vice versa)
static inline uint32_t byteswap_uint32(uint32_t val)
{
#if GCC_CHECK_VER(4, 4) || GNU_BUILTIN(__builtin_bswap32)
    return __builtin_bswap32(val);
#elif defined(_MSC_VER)
    return _byteswap_ulong(val);
#else
    return ((val & 0xFF000000) >> 24) //
         | ((val & 0x00FF0000) >> 8)  //
         | ((val & 0x0000FF00) << 8)  //
         | ((val & 0x000000FF) << 24);
#endif
}

// Bswap 64-bit value (From BE to LE or vice versa)
static inline uint64_t byteswap_uint64(uint64_t val)
{
#if GCC_CHECK_VER(4, 4) || GNU_BUILTIN(__builtin_bswap64)
    return __builtin_bswap64(val);
#elif defined(_MSC_VER)
    return _byteswap_uint64(val);
#else
    val = ((val >> 8) & 0x00FF00FF00FF00FF) | ((val & 0x00FF00FF00FF00FF) << 8);
    val = ((val >> 16) & 0x0000FFFF0000FFFF) | ((val & 0x0000FFFF0000FFFF) << 16);
    val = ((val >> 32) & 0x00000000FFFFFFFF) | ((val & 0x00000000FFFFFFFF) << 32);
    return val;
#endif
}

// Get high 64 bits from signed i64 x i64 -> 128 bit multiplication
static inline uint64_t mulh_uint64(int64_t a, int64_t b)
{
#ifdef INT128_SUPPORT
    return ((int128_t)a * (int128_t)b) >> 64;
#elif defined(HOST_64BIT) && defined(_MSC_VER)
    return __mulh(a, b);
#else
    int64_t lo_lo = (a & 0xFFFFFFFF) * (b & 0xFFFFFFFF);
    int64_t hi_lo = (a >> 32) * (b & 0xFFFFFFFF);
    int64_t lo_hi = (a & 0xFFFFFFFF) * (b >> 32);
    int64_t hi_hi = (a >> 32) * (b >> 32);
    int64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    return (hi_lo >> 32) + (cross >> 32) + hi_hi;
#endif
}

// Get high 64 bits from unsigned u64 x u64 -> 128 bit multiplication
static inline uint64_t mulhu_uint64(uint64_t a, uint64_t b)
{
#ifdef INT128_SUPPORT
    return ((uint128_t)a * (uint128_t)b) >> 64;
#elif defined(HOST_64BIT) && defined(_MSC_VER)
    return __umulh(a, b);
#else
    uint64_t lo_lo = (a & 0xFFFFFFFF) * (b & 0xFFFFFFFF);
    uint64_t hi_lo = (a >> 32) * (b & 0xFFFFFFFF);
    uint64_t lo_hi = (a & 0xFFFFFFFF) * (b >> 32);
    uint64_t hi_hi = (a >> 32) * (b >> 32);
    uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    return (hi_lo >> 32) + (cross >> 32) + hi_hi;
#endif
}

// Get high 64 bits from signed * unsigned i64 x u64 -> 128 bit multiplication
static inline uint64_t mulhsu_uint64(int64_t a, uint64_t b)
{
#if defined(INT128_SUPPORT) || (defined(HOST_64BIT) && defined(_MSC_VER))
    return mulhu_uint64(a, b) - (a >= 0 ? 0 : b);
#else
    int64_t  lo_lo = (a & 0xFFFFFFFF) * (b & 0xFFFFFFFF);
    int64_t  hi_lo = (a >> 32) * (b & 0xFFFFFFFF);
    int64_t  lo_hi = (a & 0xFFFFFFFF) * (b >> 32);
    int64_t  hi_hi = (a >> 32) * (b >> 32);
    uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    return (hi_lo >> 32) + (cross >> 32) + hi_hi;
#endif
}

#endif
