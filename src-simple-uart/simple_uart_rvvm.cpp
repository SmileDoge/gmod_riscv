#include "simple_uart.h"

#include <stdio.h>

extern "C"
{
#include <fdtlib.h>

#include <devices/chardev.h>
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

	chardev_t base;
};

static simple_uart_t* get_simple_uart(chardev_t* dev)
{
	if (!dev || !dev->data) return nullptr;
	return (simple_uart_t*)dev->data;
}

static uint32_t simple_uart_poll(chardev_t* dev)
{
	simple_uart_t* uart = get_simple_uart(dev);
	if (!uart) return 0;
	uint32_t flags = 0;
	std::lock_guard<std::mutex> rx_lock(uart->rx_mutex);
	if (!uart->rx_queue.empty()) flags |= CHARDEV_RX;
	std::lock_guard<std::mutex> tx_lock(uart->tx_mutex);
	if (uart->tx_queue.size() < 4096) flags |= CHARDEV_TX; // assuming 4096 is the tx buffer size
	return flags;
}

static size_t simple_uart_read(chardev_t* dev, void* buf, size_t nbytes)
{
	simple_uart_t* uart = get_simple_uart(dev);
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

static size_t simple_uart_write(chardev_t* dev, const void* buf, size_t nbytes)
{
	simple_uart_t* uart = get_simple_uart(dev);
	if (!uart) return 0;
	size_t count = 0;
	std::lock_guard<std::mutex> lock(uart->tx_mutex);
	while (count < nbytes)
	{
		if (uart->tx_queue.size() >= 4096) break; // assuming 4096 is the tx buffer size
		uart->tx_queue.push(((const char*)buf)[count++]);
	}
	return count;
}

static void simple_uart_update(chardev_t* dev)
{
	simple_uart_t* uart = get_simple_uart(dev);
	if (!uart) return;

	uint32_t flags = simple_uart_poll(dev);

	if (flags)
	{
		chardev_notify(dev, flags);
	}
}

static void simple_uart_remove(chardev_t* dev)
{
	simple_uart_t* uart = get_simple_uart(dev);
	if (uart)
	{
		delete uart;
	}
}

chardev_t* chardev_simple_uart_create()
{
	simple_uart_t* uart = new simple_uart_t();
	uart->base.data = uart;
	uart->base.poll = simple_uart_poll;
	uart->base.read = simple_uart_read;
	uart->base.write = simple_uart_write;
	uart->base.update = simple_uart_update;
	uart->base.remove = simple_uart_remove;
	return &uart->base;
}

size_t chardev_simple_uart_push_rx(chardev_t* dev, const char* data, size_t len)
{
	simple_uart_t* uart = get_simple_uart(dev);
	if (!uart) return 0;
	size_t count = 0;
	std::lock_guard<std::mutex> lock(uart->rx_mutex);
	while (count < len)
	{
		uart->rx_queue.push(data[count++]);
	}
	return count;
}

size_t chardev_simple_uart_pop_tx(chardev_t* dev, char* buf, size_t len)
{
	simple_uart_t* uart = get_simple_uart(dev);
	if (!uart) return 0;
	size_t count = 0;
	std::lock_guard<std::mutex> lock(uart->tx_mutex);
	while (count < len && !uart->tx_queue.empty())
	{
		buf[count++] = uart->tx_queue.front();
		uart->tx_queue.pop();
	}
	return count;
}

rvvm_mmio_dev_t* simple_uart_get_mmio_dev(chardev_t* uart)
{
	if (!uart) return nullptr;
	if (!uart->data) return nullptr;
	return ((simple_uart_t*)uart->data)->mmio;
}

void simple_uart_set_mmio_dev(chardev_t* uart, rvvm_mmio_dev_t* mmio)
{
	if (!uart || !uart->data) return;
	((simple_uart_t*)uart->data)->mmio = mmio;
	}
/*
* // chardev_mem.c
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "chardev.h"
#include "chardev_mem.h"

typedef struct {
    chardev_t base;
    uint8_t* rx_buf;
    size_t   rx_head, rx_tail, rx_size;
    uint8_t* tx_buf;
    size_t   tx_head, tx_tail, tx_size;
    pthread_mutex_t lock;
} mem_dev_t;

static mem_dev_t* get_mem_dev(chardev_t* dev) {
    return (mem_dev_t*)dev->data;
}

static uint32_t mem_poll(chardev_t* dev) {
    mem_dev_t* m = get_mem_dev(dev);
    uint32_t flags = 0;
    pthread_mutex_lock(&m->lock);
    if (m->rx_head != m->rx_tail) flags |= CHARDEV_RX;
    if (((m->tx_tail + 1) % m->tx_size) != m->tx_head) flags |= CHARDEV_TX;
    pthread_mutex_unlock(&m->lock);
    return flags;
}

static size_t mem_read(chardev_t* dev, void* buf, size_t nbytes) {
    mem_dev_t* m = get_mem_dev(dev);
    size_t count = 0;
    pthread_mutex_lock(&m->lock);
    while (count < nbytes && m->rx_head != m->rx_tail) {
        ((uint8_t*)buf)[count++] = m->rx_buf[m->rx_tail];
        m->rx_tail = (m->rx_tail + 1) % m->rx_size;
    }
    pthread_mutex_unlock(&m->lock);
    return count;
}

static size_t mem_write(chardev_t* dev, const void* buf, size_t nbytes) {
    mem_dev_t* m = get_mem_dev(dev);
    size_t count = 0;
    pthread_mutex_lock(&m->lock);
    while (count < nbytes) {
        size_t next = (m->tx_head + 1) % m->tx_size;
        if (next == m->tx_tail) break; // full
        m->tx_buf[m->tx_head] = ((const uint8_t*)buf)[count++];
        m->tx_head = next;
    }
    pthread_mutex_unlock(&m->lock);
    return count;
}

static void mem_update(chardev_t* dev) {
    // nothing periodic
    (void)dev;
}

static void mem_remove(chardev_t* dev) {
    mem_dev_t* m = get_mem_dev(dev);
    free(m->rx_buf);
    free(m->tx_buf);
    pthread_mutex_destroy(&m->lock);
    free(m);
}

chardev_t* chardev_mem_create(size_t rx_size, size_t tx_size) {
    mem_dev_t* m = calloc(1, sizeof(*m));
    m->rx_size = rx_size + 1;
    m->tx_size = tx_size + 1;
    m->rx_buf = malloc(m->rx_size);
    m->tx_buf = malloc(m->tx_size);
    pthread_mutex_init(&m->lock, NULL);
    m->base.data   = m;
    m->base.poll   = mem_poll;
    m->base.read   = mem_read;
    m->base.write  = mem_write;
    m->base.update = mem_update;
    m->base.remove = mem_remove;
    return &m->base;
}

size_t chardev_mem_push_rx(chardev_t* dev, const uint8_t* data, size_t len) {
    mem_dev_t* m = get_mem_dev(dev);
    size_t count = 0;
    pthread_mutex_lock(&m->lock);
    while (count < len) {
        size_t next = (m->rx_head + 1) % m->rx_size;
        if (next == m->rx_tail) break; // full
        m->rx_buf[m->rx_head] = data[count++];
        m->rx_head = next;
    }
    pthread_mutex_unlock(&m->lock);
    return count;
}

size_t chardev_mem_pop_tx(chardev_t* dev, uint8_t* buf, size_t len) {
    mem_dev_t* m = get_mem_dev(dev);
    size_t count = 0;
    pthread_mutex_lock(&m->lock);
    while (count < len && m->tx_tail != m->tx_head) {
        buf[count++] = m->tx_buf[m->tx_tail];
        m->tx_tail = (m->tx_tail + 1) % m->tx_size;
    }
    pthread_mutex_unlock(&m->lock);
    return count;
}

*/

/*
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
*/