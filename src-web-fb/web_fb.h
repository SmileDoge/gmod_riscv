#pragma once

#include <device.h>
#include <rvvmlib.h>

RV_EXPORT const char* device_get_name();
RV_EXPORT int device_get_version();

RV_EXPORT void device_init(GarrysMod::Lua::ILuaBase* LUA);
RV_EXPORT void device_register_functions(GarrysMod::Lua::ILuaBase* LUA);
RV_EXPORT void device_close(GarrysMod::Lua::ILuaBase* LUA);

typedef struct web_fb_t web_fb_t;

web_fb_t* web_fb_init(rvvm_machine_t* machine, size_t addr, int width, int height, uint16_t port);