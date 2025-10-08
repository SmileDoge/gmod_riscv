#pragma once

extern "C"
{
#include <rvvmlib.h>

#include "devices/riscv-aclint.h"
#include "devices/riscv-aplic.h"
#include "devices/riscv-imsic.h"
#include "devices/riscv-plic.h"

#include <devices/rtc-goldfish.h>

#include <devices/syscon.h>

#include <devices/rtl8169.h>

#include <devices/nvme.h>

#include <devices/i2c-oc.h>
#include <devices/i2c-hid.h>
#include <devices/hid_api.h>
}

#ifdef GMOD_RISCV_EXPORTS
#define GMOD_API __declspec(dllexport)
#else
#define GMOD_API __declspec(dllimport)
#endif

typedef struct gmod_machine_t gmod_machine_t;

GMOD_API gmod_machine_t* get_machine(int id);

GMOD_API gmod_machine_t* gmod_machine_create(int id, int ram_size, int harts_num, bool is_64bit);

GMOD_API int gmod_machine_get_id(gmod_machine_t* machine);
GMOD_API rvvm_machine_t* gmod_machine_get_rvvm_machine(gmod_machine_t* machine);

GMOD_API void gmod_machine_destroy(gmod_machine_t* machine);

GMOD_API bool gmod_machine_start(gmod_machine_t* machine);
GMOD_API bool gmod_machine_pause(gmod_machine_t* machine);
GMOD_API bool gmod_machine_reset(gmod_machine_t* machine, bool reset = false);

GMOD_API bool gmod_machine_load_def_devices(gmod_machine_t* machine);

GMOD_API bool gmod_machine_load_bootrom(gmod_machine_t* machine, const char* path);
GMOD_API bool gmod_machine_load_kernel(gmod_machine_t* machine, const char* path);
GMOD_API bool gmod_machine_load_dtb(gmod_machine_t* machine, const char* path);
GMOD_API bool gmod_machine_attach_nvme(gmod_machine_t* machine, const char* path, bool rw = false);

GMOD_API bool gmod_machine_dump_dtb(gmod_machine_t* machine, const char* path);

GMOD_API bool gmod_machine_is_running(gmod_machine_t* machine);
GMOD_API bool gmod_machine_is_powered(gmod_machine_t* machine);

GMOD_API bool gmod_machine_append_cmdline(gmod_machine_t* machine, const char* cmd);
GMOD_API bool gmod_machine_set_cmdline(gmod_machine_t* machine, const char* cmd);

GMOD_API rvvm_addr_t gmod_machine_get_opt(gmod_machine_t* machine, uint32_t opt);
GMOD_API bool gmod_machine_set_opt(gmod_machine_t* machine, uint32_t opt, rvvm_addr_t value);

GMOD_API bool gmod_machine_attach_keyboard(gmod_machine_t* machine);
GMOD_API bool gmod_machine_attach_mouse(gmod_machine_t* machine);

GMOD_API bool gmod_machine_keyboard_press(gmod_machine_t* machine, hid_key_t key);
GMOD_API bool gmod_machine_keyboard_release(gmod_machine_t* machine, hid_key_t key);

GMOD_API bool gmod_machine_mouse_press(gmod_machine_t* machine, hid_btns_t btns);
GMOD_API bool gmod_machine_mouse_release(gmod_machine_t* machine, hid_btns_t btns);
GMOD_API bool gmod_machine_mouse_scroll(gmod_machine_t* machine, int32_t offset);
GMOD_API bool gmod_machine_mouse_move(gmod_machine_t* machine, int32_t x, int32_t y);
GMOD_API bool gmod_machine_mouse_place(gmod_machine_t* machine, int32_t x, int32_t y);
GMOD_API bool gmod_machine_mouse_resolution(gmod_machine_t* machine, uint32_t x, uint32_t y);

GMOD_API void gmod_machine_shutdown_all();