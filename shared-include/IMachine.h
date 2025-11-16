#pragma once

#include "stdint.h"

#include <string>

#include "IDevice.h"
#include "IEmulator.h"

#include "FDTNode.h"

#define MACHINE_PATH EMULATOR_SHARED_PATH"Machine"

typedef struct
{
	bool ready;
	bool running;
	bool powered;
} MachineStatus;


namespace MachineToSubprocess
{
	enum class Type : uint8_t
	{
		SET_BIOS_PATH,
		SET_KERNEL_PATH,
		SET_DTB_PATH,

		DUMP_DTB,

		SET_CMD_LINE,
		APPEND_CMD_LINE,

		START_RESUME,
		PAUSE,
		RESTART,
		SHUTDOWN,

		ATTACH_DEVICE,
	};

	struct SetPath
	{
		char path[504];
	};

	struct AttachDevice
	{
		uint32_t id;
		uint64_t addr;
	};
}

namespace MachineFromSubprocess
{
	enum class Type : uint8_t
	{
		NONE,
	};
}

typedef struct alignas(8)
{
	MachineToSubprocess::Type typeToSubprocess;
	MachineFromSubprocess::Type typeFromSubprocess;

	union {
		MachineToSubprocess::SetPath setPath;

		MachineToSubprocess::AttachDevice attachDevice;
	};
} MachineMessage;

#define MACHINE_MAX_MESSAGES 16

class IMachine
{
public:
	virtual ~IMachine() = default;

	virtual uint32_t GetID() = 0;

	virtual uint32_t GetHartCount() = 0;
	virtual uint64_t GetRAMCount() = 0;

	virtual bool IsRunning() = 0;
	virtual bool IsPowered() = 0;

	virtual bool GetBIOSPath(std::string& path) = 0;
	virtual bool GetKernelPath(std::string& path) = 0;
	virtual bool GetDTBPath(std::string& path) = 0;

	virtual bool AttachDevice(IDevice* dev, uint64_t addr) = 0;
	virtual bool RemoveDevice(IDevice* dev) = 0;

	// Subproccess only

	virtual std::unique_ptr<FDTNode> GetFDTRoot() { return nullptr; };
	virtual std::unique_ptr<FDTNode> GetFDTSoc() { return nullptr; };

	virtual bool WriteRAM(uint64_t dest, const void* src, size_t size) { return false; };
	virtual bool ReadRAM(void* dest, uint64_t src, size_t size) { return false; };
	virtual void* GetDMAPointer(uint64_t addr, size_t size) { return nullptr; };

	virtual void* GetRawMachine() { return nullptr; };

	static std::string GetSharedPath(uint32_t id)
	{
		std::string out = MACHINE_PATH;
		
		out += std::to_string(id);

		return out;
	}

	static std::string GetStatusSharedPath(uint32_t id)
	{
		std::string out = GetSharedPath(id);
		out += "Status";
		return out;
	}

	static std::string GetPipeSharedPath(uint32_t id)
	{
		std::string out = GetSharedPath(id);
		out += "Pipe";
		return out;
	}
};

typedef struct
{
	uint32_t id;
} GmodMachineLuaProxy;