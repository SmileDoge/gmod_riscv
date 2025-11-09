#include "GmodEmulatorLua.h"

#include "GmodMachineLua.h"
#include "GmodDeviceLua.h"

GmodEmulator* GmodEmulatorLua::_emu = nullptr;

void GmodEmulatorLua::Initialize(GarrysMod::Lua::ILuaBase* LUA, GmodEmulator* emu)
{
    _emu = emu;

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
        LUA->CreateTable();
			LUA->PushCFunction(Start);
			LUA->SetField(-2, "Start");

			LUA->PushCFunction(Stop);
			LUA->SetField(-2, "Stop");

			LUA->PushCFunction(GetState);
			LUA->SetField(-2, "GetState");

			LUA->PushCFunction(GetVersion);
			LUA->SetField(-2, "GetVersion");

			LUA->PushCFunction(CreateMachine);
			LUA->SetField(-2, "CreateMachine");

			LUA->PushCFunction(DestroyMachine);
			LUA->SetField(-2, "DestroyMachine");

			LUA->PushCFunction(CreateDevice);
			LUA->SetField(-2, "CreateDevice");

			LUA->PushCFunction(GetMachineByID);
			LUA->SetField(-2, "GetMachineByID");

			LUA->PushCFunction(GetDeviceByID);
			LUA->SetField(-2, "GetDeviceByID");
		LUA->SetField(-2, "riscv");
    LUA->Pop();
}

GmodEmulator* GmodEmulatorLua::GetEmulator()
{
    return _emu;
}

LUA_METHOD_DEFINE(Start)
{
	EmulatorState state = GetEmulator()->GetState();

	if (state == EmulatorState::NOT_STARTED || state == EmulatorState::CHILD_PROCESS_EXITED)
		LUA->PushBool(GetEmulator()->Start(LUA, LUA->GetString(1)));
	else
		LUA->ThrowError("GmodEmulator already started!");

	return 1;
}

LUA_METHOD_DEFINE(Stop)
{
	EmulatorState state = GetEmulator()->GetState();

	if (state != EmulatorState::NOT_STARTED)
		GetEmulator()->Stop();
	else
		LUA->ThrowError("GmodEmulator already stopped!");

	return 0;
}

LUA_METHOD_DEFINE(GetState)
{
	EmulatorState state = GetEmulator()->GetState();
	
	LUA->PushNumber(static_cast<uint32_t>(state));

	return 1;
}

LUA_METHOD_DEFINE(GetVersion)
{
	LUA->PushNumber(GetEmulator()->GetVersion());

	return 1;
}

LUA_METHOD_DEFINE(CreateMachine)
{
    uint32_t id = LUA->CheckNumber(1);
	uint32_t hart_count = LUA->CheckNumber(2);
	uint64_t ram_count = static_cast<uint64_t>(LUA->CheckNumber(3));

	GmodMachine* machine = GetEmulator()->CreateMachine(id, hart_count, ram_count);

	if (!machine)
	{
		RV_ERROR("Created machine is null!");

		return 0;
	}

    return GmodMachineLua::CreateMachineProxy(LUA, machine);
}

LUA_METHOD_DEFINE(DestroyMachine)
{
	uint32_t id = LUA->CheckNumber(1);
	GetEmulator()->DestroyMachine(id);
	return 0;
}

LUA_METHOD_DEFINE(CreateDevice)
{
	//assert(false && "Not implemented yet!");

	std::string type = LUA->CheckString(1);
	LUA->CheckType(2, GarrysMod::Lua::Type::Table);

	std::string json_txt;

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->GetField(-1, "util");
			LUA->GetField(-1, "TableToJSON");
				LUA->Push(2);
			LUA->Call(1, 1);

			json_txt = LUA->GetString(-1);
			
			LUA->Pop();
		LUA->Pop();
	LUA->Pop();

	RV_DEBUG("JSON String: %s", json_txt.c_str());

	nlohmann::json json = nlohmann::json::parse(json_txt);

	GmodDeviceProxy* dev_proxy = GetEmulator()->CreateDevice(type, json);

	int metatable_index = GetEmulator()->GetDeviceMetaTableIndex(type);

	if (metatable_index)
	{
		return GmodDeviceLua::CreateDeviceProxyCustom(LUA, dev_proxy, metatable_index);
	}

	return GmodDeviceLua::CreateDeviceProxyDefault(LUA, dev_proxy);
}

LUA_METHOD_DEFINE(GetMachineByID)
{
	uint32_t id = LUA->CheckNumber(1);
	GmodMachine* machine = GetEmulator()->GetMachine(id);

	return GmodMachineLua::CreateMachineProxy(LUA, machine);
}

LUA_METHOD_DEFINE(GetDeviceByID)
{
	uint32_t id = LUA->CheckNumber(1);
	GmodDeviceProxy* device = GetEmulator()->GetDeviceProxy(id);

	assert(false && "Not implemented yet!");
	return 0; // TODO
}