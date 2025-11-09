#include "GmodDeviceManager.h"

GmodDeviceManager::GmodDeviceManager()
{
}

GmodDeviceManager::~GmodDeviceManager()
{
}

bool GmodDeviceManager::RegisterDevice(const char* name, DeviceFactoryFunc factory, DeviceRegisterFunc register_func, DeviceUnregisterFunc unregister_func)
{
	if (name == nullptr || factory == nullptr)
		return false;

	RegisteredDevice reg_dev;

	reg_dev.name = name;
	reg_dev.factory = factory;
	reg_dev.register_func = register_func;
	reg_dev.unregister_func = unregister_func;
	reg_dev.metatable_index = 0;

	device_factories[std::string(name)] = reg_dev;

	return true;
}

bool GmodDeviceManager::HasDevice(const std::string& name) const
{
	if (device_factories.find(name) == device_factories.end())
		return false;

	return true;
}

IDevice* GmodDeviceManager::CreateDevice(const std::string& name, uint32_t id, IEmulator* emulator)
{
	if (!HasDevice(name))
		return nullptr;

	IDevice* dev = device_factories.at(name).factory(id, emulator, DeviceRealm::GMOD);

	return dev;
}

int GmodDeviceManager::GetMetatableIndex(const std::string& name) const
{
	if (!HasDevice(name))
		return 0;

	return device_factories.at(name).metatable_index;
}

void GmodDeviceManager::NotifyRegisterDevices(const std::string& name, IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA)
{
	if (!HasDevice(name))
		return;

	RegisteredDevice& reg_dev = device_factories.at(name);

	if (reg_dev.register_func)
		reg_dev.metatable_index = reg_dev.register_func(emulator, LUA);
}

void GmodDeviceManager::NotifyUnregisterDevices(const std::string& name, IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA)
{
	if (!HasDevice(name))
		return;

	RegisteredDevice& reg_dev = device_factories.at(name);

	if (reg_dev.unregister_func)
		reg_dev.unregister_func(emulator, LUA);
}

void GmodDeviceManager::NotifyAllRegisterDevices(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA)
{
	for (const auto& [name, factory] : device_factories)
		NotifyRegisterDevices(name, emulator, LUA);
}

void GmodDeviceManager::NotifyAllUnregisterDevices(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA)
{
	for (const auto& [name, factory] : device_factories)
		NotifyUnregisterDevices(name, emulator, LUA);
}
