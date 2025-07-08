#pragma once

#include <rvvmlib.h>
#include <GarrysMod/Lua/Interface.h>

typedef struct mmio_atomic_t mmio_atomic_t;

typedef struct mmio_atomic_params_t
{
	size_t size;
} mmio_atomic_params_t;

mmio_atomic_t* mmio_atomic_init(rvvm_machine_t* machine, mmio_atomic_params_t params);

void mmio_atomic_read(mmio_atomic_t* dev, void* data, size_t offset, uint8_t size);
void mmio_atomic_write(mmio_atomic_t* dev, void* data, size_t offset, uint8_t size);

void mmio_atomic_set_use_atomic(mmio_atomic_t* dev, bool use_atomic);
bool mmio_atomic_get_use_atomic(mmio_atomic_t* dev);

void mmio_atomic_mutex_lock(mmio_atomic_t* dev);
void mmio_atomic_mutex_unlock(mmio_atomic_t* dev);

const char* mmio_atomic_get_name();
int mmio_atomic_get_version();

void mmio_atomic_init_lua(GarrysMod::Lua::ILuaBase* LUA);
void mmio_atomic_register_functions(GarrysMod::Lua::ILuaBase* LUA);
void mmio_atomic_close(GarrysMod::Lua::ILuaBase* LUA);

/*
RV_EXPORT const char* device_get_name();
RV_EXPORT int device_get_version();

RV_EXPORT void device_init(GarrysMod::Lua::ILuaBase* LUA);
RV_EXPORT void device_register_functions(GarrysMod::Lua::ILuaBase* LUA);
RV_EXPORT void device_close(GarrysMod::Lua::ILuaBase* LUA);
*/