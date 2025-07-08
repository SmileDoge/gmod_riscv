#pragma once

#include <device.h>
#include <rvvmlib.h>

RV_EXPORT const char* device_get_name();
RV_EXPORT int device_get_version();

RV_EXPORT void device_init(GarrysMod::Lua::ILuaBase* LUA);
RV_EXPORT void device_register_functions(GarrysMod::Lua::ILuaBase* LUA);
RV_EXPORT void device_close(GarrysMod::Lua::ILuaBase* LUA);

typedef struct simple_uart_t simple_uart_t;

simple_uart_t* simple_uart_init(rvvm_machine_t* machine, size_t addr, size_t size, bool add_chosen = false);

void simple_uart_write_buf(simple_uart_t* uart, const char* buf, size_t size);
size_t simple_uart_read_buf(simple_uart_t* uart, char* buf, size_t size);

rvvm_mmio_dev_t* simple_uart_get_mmio_dev(simple_uart_t* uart);