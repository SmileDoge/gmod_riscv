# Garry's Mod RISC-V Emulator

Based on [Modified RVVM](https://github.com/SmileDoge/RVVM/tree/gmod)

---

## Installation

To install from the releases, simply drag and drop all files into the root directory of your server or client, where `srcds.exe` or `hl2.exe` is located.

---

## Creating Custom Devices

- Check out the examples in the `src-simple-uart`, `src-web-fb` folders and `mmio_atomic.cpp` in main project.
- Device DLLs are loaded dynamically from the `./devices/*.dll` directory.

---


## Device Notes

* **load\_def\_devices** loads the following default devices: `clint`, `plic`, `rtc_goldfish`, `pci_bus`, `rtl8169`.

* **web\_fb** requires TCP port `8001` to be open.

* **mmio\_atomic** needs a driver but can essentially use `/dev/mem`.

* **simple\_uart** is still a work in progress and does not yet support interrupts.

---

## Build

Currently, building is only supported on **x86 Windows**.  
However, you can manually add `libturbojpeg` and compile [RVVM](https://github.com/SmileDoge/RVVM/tree/gmod)

---

## Network Setup

To enable networking within the emulator, run the following commands:

```sh
ifconfig eth0 up
ifconfig eth0 192.168.1.100 netmask 255.255.255.0
route add default gw 192.168.1.1
echo "nameserver 8.8.8.8" > /etc/resolv.conf
```

**Note:**
The `ip` command from BusyBox is slightly broken, and the default `S40network` scripts rely on it.

---

## Future Plans

- Complete implementation of all devices.  
- Refactor the codebase for better maintainability and extensibility.

