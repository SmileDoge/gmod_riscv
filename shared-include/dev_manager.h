#pragma once

#include <device.h>

#include <vector>
#include <string>

#ifdef GMOD_RISCV_EXPORTS
#define GMOD_API __declspec(dllexport)
#else
#define GMOD_API __declspec(dllimport)
#endif

GMOD_API void dev_manager_init(GarrysMod::Lua::ILuaBase* LUA);
GMOD_API void dev_manager_close(GarrysMod::Lua::ILuaBase* LUA);

GMOD_API std::vector<device_info_t> dev_manager_get_devices();
GMOD_API bool dev_manager_get_device(const std::string& name, device_info_t* out_info);

GMOD_API bool dev_manager_register_device(
	DeviceGetNameFunc get_name_func, 
	DeviceGetVersionFunc get_version_func, 
	DeviceInitFunc init_func, 
	DeviceRegisterFunc reg_func, 
	DeviceCloseFunc close_func);

GMOD_API bool dev_manager_load_device(
	const std::string& file_name,
	std::string& out_name
	);

GMOD_API bool dev_manager_unload_device(const std::string& name);

GMOD_API int dev_manager_lua_nop_func(lua_State* L);