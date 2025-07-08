#pragma once

#include <GarrysMod/Lua/LuaBase.h>

extern "C"
{
#include <rvvmlib.h>
}

#define RV_EXPORT extern "C" __declspec(dllexport)

typedef const char* (*DeviceGetNameFunc)(void);
typedef int (*DeviceGetVersionFunc)(void);

typedef void (*DeviceInitFunc)(GarrysMod::Lua::ILuaBase*);
typedef void (*DeviceRegisterFunc)(GarrysMod::Lua::ILuaBase*);
typedef void (*DeviceCloseFunc)(GarrysMod::Lua::ILuaBase*);

typedef struct device_info_t
{
	const char* name;
	int version;
} device_info_t;