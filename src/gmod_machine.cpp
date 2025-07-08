#include "gmod_machine.h"

#include <map>

typedef struct gmod_machine_t
{
	int id;
	rvvm_machine_t* machine;

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

	rtc_goldfish_init_auto(machine->machine);

	pci_bus_t* pci = pci_bus_init_auto(machine->machine);

	tap_dev_t* tap = tap_open();

	if (tap)
		rtl8169_init(pci, tap);

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

void gmod_machine_shutdown_all()
{
	for (auto& [id, machine] : machines)
	{
		rvvm_free_machine(machine->machine);
		delete machine;
	}

	machines.clear();
}
