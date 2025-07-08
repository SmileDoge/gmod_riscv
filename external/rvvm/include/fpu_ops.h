/*
fpu_ops.h - FPU Rounding/Exceptions non-fenv fallback
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef LEKKIT_FPU_OPS_H
#define LEKKIT_FPU_OPS_H

#include "compiler.h"

#if CHECK_INCLUDE(fenv.h, 1)
// Target has <fenv.h>
#include <fenv.h>
#endif

#if defined(FE_ALL_EXCEPT)

// Fix rounding misoptimizations even when -frounding-math is not present
#if CLANG_CHECK_VER(12, 0)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif

#else

// Some targets (Emscripten, Windows CE) explicitly lack
// the ability to manipulate host FPU modes.
// These shims are provided to build & run on these targets.
#define HOST_NO_FENV

static forceinline int fpu_dummy_op_internal(void)
{
    return 0;
}

static forceinline int fpu_dummy_op_internal_i(int i)
{
    UNUSED(i);
    return 0;
}

#define feclearexcept(i) fpu_dummy_op_internal_i(i)
#define feraiseexcept(i) fpu_dummy_op_internal_i(i)
#define fetestexcept(i)  fpu_dummy_op_internal_i(i)

#define fegetround()     fpu_dummy_op_internal()
#define fesetround(i)    fpu_dummy_op_internal_i(i)

#endif

#ifndef FE_DIVBYZERO
#define FE_DIVBYZERO 0
#endif

#ifndef FE_INEXACT
#define FE_INEXACT 0
#endif

#ifndef FE_INVALID
#define FE_INVALID 0
#endif

#ifndef FE_OVERFLOW
#define FE_OVERFLOW 0
#endif

#ifndef FE_UNDERFLOW
#define FE_UNDERFLOW 0
#endif

#ifndef FE_ALL_EXCEPT
#define FE_ALL_EXCEPT 0
#endif

#ifndef FE_DOWNWARD
#define FE_DOWNWARD 0
#endif

#ifndef FE_TONEAREST
#define FE_TONEAREST 0
#endif

#ifndef FE_TOWARDZERO
#define FE_TOWARDZERO 0
#endif

#ifndef FE_UPWARD
#define FE_UPWARD 0
#endif

#ifndef FE_DFL_ENV
#define FE_DFL_ENV 0
#endif

#endif
