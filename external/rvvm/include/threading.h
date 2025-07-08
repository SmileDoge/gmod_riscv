/*
threading.h - Threading, Futexes, Conditional variables, Threadpool
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef LEKKIT_THREADING_H
#define LEKKIT_THREADING_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Threads
 */

typedef struct thread_ctx thread_ctx_t;

typedef void* (*thread_func_t)(void*);

thread_ctx_t* thread_create_ex(thread_func_t func, void* arg, uint32_t stack_size);
thread_ctx_t* thread_create(thread_func_t func, void* arg);
bool          thread_join(thread_ctx_t* thread);
bool          thread_detach(thread_ctx_t* thread); // NOTE: Detaching is not safe in libraries

/*
 * Futexes (Possibly emulated)
 *
 * Work only within current process bounds.
 * Please prefer a conditional variable if you value precise timeout timing.
 */

#define THREAD_FUTEX_INFINITE ((uint64_t)-1)

bool thread_futex_wait(void* ptr, uint32_t val, uint64_t timeout_ns);
void thread_futex_wake(void* ptr, uint32_t num);

/*
 * Conditional variables (More like events)
 */

// No condvar_wait() timeout
#define CONDVAR_INFINITE ((uint64_t)-1)

typedef struct cond_var cond_var_t;

cond_var_t* condvar_create(void);
bool        condvar_wait_ns(cond_var_t* cond, uint64_t timeout_ns);
bool        condvar_wait(cond_var_t* cond, uint64_t timeout_ms);
bool        condvar_wake(cond_var_t* cond);
bool        condvar_wake_all(cond_var_t* cond);
uint32_t    condvar_waiters(cond_var_t* cond);
void        condvar_free(cond_var_t* cond);

/*
 * Shared threadpool
 */

#define THREAD_MAX_VA_ARGS 8

typedef void* (*thread_func_va_t)(void**);

// Execute task in threadpool
void thread_create_task(thread_func_t func, void* arg);
void thread_create_task_va(thread_func_va_t func, void** args, unsigned arg_count);

#endif
