#pragma once

#include "IDeviceManager.h"

#include <map>

#include <string>

class SubprocessDeviceManager : public IDeviceManager
{
public:
	SubprocessDeviceManager();
	~SubprocessDeviceManager() override;

	virtual bool RegisterDevice(const char* name, DeviceFactoryFunc factory, DeviceRegisterFunc register_func, DeviceUnregisterFunc unregister_func) override;

	bool HasDevice(const std::string& name) const;

	IDevice* CreateDevice(const std::string& name, uint32_t id, IEmulator* emulator);

private:
	std::map<std::string, DeviceFactoryFunc> device_factories;
};