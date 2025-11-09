#pragma once

#include "GmodEmulator.h"
#include "GmodMachine.h"

#include <GarrysMod/Lua/Interface.h>

#define LUA_METHOD_DECLARE(name) \
	static int name(lua_State* L) { \
		GarrysMod::Lua::ILuaBase* LUA = L->luabase; \
		LUA->SetState(L); \
		return GmodEmulatorLua::name##__IMPL(LUA); \
	}; \
	static int name##__IMPL(GarrysMod::Lua::ILuaBase* LUA);

#define LUA_METHOD_DEFINE(name) \
int GmodEmulatorLua::name##__IMPL(GarrysMod::Lua::ILuaBase* LUA)
    

class GmodEmulatorLua
{
public:
	static void Initialize(GarrysMod::Lua::ILuaBase* LUA, GmodEmulator* emu);

	//static int CreateMachine(GarrysMod::Lua::ILuaBase* LUA);
	//static int DestroyMachine(GarrysMod::Lua::ILuaBase* LUA);

	//static int CreateDevice(GarrysMod::Lua::ILuaBase* LUA);

	LUA_METHOD_DECLARE(GetState)
	LUA_METHOD_DECLARE(GetVersion)

	LUA_METHOD_DECLARE(CreateMachine)
	LUA_METHOD_DECLARE(DestroyMachine)

	LUA_METHOD_DECLARE(CreateDevice)

	LUA_METHOD_DECLARE(GetMachineByID)
	LUA_METHOD_DECLARE(GetDeviceByID)

	static GmodEmulator* GetEmulator();
private:
	static GmodEmulator* _emu;
};