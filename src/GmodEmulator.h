#pragma once

#include "IEmulator.h"
#include "ILogger.h"

#include "GmodMachine.h"
#include "GmodDeviceManager.h"

#include "ProcessHandle.h"

#include "spsc_shared_manager.hpp"

#include <GarrysMod/Lua/Interface.h>

#include "nlohmann/json.hpp"

#include <queue>
#include <map>
#include <unordered_map>
#include <vector>

typedef struct
{
	ILogType type;
	std::string str;
} LogMessage;

class GmodEmulator : public IEmulator, public ILogger
{
public:
	GmodEmulator();
	~GmodEmulator() noexcept override;

	virtual EmulatorState GetState();
	virtual int GetVersion();

	virtual IDevice* GetDevice(uint32_t id);
	GmodDeviceProxy* GetDeviceProxy(uint32_t id);

	virtual ILogger* GetLogger();

	virtual GmodDeviceProxy* CheckDeviceProxy(GarrysMod::Lua::ILuaBase* LUA, int index, bool throw_error = true);

	virtual GarrysMod::Lua::CFunc GetMetaToString();
	virtual GarrysMod::Lua::CFunc GetMetaIndex();

	virtual void LogString(ILogType type, const std::string& string);

	virtual void LogInfo(const char* format, ...);
	virtual void LogWarn(const char* format, ...);
	virtual void LogError(const char* format, ...);
	virtual void LogDebug(const char* format, ...);

	virtual void LogDebugWithLine(int line, const std::string& file, const char* format, ...);

	GmodMachine* GetMachine(uint32_t id);

	GmodMachine* CreateMachine(uint32_t id, uint32_t hart_count, uint64_t ram_count);
	void DestroyMachine(uint32_t id, bool delete_from_map = true);

	GmodDeviceProxy* CreateDevice(const std::string& type, nlohmann::json& json);
	void DestroyDevice(GmodDeviceProxy* device, bool delete_from_map = true);

	bool RegisterDevice(const std::string& type, DeviceFactoryFunc factory, DeviceRegisterFunc register_func, DeviceUnregisterFunc unregister_func);
	int GetDeviceMetaTableIndex(const std::string& type);

	void NotifyAllRegister(GarrysMod::Lua::ILuaBase* LUA);
	void NotifyAllUnregister(GarrysMod::Lua::ILuaBase* LUA);

	bool Start(GarrysMod::Lua::ILuaBase* LUA);
	void Stop();

	void InitLogForConsole();
	void InitLogForGmod(GarrysMod::Lua::ILuaBase* LUA);

	SharedSPSC* GetPair() noexcept;

	bool Update();
private:

	bool InitStatus();
	bool InitPair();
	bool InitProcess();

	void OnCloseProcess();
	void CloseProcess();
	void WaitProcess();

	void ProcessMessages();

	void ProcessLogMessage();

private:
	EmulatorState state;

	Process subprocess;

	SharedMemory<EmulatorStatus> status;
	SharedSPSC pair;

	std::queue<LogMessage> log_messages_queue;
	GarrysMod::Lua::ILuaBase* L;

	std::unordered_map<uint32_t, GmodMachine*> machines;

	//std::vector<IDevice*> devices;
	//std::unordered_map<uint32_t, IDevice*> devices;
	std::unordered_map<uint32_t, GmodDeviceProxy*> devices;

	EmulatorMessage receive_msg;

	GmodDeviceManager device_manager;
};

extern ILogger* g_Logger;