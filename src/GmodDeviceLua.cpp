#include "GmodDeviceLua.h"

#include "GmodMachineLua.h"

GmodEmulator* GmodDeviceLua::_emu = nullptr;
int GmodDeviceLua::device_metatable = 0;

void GmodDeviceLua::Initialize(GarrysMod::Lua::ILuaBase* LUA, GmodEmulator* emu)
{
	_emu = emu;

	device_metatable = LUA->CreateMetaTable("GmodDeviceBase");
		SET_LUA_METHOD_DEVICE(IsValid);


		SET_LUA_METHOD_DEVICE(GetType);
		SET_LUA_METHOD_DEVICE(GetID);
		SET_LUA_METHOD_DEVICE(GetName);
		SET_LUA_METHOD_DEVICE(GetSize);

		SET_LUA_METHOD_DEVICE(GetAddress);
		SET_LUA_METHOD_DEVICE(GetMachine);

		SET_LUA_METHOD_DEVICE(IsCreatedOnSubprocess);
		SET_LUA_METHOD_DEVICE(IsAttachedOnSubprocess);

		SET_LUA_METHOD_DEVICE(WaitForCreate);
		SET_LUA_METHOD_DEVICE(WaitForAttach);

		LUA->PushCFunction(meta__tostring);
		LUA->SetField(-2, "__tostring");
	
		LUA->Push(-1);
		LUA->SetField(-2, "__index");
	LUA->Pop();

	LUA->PushMetaTable(device_metatable);
		LUA->PushNumber(device_metatable);
		LUA->SetField(-2, "__device_type");
	LUA->Pop();
}

int GmodDeviceLua::GetDeviceMetatable()
{
	return device_metatable;
}

GmodDeviceProxy* GmodDeviceLua::CheckDeviceProxy(GarrysMod::Lua::ILuaBase* LUA, int index, bool throw_error)
{
	if (!LUA->IsType(index, GarrysMod::Lua::Type::UserData))
	{
		if (throw_error)
			LUA->ArgError(index, "Device expected, got invalid userdata");
		
		return nullptr;
	}

	LUA->GetField(index, "__device_type");

	if (!LUA->IsType(-1, GarrysMod::Lua::Type::Number))
	{
		if (throw_error)
			LUA->ArgError(index, "Uncorrect userdata passed!");

		LUA->Pop();
		return nullptr;
	}

	int custom_device_mt_index = LUA->GetNumber();

	LUA->Pop();

	GmodDeviceLuaProxy* lua_proxy = LUA->GetUserType<GmodDeviceLuaProxy>(index, custom_device_mt_index);

	GmodDeviceProxy* proxy = GetEmulator()->GetDeviceProxy(lua_proxy->id);

	if (!proxy && throw_error)
	{
		LUA->ArgError(index, "Device expected, got null device");

		return nullptr;
	}

	return proxy;
}

int GmodDeviceLua::CreateDeviceProxyDefault(GarrysMod::Lua::ILuaBase* LUA, GmodDeviceProxy* dev_proxy)
{
	//if (machine)
		//LUA->PushUserType_Value<GmodMachineLuaProxy>({ machine->GetID() }, machine_metatable);
	//else
		//LUA->PushUserType_Value<GmodMachineLuaProxy>({ ((uint32_t)-1) }, machine_metatable);

	if (dev_proxy)
		LUA->PushUserType_Value<GmodDeviceLuaProxy>({ dev_proxy->id, dev_proxy->machine }, device_metatable);
	else
		LUA->PushUserType_Value<GmodDeviceLuaProxy>({ ((uint32_t)-1), nullptr }, device_metatable);

	return 1;
}

int GmodDeviceLua::CreateDeviceProxyCustom(GarrysMod::Lua::ILuaBase* LUA, GmodDeviceProxy* dev_proxy, int metatable_index)
{
	if (dev_proxy)
		LUA->PushUserType_Value<GmodDeviceLuaProxy>({ dev_proxy->id, dev_proxy->machine }, metatable_index);
	else
		LUA->PushUserType_Value<GmodDeviceLuaProxy>({ ((uint32_t)-1), nullptr }, metatable_index);

	return 1;
}

GmodEmulator* GmodDeviceLua::GetEmulator()
{
	return _emu;
}

LUA_METHOD_DEFINE_DEVICE(IsValid)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1, false);

	LUA->PushBool(device != nullptr);

	return 1;
}

LUA_METHOD_DEFINE_DEVICE(GetType)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1);

	LUA->PushNumber(static_cast<double>(device->device->GetType()));

	return 1;
}


LUA_METHOD_DEFINE_DEVICE(GetID)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1);

	LUA->PushNumber(device->id);

	return 1;
}

LUA_METHOD_DEFINE_DEVICE(GetName)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1);

	LUA->PushString(device->device->GetName());

	return 1;
}

LUA_METHOD_DEFINE_DEVICE(GetSize)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1);

	LUA->PushNumber(device->device->GetSize());

	return 1;
}

LUA_METHOD_DEFINE_DEVICE(GetAddress)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1);

	LUA->PushNumber(device->address);

	return 1;
}

LUA_METHOD_DEFINE_DEVICE(GetMachine)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1);

	return GmodMachineLua::CreateMachineProxy(LUA, (GmodMachine*)device->machine);
}

LUA_METHOD_DEFINE_DEVICE(IsCreatedOnSubprocess)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1);

	LUA->PushBool(device->device->IsCreatedOnSubprocess());

	return 1;
}

LUA_METHOD_DEFINE_DEVICE(IsAttachedOnSubprocess)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1);

	LUA->PushBool(device->device->IsAttachedOnSubprocess());

	return 1;
}

LUA_METHOD_DEFINE_DEVICE(WaitForCreate)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1);
	int ms = LUA->GetType(2) == GarrysMod::Lua::Type::Number ? LUA->GetNumber(2) : 200;

	device->device->WaitForCreate(ms);

	return 0;
}

LUA_METHOD_DEFINE_DEVICE(WaitForAttach)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1);
	int ms = LUA->GetType(2) == GarrysMod::Lua::Type::Number ? LUA->GetNumber(2) : 200;

	device->device->WaitForAttach(ms);

	return 0;
}

LUA_METHOD_DEFINE_DEVICE(meta__tostring)
{
	GmodDeviceProxy* device = CheckDeviceProxy(LUA, 1, false);

	std::string name = "GmodDevice";

	if (LUA->GetMetaTable(1))
	{
		LUA->GetField(-1, "MetaName");
		name = LUA->GetString();
		LUA->Pop(2);
	}

	char buffer[512];

	if (device)
		if (device->machine)
			snprintf(buffer, 512, "%s: %p:%p (ID: %d) attached to machine (ID: %d) | '%s'", name.c_str(), device->address, (uint64_t)device->device->GetSize(), device->id, device->machine->GetID(), device->device->GetName());
		else
			snprintf(buffer, 512, "%s: (ID: %d) | '%s'", name.c_str(), device->id, device->device->GetName());
	else
		snprintf(buffer, 512, "%s: (invalid)", name.c_str());

	LUA->PushString(buffer);

	return 1;
}

LUA_METHOD_DEFINE_DEVICE(meta__index)
{
	if (LUA->GetMetaTable(1))
	{
		LUA->Push(2); // ебашим 2 аргумент, то есть ключ на вверх стека
		LUA->RawGet(-2); // на -2 находится метатаблица, по идее кастомного девайса, иначе базового девайса, иначе нихуя ¯\_(ツ)_/¯

		if (!LUA->IsType(-1, GarrysMod::Lua::Type::Nil)) // все гуд возвращаем то что нашли
			return 1;

		// все бед
		LUA->PushMetaTable(device_metatable); // пушим дефолт метатаблицу

		LUA->Push(2); // та же процедура
		LUA->RawGet(-2);

		return 1; // либо nil, либо то что нашли, дальше ничего
	}

	return 0;
}