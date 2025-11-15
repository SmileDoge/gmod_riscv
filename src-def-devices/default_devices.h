#pragma once

#include "IDevice.h"
#include "IEmulator.h"
#include "IMachine.h"

BEGIN_BUILTIN_DEVICE(NVMEDevice, "nvme_device")

public:
	virtual bool OnCreate(nlohmann::json& json) override;

	virtual size_t GetSize() override;
	virtual void OnAttach(IMachine* machine, uint64_t addr) override;
	virtual bool OnWrite(void* data, size_t offset, uint8_t size) { return true; };
	virtual bool OnRead(void* data, size_t offset, uint8_t size) { return true; };

private:

	std::string path = "";
	bool rw = false;

END_DEVICE()
DECLARE_FACTORY(NVMEDevice)
DECLARE_REGISTER_FUNC(NVMEDevice)

void RegisterDefaultDevices(IEmulator* emu);