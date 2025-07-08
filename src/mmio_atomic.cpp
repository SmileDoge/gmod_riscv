#include "mmio_atomic.h"

#include <stdio.h>

#include <mutex>

#include <Windows.h>

#include <gmod_machine.h>

struct atomic_mutex
{
	LONG m;

	atomic_mutex() : m(0) {}

	void lock()
	{
		while (InterlockedCompareExchange(&m, 1, 0) != 0)
		{
			Sleep(0);
		}
	}

	void unlock()
	{
		InterlockedExchange(&m, 0);
	}
};

struct mmio_atomic_t
{
	rvvm_mmio_dev_t* mmio;

	bool is_atomic_op_rvvm;
	bool is_atomic_op_gmod;
	//std::mutex mem_mutex;
	atomic_mutex mem_mutex;

	void* mem;
};

static bool mmio_atomic_write(rvvm_mmio_dev_t* dev, void* data, size_t offset, uint8_t size)
{
	mmio_atomic_t* dev_atomic = (mmio_atomic_t*)dev->data;

	if (offset < 4)
	{
		if (size == 1)
		{
			switch (offset)
			{
			case 0: // use atomic
				dev_atomic->is_atomic_op_rvvm = ((*(uint8_t*)data) > 0) ? true : false;
				break;
			case 1: // lock mutex
				dev_atomic->mem_mutex.lock();
				break;
			case 2: // unlock mutex
				dev_atomic->mem_mutex.unlock();
				break;
			case 3: // nop
				break;
			default:
				break;
			}
		}

		return true;
	}
	else
	{
		offset -= 4;
	}

	if (dev_atomic->is_atomic_op_rvvm)
	{

		switch (size)
		{
		case 1:
			InterlockedExchange8((volatile char*)dev_atomic->mem + offset, *(char*)data);
			break;
		case 2:
			InterlockedExchange16((volatile short*)dev_atomic->mem + offset, *(short*)data);
			break;
		case 4:
			InterlockedExchange((volatile unsigned int*)dev_atomic->mem + offset, *(unsigned int*)data);
			break;
		default:
			return false;
			break;
		}
	}
	else
	{
		switch (size)
		{
		case 1:
			memcpy((char*)dev_atomic->mem + offset, data, size);
			break;
		case 2:
			memcpy((char*)dev_atomic->mem + offset, data, size);
			break;
		case 4:
			memcpy((char*)dev_atomic->mem + offset, data, size);
			break;
		case 8:
			memcpy((char*)dev_atomic->mem + offset, data, size);
			break;
		default:
			return false;
			break;
		}
	}

	return true;
}

static bool mmio_atomic_read(rvvm_mmio_dev_t* dev, void* data, size_t offset, uint8_t size)
{
	mmio_atomic_t* dev_atomic = (mmio_atomic_t*)dev->data;

	if (offset < 4)
	{
		if (size != 1)
		{
			*(uint8_t*)data = 0;
			return true;
		}

		switch (offset)
		{
		case 0: // use atomic
			*(uint8_t*)data = dev_atomic->is_atomic_op_rvvm ? 1 : 0;
			break;
		case 1: // lock mutex - read nop
			break;
		case 2: // unlock mutex - read nop
			break;
		case 3: // nop
			break;
		default:
			break;
		}

		return true;
	}
	else
	{
		offset -= 4;
	}

	if (dev_atomic->is_atomic_op_rvvm)
	{
		switch (size)
		{
		case 1:
			*(char*)data = _InterlockedCompareExchange8((volatile char*)dev_atomic->mem + offset, 0, 0);
			break;
		case 2:
			*(short*)data = _InterlockedCompareExchange16((volatile short*)dev_atomic->mem + offset, 0, 0);
			break;
		case 4:
			*(unsigned int*)data = _InterlockedCompareExchange((volatile unsigned int*)dev_atomic->mem + offset, 0, 0);
			break;
		default:
			return false;
			break;
		}
	}
	else
	{
		switch (size)
		{
		case 1:
			*(char*)data = *((char*)dev_atomic->mem + offset);
			break;
		case 2:
			*(short*)data = *((short*)dev_atomic->mem + offset);
			break;
		case 4:
			*(unsigned int*)data = *((unsigned int*)dev_atomic->mem + offset);
			break;
		case 8:
			*(uint64_t*)data = *((uint64_t*)dev_atomic->mem + offset);
			break;
		default:
			return false;
			break;
		}
	}

	return true;
}

static void mmio_atomic_update(rvvm_mmio_dev_t* dev)
{
	return;
}

static void mmio_atomic_remove(rvvm_mmio_dev_t* dev)
{
	mmio_atomic_t* mmio_atomic = (mmio_atomic_t*)dev->data;

	if (mmio_atomic)
	{
		if (mmio_atomic->mem)
		{
			_aligned_free(mmio_atomic->mem);
			mmio_atomic->mem = nullptr;
		}
		delete mmio_atomic;
	}
}

static const rvvm_mmio_type_t mmio_atomic_type = { "mmio_atomic", mmio_atomic_remove, mmio_atomic_update, 0 };


mmio_atomic_t* mmio_atomic_init(rvvm_machine_t* machine, mmio_atomic_params_t params)
{
	mmio_atomic_t* mmio_atomic = new mmio_atomic_t();

	rvvm_mmio_dev_t mmio_desc = { 0 };

	mmio_desc.addr = 0x12100000;
	mmio_desc.size = params.size;
	mmio_desc.read = mmio_atomic_read;
	mmio_desc.write = mmio_atomic_write;
	mmio_desc.data = mmio_atomic;
	mmio_desc.type = &mmio_atomic_type;

	rvvm_mmio_dev_t* mmio = rvvm_attach_mmio(machine, &mmio_desc);

	if (!mmio)
	{
		delete mmio_atomic;
		return nullptr;
	}

	mmio_atomic->mmio = mmio;
	mmio_atomic->mem = _aligned_malloc(params.size, alignof(std::atomic<uint32_t>));
	mmio_atomic->is_atomic_op_rvvm = true;
	mmio_atomic->is_atomic_op_gmod = true;

	if (!mmio_atomic->mem)
	{
		rvvm_remove_mmio(mmio);

		return nullptr;
	}

	memset(mmio_atomic->mem, 0, params.size);

	return mmio_atomic;
}

void mmio_atomic_read(mmio_atomic_t* dev, void* data, size_t offset, uint8_t size)
{

	if (dev->is_atomic_op_gmod)
	{
		switch (size)
		{
		case 1:
			*(char*)data = _InterlockedCompareExchange8((volatile char*)dev->mem + offset, 0, 0);
			break;
		case 2:
			*(short*)data = _InterlockedCompareExchange16((volatile short*)dev->mem + offset, 0, 0);
			break;
		case 4:
			*(unsigned int*)data = _InterlockedCompareExchange((volatile unsigned int*)dev->mem + offset, 0, 0);
			break;
		default:
			memcpy(data, (char*)dev->mem + offset, size); // no atomic
			break;
		}
	}
	else
	{
		switch (size)
		{
		case 1:
			*(char*)data = *((char*)dev->mem + offset);
			break;
		case 2:
			*(short*)data = *((short*)dev->mem + offset);
			break;
		case 4:
			*(unsigned int*)data = *((unsigned int*)dev->mem + offset);
			break;
		default:
			memcpy(data, (char*)dev->mem + offset, size);
			break;
		}
	}
}

void mmio_atomic_write(mmio_atomic_t* dev, void* data, size_t offset, uint8_t size)
{
	if (dev->is_atomic_op_gmod)
	{
		switch (size)
		{
		case 1:
			InterlockedExchange8((volatile char*)dev->mem + offset, *(char*)data);
			break;
		case 2:
			InterlockedExchange16((volatile short*)dev->mem + offset, *(short*)data);
			break;
		case 4:
			InterlockedExchange((volatile unsigned int*)dev->mem + offset, *(unsigned int*)data);
			break;
		default:
			memcpy((char*)dev->mem + offset, data, size); // no atomic
			break;
		}
	}
	else
	{
		switch (size)
		{
		case 1:
			*((char*)dev->mem + offset) = *(char*)data;
			break;
		case 2:
			*((short*)dev->mem + offset) = *(short*)data;
			break;
		case 4:
			*((unsigned int*)dev->mem + offset) = *(unsigned int*)data;
			break;
		default:
			memcpy((char*)dev->mem + offset, data, size);
			break;
		}
	}
}

void mmio_atomic_set_use_atomic(mmio_atomic_t* dev, bool use_atomic)
{
	dev->is_atomic_op_gmod = use_atomic;
}

bool mmio_atomic_get_use_atomic(mmio_atomic_t* dev)
{
	return dev->is_atomic_op_gmod;
}

void mmio_atomic_mutex_lock(mmio_atomic_t* dev)
{
	dev->mem_mutex.lock();
}

void mmio_atomic_mutex_unlock(mmio_atomic_t* dev)
{
	dev->mem_mutex.unlock();
}

static int mmio_atomic_mt = 0;

#define ATOMIC_FUNCTION_READ(name, type) \
LUA_FUNCTION(atomic_read##name) \
{ \
	mmio_atomic_t* atomic = LUA->GetUserType<mmio_atomic_t>(1, mmio_atomic_mt); \
	int offset = LUA->CheckNumber(2); \
	if (!atomic) return 0; \
	type data = 0; \
	mmio_atomic_read(atomic, &data, offset, sizeof(type)); \
	LUA->PushNumber(data); \
	return 1; \
}

#define ATOMIC_FUNCTION_WRITE(name, type) \
LUA_FUNCTION(atomic_write##name) \
{ \
	mmio_atomic_t* atomic = LUA->GetUserType<mmio_atomic_t>(1, mmio_atomic_mt); \
	int offset = LUA->CheckNumber(2); \
	type data = (type)LUA->CheckNumber(3); \
	if (!atomic) return 0; \
	mmio_atomic_write(atomic, &data, offset, sizeof(type)); \
	return 0; \
}

#define ATOMIC_PUSH_FUNCTION_READ(name, lua_name) \
LUA->PushCFunction(atomic_read##name); \
LUA->SetField(-2, "Read"##lua_name);

#define ATOMIC_PUSH_FUNCTION_WRITE(name, lua_name) \
LUA->PushCFunction(atomic_write##name); \
LUA->SetField(-2, "Write"##lua_name);

ATOMIC_FUNCTION_READ(int8, int8_t)
ATOMIC_FUNCTION_READ(int16, int16_t)
ATOMIC_FUNCTION_READ(int32, int32_t)

ATOMIC_FUNCTION_READ(uint8, uint8_t)
ATOMIC_FUNCTION_READ(uint16, uint16_t)
ATOMIC_FUNCTION_READ(uint32, uint32_t)

ATOMIC_FUNCTION_WRITE(int8, int8_t)
ATOMIC_FUNCTION_WRITE(int16, int16_t)
ATOMIC_FUNCTION_WRITE(int32, int32_t)

ATOMIC_FUNCTION_WRITE(uint8, uint8_t)
ATOMIC_FUNCTION_WRITE(uint16, uint16_t)
ATOMIC_FUNCTION_WRITE(uint32, uint32_t)

ATOMIC_FUNCTION_READ(float, float)
ATOMIC_FUNCTION_WRITE(float, float)

LUA_FUNCTION(atomic_readzstring)
{
	mmio_atomic_t* atomic = LUA->GetUserType<mmio_atomic_t>(1, mmio_atomic_mt);
	int offset = LUA->CheckNumber(2);
	if (!atomic) return 0;

	int l_offset = offset;

	while (true)
	{
		char c = 0;
		mmio_atomic_read(atomic, &c, l_offset, sizeof(char));
		if (c == 0) break;
		l_offset++;
	}

	unsigned int size = l_offset - offset;

	char* data = new char[size + 1];

	mmio_atomic_read(atomic, data, offset, size);

	data[size] = 0;

	LUA->PushString(data, size);

	delete[] data;

	return 1;
}

LUA_FUNCTION(atomic_readdata)
{
	mmio_atomic_t* atomic = LUA->GetUserType<mmio_atomic_t>(1, mmio_atomic_mt);
	int offset = LUA->CheckNumber(2);
	unsigned int size = LUA->CheckNumber(3);
	if (!atomic) return 0;

	char* data = new char[size];

	mmio_atomic_read(atomic, data, offset, size);

	LUA->PushString(data, size);

	delete[] data;

	return 1;
}

LUA_FUNCTION(atomic_writedata)
{
	mmio_atomic_t* atomic = LUA->GetUserType<mmio_atomic_t>(1, mmio_atomic_mt);
	int offset = LUA->CheckNumber(2);
	LUA->CheckType(3, GarrysMod::Lua::Type::String);

	unsigned int size = 0;
	const char* data = LUA->GetString(3, &size);

	if (!atomic) return 0;

	mmio_atomic_write(atomic, (void*)data, offset, size);

	return 0;
}

LUA_FUNCTION(mmio_atomic_create)
{
	int id = LUA->CheckNumber(1);
	unsigned int size = LUA->CheckNumber(2);

	gmod_machine_t* machine = get_machine(id);

	if (!machine) {
		LUA->PushBool(false);
		return 1;
	}

	mmio_atomic_t* mmio_atomic = mmio_atomic_init(gmod_machine_get_rvvm_machine(machine), {size});

	if (!mmio_atomic) {
		LUA->PushBool(false);
		return 1;
	}

	LUA->PushUserType(mmio_atomic, mmio_atomic_mt);
	if (LUA->PushMetaTable(mmio_atomic_mt)) LUA->SetMetaTable(-2);

	return 1;
}

const char* mmio_atomic_get_name()
{
	return "mmio_atomic";
}

int mmio_atomic_get_version()
{
	return 1;
}

void mmio_atomic_init_lua(GarrysMod::Lua::ILuaBase* LUA)
{
	mmio_atomic_mt = LUA->CreateMetaTable("mmio_atomic");

	ATOMIC_PUSH_FUNCTION_READ(int8  , "Int8");
	ATOMIC_PUSH_FUNCTION_READ(int16 , "Int16");
	ATOMIC_PUSH_FUNCTION_READ(int32 , "Int32");
	ATOMIC_PUSH_FUNCTION_READ(uint8 , "UInt8");
	ATOMIC_PUSH_FUNCTION_READ(uint16, "UInt16");
	ATOMIC_PUSH_FUNCTION_READ(uint32, "UInt32");

	ATOMIC_PUSH_FUNCTION_WRITE(int8  , "Int8");
	ATOMIC_PUSH_FUNCTION_WRITE(int16 , "Int16");
	ATOMIC_PUSH_FUNCTION_WRITE(int32 , "Int32");
	ATOMIC_PUSH_FUNCTION_WRITE(uint8 , "UInt8");
	ATOMIC_PUSH_FUNCTION_WRITE(uint16, "UInt16");
	ATOMIC_PUSH_FUNCTION_WRITE(uint32, "UInt32");

	ATOMIC_PUSH_FUNCTION_READ(float, "Float");
	ATOMIC_PUSH_FUNCTION_WRITE(float, "Float");

	LUA->PushCFunction(atomic_readzstring);
	LUA->SetField(-2, "ReadZString");

	LUA->PushCFunction(atomic_readdata);
	LUA->SetField(-2, "ReadData");

	LUA->PushCFunction(atomic_writedata);
	LUA->SetField(-2, "WriteData");

	LUA->Push(-1);
	LUA->SetField(-2, "__index");

	LUA->Pop();
}

void mmio_atomic_register_functions(GarrysMod::Lua::ILuaBase* LUA)
{
	LUA->PushCFunction(mmio_atomic_create);
	LUA->SetField(-2, "mmio_atomic_create");
}

#include <dev_manager.h>

void mmio_atomic_close(GarrysMod::Lua::ILuaBase* LUA)
{
	if (LUA->PushMetaTable(mmio_atomic_mt))
	{
		LUA->PushCFunction(dev_manager_lua_nop_func);
		LUA->SetField(-2, "__index");

		LUA->Pop();
	}

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "riscv");
	LUA->GetField(-1, "devices");

	LUA->PushNil();
	LUA->SetField(-2, "mmio_atomic_init");

	LUA->Pop();
	LUA->Pop();
	LUA->Pop();
}
