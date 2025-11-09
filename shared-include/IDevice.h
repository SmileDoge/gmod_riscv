#pragma once

#include "stdint.h"

#include "nlohmann/json.hpp"

#include <GarrysMod/Lua/Interface.h>
#include "SharedPaths.h"

#include "shared_memory_manager.hpp"

#include <chrono>
#include <thread>

#ifdef _WIN32
#define DEVICE_EXPORT extern "C" __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define DEVICE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

enum class DeviceType
{
	BUILTIN, // just proxy from rvvm standard device, rvvm_mmio_dev_t is created inside IDevice
	USER, // custom user device, rvvm_mmio_dev_t is created in IMachine::AttachDevice
};

enum class DeviceRealm
{
	GMOD,
	SUBPROC,
};

class IMachine;
class IEmulator;

typedef struct
{
	bool is_created;
	bool is_attached;
} DeviceStatus;

class IDevice
{
public:
	virtual ~IDevice() = default;

	// Shared methods

	virtual DeviceType GetType() = 0;
	virtual DeviceRealm GetRealm() = 0;
	virtual const char* GetName() = 0;
	virtual uint32_t GetUniqueID() = 0;
	virtual IEmulator* GetEmulator() = 0;

	virtual size_t GetSize() = 0;

	virtual void* GetMapping() { return nullptr; };

	virtual bool OnCreate(nlohmann::json& json) { return true; };

	virtual void OnAttach(IMachine* machine, uint64_t addr) { };
	virtual void OnRemove() {};

	// Gmod/Main realm

	virtual void OnAttachToSubprocess() {};

	virtual void SetStatus(bool* is_created, bool* is_attached) {};

	virtual bool IsCreatedOnSubprocess() { return false; };
	virtual bool IsAttachedOnSubprocess() { return false; };

	void WaitForCreate(int timeout_ms)
	{
		int waited = 0;
		const int sleep_interval = 10;

		while (!IsCreatedOnSubprocess() && (timeout_ms < 0 || waited < timeout_ms))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_interval));
			waited += sleep_interval;
		}
	}

	void WaitForAttach(int timeout_ms)
	{
		int waited = 0;
		const int sleep_interval = 10;

		while (!IsAttachedOnSubprocess() && (timeout_ms < 0 || waited < timeout_ms))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_interval));
			waited += sleep_interval;
		}
	}

	// Subprocess realm

	virtual bool OnWrite(void* data, size_t offset, uint8_t size) = 0;
	virtual bool OnRead(void* data, size_t offset, uint8_t size) = 0;

	virtual void OnUpdate() {};
	virtual void OnReset() {};

	// rvvm_mmio_dev_t
	virtual void* GetRawDev() = 0;
	virtual void SetRawDev(void* dev) = 0;

	virtual void* GetCreatedDev() { return nullptr; };

	static std::string GetSharedStatus(uint32_t id, const std::string& purpose = "General")
	{
		std::string out = DEVICE_PATH;

		out += std::to_string(id);

		out += "Status";

		out += purpose;

		return out;
	}

	static std::string GetSharedPipe(uint32_t id, const std::string& purpose = "General")
	{
		std::string out = DEVICE_PATH;

		out += std::to_string(id);

		out += "Pipe";

		out += purpose;

		return out;
	}
};

typedef struct
{
	uint32_t id;

	IMachine* machine;
} GmodDeviceLuaProxy;

#define CHECK_DEVICE_WITH_EMULATOR(class_name, index, emulator) ((class_name*)(emulator->CheckDeviceProxy(LUA, index)))
#define GET_DEVICE_WITH_EMULATOR(class_name, index, emulator) ((class_name*)(emulator->CheckDeviceProxy(LUA, index, false)))

#define CHECK_DEVICE(class_name, index) CHECK_DEVICE_WITH_EMULATOR(class_name, index, g_Emulator)
#define GET_DEVICE(class_name, index) GET_DEVICE_WITH_EMULATOR(class_name, index, g_Emulator)

#define DEVICE_DECLARE_METHOD(class_name, name) \
	static int name(lua_State* L) { \
		GarrysMod::Lua::ILuaBase* LUA = L->luabase; \
		LUA->SetState(L); \
		return class_name::name##__IMPL(LUA); \
	}; \
	static int name##__IMPL(GarrysMod::Lua::ILuaBase* LUA);

#define DEVICE_CREATE_METHOD(class_name, name) \
int class_name::name##__IMPL(GarrysMod::Lua::ILuaBase* LUA)

#define DEVICE_SET_LUA_METHOD(class_name, name) \
	LUA->PushCFunction(class_name::name); \
	LUA->SetField(-2, #name);

#define DECLARE_REGISTER_FUNC(class_name) \
int Register##class_name(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA);

#define DECLARE_UNREGISTER_FUNC(class_name) \
void Unregister##class_name(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA);

#define CREATE_REGISTER_FUNC(class_name) \
int Register##class_name(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA)

#define CREATE_UNREGISTER_FUNC(class_name) \
void Unregister##class_name(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA)

#define DECLARE_REGISTER_FUNC_DLL(class_name) \
DEVICE_EXPORT int RegisterDevice(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA);

#define DECLARE_UNREGISTER_FUNC_DLL(class_name) \
DEVICE_EXPORT void UnregisterDevice(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA);

#define CREATE_REGISTER_FUNC_DLL(class_name) \
DEVICE_EXPORT int RegisterDevice(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA)

#define CREATE_UNREGISTER_FUNC_DLL(class_name) \
DEVICE_EXPORT void UnregisterDevice(IEmulator* emulator, GarrysMod::Lua::ILuaBase* LUA)

#define DECLARE_FACTORY(class_name) \
extern ILogger* g_Logger; \
IDevice* Create##class_name(uint32_t id, IEmulator* emulator, DeviceRealm realm);

#define CREATE_FACTORY(class_name) \
IDevice* Create##class_name(uint32_t id, IEmulator* emulator, DeviceRealm realm) \
{ \
	auto dev = new class_name(realm, id, emulator); \
	return dev; \
}

#define DECLARE_DLL_FACTORY(class_name) \
extern ILogger* g_Logger; \
DEVICE_EXPORT IDevice* CreateDevice(uint32_t id, IEmulator* emulator, DeviceRealm realm);

#define CREATE_DLL_FACTORY(class_name) \
DEVICE_EXPORT IDevice* CreateDevice(uint32_t id, IEmulator* emulator, DeviceRealm realm) \
{ \
	auto dev = new class_name(realm, id, emulator); \
	g_Logger = emulator->GetLogger(); \
	return dev; \
}
/*

	virtual void SetStatus(bool* is_created, bool* is_attached) {};

	virtual bool IsCreatedOnSubprocess() { return false; };
	virtual bool IsAttachedOnSubprocess() { return false; };*/
#define BEGIN_DEVICE(class_name, str_name, type) \
class class_name : public IDevice \
{ \
public: \
	class_name(DeviceRealm realm, uint32_t unid, IEmulator* emu) : _unid(unid), _realm(realm), _emu(emu), _dev(nullptr) { \
		if (realm == DeviceRealm::GMOD) _status.CreateShared(GetSharedStatus(unid, "Status")); else _status.OpenShared(GetSharedStatus(unid, "Status")); \
		_status->is_created = false; \
		_status->is_attached = false; \
	} \
	~class_name() override {} \
	virtual DeviceType GetType() override { return type; } \
	virtual DeviceRealm GetRealm() override { return _realm; } \
	virtual const char* GetName() override { return str_name; } \
	virtual uint32_t GetUniqueID() override { return _unid; } \
	virtual IEmulator* GetEmulator() override { return _emu; } \
	virtual void SetStatus(bool* is_created, bool* is_attached) override { if (is_created) _status->is_created = *is_created; if (is_attached) _status->is_attached = *is_attached; } \
	virtual bool IsCreatedOnSubprocess() override { return _status->is_created; } \
	virtual bool IsAttachedOnSubprocess() override { return _status->is_attached; } \
	virtual void* GetRawDev() { return _dev; } \
	virtual void SetRawDev(void* dev) { _dev = dev; }

#define BEGIN_USER_DEVICE(class_name, str_name) BEGIN_DEVICE(class_name, str_name, DeviceType::USER)

#define BEGIN_BUILTIN_DEVICE(class_name, str_name) \
BEGIN_DEVICE(class_name, str_name, DeviceType::BUILTIN) \
	virtual void* GetCreatedDev() override { return _dev; } \

#define END_DEVICE() \
private: \
	void* _dev; \
	IEmulator* _emu; \
	DeviceRealm _realm; \
	uint32_t _unid; \
	SharedMemory<DeviceStatus> _status; \
};

