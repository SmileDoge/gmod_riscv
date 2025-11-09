#pragma once

#include "IEmulator.h"

#include "IMachine.h"

#include "rvvmlib.h"

#include "shared_memory_manager.hpp"
#include "spsc_shared_manager.hpp"

#include "SubprocessDeviceProxy.h"

#include <vector>

class SubprocessMachine : public IMachine
{
public:
	SubprocessMachine();
	~SubprocessMachine() override;

	virtual uint32_t GetID();

	virtual uint32_t GetHartCount();
	virtual uint64_t GetRAMCount();

	virtual bool IsRunning();
	virtual bool IsPowered();

	virtual bool GetBIOSPath(std::string& path);
	virtual bool GetKernelPath(std::string& path);
	virtual bool GetDTBPath(std::string& path);

	virtual bool AttachDevice(IDevice* dev, uint64_t addr);
	virtual bool RemoveDevice(IDevice* dev);

	virtual std::unique_ptr<FDTNode> GetFDTRoot();
	virtual std::unique_ptr<FDTNode> GetFDTSoc();

	virtual bool WriteRAM(uint64_t dest, const void* src, size_t size);
	virtual bool ReadRAM(void* dest, uint64_t src, size_t size);
	virtual void* GetDMAPointer(uint64_t addr, size_t size);

	virtual void* GetRawMachine();

	IDevice* GetDevice(uint32_t id) noexcept;

	bool Initialize(uint32_t id, uint32_t hart_count, uint64_t ram_count);
	void Deinitialize();

	bool SetCommandLine(const std::string& cmd_line);
	bool AppendCommandLine(const std::string& cmd_line);

	bool SetBIOSPath(const std::string& path);
	bool SetKernelPath(const std::string& path);
	bool SetDTBPath(const std::string& path);

	bool AttachUserDevice(IDevice* dev, uint64_t addr);
	bool AttachBuiltinDevice(IDevice* dev, uint64_t addr);

	bool StartResume();
	bool Pause();

	bool Restart();
	bool Shutdown();

	void Update();

	rvvm_machine_t* GetRVMachine();

private:

	static bool DEV_Write(rvvm_mmio_dev_t* dev, void* dest, size_t offset, uint8_t size);
	static bool DEV_Read(rvvm_mmio_dev_t* dev, void* dest, size_t offset, uint8_t size);

	static void DEV_Remove(rvvm_mmio_dev_t* dev);
	static void DEV_Update(rvvm_mmio_dev_t* dev);
	static void DEV_Reset(rvvm_mmio_dev_t* dev);

private:

	void ProcessMessages();

	void ProcessDumpDTB();
	void ProcessSetPath(MachineToSubprocess::Type type);
	void ProcessAttachDevice(uint32_t id, uint64_t addr);

private:
	rvvm_machine_t* rv_machine;

	SharedMemory<MachineStatus> status;

	SharedSPSC pair;

	std::unordered_map<uint32_t, SubprocessDeviceProxy*> devices;

	uint32_t id;

	std::string bios_path;
	bool bios_path_present;

	std::string kernel_path;
	bool kernel_path_present;

	std::string dtb_path;
	bool dtb_path_present;

	uint64_t ram_count;
	uint32_t hart_count;

	MachineMessage receive_msg;
};