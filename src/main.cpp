#include <GarrysMod/Lua/Interface.h>

#include "windows.h"

#include <thread>
#include <map>

extern "C"
{
#include <rvvmlib.h>

#include <devices/riscv-aclint.h>
#include <devices/riscv-plic.h>

#include <devices/rtc-goldfish.h>
}

#include "mmio_atomic.h"

#include <vector>
#include <string>
#include "gmod_machine.h"

#include "dev_manager.h"

LUA_FUNCTION(create_machine)
{
	int id = LUA->CheckNumber(1);
	int ram_size = LUA->CheckNumber(2);
	int harts_num = LUA->IsType(3, GarrysMod::Lua::Type::Number) ? LUA->GetNumber(3) : 1;
	bool is_64bit = LUA->IsType(4, GarrysMod::Lua::Type::Bool) ? LUA->GetBool(4) : true;

	gmod_machine_t* machine = gmod_machine_create(id, ram_size, harts_num, is_64bit);

	if (machine)
		LUA->PushBool(true);
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(is_machine_running)
{
	int id = LUA->CheckNumber(1);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushBool(gmod_machine_is_running(machine));
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(is_machine_powered)
{
	int id = LUA->CheckNumber(1);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushBool(gmod_machine_is_powered(machine));
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(is_machine_exists)
{
	int id = LUA->CheckNumber(1);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushBool(true);
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(destroy_machine)
{
	int id = LUA->CheckNumber(1);
	
	gmod_machine_destroy(get_machine(id));

	return 0;
}

LUA_FUNCTION(load_bootrom)
{
	int id = LUA->CheckNumber(1);
	const char* path = LUA->CheckString(2);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushBool(gmod_machine_load_bootrom(machine, path));
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(load_kernel)
{
	int id = LUA->CheckNumber(1);
	const char* path = LUA->CheckString(2);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushBool(gmod_machine_load_kernel(machine, path));
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(set_cmdline)
{
	int id = LUA->CheckNumber(1);
	const char* cmdline = LUA->CheckString(2);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
	{
		gmod_machine_set_cmdline(machine, cmdline);
		LUA->PushBool(true);
	}
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(append_cmdline)
{
	int id = LUA->CheckNumber(1);
	const char* cmdline = LUA->CheckString(2);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
	{
		gmod_machine_append_cmdline(machine, cmdline);
		LUA->PushBool(true);
	}
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(dump_dtb)
{
	int id = LUA->CheckNumber(1);
	const char* path = LUA->CheckString(2);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushBool(gmod_machine_dump_dtb(machine, path));
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(load_dtb)
{
	int id = LUA->CheckNumber(1);
	const char* path = LUA->CheckString(2);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushBool(gmod_machine_load_dtb(machine, path));
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(get_opt)
{
	int id = LUA->CheckNumber(1);
	int opt = LUA->CheckNumber(2);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushNumber(gmod_machine_get_opt(machine, opt));
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(set_opt)
{
	int id = LUA->CheckNumber(1);
	int opt = LUA->CheckNumber(2);
	int value = LUA->CheckNumber(3);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushBool(gmod_machine_set_opt(machine, opt, value));
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(start_machine)
{
	int id = LUA->CheckNumber(1);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushBool(gmod_machine_start(machine));
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(pause_machine)
{
	int id = LUA->CheckNumber(1);
	gmod_machine_t* machine = get_machine(id);

	if (machine)
		LUA->PushBool(gmod_machine_pause(machine));
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(reset_machine)
{
	int id = LUA->CheckNumber(1);
	bool reset = LUA->IsType(2, GarrysMod::Lua::Type::Bool) ? LUA->GetBool(2) : false;
	gmod_machine_t* machine = get_machine(id);

	if (machine)
	{
		gmod_machine_reset(machine, reset);
		LUA->PushBool(true);
	}
	else
		LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(load_def_devices)
{
	int id = LUA->CheckNumber(1);
	gmod_machine_t* machine = get_machine(id);

	if (!machine) return 0;

	gmod_machine_load_def_devices(machine);

	return 0;
}

LUA_FUNCTION(attach_nvme)
{
	int id = LUA->CheckNumber(1);
	gmod_machine_t* machine = get_machine(id);

	if (!machine) return 0;

	const char* path = LUA->CheckString(2);
	bool rw = LUA->IsType(3, GarrysMod::Lua::Type::Bool) ? LUA->GetBool(3) : false;

	LUA->PushBool(gmod_machine_attach_nvme(machine, path, rw));

	return 1;
}

std::thread event_thread;
static bool thread_running = false;

static void thread_func()
{
	while (thread_running)
	{
		rvvm_external_tick_eventloop(true);
		std::this_thread::sleep_for(std::chrono::nanoseconds(10));
	}
}

LUA_FUNCTION(init_thread)
{
	if (thread_running) return 0;

	thread_running = true;
	event_thread = std::thread(thread_func);

	event_thread.detach();

	return 0;
}

LUA_FUNCTION(get_devices)
{
	LUA->CreateTable();
	auto devices = dev_manager_get_devices();
	for (const auto& device : devices)
	{
		LUA->PushString(device.name);
		LUA->PushNumber(device.version);
		LUA->SetTable(-3);
	}
	return 1;
}

LUA_FUNCTION(get_device)
{
	const char* name = LUA->CheckString(1);
	device_info_t info;
	if (dev_manager_get_device(name, &info))
	{
		LUA->CreateTable();
		LUA->PushString(info.name);
		LUA->SetField(-2, "name");
		LUA->PushNumber(info.version);
		LUA->SetField(-2, "version");
		return 1;
	}
	LUA->PushNil();
	return 1;
}

LUA_FUNCTION(load_device)
{
	const char* file_name = LUA->CheckString(1);
	std::string out_name;
	if (dev_manager_load_device(file_name, out_name))
	{
		LUA->PushString(out_name.c_str());
		return 1;
	}
	LUA->PushNil();
	return 1;
}

LUA_FUNCTION(unload_device)
{
	const char* name = LUA->CheckString(1);
	
	LUA->PushBool(dev_manager_unload_device(name));

	return 1;
}

LUA_FUNCTION(attach_keyboard)
{
	int id = LUA->CheckNumber(1);
	gmod_machine_t* machine = get_machine(id);
	if (!machine) {
		LUA->PushBool(false);
		return 1;
	}
	LUA->PushBool(gmod_machine_attach_keyboard(machine));
	return 1;
}

LUA_FUNCTION(attach_mouse)
{
	int id = LUA->CheckNumber(1);
	gmod_machine_t* machine = get_machine(id);
	if (!machine) {
		LUA->PushBool(false);
		return 1;
	}
	LUA->PushBool(gmod_machine_attach_mouse(machine));
	return 1;
}

LUA_FUNCTION(keyboard_press)
{
	int id = LUA->CheckNumber(1);
	hid_key_t key = LUA->CheckNumber(2);
	gmod_machine_t* machine = get_machine(id);
	if (machine)
		LUA->PushBool(gmod_machine_keyboard_press(machine, key));
	else
		LUA->PushBool(false);
	return 1;
}

LUA_FUNCTION(keyboard_release)
{
	int id = LUA->CheckNumber(1);
	hid_key_t key = LUA->CheckNumber(2);
	gmod_machine_t* machine = get_machine(id);
	if (machine)
		LUA->PushBool(gmod_machine_keyboard_release(machine, key));
	else
		LUA->PushBool(false);
	return 1;
}

LUA_FUNCTION(mouse_press)
{
	int id = LUA->CheckNumber(1);
	hid_btns_t btns = LUA->CheckNumber(2);
	gmod_machine_t* machine = get_machine(id);
	if (machine)
		LUA->PushBool(gmod_machine_mouse_press(machine, btns));
	else
		LUA->PushBool(false);
	return 1;
}

LUA_FUNCTION(mouse_release)
{
	int id = LUA->CheckNumber(1);
	hid_btns_t btns = LUA->CheckNumber(2);
	gmod_machine_t* machine = get_machine(id);
	if (machine)
		LUA->PushBool(gmod_machine_mouse_release(machine, btns));
	else
		LUA->PushBool(false);
	return 1;
}

LUA_FUNCTION(mouse_scroll)
{
	int id = LUA->CheckNumber(1);
	int32_t offset = LUA->CheckNumber(2);
	gmod_machine_t* machine = get_machine(id);
	if (machine)
		LUA->PushBool(gmod_machine_mouse_scroll(machine, offset));
	else
		LUA->PushBool(false);
	return 1;
}

LUA_FUNCTION(mouse_move)
{
	int id = LUA->CheckNumber(1);
	int32_t x = LUA->CheckNumber(2);
	int32_t y = LUA->CheckNumber(3);
	gmod_machine_t* machine = get_machine(id);
	if (machine)
		LUA->PushBool(gmod_machine_mouse_move(machine, x, y));
	else
		LUA->PushBool(false);
	return 1;
}

LUA_FUNCTION(mouse_place)
{
	int id = LUA->CheckNumber(1);
	int32_t x = LUA->CheckNumber(2);
	int32_t y = LUA->CheckNumber(3);
	gmod_machine_t* machine = get_machine(id);
	if (machine)
		LUA->PushBool(gmod_machine_mouse_place(machine, x, y));
	else
		LUA->PushBool(false);
	return 1;
}

LUA_FUNCTION(mouse_resolution)
{
	int id = LUA->CheckNumber(1);
	uint32_t x = LUA->CheckNumber(2);
	uint32_t y = LUA->CheckNumber(3);
	gmod_machine_t* machine = get_machine(id);
	if (machine)
		LUA->PushBool(gmod_machine_mouse_resolution(machine, x, y));
	else
		LUA->PushBool(false);
	return 1;
}

void alloc_console()
{
	AllocConsole();
	freopen("CONIN$", "r", __acrt_iob_func(0));
	freopen("CONOUT$", "w", __acrt_iob_func(1));
	freopen("CONOUT$", "w", __acrt_iob_func(2));

	auto raw_output = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	auto raw_input = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	SetStdHandle(STD_INPUT_HANDLE, raw_input);
	SetStdHandle(STD_OUTPUT_HANDLE, raw_output);
	SetStdHandle(STD_ERROR_HANDLE, raw_output);

	SetConsoleOutputCP(CP_UTF8);
}

GMOD_MODULE_OPEN()
{
	alloc_console();

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->CreateTable();
			LUA->PushCFunction(create_machine);
			LUA->SetField(-2, "create_machine");

			LUA->PushCFunction(is_machine_running);
			LUA->SetField(-2, "is_machine_running");

			LUA->PushCFunction(is_machine_powered);
			LUA->SetField(-2, "is_machine_powered");

			LUA->PushCFunction(is_machine_exists);
			LUA->SetField(-2, "is_machine_exists");

			LUA->PushCFunction(destroy_machine);
			LUA->SetField(-2, "destroy_machine");

			LUA->PushCFunction(load_bootrom);
			LUA->SetField(-2, "load_bootrom");

			LUA->PushCFunction(load_kernel);
			LUA->SetField(-2, "load_kernel");

			LUA->PushCFunction(set_cmdline);
			LUA->SetField(-2, "set_cmdline");

			LUA->PushCFunction(append_cmdline);
			LUA->SetField(-2, "append_cmdline");

			LUA->PushCFunction(dump_dtb);
			LUA->SetField(-2, "dump_dtb");

			LUA->PushCFunction(load_dtb);
			LUA->SetField(-2, "load_dtb");

			LUA->PushCFunction(get_opt);
			LUA->SetField(-2, "get_opt");

			LUA->PushCFunction(set_opt);
			LUA->SetField(-2, "set_opt");

			LUA->PushCFunction(start_machine);
			LUA->SetField(-2, "start_machine");

			LUA->PushCFunction(pause_machine);
			LUA->SetField(-2, "pause_machine");

			LUA->PushCFunction(reset_machine);
			LUA->SetField(-2, "reset_machine");

			LUA->PushCFunction(load_def_devices);
			LUA->SetField(-2, "load_def_devices");

			LUA->PushCFunction(attach_nvme);
			LUA->SetField(-2, "attach_nvme");

			// Devices table

			LUA->CreateTable();
				LUA->PushCFunction(get_devices);
				LUA->SetField(-2, "get_devices");

				LUA->PushCFunction(get_device);
				LUA->SetField(-2, "get_device");

				LUA->PushCFunction(load_device);
				LUA->SetField(-2, "load_device");

				LUA->PushCFunction(unload_device);
				LUA->SetField(-2, "unload_device");
			LUA->SetField(-2, "devices");

			
			LUA->PushCFunction(attach_keyboard);
			LUA->SetField(-2, "attach_keyboard");

			LUA->PushCFunction(attach_mouse);
			LUA->SetField(-2, "attach_mouse");

			LUA->CreateTable();
				LUA->PushCFunction(keyboard_press);
				LUA->SetField(-2, "keyboard_press");

				LUA->PushCFunction(keyboard_release);
				LUA->SetField(-2, "keyboard_release");

				LUA->PushCFunction(mouse_press);
				LUA->SetField(-2, "mouse_press");

				LUA->PushCFunction(mouse_release);
				LUA->SetField(-2, "mouse_release");

				LUA->PushCFunction(mouse_scroll);
				LUA->SetField(-2, "mouse_scroll");

				LUA->PushCFunction(mouse_move);
				LUA->SetField(-2, "mouse_move");

				LUA->PushCFunction(mouse_place);
				LUA->SetField(-2, "mouse_place");

				LUA->PushCFunction(mouse_resolution);
				LUA->SetField(-2, "mouse_resolution");
			LUA->SetField(-2, "hid");

			LUA->PushCFunction(init_thread);
			LUA->SetField(-2, "init_thread");

			LUA->PushString(RVVM_VERSION);
			LUA->SetField(-2, "rvvm_version");

			LUA->PushNumber(RVVM_ABI_VERSION);
			LUA->SetField(-2, "rvvm_abi_version");
		LUA->SetField(-2, "riscv");
	LUA->Pop();

	dev_manager_init(LUA);

	dev_manager_register_device(mmio_atomic_get_name, mmio_atomic_get_version, mmio_atomic_init_lua, mmio_atomic_register_functions, mmio_atomic_close);

	return 0;
}

GMOD_MODULE_CLOSE()
{
	gmod_machine_shutdown_all();

	thread_running = false;
	if (event_thread.joinable())
		event_thread.join();

	dev_manager_close(LUA);

	return 0;
}
