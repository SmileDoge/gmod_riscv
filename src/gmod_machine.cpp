#include "gmod_machine.h"

#include <map>

typedef struct gmod_machine_t
{
	int id;
	rvvm_machine_t* machine;

	hid_keyboard_t* keyboard;
	hid_mouse_t* mouse;

	tap_dev_t* tap;
} gmod_machine_t;

std::map<int, gmod_machine_t*> machines;

gmod_machine_t* get_machine(int id)
{
	auto it = machines.find(id);
	if (it != machines.end())
	{
		return it->second;
	}
	return nullptr;
}

gmod_machine_t* gmod_machine_create(int id, int ram_size, int harts_num, bool is_64bit)
{
	if (get_machine(id) != nullptr)
	{
		return nullptr;
	}

	rvvm_machine_t* machine = rvvm_create_machine(ram_size, harts_num, is_64bit ? "rv64" : "rv32");

	if (!machine)
	{
		return nullptr;
	}

	rvvm_external_set_manual(true);

	gmod_machine_t* gmod_machine = new gmod_machine_t();

	gmod_machine->id = id;
	gmod_machine->machine = machine;
	gmod_machine->tap = nullptr;

	machines.emplace(id, gmod_machine);

	return gmod_machine;
}

int gmod_machine_get_id(gmod_machine_t* machine)
{
	if (machine)
		return machine->id;

	return 0;
}

rvvm_machine_t* gmod_machine_get_rvvm_machine(gmod_machine_t* machine)
{
	if (machine)
		return machine->machine;

	return nullptr;
}

void gmod_machine_destroy(gmod_machine_t* machine)
{
	if (!machine) return;

	auto it = machines.find(machine->id);
	if (it != machines.end())
	{
		if (machine->tap)
			tap_close(machine->tap);

		rvvm_free_machine(machine->machine);
		delete machine;
		machines.erase(it);
	}
	else
	{
		rvvm_free_machine(machine->machine);
		delete machine;
	}
}

bool gmod_machine_start(gmod_machine_t* machine)
{
	if (!machine) return false;

	return rvvm_start_machine(machine->machine);
}

bool gmod_machine_pause(gmod_machine_t* machine)
{
	if (!machine) return false;

	return rvvm_pause_machine(machine->machine);
}

bool gmod_machine_reset(gmod_machine_t* machine, bool reset)
{
	if (!machine) return false;

	rvvm_reset_machine(machine->machine, reset);

	return true;
}

bool gmod_machine_load_def_devices(gmod_machine_t* machine)
{
	if (!machine) return false;

	riscv_clint_init_auto(machine->machine);
	riscv_plic_init_auto(machine->machine);
	/*riscv_imsic_init_auto(machine->machine);
	riscv_aplic_init_auto(machine->machine);*/

	rtc_goldfish_init_auto(machine->machine);

	pci_bus_t* pci = pci_bus_init_auto(machine->machine);

	tap_dev_t* tap = tap_open();

	if (tap)
	{
		rtl8169_init(pci, tap);
	}

	i2c_oc_init_auto(machine->machine);

	syscon_init_auto(machine->machine);

	return true;
}

bool gmod_machine_load_bootrom(gmod_machine_t* machine, const char* path)
{
	if (!machine) return false;

	return rvvm_load_bootrom(machine->machine, path);
}

bool gmod_machine_load_kernel(gmod_machine_t* machine, const char* path)
{
	if (!machine) return false;

	return rvvm_load_kernel(machine->machine, path);
}

bool gmod_machine_load_dtb(gmod_machine_t* machine, const char* path)
{
	if (!machine) return false;

	return rvvm_load_dtb(machine->machine, path);
}

bool gmod_machine_attach_nvme(gmod_machine_t* machine, const char* path, bool rw)
{
	if (!machine) return false;

	return (nvme_init_auto(machine->machine, path, rw) != nullptr);
}

bool gmod_machine_dump_dtb(gmod_machine_t* machine, const char* path)
{
	if (!machine) return false;

	return rvvm_dump_dtb(machine->machine, path);
}

bool gmod_machine_is_running(gmod_machine_t* machine)
{
	if (!machine) return false;

	return rvvm_machine_running(machine->machine);
}

bool gmod_machine_is_powered(gmod_machine_t* machine)
{
	if (!machine) return false;

	return rvvm_machine_powered(machine->machine);
}

bool gmod_machine_append_cmdline(gmod_machine_t* machine, const char* cmd)
{
	if (!machine) return false;

	rvvm_append_cmdline(machine->machine, cmd);

	return false;
}

bool gmod_machine_set_cmdline(gmod_machine_t* machine, const char* cmd)
{
	if (!machine) return false;

	rvvm_set_cmdline(machine->machine, cmd);

	return true;
}

rvvm_addr_t gmod_machine_get_opt(gmod_machine_t* machine, uint32_t opt)
{
	return rvvm_get_opt(machine->machine, opt);
}

bool gmod_machine_set_opt(gmod_machine_t* machine, uint32_t opt, rvvm_addr_t value)
{
	return rvvm_set_opt(machine->machine, opt, value);
}

bool gmod_machine_attach_keyboard(gmod_machine_t* machine)
{
	if (!machine) return false;
	if (machine->keyboard) return true;

	hid_keyboard_t* keyboard = hid_keyboard_init_auto(machine->machine);

	if (!keyboard) return false;

	machine->keyboard = keyboard;

	return true;
}

bool gmod_machine_attach_mouse(gmod_machine_t* machine)
{
	if (!machine) return false;
	if (machine->mouse) return true;

	hid_mouse_t* mouse = hid_mouse_init_auto(machine->machine);

	if (!mouse) return false;

	machine->mouse = mouse;

	return true;
}

GMOD_API bool gmod_machine_keyboard_press(gmod_machine_t* machine, hid_key_t key)
{
	if (!machine || !machine->keyboard) return false;
	hid_keyboard_press(machine->keyboard, key);
	return true;
}

GMOD_API bool gmod_machine_keyboard_release(gmod_machine_t* machine, hid_key_t key)
{
	if (!machine || !machine->keyboard) return false;
	hid_keyboard_release(machine->keyboard, key);
	return true;
}

GMOD_API bool gmod_machine_mouse_press(gmod_machine_t* machine, hid_btns_t btns)
{
	if (!machine || !machine->mouse) return false;
	hid_mouse_press(machine->mouse, btns);
	return true;
}

GMOD_API bool gmod_machine_mouse_release(gmod_machine_t* machine, hid_btns_t btns)
{
	if (!machine || !machine->mouse) return false;
	hid_mouse_release(machine->mouse, btns);
	return true;
}

GMOD_API bool gmod_machine_mouse_scroll(gmod_machine_t* machine, int32_t offset)
{
	if (!machine || !machine->mouse) return false;
	hid_mouse_scroll(machine->mouse, offset);
	return true;
}

GMOD_API bool gmod_machine_mouse_move(gmod_machine_t* machine, int32_t x, int32_t y)
{
	if (!machine || !machine->mouse) return false;
	hid_mouse_move(machine->mouse, x, y);
	return true;
}

GMOD_API bool gmod_machine_mouse_place(gmod_machine_t* machine, int32_t x, int32_t y)
{
	if (!machine || !machine->mouse) return false;
	hid_mouse_place(machine->mouse, x, y);
	return true;
}

GMOD_API bool gmod_machine_mouse_resolution(gmod_machine_t* machine, uint32_t x, uint32_t y)
{
	if (!machine || !machine->mouse) return false;
	hid_mouse_resolution(machine->mouse, x, y);
	return true;
}

void gmod_machine_shutdown_all()
{
	for (auto& [id, machine] : machines)
	{
		rvvm_free_machine(machine->machine);
		delete machine;
	}

	machines.clear();
}
