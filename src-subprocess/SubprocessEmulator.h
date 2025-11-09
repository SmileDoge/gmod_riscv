#pragma once

#include "IEmulator.h"
#include "ILogger.h"

#include "spsc_shared_manager.hpp"

#include "ProcessHandle.h"

#include "SubprocessMachine.h"
#include "SubprocessDeviceManager.h"

#include <thread>

#include <map>

class SubprocessEmulator : public IEmulator, public ILogger
{
public:
	SubprocessEmulator(int argc, char** argv);
	~SubprocessEmulator() override;

	virtual EmulatorState GetState();
	virtual int GetVersion();

	virtual IDevice* GetDevice(uint32_t id);
	virtual ILogger* GetLogger();

	virtual void LogString(ILogType type, const std::string& string);

	virtual void LogInfo(const char* format, ...);
	virtual void LogWarn(const char* format, ...);
	virtual void LogError(const char* format, ...);
	virtual void LogDebug(const char* format, ...);

	virtual void LogDebugWithLine(int line, const std::string& file, const char* format, ...);

	void CreateMachine(uint32_t id, uint32_t hart_count, uint64_t ram_count);
	void DestroyMachine(uint32_t id);
	void DestroyDevice(uint32_t id);

	bool RegisterDevice(const std::string& type, DeviceFactoryFunc factory);

	bool Start();
	void Stop();
	int StartLoop();
private:

	bool InitStatus();
	bool InitPair(uint32_t capacity, uint32_t item_size);

	void ProcessMessages();

	void ProcessCreateMachine();
	void ProcessDestroyMachine();
	void ProcessCreateDevice();

	void RunEventThread();
	static void EventThread();

private:
	bool run_event_thread;
	std::thread event_thread;

	EmulatorState state;

	SharedMemory<EmulatorStatus> status;
	SharedSPSC pair;
	
	std::map<uint32_t, SubprocessMachine*> machines;

	//std::vector<IDevice*> devices;
	std::unordered_map<uint32_t, IDevice*> devices;

	uint32_t parent_pid;
	Process parent_proc;

	EmulatorMessage receive_msg;

	SubprocessDeviceManager device_manager;
};

extern SubprocessEmulator* g_Emulator;

extern ILogger* g_Logger;