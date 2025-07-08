#include "web_fb.h"

#include <GarrysMod/Lua/Interface.h>

#include <gmod_machine.h>

#include "mongoose.h"

extern "C"
{
#include <fdtlib.h>
}

#include <turbojpeg.h>
#include <jpeglib.h>

#include <thread>
#include <vector>
#include <mutex>

#define FB_FPS 30

typedef struct web_fb_t
{
    rvvm_mmio_dev_t* mmio;

    int width;
	int height;

    uint8_t* buffer;
	uint8_t* send_buffer;

	tjhandle tj_compressor;
	uint8_t* jpeg_buf;
	unsigned long jpeg_size;

	mg_connection* server;
	mg_timer* timer;
	std::vector<mg_connection*> connections;
} web_fb_t;

web_fb_t* fb_to_clear = nullptr;
std::atomic<bool> fb_cleared = true;
std::mutex crit_mutex;

RV_EXPORT const char* device_get_name()
{
	return "web_fb";
}

RV_EXPORT int device_get_version()
{
	return 1;
}

LUA_FUNCTION(web_fb_create)
{
	int id = LUA->CheckNumber(1);
	int addr = LUA->CheckNumber(2);
	int width = LUA->CheckNumber(3);
	int height = LUA->CheckNumber(4);

	gmod_machine_t* machine = get_machine(id);

	if (!machine) {
		LUA->PushBool(false);
		return 1;
	}

	web_fb_t* web_fb = web_fb_init(gmod_machine_get_rvvm_machine(machine), addr, width, height);

	if (!web_fb) {
		LUA->PushBool(false);
		return 1;
	}

	LUA->PushBool(true);

	return 1;
}

mg_mgr web_fb_mgr;
std::thread web_fb_thread;
bool web_fb_finished;

LUA_FUNCTION(web_fb_mgr_poll)
{
	if (!web_fb_finished)
		mg_mgr_poll(&web_fb_mgr, 0);

	return 0;
}

RV_EXPORT void device_init(GarrysMod::Lua::ILuaBase* LUA)
{
	memset(&web_fb_mgr, 0, sizeof(web_fb_mgr));
	mg_mgr_init(&web_fb_mgr);

	web_fb_finished = false;

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "hook");
	LUA->GetField(-1, "Add");
	LUA->PushString("Think");
	LUA->PushString("web_fb_mgr_poll");
	LUA->PushCFunction(web_fb_mgr_poll);
	LUA->Call(3, 0);
	LUA->Pop(2);
}

RV_EXPORT void device_register_functions(GarrysMod::Lua::ILuaBase* LUA)
{
	LUA->PushCFunction(web_fb_create);
	LUA->SetField(-2, "web_fb_create");
}

RV_EXPORT void device_close(GarrysMod::Lua::ILuaBase* LUA)
{
	mg_mgr_free(&web_fb_mgr);

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "riscv");
	LUA->GetField(-1, "devices");

	LUA->PushNil();
	LUA->SetField(-2, "web_fb_create");

	LUA->Pop();
	LUA->Pop();
	LUA->Pop();
}

static void web_fb_update(rvvm_mmio_dev_t* dev)
{
    return;
}

static void web_fb_remove(rvvm_mmio_dev_t* dev)
{
    web_fb_t* fb = (web_fb_t*)dev->data;
    if (fb)
    {
		for (auto conn : fb->connections)
			mg_close_conn(conn);
		fb->server->is_closing = 1;
		fb->timer->flags = MG_TIMER_AUTODELETE;
		mg_mgr_poll(&web_fb_mgr, 0);


		//fb->server->is_closing = 1;

		//mg_mgr_poll(&web_fb_mgr, 0);


		tjDestroy(fb->tj_compressor);
		if (fb->jpeg_buf) tjFree(fb->jpeg_buf);

		fb->tj_compressor = nullptr;
		fb->jpeg_buf = nullptr;

		delete[] fb->buffer;
		delete[] fb->send_buffer;
        delete fb;
    }
}

static const rvvm_mmio_type_t web_fb_type = { "web_fb", web_fb_remove, web_fb_update, 0 };

static void web_fb_timer(void* data)
{
	web_fb_t* fb = (web_fb_t*)data;

	if (!fb->tj_compressor)
		return;

	memcpy(fb->send_buffer, fb->buffer, fb->width * fb->height * 4);

	if (tjCompress2(fb->tj_compressor, fb->send_buffer, fb->width, 0, fb->height,
		TJPF_ARGB, &fb->jpeg_buf, &fb->jpeg_size, TJSAMP_420, 75, TJFLAG_FASTDCT))
	{
		printf("Error compressing image on fb %p: %s\n", (uint32_t)fb->mmio->addr, tjGetErrorStr());
		return;
	}

	for (auto conn : fb->connections)
	{
		if (conn->is_closing)
			continue;
		if (fb->jpeg_buf == nullptr || fb->jpeg_size == 0)
			continue;
		mg_printf(conn, "--frame\r\n"
			"Content-Type: image/jpeg\r\n"
			"Content-Length: %zu\r\n\r\n", fb->jpeg_size);
		mg_send(conn, fb->jpeg_buf, fb->jpeg_size);
		mg_send(conn, "\r\n", 2);
	}
}

static void web_fb_event_handler(struct mg_connection* conn, int ev, void* ev_data)
{
	web_fb_t* fb = (web_fb_t*)conn->fn_data;

	if (ev == MG_EV_HTTP_MSG)
	{

		mg_http_message* hm = (mg_http_message*)ev_data;
		if (mg_match(hm->uri, mg_str("/stream"), NULL))
		{
			fb->connections.push_back(conn);

			mg_printf(conn, "HTTP/1.1 200 OK\r\n"
				"Content-Type: multipart/x-mixed-replace; boundary=--frame\r\n"
				"Cache-Control: no-cache\r\n"
				"Pragma: no-cache\r\n\r\n");
		}
	}
	else if (ev == MG_EV_CLOSE)
	{
		auto it = std::find(fb->connections.begin(), fb->connections.end(), conn);
		if (it != fb->connections.end())
			fb->connections.erase(it);
	}
}

web_fb_t* web_fb_init(rvvm_machine_t* machine, size_t addr, int width, int height)
{
    int size = width * height * 4;

    web_fb_t* fb = new web_fb_t();

	fb->buffer = new uint8_t[size];
	fb->send_buffer = new uint8_t[size];
	fb->width = width;
	fb->height = height;

	memset(fb->buffer, 0, size);
	memset(fb->send_buffer, 0, size);

    rvvm_mmio_dev_t fb_mmio = { 0 };

    fb_mmio.addr = addr;
    fb_mmio.size = size;
	fb_mmio.mapping = fb->buffer;
	fb_mmio.data = fb;
	fb_mmio.type = &web_fb_type;

	rvvm_mmio_dev_t* mmio = rvvm_attach_mmio(machine, &fb_mmio);

    if (!mmio)
    {
		delete[] fb->buffer;
		delete fb;
		return nullptr;
    }

	fb->mmio = mmio;

	fb->server = mg_http_listen(&web_fb_mgr, "http://0.0.0.0:8001", web_fb_event_handler, fb);

	if (!fb->server)
	{
		delete[] fb->buffer;
		delete fb;
		return nullptr;
	}

	fb->timer = mg_timer_add(&web_fb_mgr, 1000 / FB_FPS, MG_TIMER_REPEAT, web_fb_timer, fb);

	fb->tj_compressor = tjInitCompress(); 
	fb->jpeg_buf = nullptr;
	fb->jpeg_size = 0;

    struct fdt_node* fb_fdt = fdt_node_create_reg("framebuffer", fb_mmio.addr);
    fdt_node_add_prop_reg(fb_fdt, "reg", fb_mmio.addr, fb_mmio.size);
    fdt_node_add_prop_str(fb_fdt, "compatible", "simple-framebuffer");
    fdt_node_add_prop_str(fb_fdt, "format", "a8r8g8b8");
    fdt_node_add_prop_u32(fb_fdt, "width", fb->width);
    fdt_node_add_prop_u32(fb_fdt, "height", fb->height);
    fdt_node_add_prop_u32(fb_fdt, "stride", width * 4);

    fdt_node_add_child(rvvm_get_fdt_soc(machine), fb_fdt);

	return fb;
}