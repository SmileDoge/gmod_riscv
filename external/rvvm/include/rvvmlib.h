/*
rvvmlib.h - RISC-V Virtual Machine Public API
Copyright (C) 2020-2024 LekKit <github.com/LekKit>
                        cerg2010cerg2010 <github.com/cerg2010cerg2010>
                        Mr0maks <mr.maks0443@gmail.com>
                        KotB <github.com/0xCatPKG>
                        X547 <github.com/X547>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef LIBRVVM_H
#define LIBRVVM_H

#ifdef __cplusplus
#define RVVM_EXTERN_C_BEGIN extern "C" {
#define RVVM_EXTERN_C_END }
#else
#define RVVM_EXTERN_C_BEGIN
#define RVVM_EXTERN_C_END
#endif

RVVM_EXTERN_C_BEGIN

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(_WIN32) && defined(RVVMLIB_SHARED)
#define PUBLIC __declspec(dllimport)
#elif defined(_WIN32) && defined(USE_LIB)
#define PUBLIC __declspec(dllexport)
#elif __GNUC__ >= 4 && (defined(RVVMLIB_SHARED) || defined(USE_LIB))
#define PUBLIC __attribute__((visibility("default")))
#else
// It's a static lib or rvvm-cli
#define PUBLIC
#endif

#ifndef RVVM_VERSION
//! Version string
#define RVVM_VERSION "0.7-git"
#endif

//! Increments on each API/ABI breakage, equal to -1 in unstable staging builds
#define RVVM_ABI_VERSION 9

//! \brief  Check librvvm ABI compatibility via rvvm_check_abi(RVVM_ABI_VERSION)
//! \return True if librvvm supports the header ABI
PUBLIC bool rvvm_check_abi(int abi);

/*
 * TODO: Rename this header into librvvm.h after API freeze for 1.0?
 */

/**
 * @defgroup rvvm_machine_api RVVM Machine Management API
 * @addtogroup rvvm_machine_api
 * @{
 */

//! Physical memory address or similar opaque type
typedef uint64_t rvvm_addr_t;

//! Machine handle
typedef struct rvvm_machine_t rvvm_machine_t;

// Machine options (Settable)
#define RVVM_OPT_NONE         0x0
#define RVVM_OPT_RESET_PC     0x1 //!< Physical jump address at reset, defaults to 0x80000000
#define RVVM_OPT_DTB_ADDR     0x2 //!< Pass DTB address if non-zero, omits FDT generation
#define RVVM_OPT_TIME_FREQ    0x3 //!< Machine timer frequency, 10Mhz by default
#define RVVM_OPT_HW_IMITATE   0x4 //!< Imitate traits or identity of physical hardware
#define RVVM_OPT_MAX_CPU_CENT 0x5 //!< Maximum CPU load % per guest/host CPUs
#define RVVM_OPT_JIT          0x6 //!< Enable JIT
#define RVVM_OPT_JIT_CACHE    0x7 //!< Amount of per-core JIT cache (In bytes)
#define RVVM_OPT_JIT_HARVARD  0x8 //!< No dirty code tracking, explicit ifence, slower

// Machine options (Special function or read-only)
#define RVVM_OPT_MEM_BASE   0x80000001U //!< Physical RAM base address, defaults to 0x80000000
#define RVVM_OPT_MEM_SIZE   0x80000002U //!< Physical RAM size
#define RVVM_OPT_HART_COUNT 0x80000003U //!< Amount of harts

// Internal use ONLY!
#define RVVM_OPTS_ARR_SIZE 0x9

//! Default memory base address
#define RVVM_DEFAULT_MEMBASE 0x80000000U

//! \brief Creates a new virtual machine
//! \param mem_size   Amount of memory (in bytes), should be page-aligned
//! \param hart_count Amount of HARTs (cores), should be >=1
//! \param isa        String describing the CPU instruction set, or NULL to pick rv64
//! \return Valid machine handle, or NULL on failure
PUBLIC rvvm_machine_t* rvvm_create_machine(size_t mem_size, size_t hart_count, const char* isa);

//! \brief Set a new kernel cmdline for a manually loaded kernel
PUBLIC void rvvm_set_cmdline(rvvm_machine_t* machine, const char* str);

//! \brief Append to the kernel cmdline for a manually loaded kernel
PUBLIC void rvvm_append_cmdline(rvvm_machine_t* machine, const char* str);

//! \brief Load M-mode firmware (bootrom), which is executed from RAM base on reset
PUBLIC bool rvvm_load_bootrom(rvvm_machine_t* machine, const char* path);

//! \brief Load S-mode payload (kernel), which is usually the next stage after OpenSBI
PUBLIC bool rvvm_load_kernel(rvvm_machine_t* machine, const char* path);

//! \brief Load a custom Device Tree blob, which is passed to guest at reset.
PUBLIC bool rvvm_load_dtb(rvvm_machine_t* machine, const char* path);

//! \brief Dump generated Device Tree to a file
PUBLIC bool rvvm_dump_dtb(rvvm_machine_t* machine, const char* path);

//! \brief Get machine option value
PUBLIC rvvm_addr_t rvvm_get_opt(rvvm_machine_t* machine, uint32_t opt);

//! \brief Set machine option
PUBLIC bool rvvm_set_opt(rvvm_machine_t* machine, uint32_t opt, rvvm_addr_t val);

//! \brief  Powers up or resumes a paused machine, returns immediately
//! \return Machine start success, false if it was already running
PUBLIC bool rvvm_start_machine(rvvm_machine_t* machine);

//! \brief  Pauses the machine (Stops the vCPUs)
//! \return Machine pause success, false if it wasn't running
PUBLIC bool rvvm_pause_machine(rvvm_machine_t* machine);

//! \brief  Reset the machine (Continues running if it was powered)
PUBLIC void rvvm_reset_machine(rvvm_machine_t* machine, bool reset);

//! \brief  Returns true if the machine is currently running and not paused
PUBLIC bool rvvm_machine_running(rvvm_machine_t* machine);

//! \brief  Returns true if the machine is powered on (Even when it's paused)
PUBLIC bool rvvm_machine_powered(rvvm_machine_t* machine);

//! \brief   Complete machine state cleanup (Frees memory, attached devices, internal structures)
//! \warning After this call, none of the handles previously attached to this machine are valid
PUBLIC void rvvm_free_machine(rvvm_machine_t* machine);

//! \brief Run the event loop in the calling thread, returns when any machine is paused or powered off
PUBLIC void rvvm_run_eventloop(void);

//! \brief External tick event loop
PUBLIC void rvvm_external_tick_eventloop(bool manual);

//! \brief Set the event loop to manual mode
PUBLIC void rvvm_external_set_manual(bool manual);

//! \brief Initialize machine for single-step
PUBLIC void rvvm_external_init_single_step(rvvm_machine_t* machine);

//! \brief Step the machine in current thread
PUBLIC void rvvm_external_step_machine(rvvm_machine_t* machine, uint16_t hart_id);

//! \brief Tick events for the machine
PUBLIC void rvvm_external_eventloop_tick_machine(rvvm_machine_t* machine);

PUBLIC void rvvm_external_global_lock();

PUBLIC void rvvm_external_global_unlock();

/** @}*/

/**
 * @defgroup rvvm_device_api RVVM Device API
 * @addtogroup rvvm_device_api
 * @{
 */

//! FDT node for Device Tree generation
struct fdt_node;

//! MMIO device handle
typedef struct rvvm_mmio_dev_t rvvm_mmio_dev_t;

//! MMIO read/write handler, offset is always aligned to operation size
typedef bool (*rvvm_mmio_handler_t)(rvvm_mmio_dev_t* dev, void* dest, size_t offset, uint8_t size);

//! Dummy MMIO read/write: Reads zeros, ignores writes, never faults
PUBLIC bool rvvm_mmio_none(rvvm_mmio_dev_t* dev, void* dest, size_t offset, uint8_t size);

//! Interrupt controller handle
typedef struct rvvm_intc_t rvvm_intc_t;

//! PCI Bus (PCI Express Root Complex) handle
typedef struct rvvm_pci_bus pci_bus_t;

//! I2C Bus handle
typedef struct rvvm_i2c_bus i2c_bus_t;

//! MMIO device type-specific information and handlers (Cleanup, reset, serialize)
typedef struct {
    //! Human-readable device name
    const char* name;

    //! Called to free device state (LIFO order), dev->data is simply freed if this is NULL
    void (*remove)(rvvm_mmio_dev_t* dev);

    //! Called periodically from event thread
    void (*update)(rvvm_mmio_dev_t* dev);

    //! Called on machine reset
    void (*reset)(rvvm_mmio_dev_t* dev);

    /*
     * TODO
     * void (*suspend)(rvvm_mmio_dev_t* dev, rvvm_state_t* state);
     * void (*resume)(rvvm_mmio_dev_t* dev, rvvm_state_t* state);
     */

} rvvm_mmio_type_t;

//! MMIO region description
struct rvvm_mmio_dev_t {
    rvvm_addr_t addr;        //!< MMIO region address in machine physical memory
    size_t      size;        //!< MMIO region size, zero means a device placeholder
    void*       data;        //!< Device-specific data pointer, freed on removal if type->remove is NULL
    void*       mapping;     //!< Directly mapped host memory region, read/write called on dirtying if non-NULL
    rvvm_machine_t* machine; //!< Owner machine handle

    //! Device class specific operations & info, may be NULL
    const rvvm_mmio_type_t* type;

    //! Called on MMIO region read if non-NULL
    rvvm_mmio_handler_t read;

    //! Called on MMIO region write if non-NULL
    rvvm_mmio_handler_t write;

    uint8_t min_op_size; //!< Minimum MMIO operation size allowed
    uint8_t max_op_size; //!< Maximum MMIO operation size allowed
};

//! \brief  Writes data to machine physical memory
PUBLIC bool rvvm_write_ram(rvvm_machine_t* machine, rvvm_addr_t dest, const void* src, size_t size);

//! \brief  Reads data from machine physical memory
PUBLIC bool rvvm_read_ram(rvvm_machine_t* machine, void* dest, rvvm_addr_t src, size_t size);

//! \brief  Directly access machine physical memory (DMA)
//! \return Pointer to machine DMA region, or NULL on failure
PUBLIC void* rvvm_get_dma_ptr(rvvm_machine_t* machine, rvvm_addr_t addr, size_t size);

//! \brief  Get usable address for a MMIO region
//! \return Usable physical memory address, which is equal to addr if the requested region is free
PUBLIC rvvm_addr_t rvvm_mmio_zone_auto(rvvm_machine_t* machine, rvvm_addr_t addr, size_t size);

//! \brief   Attach (custom-written) MMIO device to the machine by it's description, free it's state on failure
//! \param   mmio_desc MMIO region description, doesn't need to be kept
//! \return  Valid MMIO region handle, or NULL on failure
//! \warning Dereferencing an attached rvvm_mmio_dev_t* should be done thread-safely using atomics/fences
PUBLIC rvvm_mmio_dev_t* rvvm_attach_mmio(rvvm_machine_t* machine, const rvvm_mmio_dev_t* mmio_desc);

//! \brief Detach (pull out) MMIO device from the owning machine, free it's state
PUBLIC void rvvm_remove_mmio(rvvm_mmio_dev_t* mmio_dev);

//! \brief Clean up MMIO device state if it's not attached to any machine
PUBLIC void rvvm_cleanup_mmio_desc(const rvvm_mmio_dev_t* mmio_desc);

//! \brief Get wired interrupt controller handle of this machine or NULL
PUBLIC rvvm_intc_t* rvvm_get_intc(rvvm_machine_t* machine);
PUBLIC void         rvvm_set_intc(rvvm_machine_t* machine, rvvm_intc_t* intc);

//! \brief Get PCI rooot complex handle of this machine or NULL
PUBLIC pci_bus_t* rvvm_get_pci_bus(rvvm_machine_t* machine);
PUBLIC void       rvvm_set_pci_bus(rvvm_machine_t* machine, pci_bus_t* pci_bus);

//! \brief Get I2C bus handle of this machine, or NULL
PUBLIC i2c_bus_t* rvvm_get_i2c_bus(rvvm_machine_t* machine);
PUBLIC void       rvvm_set_i2c_bus(rvvm_machine_t* machine, i2c_bus_t* i2c_bus);

//! \brief Get root FDT node (For custom FDT generation)
PUBLIC struct fdt_node* rvvm_get_fdt_root(rvvm_machine_t* machine);

//! \brief Get /soc FDT node (For custom FDT generation)
PUBLIC struct fdt_node* rvvm_get_fdt_soc(rvvm_machine_t* machine);

/** @}*/

/**
 * @defgroup rvvm_irq_api RVVM Interrupt API
 * @addtogroup rvvm_irq_api
 * @{
 */

//! \brief Sends an MSI IRQ by issuing a 32-bit little-endian memory write
PUBLIC bool rvvm_send_msi_irq(rvvm_machine_t* machine, rvvm_addr_t addr, uint32_t val);

//! \brief Attach an MSI IRQ controller, semantically almost same as rvvm_attach_mmio()
PUBLIC bool rvvm_attach_msi_target(rvvm_machine_t* machine, const rvvm_mmio_dev_t* mmio_desc);

//! Interrupt line identifier
typedef uint32_t rvvm_irq_t;

//! Interrupt controller description
struct rvvm_intc_t {
    void* data;
    uint32_t last_irq;
    bool (*send_irq)(rvvm_intc_t* intc, rvvm_irq_t irq);
    bool (*raise_irq)(rvvm_intc_t* intc, rvvm_irq_t irq);
    bool (*lower_irq)(rvvm_intc_t* intc, rvvm_irq_t irq);
    rvvm_irq_t (*alloc_irq)(rvvm_intc_t* intc);
    uint32_t (*fdt_phandle)(rvvm_intc_t* intc);
    size_t (*fdt_irq_cells)(rvvm_intc_t* intc, rvvm_irq_t irq, uint32_t* cells, size_t size);
};

//! \brief Allocate a new IRQ pin on the IRQ controller
PUBLIC rvvm_irq_t rvvm_alloc_irq(rvvm_intc_t* intc);

//! \brief Send an edge-triggered IRQ through IRQ controller pin
PUBLIC bool rvvm_send_irq(rvvm_intc_t* intc, rvvm_irq_t irq);

//! \brief Assert/lower an IRQ pin on the IRQ controller
PUBLIC bool rvvm_raise_irq(rvvm_intc_t* intc, rvvm_irq_t irq);
PUBLIC bool rvvm_lower_irq(rvvm_intc_t* intc, rvvm_irq_t irq);

//! \brief Add interrupt-parent & interrupts fields to device FDT node
PUBLIC bool rvvm_fdt_describe_irq(struct fdt_node* node, rvvm_intc_t* intc, rvvm_irq_t irq);

//! \brief Get interrupt-parent FDT phandle of an interrupt controller
PUBLIC uint32_t rvvm_fdt_intc_phandle(rvvm_intc_t* intc);

//! \brief Get interrupts-extended FDT cells for an IRQ
PUBLIC size_t rvvm_fdt_irq_cells(rvvm_intc_t* intc, rvvm_irq_t irq, uint32_t* cells, size_t size);

/** @}*/

/**
 * @defgroup rvvm_userland_api RVVM Userland Emulation API
 * @addtogroup rvvm_userland_api
 * @{
 */

//! Thread handle
typedef struct rvvm_hart_t rvvm_hart_t;

//! Base X0 register index
#define RVVM_REGID_X0    0

//! Base F0 FPU register index, FPU regs are operated in binary form
#define RVVM_REGID_F0    32

//! Program counter register
#define RVVM_REGID_PC    1024

//! CSR cause register
#define RVVM_REGID_CAUSE 1025

//! CSR tval register
#define RVVM_REGID_TVAL  1026

//! \brief Create a userland process context
PUBLIC rvvm_machine_t* rvvm_create_userland(const char* isa);

//! \brief Flush instruction cache for a specified memory range
PUBLIC void rvvm_flush_icache(rvvm_machine_t* machine, rvvm_addr_t addr, size_t size);

//! \brief Create userland process thread
PUBLIC rvvm_hart_t* rvvm_create_user_thread(rvvm_machine_t* machine);

//! \brief Destroy userland process thread
PUBLIC void rvvm_free_user_thread(rvvm_hart_t* thread);

//! \brief  Run a userland thread until a trap happens.
//! \return Returns trap cause. PC points to faulty instruction upon return.
PUBLIC rvvm_addr_t rvvm_run_user_thread(rvvm_hart_t* thread);

//! \brief Read thread context register
PUBLIC rvvm_addr_t rvvm_read_cpu_reg(rvvm_hart_t* thread, size_t reg_id);

//! \brief Write thread context register
PUBLIC void rvvm_write_cpu_reg(rvvm_hart_t* thread, size_t reg_id, rvvm_addr_t reg);

/** @}*/

RVVM_EXTERN_C_END

#endif
