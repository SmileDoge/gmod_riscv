#pragma once

#include <stdint.h>

class IMachine;
class IDevice;

typedef struct
{
	uint32_t id;

	IMachine* machine;
	IDevice* device;

	uint64_t address;
} GmodDeviceProxy;