#pragma once

#include <stdint.h>

#include "IEmulator.h"
#include "IDevice.h"

#include <GarrysMod/Lua/LuaBase.h>

typedef IDevice* (*DeviceFactoryFunc)(uint32_t id, IEmulator* emulator, DeviceRealm realm);

// return metatable index
typedef int (*DeviceRegisterFunc)(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA);
typedef void (*DeviceUnregisterFunc)(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA);

class IDeviceManager
{
public:
	virtual ~IDeviceManager() = default;

	virtual bool RegisterDevice(const char* name, DeviceFactoryFunc factory, DeviceRegisterFunc register_func, DeviceUnregisterFunc unregister_func) = 0;
};