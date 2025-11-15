#include "simple_device.h"

#include "IMachine.h"

extern "C"
{
#include "rvvmlib.h"

#include "devices/ns16550a.h"
}

static int simple_device_metatable = 0;
static IEmulator* g_Emulator = nullptr;

bool SimpleDevice::OnCreate(nlohmann::json& json)
{
	if (json.contains("add_chosen") && json["add_chosen"].is_boolean())
		add_chosen = json["add_chosen"].get<bool>();

	return true;
}

size_t SimpleDevice::GetSize()
{
	return 0x1000;
}

void SimpleDevice::OnAttach(IMachine* machine, uint64_t addr)
{
 	if (!pair.Initialize(GetSharedPipe(GetUniqueID()), sizeof(SimpleDeviceMessage), SIMPLE_DEVICE_BUFFER_COUNT, GetRealm() == DeviceRealm::GMOD))
	{
		//g_Emulator->GetLogger()->LogError("Error init pipe!");
		GetEmulator()->GetLogger()->LogError("Error init pipe!");
		return;
	}

#ifndef RVVM_GMOD_SIDE
	memset(&base, 0, sizeof(base));

	base.data = this;
	base.poll = CharDevPool;
	base.read = CharDevRead;
	base.write = CharDevWrite;
	base.update = CharDevUpdate;

	rvvm_machine_t* rv_machine = (rvvm_machine_t*)machine->GetRawMachine();

	rvvm_intc_t* intc = rvvm_get_intc(rv_machine);
	rvvm_irq_t irq = rvvm_alloc_irq(intc);

	rvvm_mmio_dev_t* dev = ns16550a_init(rv_machine, &base, addr, intc, irq);

	if (add_chosen)
	{
		auto chosen = machine->GetFDTRoot()->Find("chosen");

		chosen->AddProp("stdout-path", "/soc/uart@10000000");
	}

	last_time = std::chrono::high_resolution_clock::now();

	SetRawDev((void*)dev);
#endif
}

CREATE_FACTORY(SimpleDevice)

//char receive_buffer[SIMPLE_DEVICE_BUFFER];

/*
DEVICE_CREATE_METHOD(SimpleDevice, Read)
{
	GmodDeviceProxy* proxy = g_Emulator->CheckDeviceProxy(LUA, 1);

	SimpleDevice* device = (SimpleDevice*)proxy->device;

	uint32_t avail = device->pipe.SizeToMain();

	uint32_t to_read = (std::min<size_t>(avail, SIMPLE_DEVICE_BUFFER));
	size_t count = 0;

	//memset(receive_buffer, 0, SIMPLE_DEVICE_BUFFER);

	//while (count < device->pipe.SizeToMain())
		//if (!device->pipe.PopFromWorker(&receive_buffer[count++])) break;

	//LUA->PushString(receive_buffer, count);

	for (uint32_t i = 0; i < to_read; i++)
	{
		if (!device->pipe.PopFromWorker(&receive_buffer[count])) break;
		++count;
	}

	LUA->PushString(receive_buffer, count);

	return 1;
}

DEVICE_CREATE_METHOD(SimpleDevice, Write)
{
	GmodDeviceProxy* proxy = g_Emulator->CheckDeviceProxy(LUA, 1);
	LUA->CheckString(2);

	SimpleDevice* device = (SimpleDevice*)proxy->device;

	uint32_t to_write = 0;

	const char* buf = LUA->GetString(2, &to_write);

	if (!buf || to_write == 0)
	{
		LUA->PushNumber(0);
		return 1;
	}

	uint32_t capacity = SIMPLE_DEVICE_BUFFER;

	uint32_t used = device->pipe.SizeToWorker();
	uint32_t free_slots = (used < capacity) ? (capacity - used) : 0;

	if (to_write > free_slots)
		LUA->ThrowError("Buffer overflow!");

	uint32_t written = 0;

	while (written < to_write)
	{
		if (!device->pipe.PushToWorker(&buf[written])) break;
		++written;
	}

	LUA->PushNumber(written);
	return 1;
}
*/

SimpleDeviceMessage recv_msg_gmod{};
SimpleDeviceMessage send_msg_gmod{};

DEVICE_CREATE_METHOD(SimpleDevice, Read)
{
	GmodDeviceProxy* proxy = g_Emulator->CheckDeviceProxy(LUA, 1);

	SimpleDevice* device = (SimpleDevice*)proxy->device;

	if (device->pair.PopFromWorker(&recv_msg_gmod))
	{
		LUA->PushString(recv_msg_gmod.data, recv_msg_gmod.size);
		return 1;
	}

	return 0;
}

DEVICE_CREATE_METHOD(SimpleDevice, Write)
{
	GmodDeviceProxy* proxy = g_Emulator->CheckDeviceProxy(LUA, 1);
	LUA->CheckString(2);

	SimpleDevice* device = (SimpleDevice*)proxy->device;

	uint32_t len = 0;
	const char* data = LUA->GetString(2, &len);

	if (len > 0)
	{
		send_msg_gmod.size = len;
		memcpy(send_msg_gmod.data, data, len);

		if (!device->pair.PushToWorker(&send_msg_gmod))
			len = 0;
	}

	LUA->PushNumber(len);
	return 1;
}

CREATE_REGISTER_FUNC(SimpleDevice)
{
	g_Emulator = emulator;

	simple_device_metatable = LUA->CreateMetaTable("SimpleDevice");
		DEVICE_SET_LUA_METHOD(SimpleDevice, Read)
		DEVICE_SET_LUA_METHOD(SimpleDevice, Write)

		LUA->PushCFunction(g_Emulator->GetMetaToString());
		LUA->SetField(-2, "__tostring");

		LUA->PushCFunction(g_Emulator->GetMetaIndex());
		LUA->SetField(-2, "__index");
	LUA->Pop();

	LUA->PushMetaTable(simple_device_metatable);
		LUA->PushNumber(simple_device_metatable);
		LUA->SetField(-2, "__device_type");
	LUA->Pop();

	return simple_device_metatable;
}

// rx = чтение из девайса, то есть GMOD TO WORKER
// tx = запись из девайса, то есть WORKER TO GMOD

uint32_t SimpleDevice::CharDevPool(chardev_t* dev)
{
	SimpleDevice* uart = GetDevice(dev);

	if (!uart) return 0;

	uint32_t flags = 0;

	std::lock_guard<std::mutex> rx_lock(uart->rx_mutex);
	if (!uart->rx_queue.empty()) flags |= CHARDEV_RX;

	std::lock_guard<std::mutex> tx_lock(uart->tx_mutex);
	if (uart->tx_queue.size() < SIMPLE_DEVICE_BUFFER_SIZE) flags |= CHARDEV_TX; // assuming 4096 is the tx buffer size

	return flags;
}

size_t SimpleDevice::CharDevRead(chardev_t* dev, void* buf, size_t nbytes)
{
	SimpleDevice* uart = GetDevice(dev);

	if (!uart) return 0;

	size_t count = 0;

	std::lock_guard<std::mutex> lock(uart->rx_mutex);

	while (count < nbytes && !uart->rx_queue.empty())
	{
		((char*)buf)[count++] = uart->rx_queue.front();
		uart->rx_queue.pop();
	}

	return count;
}

size_t SimpleDevice::CharDevWrite(chardev_t* dev, const void* buf, size_t nbytes)
{
	SimpleDevice* uart = GetDevice(dev);

	if (!uart) return 0;
	
	size_t count = 0;

	std::lock_guard<std::mutex> lock(uart->tx_mutex);

	while (count < nbytes)
	{
		if (uart->tx_queue.size() >= SIMPLE_DEVICE_BUFFER_SIZE) break;
		uart->tx_queue.push(((const char*)buf)[count++]);
	}

	return count;
}


void SimpleDevice::UpdateQueue()
{
	char receive_buffer_in_sub[SIMPLE_DEVICE_BUFFER_SIZE];

	SimpleDeviceMessage recv_msg_subprocess{};
	SimpleDeviceMessage send_msg_subprocess{};

	size_t len = PopFromTX(receive_buffer_in_sub, SIMPLE_DEVICE_BUFFER_SIZE);

	if (len > 0)
	{
		send_msg_subprocess.size = len;
		memcpy(send_msg_subprocess.data, receive_buffer_in_sub, len);

		pair.PushToMain(&send_msg_subprocess);
	}


	while (pair.PopFromMain(&recv_msg_subprocess))
		PushToRX(recv_msg_subprocess.data, recv_msg_subprocess.size);
}

size_t SimpleDevice::PushToRX(const char* data, size_t len)
{
	size_t count = 0;

	std::lock_guard<std::mutex> lock(rx_mutex);

	while (count < len)
	{
		rx_queue.push(data[count++]);
	}

	return count;
}

size_t SimpleDevice::PopFromTX(char* data, size_t len)
{
	size_t count = 0;

	std::lock_guard<std::mutex> lock(tx_mutex);

	while (count < len && !tx_queue.empty())
	{
		data[count++] = tx_queue.front();
		tx_queue.pop();
	}

	return count;
}


void SimpleDevice::CharDevUpdate(chardev_t* dev)
{
	SimpleDevice* uart = GetDevice(dev);

	if (!uart) return;

	uint32_t flags = CharDevPool(dev);

	if (flags)
		chardev_notify(dev, flags);

	auto current_time = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> delta = current_time - uart->last_time;

	if (delta.count() >= 1.0 / SIMPLE_DEVICE_UPDATE_FPS)
	{
		uart->UpdateQueue();
		uart->last_time = current_time;
	}
}
/*
size_t SimpleDevice::CharDevRead(chardev_t* dev, void* buf, size_t nbytes)
{
	SimpleDevice* uart = GetDevice(dev);

	if (!uart) return 0;

	size_t count = 0;

	char outch = 0;
	while (count < nbytes)
	{
		if (!uart->pipe.PopFromMain(&outch)) break;

		((char*)buf)[count++] = outch;
	}

	return count;
}

size_t SimpleDevice::CharDevWrite(chardev_t* dev, const void* buf, size_t nbytes)
{
	SimpleDevice* uart = GetDevice(dev);

	if (!uart) return 0;

	size_t count = 0;

	while (count < nbytes)
	{
		char outch = ((const char*)buf)[count++];

		if (!uart->pipe.PushToMain(&outch)) break;
	}

	return count;
}

void SimpleDevice::CharDevUpdate(chardev_t* dev)
{
	SimpleDevice* uart = GetDevice(dev);

	if (!uart) return;

	uint32_t flags = CharDevPool(dev);

	if (flags)
		chardev_notify(dev, flags);
}
*/