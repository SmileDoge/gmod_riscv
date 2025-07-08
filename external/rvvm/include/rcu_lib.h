/*
rcu_lib.h - Read-copy-update (RCU)
Copyright (C) 2024  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_RCU_LIB_H
#define RVVM_RCU_LIB_H

#include "atomics.h"

#if defined(GNU_EXTS)
#define RCU_THREAD_LOCAL __thread
#elif defined(_MSC_VER)
#define RCU_THREAD_LOCAL __declspec(thread)
#else
// No thread locals available, fallback to shared reader state
#define RCU_THREAD_LOCAL
#define RCU_DUMMY_SHARED_STATE
#endif

#define RCU_INTERNAL_STATE_GEN 0x80000000U

#define RCU_INTERNAL_STATE_READERS(state) ((uint16_t)(state))

extern RCU_THREAD_LOCAL uint32_t rcu_internal_reader_state;

extern uint32_t rcu_internal_writer_waiting;

#if !defined(RCU_DUMMY_SHARED_STATE)

extern uint32_t rcu_internal_writer_gen;

extern uint32_t rcu_has_remote_barriers;

#endif

slow_path void rcu_internal_wake_writer(uint32_t state);

/*
 * RCU API
 */

// Register rcu reader thread, MUST be called before rcu_read_lock()
void rcu_register_thread(void);

// Deregister rcu reader thread, MUST be called before thread termination
void rcu_deregister_thread(void);

// Wait for next rcu grace period, all pointers previously replaced may be freed afterwards
void rcu_synchronize(void);

// Enter rcu reader section
static forceinline void rcu_read_lock(void)
{
    uint32_t* self = &rcu_internal_reader_state;
    atomic_compiler_barrier();
#if defined(RCU_DUMMY_SHARED_STATE)
    atomic_add_uint32(self, 1);
#else
    uint32_t state = atomic_load_uint32_ex(self, ATOMIC_RELAXED);
    if (likely(!RCU_INTERNAL_STATE_READERS(state))) {
        state = atomic_load_uint32_ex(&rcu_internal_writer_gen, ATOMIC_RELAXED);
        atomic_store_uint32_ex(self, state, ATOMIC_RELAXED);
        atomic_compiler_barrier();
        if (unlikely(!atomic_load_uint32_ex(&rcu_has_remote_barriers, ATOMIC_RELAXED))) {
            // Perform a StoreLoad barrier if remote barriers are not supported.
            // This prevents rcu_dereference() reordering before rcu reader section.
            atomic_fence_ex(ATOMIC_SEQ_CST);
        }
    } else {
        atomic_store_uint32_ex(self, state + 1, ATOMIC_RELAXED);
    }
#endif
}

// Exit rcu reader section
static forceinline void rcu_read_unlock(void)
{
    uint32_t* self = &rcu_internal_reader_state;
    atomic_compiler_barrier();
#if defined(RCU_DUMMY_SHARED_STATE)
    uint32_t state = atomic_sub_uint32(self, 1);
#else
    uint32_t state = atomic_load_uint32_ex(self, ATOMIC_RELAXED);

#if defined(__SANITIZE_THREAD__) || defined(__x86_64__)
    atomic_store_uint32_ex(self, state - 1, ATOMIC_RELEASE);
#else
    atomic_compiler_barrier();
    if (unlikely(!atomic_load_uint32_ex(&rcu_has_remote_barriers, ATOMIC_RELAXED))) {
        // Perform a LoadStore barrier if remote barriers are not supported.
        // This prevents rcu_dereference() reordering after rcu reader section.
        atomic_fence_ex(ATOMIC_RELEASE);
    }
    atomic_store_uint32_ex(self, state - 1, ATOMIC_RELAXED);
#endif

#endif

    // Wake up RCU writer thread if needed
    if (unlikely(atomic_load_uint32_ex(&rcu_internal_writer_waiting, ATOMIC_RELAXED))) {
        rcu_internal_wake_writer(state);
    }
}

// Assign a pointer from rcu writer thread
#define rcu_assign_pointer(rcu_ptr, val) atomic_store_pointer(&rcu_ptr, val)

// Swap a pointer from rcu writer thread
#define rcu_swap_pointer(rcu_ptr, val) atomic_swap_pointer(&rcu_ptr, val)

// Dereference a pointer from rcu reader thread
#define rcu_dereference(rcu_ptr) atomic_load_pointer(&rcu_ptr)

#ifdef USE_INFRASTRUCTURE_TESTS

// Run an rcu torture test to check correctness of implementation
void rcu_torture_test(uint32_t reader_threads, uint32_t secs);

#endif

#endif
