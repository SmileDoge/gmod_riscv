#include "SubprocessEmulator.h"

extern "C"
{
#include <rvvmlib.h>

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

#include <devices/ns16550a.h>
}

#ifndef _WIN32
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#endif

SubprocessEmulator* g_Emulator = nullptr;
ILogger* g_Logger = nullptr;

SubprocessEmulator::SubprocessEmulator(int argc, char** argv)
{
	run_event_thread = false;

	state = EmulatorState::STARTING;

	parent_pid = atoi(argv[1]);

	parent_proc = Process::Open(parent_pid);

	receive_msg = EmulatorMessage{};
}

SubprocessEmulator::~SubprocessEmulator()
{
	run_event_thread = false;
	event_thread.join();

	Stop();
}

bool SubprocessEmulator::Start()
{
	uint32_t capacity = EMULATOR_MAX_MESSAGES;
	uint32_t item_size = sizeof(EmulatorMessage);

	g_Logger = this;

#ifdef WIN32
	HANDLE handleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD consoleMode;
	GetConsoleMode(handleOut, &consoleMode);
	consoleMode |= 0x0004;
	consoleMode |= 0x0008;
	SetConsoleMode(handleOut, consoleMode);

	SetConsoleOutputCP(CP_UTF8);

	SetConsoleTitleA("RVVM Subprocess Emulator");
#endif

	if (!InitStatus())
	{
#ifdef _WIN32
		RV_ERROR("InitStatus error - %d\n", GetLastError());
#else
		RV_ERROR("InitStatus error - %d (%s)\n", errno, strerror(errno));
#endif
		return false;
	}

	if (!InitPair(capacity, item_size))
	{
		RV_ERROR("InitPair error\n");
		return false;
	}

	status->subprocessVersion = GetVersion();

	status->initialized = true;

	g_Emulator = this;

	return true;
}

void SubprocessEmulator::Stop()
{
	pair.Close();
	status.Close();
}

EmulatorState SubprocessEmulator::GetState()
{
	return state;
}

int SubprocessEmulator::GetVersion()
{
	return EMULATOR_VERSION;
}

IDevice* SubprocessEmulator::GetDevice(uint32_t id)
{
	if (devices.find(id) == devices.end())
		return nullptr;

	return devices.at(id);

	/*
	for (auto& device : devices)
	{
		if (device->GetUniqueID() == id)
			return device;
	}

	return nullptr;*/
}

ILogger* SubprocessEmulator::GetLogger()
{
	return this;
}

void SubprocessEmulator::LogString(ILogType type, const std::string& string)
{
	if (pair.IsInitialized())
	{
		EmulatorMessage send_msg{};

		send_msg.typeFromSubprocess = EmulatorFromSubprocess::Type::LOG_MESSAGE;

		send_msg.logMessage.type = type;

		snprintf(send_msg.logMessage.str, sizeof(send_msg.logMessage.str), string.c_str());

		pair.PushToMain(&send_msg);
	}

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

void SubprocessEmulator::LogInfo(const char* format, ...)
{
	VAR_ARG()

	LogString(ILogType::LOG_INFO, std::string(zc.data(), zc.size()));
}

void SubprocessEmulator::LogWarn(const char* format, ...)
{
	VAR_ARG()

	LogString(ILogType::LOG_WARN, std::string(zc.data(), zc.size()));
}

void SubprocessEmulator::LogError(const char* format, ...)
{
	VAR_ARG()

	LogString(ILogType::LOG_ERROR, std::string(zc.data(), zc.size()));
}

void SubprocessEmulator::LogDebug(const char* format, ...)
{
	VAR_ARG()

	LogString(ILogType::LOG_DEBUG, std::string(zc.data(), zc.size()));
}

void SubprocessEmulator::LogDebugWithLine(int line, const std::string& file, const char* format, ...)
{
	VAR_ARG()

	LogDebug("[%s:%d] - %s", file.c_str(), line, zc.data());
}

void SubprocessEmulator::CreateMachine(uint32_t id, uint32_t hart_count, uint64_t ram_count)
{
	if (machines.find(id) != machines.end())
	{
		RV_ERROR("Machine with id %d already exists!", id);
		return;
	}

	SubprocessMachine* machine = new SubprocessMachine();

	if (!machine->Initialize(id, hart_count, ram_count))
	{
		RV_ERROR("machine->Initialize error!");
		delete machine;
		return;
	}
	
	machines[id] = machine;

	RV_DEBUG_LINE("Created machine id=%d, hart_count=%d, ram_count=%llu", id, hart_count, ram_count);

	machine->Update();
}

void SubprocessEmulator::DestroyMachine(uint32_t id)
{
	if (machines.find(id) == machines.end())
	{
		RV_ERROR("Machine with id %d does not exist!", id);
		return;
	}

	SubprocessMachine* machine = machines[id];

	/*
	
	std::vector<GmodDeviceProxy*> devices_to_destroy;

	for (auto& [id, device] : devices)
		if (machine->GetDevice(id) != nullptr)
			devices_to_destroy.push_back(machine->GetDeviceProxy(id));

	for (auto& dev : devices_to_destroy)
		DestroyDevice(dev);
	*/

	std::vector<uint32_t> devices_to_destroy;

	machine->Deinitialize();

	for (auto& [id, device] : devices)
		if (machine->GetDevice(id) != nullptr)
			devices_to_destroy.push_back(id);

	for (auto& id : devices_to_destroy)
		DestroyDevice(id);

	delete machine;

	machines.erase(id);

	RV_DEBUG_LINE("Deleted machine id=%d", id);
}

void SubprocessEmulator::DestroyDevice(uint32_t id)
{
	IDevice* device = GetDevice(id);

	if (!device)
		return;

	RV_DEBUG("Device %d '%s' destroyed!", device->GetUniqueID(), device->GetName());

	devices.erase(id);

	device->OnRemove();

	delete device;
}

bool SubprocessEmulator::RegisterDevice(const std::string& type, DeviceFactoryFunc factory)
{
	if (device_manager.HasDevice(type))
		return false;

	return device_manager.RegisterDevice(type.c_str(), factory, nullptr, nullptr);
}

int SubprocessEmulator::StartLoop()
{
	RunEventThread();

	while (true)
	{
		if (!parent_proc.is_running()) break;

		if (status->disableSubprocess) break;

		ProcessMessages();

		for (auto& [id, machine] : machines)
		{
			machine->Update();
		}

		std::this_thread::sleep_for(std::chrono::nanoseconds(100));
	}

	return 0;
}

bool SubprocessEmulator::InitStatus()
{
	if (!status.OpenShared(EMULATOR_STATUS_PATH))
		return false;

	return true;
}

bool SubprocessEmulator::InitPair(uint32_t capacity, uint32_t item_size)
{
	if (!pair.Initialize(EMULATOR_PIPE_PATH, item_size, capacity, true))
	{
		printf("[SubprocessEmulator::InitPair] pair.Initialize error\n");
		return false;
	}

	return true;
}

void SubprocessEmulator::ProcessMessages()
{
	while (pair.PopFromMain(&receive_msg))
	{
		switch (receive_msg.typeToSubprocess)
		{
		case EmulatorToSubprocess::Type::CREATE_MACHINE:
			ProcessCreateMachine();
			break;
		case EmulatorToSubprocess::Type::DESTROY_MACHINE:
			ProcessDestroyMachine();
			break;
		case EmulatorToSubprocess::Type::CREATE_DEVICE:
			ProcessCreateDevice();
			break;
		default:
			RV_WARN("Unknown message type! %d", receive_msg.typeToSubprocess);
			break;
		}
	}
}

void SubprocessEmulator::ProcessCreateMachine()
{
	CreateMachine(
		receive_msg.createMachine.id,
		receive_msg.createMachine.hartCount,
		receive_msg.createMachine.ramCount
	);
}

void SubprocessEmulator::ProcessDestroyMachine()
{
	DestroyMachine(
		receive_msg.destroyMachine.id
	);
}

void SubprocessEmulator::ProcessCreateDevice()
{
	uint32_t id = receive_msg.createDevice.id;
	nlohmann::json json = nlohmann::json::parse(receive_msg.createDevice.jsonString);
	std::string type = receive_msg.createDevice.deviceType;

	if (GetDevice(id))
	{
		RV_ERROR("Received create device with %d id, but already created!", id);
		return;
	}

	RV_DEBUG("Creating device '%s' with id %d", type.c_str(), id);

	IDevice* device = device_manager.CreateDevice(type, id, this);

	if (!device)
	{
		RV_ERROR("device_manager.CreateDevice returned null! type='%s'", type.c_str());

		return;
	}

	if (!device->OnCreate(json))
	{
		RV_ERROR("Error occured on creating %s device!", type.c_str());

		delete device;

		return;
	}

	bool is_created = true;

	device->SetStatus(&is_created, nullptr);

	devices[id] = device;

	RV_DEBUG_LINE("Created device! type: %s, uid: %d", type.c_str(), device->GetUniqueID());
}

void SubprocessEmulator::RunEventThread()
{
	run_event_thread = true;
	event_thread = std::thread(EventThread);
}

void SubprocessEmulator::EventThread()
{
	while (g_Emulator->run_event_thread)
	{
		rvvm_external_tick_eventloop(true);
		std::this_thread::sleep_for(std::chrono::nanoseconds(100));
	}
}
