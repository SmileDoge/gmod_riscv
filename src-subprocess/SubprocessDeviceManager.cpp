#include "SubprocessDeviceManager.h"

SubprocessDeviceManager::SubprocessDeviceManager()
{
}

SubprocessDeviceManager::~SubprocessDeviceManager()
{
}

bool SubprocessDeviceManager::RegisterDevice(const char* name, DeviceFactoryFunc factory, DeviceRegisterFunc register_func, DeviceUnregisterFunc unregister_func)
{
	if (name == nullptr || factory == nullptr)
		return false;

	device_factories[std::string(name)] = factory;

	return true;
}

bool SubprocessDeviceManager::HasDevice(const std::string& name) const
{
	if (device_factories.find(name) == device_factories.end())
		return false;

	return true;
}

IDevice* SubprocessDeviceManager::CreateDevice(const std::string& name, uint32_t id, IEmulator* emulator)
{
	if (!HasDevice(name))
		return nullptr;

	IDevice* dev = device_factories.at(name)(id, emulator, DeviceRealm::SUBPROC);

	return dev;
}
