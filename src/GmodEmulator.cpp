#include "GmodEmulator.h"

#include "stddef.h"

#include "GmodDeviceLua.h"

#ifdef _DEBUG
#define CHILD_PROCESS_EXE "M:\\Projects\\C++\\gmod_riscv\\out\\x86_64\\Debug\\rvvm_subprocess.exe"
#else
#define CHILD_PROCESS_EXE "M:\\Projects\\C++\\gmod_riscv\\out\\x86_64\\Release\\rvvm_subprocess.exe"
#endif

ILogger* g_Logger = nullptr;

GmodEmulator::GmodEmulator()
{
	state = EmulatorState::NOT_STARTED;

	L = nullptr;

	receive_msg = EmulatorMessage{};
}

GmodEmulator::~GmodEmulator() noexcept
{
	Stop();
}

bool GmodEmulator::Start(GarrysMod::Lua::ILuaBase* LUA)
{
	L = LUA;

	if (LUA)
		InitLogForGmod(LUA);
	else
		InitLogForConsole();

	if (!InitStatus())
	{
		RV_ERROR("InitStatus error");
		return false;
	}

	if (!InitPair())
	{
		RV_ERROR("InitPair error");
		return false;
	}

	if (!InitProcess())
	{
		RV_ERROR("InitProcess error");
		return false;
	}

	status->mainVersion = GetVersion();

	state = EmulatorState::WAITING_CHILD_PROCESS;

	WaitProcess();

	if (state == EmulatorState::WAITING_CHILD_PROCESS)
	{
		RV_ERROR("Subprocess Timeout");

		return false;
	}

	if (status->mainVersion != status->subprocessVersion)
	{
		RV_ERROR("Versions is not equals! main [%d] <-> subprocess [%d]", status->mainVersion, status->subprocessVersion);

		CloseProcess();

		return false;
	}

	return true;
}

void GmodEmulator::Stop()
{
	for (auto& [id, machine] : machines)
	{
		machine->Deinitialize();

		delete machine;
	}

	for (auto& [id, device] : devices)
	{
		DestroyDevice(device, false);
	}

	machines.clear();
	devices.clear();

	CloseProcess();

	pair.Close();
	status.Close();

	L = nullptr;
}

EmulatorState GmodEmulator::GetState()
{
	return state;
}

int GmodEmulator::GetVersion()
{
	return EMULATOR_VERSION;
}

IDevice* GmodEmulator::GetDevice(uint32_t id)
{
	if (devices.find(id) == devices.end())
		return nullptr;

	return devices.at(id)->device;
}

GmodDeviceProxy* GmodEmulator::GetDeviceProxy(uint32_t id)
{
	if (devices.find(id) == devices.end())
		return nullptr;

	return devices.at(id);
}

ILogger* GmodEmulator::GetLogger()
{
	return this;
}

GmodDeviceProxy* GmodEmulator::CheckDeviceProxy(GarrysMod::Lua::ILuaBase* LUA, int index, bool throw_error)
{
	return GmodDeviceLua::CheckDeviceProxy(LUA, index, throw_error);
}

GarrysMod::Lua::CFunc GmodEmulator::GetMetaToString()
{
	return GmodDeviceLua::meta__tostring;
}

GarrysMod::Lua::CFunc GmodEmulator::GetMetaIndex()
{
	return GmodDeviceLua::meta__index;
}

void PassLuaColor(GarrysMod::Lua::ILuaBase* LUA, ILogType type)
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);

	LUA->GetField(-1, "Color");

	switch (type)
	{
	case ILogType::LOG_INFO:
		LUA->PushNumber(0);
		LUA->PushNumber(255);
		LUA->PushNumber(0);
		break;
	case ILogType::LOG_WARN:
		LUA->PushNumber(255);
		LUA->PushNumber(255);
		LUA->PushNumber(0);
		break;
	case ILogType::LOG_ERROR:
		LUA->PushNumber(255);
		LUA->PushNumber(0);
		LUA->PushNumber(0);
		break;
	case ILogType::LOG_DEBUG:
		LUA->PushNumber(255);
		LUA->PushNumber(0);
		LUA->PushNumber(255);
		break;
	default:
		LUA->PushNumber(255);
		LUA->PushNumber(255);
		LUA->PushNumber(255);
		break;
	}
	LUA->Call(3, 1);

	LUA->Remove(-2);
}

const char* GetTextType(ILogType type)
{
	switch (type)
	{
	case ILogType::LOG_INFO:
		return "INFO";
		break;
	case ILogType::LOG_WARN:
		return "WARN";
		break;
	case ILogType::LOG_ERROR:
		return "ERROR";
		break;
	case ILogType::LOG_DEBUG:
		return "DEBUG";
		break;
	case ILogType::WHITE:
	default:
		return "UNDF";
		break;
	}
}

void GmodEmulator::LogString(ILogType type, const std::string& string)
{
	if (L)
	{
		auto& LUA = L;

		LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
			LUA->GetField(-1, "MsgC");

			PassLuaColor(LUA, ILogType::WHITE);
			LUA->PushString("RVGMEmulator = ");

			PassLuaColor(LUA, type);
			LUA->PushString(GetTextType(type));

			PassLuaColor(LUA, ILogType::WHITE);
			LUA->PushString(" - ");

			LUA->PushString(string.data(), string.size());
			LUA->PushString("\n");

			LUA->Call(8, 0);
		LUA->Pop();
	}
	else
	{
		time_t rawtime;
		time(&rawtime);
		struct tm* curr_time = localtime(&rawtime);

		const char* prefix;

		switch (type)
		{
		case ILogType::LOG_INFO:
			prefix = "\x1b[92mINFO\x1b[97m";
			break;
		case ILogType::LOG_WARN:
			prefix = "\x1b[93mWARN\x1b[97m";
			break;
		case ILogType::LOG_ERROR:
			prefix = "\x1b[91mERROR\x1b[97m";
			break;
		case ILogType::LOG_DEBUG:
			prefix = "\x1b[96mDEBUG\x1b[97m";
			break;
		case ILogType::WHITE:
		default:
			prefix = "\x1b[96mUNDF\x1b[97m";
			break;
		}

		printf("\x1b[97m[%02d:%02d:%02d] %s\x1b[97m - %s\x1b[0m\n", curr_time->tm_hour, curr_time->tm_min, curr_time->tm_sec, prefix, string.c_str());
	}
}


#define VAR_ARG() \
va_list args; \
va_start(args, format); \
va_list args_copy; \
va_copy(args_copy, args); \
int len = std::vsnprintf(nullptr, 0, format, args_copy); \
va_end(args_copy); \
if (len < 0) { \
	va_end(args); \
	return; \
} \
std::vector<char> zc(static_cast<size_t>(len) + 1); \
std::vsnprintf(zc.data(), zc.size(), format, args); \
va_end(args);

void GmodEmulator::LogInfo(const char* format, ...)
{
	VAR_ARG()

	LogString(ILogType::LOG_INFO, std::string(zc.data(), zc.size()));
}

void GmodEmulator::LogWarn(const char* format, ...)
{
	VAR_ARG()

	LogString(ILogType::LOG_WARN, std::string(zc.data(), zc.size()));
}

void GmodEmulator::LogError(const char* format, ...)
{
	VAR_ARG()

	LogString(ILogType::LOG_ERROR, std::string(zc.data(), zc.size()));
}

void GmodEmulator::LogDebug(const char* format, ...)
{
	VAR_ARG()

	LogString(ILogType::LOG_DEBUG, std::string(zc.data(), zc.size()));
}

void GmodEmulator::LogDebugWithLine(int line, const std::string& file, const char* format, ...)
{
	VAR_ARG()

	LogDebug("[%s:%d] - %s", file.c_str(), line, zc.data());
}

GmodMachine* GmodEmulator::GetMachine(uint32_t id)
{
	if (machines.find(id) == machines.end())
		return nullptr;

	return machines.at(id);
}

GmodMachine* GmodEmulator::CreateMachine(uint32_t id, uint32_t hart_count, uint64_t ram_count)
{
	if (state != EmulatorState::READY)
	{
		RV_ERROR("Emulator is not ready!");
		return nullptr;
	}

	if (machines.find(id) != machines.end())
	{
		RV_ERROR("Machine with id %d already exists!", id);
		return nullptr;
	}

	GmodMachine* machine = new GmodMachine();

	if (!machine->Initialize(id, hart_count, ram_count))
	{
		RV_ERROR("machine->Initialize error");
		delete machine;
		return nullptr;
	}

	EmulatorMessage msg = EmulatorMessage{};

	msg.typeToSubprocess = EmulatorToSubprocess::Type::CREATE_MACHINE;
	msg.createMachine.id = id;
	msg.createMachine.ramCount = ram_count;
	msg.createMachine.hartCount = hart_count;

	if (!pair.PushToWorker(&msg))
	{
		RV_ERROR("pair.PushToWorker error");
		machine->Deinitialize();
		delete machine;
		return nullptr;
	}

	machines[id] = machine;

	return machine;
}

void GmodEmulator::DestroyMachine(uint32_t id, bool delete_from_map)
{
	if (machines.find(id) == machines.end())
	{
		RV_ERROR("Machine with id %d does not exist!", id);
		return;
	}

	GmodMachine* machine = machines[id];

	std::vector<GmodDeviceProxy*> devices_to_destroy;

	for (auto& [id, device] : devices)
		if (machine->GetDevice(id) != nullptr)
			devices_to_destroy.push_back(machine->GetDeviceProxy(id));

	for (auto& dev : devices_to_destroy)
		DestroyDevice(dev);

	machine->Deinitialize();

	delete machine;

	if (delete_from_map)
		machines.erase(id);

	if (state != EmulatorState::READY) return;

	EmulatorMessage msg = EmulatorMessage{};
	msg.typeToSubprocess = EmulatorToSubprocess::Type::DESTROY_MACHINE;
	msg.destroyMachine.id = id;

	if (!pair.PushToWorker(&msg))
	{
		RV_ERROR("pair.PushToWorker error");
		return;
	}
}

GmodDeviceProxy* GmodEmulator::CreateDevice(const std::string& type, nlohmann::json& json)
{
	if (state != EmulatorState::READY)
	{
		RV_ERROR("Emulator is not ready!");
		return nullptr;
	}

	if (!device_manager.HasDevice(type))
	{
		RV_ERROR("Device type '%s' is not registered!", type.c_str());
		return nullptr;
	}

	std::string json_str = json.dump();

	if (json_str.length() > 1023)
	{
		RV_ERROR("Argument json string is long! maximum 1023");
		RV_DEBUG("JSON - %s", json_str.c_str());
		return nullptr;
	}

	IDevice* device = device_manager.CreateDevice(type, device_manager.GetNextUniqueID(), this);

	if (!device->OnCreate(json))
	{
		RV_ERROR("Error occured on creating %s device!", type.c_str());

		delete device;

		return nullptr;
	}

	//devices.push_back(device);

	GmodDeviceProxy* proxy = new GmodDeviceProxy();

	memset(proxy, 0, sizeof(GmodDeviceProxy));

	proxy->id = device->GetUniqueID();
	proxy->machine = nullptr;
	proxy->device = device;
	proxy->address = NULL;

	devices[device->GetUniqueID()] = proxy;

	RV_DEBUG_LINE("Created device! type: %s, uid: %d", type.c_str(), device->GetUniqueID());

	EmulatorMessage msg{};

	msg.typeToSubprocess = EmulatorToSubprocess::Type::CREATE_DEVICE;
	msg.createDevice.id = device->GetUniqueID();
	
	strncpy(msg.createDevice.deviceType, type.c_str(), sizeof(msg.createDevice.deviceType));
	strncpy(msg.createDevice.jsonString, json.dump().c_str(), sizeof(msg.createDevice.jsonString));

	pair.PushToWorker(&msg);

	return proxy;
}

void GmodEmulator::DestroyDevice(GmodDeviceProxy* device_proxy, bool delete_from_map)
{
	if (!device_proxy)
		return;

	if (!device_proxy->device)
		return;

	RV_DEBUG("Device %d '%s' destroyed!", device_proxy->id, device_proxy->device->GetName());

	if (delete_from_map)
		devices.erase(device_proxy->id);

	if (device_proxy->device->GetType() == DeviceType::USER)
		device_proxy->device->OnRemove();

	delete device_proxy->device;

	delete device_proxy;
}

bool GmodEmulator::RegisterDevice(const std::string& type, DeviceFactoryFunc factory, DeviceRegisterFunc register_func, DeviceUnregisterFunc unregister_func)
{
	if (device_manager.HasDevice(type))
		return false;

	return device_manager.RegisterDevice(type.c_str(), factory, register_func, unregister_func);
}

int GmodEmulator::GetDeviceMetaTableIndex(const std::string& type)
{
	if (!device_manager.HasDevice(type))
		return GarrysMod::Lua::Type::Nil;

	return device_manager.GetMetatableIndex(type);
}

void GmodEmulator::NotifyAllRegister(GarrysMod::Lua::ILuaBase* LUA)
{
	device_manager.NotifyAllRegisterDevices(this, LUA);
}

void GmodEmulator::NotifyAllUnregister(GarrysMod::Lua::ILuaBase* LUA)
{
	device_manager.NotifyAllUnregisterDevices(this, LUA);
}

void GmodEmulator::InitLogForConsole()
{
	g_Logger = this;

#ifdef WIN32
	HANDLE handleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD consoleMode;
	GetConsoleMode(handleOut, &consoleMode);
	consoleMode |= 0x0004;
	consoleMode |= 0x0008;
	SetConsoleMode(handleOut, consoleMode);

	SetConsoleOutputCP(CP_UTF8);

	SetConsoleTitleA("RVVM Gmod Emulator");
#endif
}

void GmodEmulator::InitLogForGmod(GarrysMod::Lua::ILuaBase* LUA)
{
	g_Logger = this;

	// gmod scrds x86_64 branch broken
#ifdef WIN32
	HANDLE handleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD consoleMode;
	GetConsoleMode(handleOut, &consoleMode);
	consoleMode |= 0x0004;
	SetConsoleMode(handleOut, consoleMode);

	SetConsoleOutputCP(CP_UTF8);
#endif
}

SharedSPSC* GmodEmulator::GetPair() noexcept
{
	return &pair;
}

bool GmodEmulator::Update()
{
	if (!subprocess.is_running())
	{
		OnCloseProcess();
		return true;
	}

	ProcessMessages();

	return false;
}

bool GmodEmulator::InitStatus()
{
	if (!status.CreateShared(EMULATOR_STATUS_PATH))
		return false;

	return true;
}

bool GmodEmulator::InitPair()
{
	size_t capacity = EMULATOR_MAX_MESSAGES;
	size_t item_size = sizeof(EmulatorMessage);

	if (!pair.Initialize(EMULATOR_PIPE_PATH, item_size, capacity, true))
	{
		printf("[GmodEmulator::InitPair] pair.Initialize error\n");
		return false;
	}

	return true;
}

bool GmodEmulator::InitProcess()
{
	if (subprocess.valid()) return false;

	char pid[16];

	sprintf(pid, "%d", GetCurrentProcessId());

	subprocess = Process::Create(CHILD_PROCESS_EXE, {pid});

	return true;
}

void GmodEmulator::OnCloseProcess()
{
	state = EmulatorState::CHILD_PROCESS_EXITED;

	subprocess.wait();
}

void GmodEmulator::CloseProcess()
{
	if (status)
		status->disableSubprocess = true;

	OnCloseProcess();
}

void GmodEmulator::WaitProcess()
{
	auto start = std::chrono::steady_clock::now();

	while (true)
	{
		auto now = std::chrono::steady_clock::now();
		int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());

		if (elapsed >= 2'000)
		{
			RV_ERROR("Subprocess process is timeout!");
			return;
		}

		if (status->initialized)
		{
			state = EmulatorState::READY;
			return;
		}
	}
}

void GmodEmulator::ProcessMessages()
{
	while (pair.PopFromWorker(&receive_msg))
	{
		switch (receive_msg.typeFromSubprocess)
		{
		case EmulatorFromSubprocess::Type::LOG_MESSAGE:
			ProcessLogMessage();
			break;
		default:
			RV_WARN("Unknown message type! %d", receive_msg.typeFromSubprocess);
			break;
		}
	}
}

void GmodEmulator::ProcessLogMessage()
{
	auto& msg = receive_msg.logMessage;

	msg.str[sizeof(msg.str) - 1] = '\0';

	char log[2048];

	snprintf(log, 2048, "[SUBPROC] - %s", msg.str);

	LogString(msg.type, log);
}