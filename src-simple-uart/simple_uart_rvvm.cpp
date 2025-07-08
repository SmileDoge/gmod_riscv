#include "simple_uart.h"

#include <stdio.h>

extern "C"
{
#include <fdtlib.h>
}

#include <queue>
#include <mutex>

struct simple_uart_t
{
	rvvm_mmio_dev_t* mmio;

	std::queue<char> tx_queue;
	std::queue<char> rx_queue;

	std::mutex tx_mutex;
	std::mutex rx_mutex;
};

static bool simple_uart_read(rvvm_mmio_dev_t* dev, void* data, size_t offset, uint8_t size)
{
	simple_uart_t* uart = (simple_uart_t*)dev->data;

	//*(uint8_t*)data = 0x00;
	uint8_t val = 0;

	if (offset == 5)
	{
		std::lock_guard<std::mutex> lock(uart->rx_mutex);

		uint8_t lsr = 0;

		if (!uart->rx_queue.empty()) lsr |= 0x01;

		lsr |= 0x20;

		val = lsr;

		/*
		if (!uart->rx_queue.empty())
		{
			*(uint8_t*)data = 0x61;
		}
		else
		{
			*(uint8_t*)data = 0x60;
		}
		*/
	}
	else if (offset == 0)
	{
		std::lock_guard<std::mutex> lock(uart->rx_mutex);
		if (!uart->rx_queue.empty())
		{
			val = uart->rx_queue.front();
			uart->rx_queue.pop();
		}
		else
		{
			val = 0x00;
		}
	}
	else if (offset == 3)
	{
		val = 0x03;
	}

	*(uint8_t*)data = val;

	return true;
}

static void simple_uart_update(rvvm_mmio_dev_t* dev)
{
	return;
}

static void simple_uart_remove(rvvm_mmio_dev_t* dev)
{
	simple_uart_t* uart = (simple_uart_t*)dev->data;
	if (uart)
	{
		delete uart;
	}
}

static const rvvm_mmio_type_t simple_uart_type = { "simple_uart", simple_uart_remove, simple_uart_update, 0 };

static bool simple_uart_write(rvvm_mmio_dev_t* dev, void* data, size_t offset, uint8_t size)
{
	simple_uart_t* uart = (simple_uart_t*)dev->data;

	if (offset == 0)
	{
		std::lock_guard<std::mutex> lock(uart->tx_mutex);

		uart->tx_queue.push(*(uint8_t*)data);
	}

	return true;
}

simple_uart_t* simple_uart_init(rvvm_machine_t* machine, size_t addr, size_t size, bool add_chosen)
{
	simple_uart_t* uart = new simple_uart_t();

	rvvm_mmio_dev_t mmio_uart = { 0 };

	mmio_uart.addr = addr; //0x10000000;
	mmio_uart.size = size; //0x1000;
	mmio_uart.read = simple_uart_read;
	mmio_uart.write = simple_uart_write;
	mmio_uart.type = &simple_uart_type;
	mmio_uart.data = uart;
	mmio_uart.min_op_size = 1;
	mmio_uart.max_op_size = 1;

	rvvm_mmio_dev_t* mmio = rvvm_attach_mmio(machine, &mmio_uart);

	if (mmio == NULL) {
		delete uart;

		return nullptr;
	}

	uart->mmio = mmio;

	struct fdt_node* uart_fdt = fdt_node_create_reg("uart", mmio_uart.addr);
	fdt_node_add_prop_reg(uart_fdt, "reg", mmio_uart.addr, mmio_uart.size);
	fdt_node_add_prop_str(uart_fdt, "compatible", "ns16550a");
	fdt_node_add_prop_u32(uart_fdt, "clock-frequency", 0x1000000);
	fdt_node_add_prop_u32(uart_fdt, "reg-shift", 0);
	fdt_node_add_prop_u32(uart_fdt, "reg-io-width", 1);
	fdt_node_add_child(rvvm_get_fdt_soc(machine), uart_fdt);

	if (add_chosen)
	{
		struct fdt_node* chosen = fdt_node_find(rvvm_get_fdt_root(machine), "chosen");
		fdt_node_add_prop_str(chosen, "stdout-path", "/soc/uart@10000000");
	}

	return uart;
}

rvvm_mmio_dev_t* simple_uart_get_mmio_dev(simple_uart_t* uart)
{
	return uart->mmio;
}

void simple_uart_write_buf(simple_uart_t* uart, const char* buf, size_t size)
{
	std::lock_guard<std::mutex> lock(uart->rx_mutex);
	for (size_t i = 0; i < size; ++i)
	{
		uart->rx_queue.push(buf[i]);
	}
}

size_t simple_uart_read_buf(simple_uart_t* uart, char* buf, size_t size)
{
	std::lock_guard<std::mutex> lock(uart->tx_mutex);
	size_t read_size = 0;
	while (!uart->tx_queue.empty() && read_size < size)
	{
		buf[read_size++] = uart->tx_queue.front();
		uart->tx_queue.pop();
	}
	return read_size;
}
