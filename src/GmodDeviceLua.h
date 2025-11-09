#pragma once

#include <GarrysMod/Lua/Interface.h>

#include "GmodEmulator.h"
#include "GmodMachine.h"
#include "IDevice.h"

#define LUA_METHOD_DECLARE_DEVICE(name) \
	static int name(lua_State* L) { \
		GarrysMod::Lua::ILuaBase* LUA = L->luabase; \
		LUA->SetState(L); \
		return GmodDeviceLua::name##__IMPL(LUA); \
	}; \
	static int name##__IMPL(GarrysMod::Lua::ILuaBase* LUA);

#define LUA_METHOD_DEFINE_DEVICE(name) \
int GmodDeviceLua::name##__IMPL(GarrysMod::Lua::ILuaBase* LUA)

#define SET_LUA_METHOD_DEVICE(name) \
	LUA->PushCFunction(GmodDeviceLua::name); \
	LUA->SetField(-2, #name);

class GmodDeviceLua
{
public:
	static void Initialize(GarrysMod::Lua::ILuaBase* LUA, GmodEmulator* emu);

	LUA_METHOD_DECLARE_DEVICE(IsValid)

	LUA_METHOD_DECLARE_DEVICE(GetType)
	LUA_METHOD_DECLARE_DEVICE(GetID)
	LUA_METHOD_DECLARE_DEVICE(GetName)
	LUA_METHOD_DECLARE_DEVICE(GetSize)

	LUA_METHOD_DECLARE_DEVICE(GetAddress)
	LUA_METHOD_DECLARE_DEVICE(GetMachine)

	LUA_METHOD_DECLARE_DEVICE(IsCreatedOnSubprocess)
	LUA_METHOD_DECLARE_DEVICE(IsAttachedOnSubprocess)

	LUA_METHOD_DECLARE_DEVICE(WaitForCreate)
	LUA_METHOD_DECLARE_DEVICE(WaitForAttach)

	LUA_METHOD_DECLARE_DEVICE(meta__tostring)
	LUA_METHOD_DECLARE_DEVICE(meta__index)

	static GmodDeviceProxy* CheckDeviceProxy(GarrysMod::Lua::ILuaBase* LUA, int index, bool throw_error = true);

	static int CreateDeviceProxyDefault(GarrysMod::Lua::ILuaBase* LUA, GmodDeviceProxy* dev_proxy);
	static int CreateDeviceProxyCustom(GarrysMod::Lua::ILuaBase* LUA, GmodDeviceProxy* dev_proxy, int metatable_index);

	static GmodEmulator* GetEmulator();

	static int GetDeviceMetatable();
private:
	static GmodEmulator* _emu;
	static int device_metatable;
};