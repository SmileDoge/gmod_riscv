#include "SubprocessMachine.h"

#include "SubprocessEmulator.h"

#include "CFDTNode.h"

extern "C"
{
#include "devices/riscv-aclint.h"
#include "devices/riscv-aplic.h"
#include "devices/riscv-imsic.h"
#include "devices/riscv-plic.h"

#include <devices/rtc-goldfish.h>

#include <devices/syscon.h>

#include <devices/rtl8169.h>

#include <devices/nvme.h>

#include <devices/i2c-oc.h>
#include <devices/i2c-hid.h>
#include <devices/hid_api.h>
}

SubprocessMachine::SubprocessMachine()
{
	id = -1;
	rv_machine = nullptr;

	bios_path_present = false;
	kernel_path_present = false;
	dtb_path_present = false;

	ram_count = 0;
	hart_count = 0;

	receive_msg = {};
}

SubprocessMachine::~SubprocessMachine()
{
	Deinitialize();
}

uint32_t SubprocessMachine::GetID()
{
	return id;
}

uint32_t SubprocessMachine::GetHartCount()
{
	return hart_count;
}

uint64_t SubprocessMachine::GetRAMCount()
{
	return ram_count;
}

bool SubprocessMachine::IsRunning()
{
	if (!rv_machine) return false;

	return rvvm_machine_running(rv_machine);
}

bool SubprocessMachine::IsPowered()
{
	if (!rv_machine) return false;

	return rvvm_machine_powered(rv_machine);
}

bool SubprocessMachine::GetBIOSPath(std::string& path)
{
	if (!bios_path_present) return false;

	path = bios_path;

	return true;
}

bool SubprocessMachine::GetKernelPath(std::string& path)
{
	if (!kernel_path_present) return false;

	path = kernel_path;

	return true;
}

bool SubprocessMachine::GetDTBPath(std::string& path)
{
	if (!dtb_path_present) return false;

	path = dtb_path;

	return true;
}

bool SubprocessMachine::AttachDevice(IDevice* dev, uint64_t addr)
{
	if (!rv_machine)
		return false;

	if (!dev)
		return false;

	if (GetDevice(dev->GetUniqueID()))
		return false;

	dev->OnAttach(this, addr);

	DeviceType type = dev->GetType();
	
	if (type == DeviceType::USER)
		return AttachUserDevice(dev, addr);
	else if (type == DeviceType::BUILTIN)
		return AttachBuiltinDevice(dev, addr);

	return false;
}

bool SubprocessMachine::RemoveDevice(IDevice* dev)
{
	return false;
}

std::unique_ptr<FDTNode> SubprocessMachine::GetFDTRoot()
{
	if (!rv_machine)
		return nullptr;

	return std::make_unique<FDTNodeImpl>(rvvm_get_fdt_root(rv_machine));
}

std::unique_ptr<FDTNode> SubprocessMachine::GetFDTSoc()
{
	if (!rv_machine)
		return nullptr;

	return std::make_unique<FDTNodeImpl>(rvvm_get_fdt_soc(rv_machine));
}

bool SubprocessMachine::WriteRAM(uint64_t dest, const void* src, size_t size)
{
	if (!rv_machine) return false;

	return rvvm_write_ram(rv_machine, dest, src, size);
}

bool SubprocessMachine::ReadRAM(void* dest, uint64_t src, size_t size)
{
	if (!rv_machine) return false;

	return rvvm_read_ram(rv_machine, dest, src, size);
}

void* SubprocessMachine::GetDMAPointer(uint64_t addr, size_t size)
{
	if (!rv_machine) return nullptr;

	return rvvm_get_dma_ptr(rv_machine, addr, size);
}

void* SubprocessMachine::GetRawMachine()
{
	return rv_machine;
}

IDevice* SubprocessMachine::GetDevice(uint32_t id) noexcept
{
	if (devices.find(id) == devices.end())
		return nullptr;

	return devices.at(id)->device;
}

bool SubprocessMachine::Initialize(uint32_t id, uint32_t hart_count, uint64_t ram_count)
{
	if (id == -1) return false;
	if (rv_machine) return false;

	if (!pair.Initialize(IMachine::GetPipeSharedPath(id), sizeof(MachineMessage), MACHINE_MAX_MESSAGES))
	{
		RV_ERROR("SubprocessMachine::Initialize - pair.Initialize error");
		return false;
	}

	if (!status.OpenShared(IMachine::GetStatusSharedPath(id)))
	{
		pair.Close();
		RV_ERROR("SubprocessMachine::Initialize - status.OpenShared error");
		return false;
	}

	rv_machine = rvvm_create_machine(ram_count, hart_count, nullptr);

	if (!rv_machine)
	{
		RV_ERROR("SubprocessMachine::Initialize - rvvm_create_machine error");
		return false;
	}

	this->id = id;

	this->ram_count = ram_count;
	this->hart_count = hart_count;

	status->ready = true;
	status->powered = false;
	status->running = false;

	riscv_clint_init_auto(rv_machine);
	riscv_plic_init_auto(rv_machine);

	rtc_goldfish_init_auto(rv_machine);

	pci_bus_init_auto(rv_machine);

	nvme_init_auto(rv_machine, "D:\\gmod_server_turbostroi_64\\rootfs.ext2", true);
		
	return true;
}

void SubprocessMachine::Deinitialize()
{
	if (!rv_machine) return;

	rvvm_free_machine(rv_machine);

	pair.Close();
	status.Close();

	id = -1;
	rv_machine = nullptr;

	bios_path_present = false;
	kernel_path_present = false;
	dtb_path_present = false;

	ram_count = 0;
	hart_count = 0;
}

bool SubprocessMachine::SetCommandLine(const std::string& cmd_line)
{
	if (!rv_machine) return false;

	rvvm_set_cmdline(rv_machine, cmd_line.c_str());

	return true;
}

bool SubprocessMachine::AppendCommandLine(const std::string& cmd_line)
{
	if (!rv_machine) return false;

	rvvm_append_cmdline(rv_machine, cmd_line.c_str());

	return true;
}

bool SubprocessMachine::SetBIOSPath(const std::string& path)
{
	if (!rv_machine) return false;

	if (!rvvm_load_bootrom(rv_machine, path.c_str()))
		return false;

	bios_path = path;
	bios_path_present = true;

	RV_DEBUG("BIOS path set to: %s", path.c_str());

	return true;
}

bool SubprocessMachine::SetKernelPath(const std::string& path)
{
	if (!rv_machine) return false;

	if (!rvvm_load_kernel(rv_machine, path.c_str()))
		return false;

	kernel_path = path;
	kernel_path_present = true;

	RV_DEBUG("Kernel path set to: %s", path.c_str());

	return true;
}

bool SubprocessMachine::SetDTBPath(const std::string& path)
{
	if (!rv_machine) return false;

	if (!rvvm_load_dtb(rv_machine, path.c_str()))
		return false;

	dtb_path = path;
	dtb_path_present = true;

	RV_DEBUG("DTB path set to: %s", path.c_str());

	return true;
}

bool SubprocessMachine::AttachUserDevice(IDevice* dev, uint64_t addr)
{
	rvvm_mmio_dev_t mmio{};

	SubprocessDeviceProxy* device_proxy = new SubprocessDeviceProxy();

	memset(device_proxy, 0, sizeof(SubprocessDeviceProxy));

	device_proxy->device = dev;
	device_proxy->machine = this;
	device_proxy->rv_type.name = dev->GetName();
	device_proxy->rv_type.remove = DEV_Remove;
	device_proxy->rv_type.update = DEV_Update;
	device_proxy->rv_type.reset = DEV_Reset;

	mmio.addr = addr;
	mmio.size = dev->GetSize();
	mmio.data = device_proxy;
	mmio.type = &device_proxy->rv_type;

	mmio.write = DEV_Write;
	mmio.read = DEV_Read;

	mmio.mapping = dev->GetMapping();

	rvvm_mmio_dev_t* created_mmio = rvvm_attach_mmio(rv_machine, &mmio);

	if (!created_mmio)
	{
		delete device_proxy;

		RV_ERROR("Error creating MMIO! addr=%016X size=%016X type='%s'", addr, dev->GetSize(), dev->GetName());

		delete dev;

		return false;
	}

	device_proxy->rv_device = created_mmio;

	RV_INFO("Finally. Created... addr=%016X size=%016X type='%s'", addr, dev->GetSize(), dev->GetName());

	devices[dev->GetUniqueID()] = device_proxy;

	dev->SetRawDev((void*)created_mmio);

	return true;
}

bool SubprocessMachine::AttachBuiltinDevice(IDevice* dev, uint64_t addr)
{
	rvvm_mmio_dev_t* mmio = (rvvm_mmio_dev_t*)dev->GetCreatedDev();

	SubprocessDeviceProxy* device_proxy = new SubprocessDeviceProxy();

	memset(device_proxy, 0, sizeof(SubprocessDeviceProxy));

	device_proxy->device = dev;
	device_proxy->machine = this;
	device_proxy->rv_device = mmio;

	devices[dev->GetUniqueID()] = device_proxy;

	return true;
}

bool SubprocessMachine::StartResume()
{
	if (!rv_machine) return false;

	return rvvm_start_machine(rv_machine);
}

bool SubprocessMachine::Pause()
{
	if (!rv_machine) return false;

	return rvvm_pause_machine(rv_machine);
}

bool SubprocessMachine::Restart()
{
	if (!rv_machine) return false;

	rvvm_reset_machine(rv_machine, true);

	return true;
}

bool SubprocessMachine::Shutdown()
{
	if (!rv_machine) return false;

	rvvm_reset_machine(rv_machine, false);

	return true;
}

void SubprocessMachine::Update()
{
	if (!rv_machine) return;

	if (!status.IsOpen()) return;

	status->running = rvvm_machine_running(rv_machine);
	status->powered = rvvm_machine_powered(rv_machine);

	ProcessMessages();
}

rvvm_machine_t* SubprocessMachine::GetRVMachine()
{
	return rv_machine;
}

bool SubprocessMachine::DEV_Write(rvvm_mmio_dev_t* dev, void* dest, size_t offset, uint8_t size)
{
	if (!dev->data) return false;

	SubprocessDeviceProxy* proxy = (SubprocessDeviceProxy*)dev->data;

	IDevice* device = proxy->device;

	return device->OnWrite(dest, offset, size);
}

bool SubprocessMachine::DEV_Read(rvvm_mmio_dev_t* dev, void* dest, size_t offset, uint8_t size)
{
	if (!dev->data) return false;

	SubprocessDeviceProxy* proxy = (SubprocessDeviceProxy*)dev->data;

	IDevice* device = proxy->device;

	return device->OnRead(dest, offset, size);
}

void SubprocessMachine::DEV_Remove(rvvm_mmio_dev_t* dev)
{
	if (!dev->data) return;

	SubprocessDeviceProxy* proxy = (SubprocessDeviceProxy*)dev->data;
	SubprocessMachine* machine = (SubprocessMachine*)proxy->machine;

	IDevice* device = proxy->device;

	device->OnRemove();

	machine->devices.erase(device->GetUniqueID());

	delete device;

	delete proxy;

	dev->data = nullptr; // с богом
}

void SubprocessMachine::DEV_Update(rvvm_mmio_dev_t* dev)
{
	if (!dev->data) return;

	SubprocessDeviceProxy* proxy = (SubprocessDeviceProxy*)dev->data;

	IDevice* device = proxy->device;

	device->OnUpdate();
}

void SubprocessMachine::DEV_Reset(rvvm_mmio_dev_t* dev)
{
	if (!dev->data) return;

	SubprocessDeviceProxy* proxy = (SubprocessDeviceProxy*)dev->data;

	IDevice* device = proxy->device;

	device->OnReset();
}

void SubprocessMachine::ProcessMessages()
{
	while (pair.PopFromMain(&receive_msg))
	{
		switch (receive_msg.typeToSubprocess)
		{
		case MachineToSubprocess::Type::SET_BIOS_PATH:
		case MachineToSubprocess::Type::SET_KERNEL_PATH:
		case MachineToSubprocess::Type::SET_DTB_PATH:
		case MachineToSubprocess::Type::SET_CMD_LINE:
		case MachineToSubprocess::Type::APPEND_CMD_LINE:
			ProcessSetPath(receive_msg.typeToSubprocess);
			break;

		case MachineToSubprocess::Type::START_RESUME:
			StartResume();
			break;
		case MachineToSubprocess::Type::PAUSE:
			Pause();
			break;
		case MachineToSubprocess::Type::RESTART:
			Restart();
			break;
		case MachineToSubprocess::Type::SHUTDOWN:
			Shutdown();
			break;

		case MachineToSubprocess::Type::ATTACH_DEVICE:
			ProcessAttachDevice(receive_msg.attachDevice.id, receive_msg.attachDevice.addr);
			break;

		case MachineToSubprocess::Type::DUMP_DTB:
			ProcessDumpDTB();
			break;

		default:
			RV_WARN("Unknown message type! %d", receive_msg.typeToSubprocess);
			break;
		}
	}
}

void SubprocessMachine::ProcessDumpDTB()
{
	if (!rv_machine)
		return;

	rvvm_dump_dtb(rv_machine, receive_msg.setPath.path);
}

void SubprocessMachine::ProcessSetPath(MachineToSubprocess::Type type)
{
	switch (type)
	{
	case MachineToSubprocess::Type::SET_BIOS_PATH:
		SetBIOSPath(receive_msg.setPath.path);
		break;
	case MachineToSubprocess::Type::SET_KERNEL_PATH:
		SetKernelPath(receive_msg.setPath.path);
		break;
	case MachineToSubprocess::Type::SET_DTB_PATH:
		SetDTBPath(receive_msg.setPath.path);
		break;
	case MachineToSubprocess::Type::SET_CMD_LINE:
		SetCommandLine(receive_msg.setPath.path);
		break;
	case MachineToSubprocess::Type::APPEND_CMD_LINE:
		AppendCommandLine(receive_msg.setPath.path);
		break;
	default:
		break;
	}
}

void SubprocessMachine::ProcessAttachDevice(uint32_t id, uint64_t addr)
{
	IDevice* dev = g_Emulator->GetDevice(id);

	if (!dev)
	{
		RV_ERROR("SubprocessMachine::ProcessAttachDevice - device with %d not created!", id);
		return;
	}

	AttachDevice(dev, addr);

	bool is_attached = true;

	dev->SetStatus(nullptr, &is_attached);
}
