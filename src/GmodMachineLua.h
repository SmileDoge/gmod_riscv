#pragma once

#include "GmodEmulator.h"
#include "GmodMachine.h"

#include <GarrysMod/Lua/Interface.h>

#define LUA_METHOD_DECLARE_MACHINE(name) \
	static int name(lua_State* L) { \
		GarrysMod::Lua::ILuaBase* LUA = L->luabase; \
		LUA->SetState(L); \
		return GmodMachineLua::name##__IMPL(LUA); \
	}; \
	static int name##__IMPL(GarrysMod::Lua::ILuaBase* LUA);

#define LUA_METHOD_DEFINE_MACHINE(name) \
int GmodMachineLua::name##__IMPL(GarrysMod::Lua::ILuaBase* LUA)

#define SET_LUA_METHOD_MACHINE(name) \
	LUA->PushCFunction(GmodMachineLua::name); \
	LUA->SetField(-2, #name);

class GmodMachineLua
{
public:
	static void Initialize(GarrysMod::Lua::ILuaBase* LUA, GmodEmulator* emu);

	LUA_METHOD_DECLARE_MACHINE(IsValid)
	
	LUA_METHOD_DECLARE_MACHINE(GetID)

	LUA_METHOD_DECLARE_MACHINE(GetHartCount)
	LUA_METHOD_DECLARE_MACHINE(GetRAMCount)
	
	LUA_METHOD_DECLARE_MACHINE(IsRunning)
	LUA_METHOD_DECLARE_MACHINE(IsPowered)

	LUA_METHOD_DECLARE_MACHINE(GetBIOSPath)
	LUA_METHOD_DECLARE_MACHINE(GetKernelPath)
	LUA_METHOD_DECLARE_MACHINE(GetDTBPath)

	LUA_METHOD_DECLARE_MACHINE(AttachDevice)
	LUA_METHOD_DECLARE_MACHINE(GetDevice)

	LUA_METHOD_DECLARE_MACHINE(IsReady)
	LUA_METHOD_DECLARE_MACHINE(WaitForReady)

	LUA_METHOD_DECLARE_MACHINE(SetCommandLine)
	LUA_METHOD_DECLARE_MACHINE(AppendCommandLine)

	LUA_METHOD_DECLARE_MACHINE(SetBIOSPath)
	LUA_METHOD_DECLARE_MACHINE(SetKernelPath)
	LUA_METHOD_DECLARE_MACHINE(SetDTBPath)

	LUA_METHOD_DECLARE_MACHINE(DumpDTB)

	LUA_METHOD_DECLARE_MACHINE(StartResume)
	LUA_METHOD_DECLARE_MACHINE(Pause)
	LUA_METHOD_DECLARE_MACHINE(Restart)
	LUA_METHOD_DECLARE_MACHINE(Shutdown)

	LUA_METHOD_DECLARE_MACHINE(Destroy)

	LUA_METHOD_DECLARE_MACHINE(meta__tostring)

	static int CreateMachineProxy(GarrysMod::Lua::ILuaBase* LUA, GmodMachine* machine);

	static GmodMachine* CheckMachine(GarrysMod::Lua::ILuaBase* LUA, int index, bool throw_error = true);

	static GmodEmulator* GetEmulator();
private:
	static GmodEmulator* _emu;

	static int machine_metatable;
};