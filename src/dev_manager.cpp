#include "dev_manager.h"

#include <filesystem>

#define DEV_DIRECTORY "./devices/"

namespace fs = std::filesystem;

#include <iostream>

#include <Windows.h>

#include <device.h>

typedef struct device_info_int_t
{
	HMODULE module;
	const char* name;
	int version;
	DeviceInitFunc init_func;
	DeviceRegisterFunc register_func;
	DeviceCloseFunc close_func;
} device_info_int_t;

std::vector<device_info_int_t> devices;

GarrysMod::Lua::ILuaBase* g_LUA = nullptr;

void SetTop(GarrysMod::Lua::ILuaBase* LUA, int new_top)
{
	int current_top = LUA->Top();
	if (new_top < current_top)
	{
		LUA->Pop(current_top - new_top);
	}
	else if (new_top > current_top)
	{
		while (current_top++ < new_top)
			LUA->PushNil();
	}
}

const char* dev_manager_load_device__int(fs::path path)
{
	if (!g_LUA) return nullptr;

	HMODULE module = LoadLibraryW(path.wstring().c_str());

	if (!module)
	{
		printf("Failed to load device module: %s\n", path.string().c_str());
		printf("Error: %d\n", GetLastError());
		return nullptr;
	}

	DeviceGetNameFunc get_name = (DeviceGetNameFunc)GetProcAddress(module, "device_get_name");
	DeviceGetVersionFunc get_version = (DeviceGetVersionFunc)GetProcAddress(module, "device_get_version");

	DeviceInitFunc init_func = (DeviceInitFunc)GetProcAddress(module, "device_init");
	DeviceRegisterFunc register_func = (DeviceRegisterFunc)GetProcAddress(module, "device_register_functions");
	DeviceCloseFunc close_func = (DeviceCloseFunc)GetProcAddress(module, "device_close");

	if (!init_func || !register_func || !close_func || !get_name || !get_version)
	{
		printf("Failed to get device functions from module: %s\n", path.filename().string().c_str());
		FreeLibrary(module);
		return nullptr;
	}

	const char* device_name = get_name();

	init_func(g_LUA);

	devices.push_back({
		module,
		device_name,
		get_version(),
		init_func,
		register_func,
		close_func
		});

	return device_name;
}

void dev_manager_load_devices()
{
	if (!fs::exists(DEV_DIRECTORY) || !fs::is_directory(DEV_DIRECTORY))
	{
		printf("Device directory does not exist: %s\n", DEV_DIRECTORY);

		return;
	}

	for (const auto& entry : fs::directory_iterator(DEV_DIRECTORY))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".dll")
		{
			dev_manager_load_device__int(entry.path());
		}
	}
}

void dev_manager_init(GarrysMod::Lua::ILuaBase* LUA)
{
	g_LUA = LUA;

	dev_manager_load_devices();

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->GetField(-1, "riscv");
			LUA->GetField(-1, "devices");

	for (auto& device : devices)
	{
		if (device.register_func)
			device.register_func(LUA);

		printf("Loaded device: %s (version %d)\n", device.name, device.version);
	}

			LUA->Pop();
		LUA->Pop();
	LUA->Pop();
}

void dev_manager_close(GarrysMod::Lua::ILuaBase* LUA)
{
	for (auto& device : devices)
	{
		if (device.close_func)
			device.close_func(LUA);

		if (device.module)
			FreeLibrary(device.module);
	}

	g_LUA = nullptr;

	devices.clear();
}

std::vector<device_info_t> dev_manager_get_devices()
{
	std::vector<device_info_t> devices;

	for (auto& device : ::devices)
		devices.push_back({
			device.name,
			device.version
			});

	return devices;
}

GMOD_API bool dev_manager_get_device(const std::string& name, device_info_t* out_info)
{
	if (!out_info) return false;

	for (auto& device : devices)
		if (device.name == name)
		{
			out_info->name = device.name;
			out_info->version = device.version;

			return true;
		}

	return false;
}

GMOD_API device_info_int_t* dev_manager_get_device__int(const std::string& name)
{
	for (auto& device : devices)
		if (device.name == name)
			return &device;

	return nullptr;
}

GMOD_API bool dev_manager_register_device(DeviceGetNameFunc get_name_func, DeviceGetVersionFunc get_version_func, DeviceInitFunc init_func, DeviceRegisterFunc reg_func, DeviceCloseFunc close_func)
{
	if (!init_func || !reg_func || !close_func || !get_name_func || !get_version_func)
		return false;

	const char* device_name = get_name_func();

	if (!device_name)
		return false;

	int top = g_LUA->Top();
	init_func(g_LUA);
	SetTop(g_LUA, top);

	top = g_LUA->Top();
	g_LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	g_LUA->GetField(-1, "riscv");
	g_LUA->GetField(-1, "devices");
	reg_func(g_LUA);
	g_LUA->Pop();
	g_LUA->Pop();
	g_LUA->Pop();
	SetTop(g_LUA, top);

	devices.push_back({
		nullptr, // module will be loaded later
		device_name,
		get_version_func(),
		init_func,
		reg_func,
		close_func
		});

	return true;
}

GMOD_API bool dev_manager_load_device(const std::string& file_name, std::string& out_name)
{
	if (!fs::exists(DEV_DIRECTORY + file_name))
		return false;

	const char* name = dev_manager_load_device__int(DEV_DIRECTORY + file_name);

	if (!name)
		return false;

	device_info_int_t* info = dev_manager_get_device__int(name);

	if (!info)
		return false;


	int top = g_LUA->Top();

	info->init_func(g_LUA);

	SetTop(g_LUA, top);

	//

	top = g_LUA->Top();

	g_LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	g_LUA->GetField(-1, "riscv");
	g_LUA->GetField(-1, "devices");

	info->register_func(g_LUA);

	g_LUA->Pop();
	g_LUA->Pop();
	g_LUA->Pop();

	SetTop(g_LUA, top);

	out_name.append(info->name);

	return true;
}

GMOD_API bool dev_manager_unload_device(const std::string& name)
{
	device_info_int_t* info = dev_manager_get_device__int(name);

	if (!info)
		return false;

	int top = g_LUA->Top();

	if (info->close_func)
		info->close_func(g_LUA);

	SetTop(g_LUA, top);

	auto it = std::find_if(devices.begin(), devices.end(),
		[&name](const device_info_int_t& device) { return device.name == name; });

	if (it != devices.end())
		devices.erase(it);

	if (info->module)
		FreeLibrary(info->module);

	return true;
}

GMOD_API int dev_manager_lua_nop_func(lua_State*)
{
	return 0;
}
