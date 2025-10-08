#include "simple_uart.h"

#include <GarrysMod/Lua/Interface.h>

#include <gmod_machine.h>

#include <stdio.h>

extern "C"
{
#include <devices/ns16550a.h>

#include <fdtlib.h>
}

static int simple_uart_mt = 0;

RV_EXPORT const char* device_get_name()
{
	return "simple_uart";
}

RV_EXPORT int device_get_version()
{
	return 1;
}

LUA_FUNCTION(uart_write)
{
	//simple_uart_t* uart = LUA->GetUserType<simple_uart_t>(1, simple_uart_mt);
	chardev_t* uart = LUA->GetUserType<chardev_t>(1, simple_uart_mt);

	if (!uart) return 0;

	uint32_t str_len = 0;

	const char* buf = LUA->GetString(2, &str_len);

	chardev_simple_uart_push_rx(uart, buf, str_len);

	return 0;
}

char read_buffer[4096] = { 0 };

LUA_FUNCTION(uart_read)
{
	//simple_uart_t* uart = LUA->GetUserType<simple_uart_t>(1, simple_uart_mt);
	chardev_t* uart = LUA->GetUserType<chardev_t>(1, simple_uart_mt);

	int out_size = LUA->GetNumber(2);

	if (out_size <= 0 || out_size > 4096) out_size = 4096;

	if (!uart) return 0;

	memset(read_buffer, 0, out_size);

	size_t size = chardev_simple_uart_pop_tx(uart, read_buffer, out_size);

	LUA->PushString(read_buffer, size);

	return 1;
}

LUA_FUNCTION(uart__tostring)
{
	chardev_t* uart = LUA->GetUserType<chardev_t>(1, simple_uart_mt);

	if (!uart)
	{
		LUA->PushString("simple_uart: nil");
		return 1;
	}

	rvvm_mmio_dev_t* dev = simple_uart_get_mmio_dev(uart);

	if (dev)
	{
		char buffer[64];

		snprintf(buffer, sizeof(buffer), "simple_uart: %p@%p", dev->addr, dev->size);

		LUA->PushString(buffer);

		return 1;
	}
	else
	{
		LUA->PushString("simple_uart: dev == nil");
		return 1;
	}
}

RV_EXPORT void device_init(GarrysMod::Lua::ILuaBase* LUA)
{
	simple_uart_mt = LUA->CreateMetaTable("simple_uart");

	LUA->PushCFunction(uart_write);
	LUA->SetField(-2, "Write");

	LUA->PushCFunction(uart_read);
	LUA->SetField(-2, "Read");

	LUA->PushCFunction(uart__tostring);
	LUA->SetField(-2, "__tostring");

	LUA->Push(-1);
	LUA->SetField(-2, "__index");

	LUA->Pop();
}

int uart_create(lua_State*);

RV_EXPORT void device_register_functions(GarrysMod::Lua::ILuaBase* LUA) // in riscv.devices
{
	LUA->PushCFunction(uart_create);
	LUA->SetField(-2, "uart_create");
}

LUA_FUNCTION(uart_create)
{
	int id = LUA->CheckNumber(1);
	int address = LUA->CheckNumber(2);
	bool add_chosen = LUA->IsType(3, GarrysMod::Lua::Type::Bool) ? LUA->GetBool(3) : false;

	gmod_machine_t* machine = get_machine(id);

	if (!machine) {
		LUA->PushBool(false);
		return 1;
	}

	rvvm_intc_t* intc = rvvm_get_intc(gmod_machine_get_rvvm_machine(machine));

	//simple_uart_t* uart = simple_uart_init(gmod_machine_get_rvvm_machine(machine), address, 0x1000, add_chosen);
	chardev_t* simple_uart = chardev_simple_uart_create();


	if (!simple_uart) {
		LUA->PushBool(false);
		return 1;
	}

	rvvm_mmio_dev_t* ns16550a = ns16550a_init(
		gmod_machine_get_rvvm_machine(machine),
		simple_uart,
		address, intc, rvvm_alloc_irq(intc)
	);

	simple_uart_set_mmio_dev(simple_uart, ns16550a);

	if (add_chosen)
	{
		struct fdt_node* chosen = fdt_node_find(rvvm_get_fdt_root(gmod_machine_get_rvvm_machine(machine)), "chosen");
		fdt_node_add_prop_str(chosen, "stdout-path", "/soc/uart@10000000");
	}

	LUA->PushUserType(simple_uart, simple_uart_mt);
	if (LUA->PushMetaTable(simple_uart_mt)) LUA->SetMetaTable(-2);

	return 1;
}

#include <dev_manager.h>

RV_EXPORT void device_close(GarrysMod::Lua::ILuaBase* LUA)
{
	if (LUA->PushMetaTable(simple_uart_mt))
	{
		LUA->PushCFunction(dev_manager_lua_nop_func);
		LUA->SetField(-2, "__tostring");

		LUA->PushCFunction(dev_manager_lua_nop_func);
		LUA->SetField(-2, "__index");

		LUA->Pop();
	}

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "riscv");
	LUA->GetField(-1, "devices");

	LUA->PushNil();
	LUA->SetField(-2, "uart_create");

	LUA->Pop(); // pop riscv.devices
	LUA->Pop(); // pop riscv
	LUA->Pop(); // pop _G
}
