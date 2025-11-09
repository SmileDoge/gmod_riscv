#pragma once

#include "IDevice.h"
#include "IMachine.h"

#include "rvvmlib.h"

typedef struct
{
	IDevice* device;
	IMachine* machine;

	rvvm_mmio_dev_t* rv_device;
	rvvm_mmio_type_t  rv_type;
} SubprocessDeviceProxy;