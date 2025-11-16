#pragma once

#include <stdint.h>

class IMachine;
class IDevice;

typedef struct
{
	uint32_t id;

	IMachine* machine;
	IDevice* device;

	alignas(8) uint64_t address;
} GmodDeviceProxy;