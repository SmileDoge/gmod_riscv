/*
rvvm_isolation.h - Process & thread isolation
Copyright (C) 2024  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_ISOLATION_H
#define RVVM_ISOLATION_H

#include "rvvmlib.h"

/*
 * Restrict the calling thread from:
 * - Accessing the filesystem
 * - Accessing PID namespace, killing processes
 * - Accessing IPC namespace
 * - Forking, executing new programs
 *
 * Additionally, all capabilities of the calling are dropped,
 * and suid privilege escalation is no longer possible.
 *
 * This is expected to be applied to all RVVM-owned threads
 * (vCPU, threadpool, event dispatch thread) without affecting
 * the process as a whole.
 */
void rvvm_restrict_this_thread(void);

/*
 * Apply same restrictions as cap_restrict_this_thread() to the whole process.
 *
 * Additionally, drop to nobody if we're root.
 *
 * NOTE: We can't implicitly enforce this in librvvm as we never know
 * when it's safe to do so. It's up to the API user to decide.
 */
PUBLIC void rvvm_restrict_process(void);

/*
 * Possible TODO for further librvvm isolation: Implement process-wide filesystem restrictions
 * - Read-only access to /etc, /usr, ... etc system dirs
 * - Read-only access to any hidden .file in $HOME (Prevent messing with .bashrc, .profile etc)
 * - Read-only access to ~/.local/bin, ~/.local/lib
 * - No access to ~/.gnupg, ~/.ssh, ~/.pki, other critical user data like crypto wallets, browser profiles and such
 *
 * This in theory could be applied to any process which uses librvvm, with an opt-out mechanism
 *
 * Easily doable through OpenBSD pledge, however Linux Landlock is per-thread only,
 * which significantly complicates the implementation.
 */

#endif
