/*
gdbstub.h - GDB Debugging Stub
Copyright (C) 2024  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_GDBSTUB_H
#define RVVM_GDBSTUB_H

#include "rvvmlib.h"

typedef struct gdb_server gdb_server_t;

bool gdbstub_init(rvvm_machine_t* machine, const char* bind);

bool gdbstub_halt(gdb_server_t* server);

#endif
