#include "GmodMachineLua.h"

#include "GmodDeviceLua.h"

GmodEmulator* GmodMachineLua::_emu = nullptr;
int GmodMachineLua::machine_metatable = 0;

void GmodMachineLua::Initialize(GarrysMod::Lua::ILuaBase* LUA, GmodEmulator* emu)
{
	_emu = emu;

	machine_metatable = LUA->CreateMetaTable("GmodMachine");
		SET_LUA_METHOD_MACHINE(IsValid);
		SET_LUA_METHOD_MACHINE(GetID);

		SET_LUA_METHOD_MACHINE(GetHartCount);
		SET_LUA_METHOD_MACHINE(GetRAMCount);
		
		SET_LUA_METHOD_MACHINE(IsRunning);
		SET_LUA_METHOD_MACHINE(IsPowered);

		SET_LUA_METHOD_MACHINE(GetBIOSPath);
		SET_LUA_METHOD_MACHINE(GetKernelPath);
		SET_LUA_METHOD_MACHINE(GetDTBPath);

		SET_LUA_METHOD_MACHINE(AttachDevice);
		SET_LUA_METHOD_MACHINE(GetDevice);

		SET_LUA_METHOD_MACHINE(IsReady);
		SET_LUA_METHOD_MACHINE(WaitForReady);

		SET_LUA_METHOD_MACHINE(SetCommandLine);
		SET_LUA_METHOD_MACHINE(AppendCommandLine);

		SET_LUA_METHOD_MACHINE(SetBIOSPath);
		SET_LUA_METHOD_MACHINE(SetKernelPath);
		SET_LUA_METHOD_MACHINE(SetDTBPath);

		SET_LUA_METHOD_MACHINE(DumpDTB);

		SET_LUA_METHOD_MACHINE(StartResume);
		SET_LUA_METHOD_MACHINE(Pause);
		SET_LUA_METHOD_MACHINE(Restart);
		SET_LUA_METHOD_MACHINE(Shutdown);

		SET_LUA_METHOD_MACHINE(Destroy);

		LUA->PushCFunction(meta__tostring);
		LUA->SetField(-2, "__tostring");

		LUA->Push(-1);
		LUA->SetField(-2, "__index");
	LUA->Pop();
}

int GmodMachineLua::CreateMachineProxy(GarrysMod::Lua::ILuaBase* LUA, GmodMachine* machine)
{
	if (machine)
		LUA->PushUserType_Value<GmodMachineLuaProxy>({ machine->GetID() }, machine_metatable);
	else
		LUA->PushUserType_Value<GmodMachineLuaProxy>({ ((uint32_t)-1) }, machine_metatable);

	return 1;
}

GmodMachine* GmodMachineLua::CheckMachine(GarrysMod::Lua::ILuaBase* LUA, int index, bool throw_error)
{
	LUA->CheckType(index, machine_metatable);

	GmodMachineLuaProxy* proxy = LUA->GetUserType<GmodMachineLuaProxy>(index, machine_metatable);

	if (!proxy)
	{
		if (throw_error)
			LUA->ArgError(index, "GmodMachine expected, got invalid userdata");

		return nullptr;
	}

	GmodMachine* machine = GetEmulator()->GetMachine(proxy->id);

	if (machine)
		return machine;

	if (throw_error)
		LUA->ArgError(index, "GmodMachine expected, got null machine");

	return nullptr;
}

GmodEmulator* GmodMachineLua::GetEmulator()
{
	return _emu;
}

LUA_METHOD_DEFINE_MACHINE(IsValid)
{
	GmodMachine* machine = CheckMachine(LUA, 1, false);

	LUA->PushBool(machine != nullptr);

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(GetID)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	LUA->PushNumber(machine->GetID());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(GetHartCount)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	LUA->PushNumber(machine->GetHartCount());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(GetRAMCount)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	LUA->PushNumber(machine->GetRAMCount());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(IsRunning)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	LUA->PushBool(machine->IsRunning());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(IsPowered)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	LUA->PushBool(machine->IsPowered());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(GetBIOSPath)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	std::string path = "";
	machine->GetBIOSPath(path);
	LUA->PushString(path.c_str());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(GetKernelPath)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	std::string path = "";
	machine->GetKernelPath(path);
	LUA->PushString(path.c_str());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(GetDTBPath)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	std::string path = "";
	machine->GetDTBPath(path);
	LUA->PushString(path.c_str());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(AttachDevice)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	GmodDeviceProxy* dev_proxy = GmodDeviceLua::CheckDeviceProxy(LUA, 2);
	uint64_t addr = LUA->CheckNumber(3);

	if (dev_proxy->machine)
		LUA->ArgError(2, "Device already attached to machine!");

	IDevice* device = dev_proxy->device;

	RV_INFO("machine: %d", machine->GetID());
	RV_INFO("device: %d '%s'", device->GetUniqueID(), device->GetName());

	if (!machine->AttachDevice(dev_proxy, addr))
		LUA->ThrowError("Device attach failed!");

	return 0;
}

LUA_METHOD_DEFINE_MACHINE(GetDevice)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	uint32_t id = LUA->CheckNumber(2);

	GmodDeviceProxy* proxy = machine->GetDeviceProxy(id);

	RV_WARN("PLEASE ADD GmodDeviceLua::CreateDeviceProxyCustom!!!!!!");

	return GmodDeviceLua::CreateDeviceProxyDefault(LUA, proxy);
}

LUA_METHOD_DEFINE_MACHINE(IsReady)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	LUA->PushBool(machine->IsReady());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(WaitForReady)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	int ms = LUA->GetType(2) == GarrysMod::Lua::Type::Number ? LUA->GetNumber(2) : 100;

	machine->WaitForReady(ms);

	return 0;
}

LUA_METHOD_DEFINE_MACHINE(SetCommandLine)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	const char* cmd_line = LUA->CheckString(2);

	LUA->PushBool(machine->SetCommandLine(cmd_line));

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(AppendCommandLine)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	const char* cmd_line = LUA->CheckString(2);

	LUA->PushBool(machine->AppendCommandLine(cmd_line));

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(SetBIOSPath)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	const char* path = LUA->CheckString(2);

	LUA->PushBool(machine->SetBIOSPath(path));

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(SetKernelPath)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	const char* path = LUA->CheckString(2);

	LUA->PushBool(machine->SetKernelPath(path));

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(SetDTBPath)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	const char* path = LUA->CheckString(2);

	LUA->PushBool(machine->SetDTBPath(path));

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(DumpDTB)
{
	GmodMachine* machine = CheckMachine(LUA, 1);
	const char* path = LUA->CheckString(2);

	LUA->PushBool(machine->DumpDTB(path));

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(StartResume)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	LUA->PushBool(machine->StartResume());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(Pause)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	LUA->PushBool(machine->Pause());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(Restart)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	LUA->PushBool(machine->Restart());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(Shutdown)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	LUA->PushBool(machine->Shutdown());

	return 1;
}

LUA_METHOD_DEFINE_MACHINE(Destroy)
{
	GmodMachine* machine = CheckMachine(LUA, 1);

	GetEmulator()->DestroyMachine(machine->GetID());

	return 0;
}

LUA_METHOD_DEFINE_MACHINE(meta__tostring)
{
	GmodMachine* machine = CheckMachine(LUA, 1, false);

	char buffer[256];

	if (machine)
		snprintf(buffer, 256, "GmodMachine: %p (ID: %d)", machine, machine->GetID());
	else
		snprintf(buffer, 256, "GmodMachine: (invalid)");

	LUA->PushString(buffer);

	return 1;
}