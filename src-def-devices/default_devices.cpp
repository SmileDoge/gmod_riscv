#include "default_devices.h"

extern "C"
{
#include "devices/nvme.h"
}

static IEmulator* g_DEmulator = nullptr;

// --------------------- NVME ---------------------

bool NVMEDevice::OnCreate(nlohmann::json& json)
{
	/*
	if (json.contains("add_chosen") && json["add_chosen"].is_boolean())
		add_chosen = json["add_chosen"].get<bool>();
	*/

	if (!json.is_object()) return false;

	if (!json.contains("path") || !json["path"].is_string()) return false;

	path = json["path"];

	if (json.contains("rw") && json["rw"].is_boolean())
		rw = json["rw"];

	return true;
}

size_t NVMEDevice::GetSize()
{
	rvvm_mmio_dev_t* dev = static_cast<rvvm_mmio_dev_t*>(GetRawDev());
	
	if (dev)
		return dev->size;
		
	return 0x1000;
}

void NVMEDevice::OnAttach(IMachine* machine, uint64_t addr)
{
#ifndef RVVM_GMOD_SIDE
	rvvm_machine_t* rv_machine = (rvvm_machine_t*)machine->GetRawMachine();

	pci_bus_t* pci = rvvm_get_pci_bus(rv_machine);
	if (!pci)
		pci = pci_bus_init_auto(rv_machine);

	pci_dev_t* dev = nvme_init_auto(rv_machine, path.c_str(), rw);

	SetRawDev((void*)dev);
#endif
}

static int nvme_device_metatable = 0;

CREATE_FACTORY(NVMEDevice)
CREATE_REGISTER_FUNC(NVMEDevice)
{
	g_DEmulator = emulator;

	nvme_device_metatable = LUA->CreateMetaTable("SimpleDevice");
		LUA->PushCFunction(g_DEmulator->GetMetaToString());
		LUA->SetField(-2, "__tostring");

		LUA->PushCFunction(g_DEmulator->GetMetaIndex());
		LUA->SetField(-2, "__index");
	LUA->Pop();

	LUA->PushMetaTable(nvme_device_metatable);
	LUA->PushNumber(nvme_device_metatable);
	LUA->SetField(-2, "__device_type");
	LUA->Pop();

	return nvme_device_metatable;
}

// --------------------- Register all ---------------------

#ifdef RVVM_GMOD_SIDE
#include "GmodEmulator.h"
#else
#include "SubprocessEmulator.h"
#endif

#ifdef RVVM_GMOD_SIDE
#define REGISTER_DEVICE(type, str_name) emu->RegisterDevice(str_name, Create##type, Register##type, nullptr)
#else
#define REGISTER_DEVICE(type, str_name) emu->RegisterDevice(str_name, Create##type)
#endif

void RegisterDefaultDevices(IEmulator* _emu)
{
#ifdef RVVM_GMOD_SIDE
	GmodEmulator* emu = static_cast<GmodEmulator*>(_emu);
#else
	SubprocessEmulator* emu = static_cast<SubprocessEmulator*>(_emu);
#endif

	REGISTER_DEVICE(NVMEDevice, "nvme_device");
}
