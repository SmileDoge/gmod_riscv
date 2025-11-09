#pragma once

#include "IDevice.h"
#include "IEmulator.h"
#include "ILogger.h"

#include "spsc_shared_manager.hpp"

extern "C"
{
#include <devices/chardev.h>
}

#include <queue>
#include <mutex>
#include <chrono>

#define SIMPLE_DEVICE_UPDATE_FPS 60.0
#define SIMPLE_DEVICE_BUFFER_SIZE 8188

typedef struct
{
	uint32_t size;
	char data[SIMPLE_DEVICE_BUFFER_SIZE];
} SimpleDeviceMessage;

#define SIMPLE_DEVICE_BUFFER_COUNT 16

//#define SIMPLE_DEVICE_BUFFER_SIZE SimpleDeviceMessage
//#define SIMPLE_DEVICE_BUFFER_COUNT 16

BEGIN_BUILTIN_DEVICE(SimpleDevice, "simple_device")

public:
	virtual bool OnCreate(nlohmann::json& json) override;

	virtual size_t GetSize() override;
	virtual void OnAttach(IMachine* machine, uint64_t addr) override;
	virtual bool OnWrite(void* data, size_t offset, uint8_t size) { return true; };
	virtual bool OnRead(void* data, size_t offset, uint8_t size) { return true; };

	void UpdateQueue();

	size_t PushToRX(const char* data, size_t len);
	size_t PopFromTX(char* data, size_t len);

	static uint32_t CharDevPool(chardev_t* dev);
	static size_t CharDevRead(chardev_t* dev, void* buf, size_t nbytes);
	static size_t CharDevWrite(chardev_t* dev, const void* buf, size_t nbytes);
	static void CharDevUpdate(chardev_t* dev);

	static SimpleDevice* GetDevice(chardev_t* dev) { if (dev) return (SimpleDevice*)dev->data; else return nullptr; };

	DEVICE_DECLARE_METHOD(SimpleDevice, Read)
	DEVICE_DECLARE_METHOD(SimpleDevice, Write)

private:
	bool add_chosen = false;
	chardev_t base = {};

	SharedSPSC pair;

	std::queue<char> tx_queue;
	std::queue<char> rx_queue;

	std::mutex tx_mutex;
	std::mutex rx_mutex;

	std::chrono::high_resolution_clock::time_point last_time;

END_DEVICE()

DECLARE_FACTORY(SimpleDevice)

DECLARE_REGISTER_FUNC(SimpleDevice)