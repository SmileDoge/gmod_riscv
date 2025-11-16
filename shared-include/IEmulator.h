#pragma once

#include "ILogger.h"
#include "IDevice.h"
#include "GmodDeviceProxy.h"

#include "stdint.h"

enum class EmulatorState
{
    // parent process
    NOT_STARTED,
    WAITING_CHILD_PROCESS,
    READY,
    CHILD_PROCESS_EXITED,

    // child process
    STARTING,
    RUNNING,
};

namespace EmulatorToSubprocess
{
    enum class Type : uint8_t
    {
        CREATE_MACHINE,
        DESTROY_MACHINE,

        CREATE_DEVICE,
    };

    struct CreateMachine
    {
        uint32_t id;
        uint32_t hartCount;
        alignas(8) uint64_t ramCount;
    };

    struct DestroyMachine
    {
        uint32_t id;
    };

    struct CreateDevice
    {
        uint32_t id;
        char deviceType[64];
        char jsonString[1024];
    };
}

namespace EmulatorFromSubprocess
{
    enum class Type : uint8_t
    {
        LOG_MESSAGE,
    };

    struct LogMessage
    {
        ILogType type;
        char str[1024];
    };
}

typedef struct alignas(8)
{
    EmulatorToSubprocess::Type typeToSubprocess;
    EmulatorFromSubprocess::Type typeFromSubprocess;
    
    union {
        EmulatorToSubprocess::CreateMachine createMachine;
        EmulatorToSubprocess::DestroyMachine destroyMachine;
        EmulatorToSubprocess::CreateDevice createDevice;

        EmulatorFromSubprocess::LogMessage logMessage;
    };
} EmulatorMessage;

typedef struct
{
    bool initialized;

    int mainVersion;
    int subprocessVersion;

    bool disableSubprocess;
} EmulatorStatus;

#define EMULATOR_VERSION 1

#define EMULATOR_MAX_MESSAGES 64

#define EMULATOR_SHARED_PATH "RVGMEmulator"
#define EMULATOR_PIPE_PATH   EMULATOR_SHARED_PATH"Pipe"
#define EMULATOR_STATUS_PATH EMULATOR_SHARED_PATH"Status"

class IEmulator {
public:
    virtual ~IEmulator() = default;

    virtual EmulatorState GetState() = 0;
    virtual int GetVersion() = 0;

    virtual IDevice* GetDevice(uint32_t id) = 0;

	virtual ILogger* GetLogger() = 0;

    // gmod only

    virtual GmodDeviceProxy* CheckDeviceProxy(GarrysMod::Lua::ILuaBase* LUA, int index, bool throw_error = true) { return nullptr; };

    virtual GarrysMod::Lua::CFunc GetMetaToString() { return nullptr; };
    virtual GarrysMod::Lua::CFunc GetMetaIndex() { return nullptr; };
};