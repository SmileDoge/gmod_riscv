#pragma once

#include "IDeviceManager.h"

#include <map>

#include <string>

typedef struct
{
	std::string name;

	DeviceFactoryFunc factory;

	DeviceRegisterFunc register_func;
	DeviceUnregisterFunc unregister_func;

	int metatable_index;
} RegisteredDevice;

class GmodDeviceManager : public IDeviceManager
{
public:
	GmodDeviceManager();
	~GmodDeviceManager() override;

	virtual bool RegisterDevice(const char* name, DeviceFactoryFunc factory, DeviceRegisterFunc register_func, DeviceUnregisterFunc unregister_func) override;

	bool HasDevice(const std::string& name) const;

	IDevice* CreateDevice(const std::string& name, uint32_t id, IEmulator* emulator);
	int GetMetatableIndex(const std::string& name) const;

	void NotifyRegisterDevices(const std::string& name, IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA);
	void NotifyUnregisterDevices(const std::string& name, IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA);

	void NotifyAllRegisterDevices(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA);
	void NotifyAllUnregisterDevices(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA);

	static uint32_t GetNextUniqueID()
	{
		static uint32_t current_id = 0;
		return current_id++;
	}

private:
	std::map<std::string, RegisteredDevice> device_factories;
};