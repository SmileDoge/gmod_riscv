#pragma once

#include "IEmulator.h"

#include "IMachine.h"
#include "IDevice.h"

#include "GmodDeviceProxy.h"

#include "shared_memory_manager.hpp"
#include "spsc_shared_manager.hpp"

#include <unordered_map>

class GmodMachine : public IMachine
{
public:
	GmodMachine();
	~GmodMachine() override;

	virtual uint32_t GetID() override;

	virtual uint32_t GetHartCount() override;
	virtual uint64_t GetRAMCount() override;

	virtual bool IsRunning() override;
	virtual bool IsPowered() override;

	virtual bool GetBIOSPath(std::string& path) override;
	virtual bool GetKernelPath(std::string& path) override;
	virtual bool GetDTBPath(std::string& path) override;

	virtual bool AttachDevice(IDevice* dev, uint64_t addr) override;
	virtual bool RemoveDevice(IDevice* dev) override;

	bool AttachDevice(GmodDeviceProxy* dev_proxy, uint64_t addr);
	bool RemoveDevice(GmodDeviceProxy* dev_proxy);

	IDevice* GetDevice(uint32_t id) noexcept;

	GmodDeviceProxy* GetDeviceProxy(uint32_t id) noexcept;

	bool IsReady() noexcept;
	void WaitForReady(int timeout_ms) noexcept;

	bool Initialize(uint32_t id, uint32_t hart_count, uint64_t ram_count);

	void Deinitialize();

	bool SetCommandLine(const std::string& cmd_line);
	bool AppendCommandLine(const std::string& cmd_line);

	bool SetBIOSPath(const std::string& path);
	bool SetKernelPath(const std::string& path);
	bool SetDTBPath(const std::string& path);

	bool DumpDTB(const std::string& path);

	bool StartResume();
	bool Pause();

	bool Restart();
	bool Shutdown();

private:
	uint32_t id;

	SharedMemory<MachineStatus> status;

	SharedSPSC pair;

	std::unordered_map<uint32_t, GmodDeviceProxy*> devices;
	//std::unordered_map<uint32_t, IDevice*> devices;

	std::string bios_path;
	bool bios_path_present;

	std::string kernel_path;
	bool kernel_path_present;

	std::string dtb_path;
	bool dtb_path_present;

	uint64_t ram_count;
	uint32_t hart_count;
};