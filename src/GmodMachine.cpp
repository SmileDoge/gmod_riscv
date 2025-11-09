#include "GmodMachine.h"

#include "GmodEmulator.h"

const char* getHuTao();

GmodMachine::GmodMachine() : id(-1), bios_path_present(false), kernel_path_present(false), dtb_path_present(false), ram_count(0), hart_count(0)
{
}

GmodMachine::~GmodMachine()
{
	Deinitialize();
}

uint32_t GmodMachine::GetID()
{
	return id;
}

uint32_t GmodMachine::GetHartCount()
{
	return hart_count;
}

uint64_t GmodMachine::GetRAMCount()
{
	return ram_count;
}

bool GmodMachine::IsRunning()
{
	if (!status.IsOpen()) return false;

	return status->running;
}

bool GmodMachine::IsPowered()
{
	if (!status.IsOpen()) return false;

	return status->powered;
}

bool GmodMachine::GetBIOSPath(std::string& path)
{
	if (!bios_path_present) return false;

	path = bios_path;

	return true;
}

bool GmodMachine::GetKernelPath(std::string& path)
{
	if (!kernel_path_present) return false;

	path = kernel_path;

	return true;
}

bool GmodMachine::GetDTBPath(std::string& path)
{
	if (!dtb_path_present) return false;

	path = dtb_path;

	return true;
}

bool GmodMachine::AttachDevice(IDevice* dev, uint64_t addr)
{
	if (!dev)
		return false;

	if (GetDevice(dev->GetUniqueID()))
		return false;

	dev->OnAttach(this, addr);

	GmodDeviceProxy* proxy = new GmodDeviceProxy();

	memset(proxy, 0, sizeof(GmodDeviceProxy));

	proxy->id = dev->GetUniqueID();
	proxy->machine = this;
	proxy->device = dev;
	proxy->address = addr;

	devices[dev->GetUniqueID()] = proxy;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::ATTACH_DEVICE;

	msg.attachDevice.id = dev->GetUniqueID();
	msg.attachDevice.addr = addr;

	return pair.PushToWorker(&msg);
}

bool GmodMachine::RemoveDevice(IDevice* dev)
{
	return false;
}

bool GmodMachine::AttachDevice(GmodDeviceProxy* dev_proxy, uint64_t addr)
{
	if (!dev_proxy)
		return false;

	IDevice* dev = dev_proxy->device;

	if (!dev)
		return false;

	if (GetDevice(dev->GetUniqueID()))
		return false;

	dev->OnAttach(this, addr);

	dev_proxy->address = addr;
	dev_proxy->machine = this;

	devices[dev->GetUniqueID()] = dev_proxy;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::ATTACH_DEVICE;

	msg.attachDevice.id = dev->GetUniqueID();
	msg.attachDevice.addr = addr;

	return pair.PushToWorker(&msg);
}

bool GmodMachine::RemoveDevice(GmodDeviceProxy* dev_proxy)
{
	return false;
}

IDevice* GmodMachine::GetDevice(uint32_t id) noexcept
{
	if (devices.find(id) == devices.end())
		return nullptr;

	return devices.at(id)->device;
}

GmodDeviceProxy* GmodMachine::GetDeviceProxy(uint32_t id) noexcept
{
	if (devices.find(id) == devices.end())
		return nullptr;

	return devices.at(id);
}

bool GmodMachine::IsReady() noexcept
{
	if (!status.IsOpen())
		return false;

	return status->ready;
}

void GmodMachine::WaitForReady(int timeout_ms) noexcept
{
	if (!status.IsOpen())
		return;

	int waited = 0;
	const int sleep_interval = 10;
	while (!status->ready && (timeout_ms < 0 || waited < timeout_ms))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_interval));
		waited += sleep_interval;
	}
}

bool GmodMachine::Initialize(uint32_t id, uint32_t hart_count, uint64_t ram_count)
{
	printf("%s\n", getHuTao());

	if (id == -1) return false;

	if (!pair.Initialize(IMachine::GetPipeSharedPath(id), sizeof(MachineMessage), MACHINE_MAX_MESSAGES, true))
	{
		RV_ERROR("GmodMachine::Initialize - pair.Initialize error");
		return false;
	}

	if (!status.CreateShared(IMachine::GetStatusSharedPath(id)))
	{
		pair.Close();
		RV_ERROR("GmodMachine::Initialize - status.CreateShared error");
		return false;
	}

	this->id = id;
	this->hart_count = hart_count;
	this->ram_count = ram_count;

	return true;
}

void GmodMachine::Deinitialize()
{
	pair.Close();
	status.Close();
}

bool GmodMachine::SetCommandLine(const std::string& cmd_line)
{
	if (!IsReady())
		return false;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::SET_CMD_LINE;

	std::snprintf(msg.setPath.path, sizeof(msg.setPath.path), "%s", cmd_line.c_str());

	return pair.PushToWorker(&msg);
}

bool GmodMachine::AppendCommandLine(const std::string& cmd_line)
{
	if (!IsReady())
		return false;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::APPEND_CMD_LINE;

	std::snprintf(msg.setPath.path, sizeof(msg.setPath.path), "%s", cmd_line.c_str());

	return pair.PushToWorker(&msg);
}

bool GmodMachine::SetBIOSPath(const std::string& path)
{
	if (!IsReady())
		return false;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::SET_BIOS_PATH;
	
	std::snprintf(msg.setPath.path, sizeof(msg.setPath.path), "%s", path.c_str());

	return pair.PushToWorker(&msg);
}

bool GmodMachine::SetKernelPath(const std::string& path)
{
	if (!IsReady())
		return false;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::SET_KERNEL_PATH;

	std::snprintf(msg.setPath.path, sizeof(msg.setPath.path), "%s", path.c_str());

	return pair.PushToWorker(&msg);
}

bool GmodMachine::SetDTBPath(const std::string& path)
{
	if (!IsReady())
		return false;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::SET_DTB_PATH;

	std::snprintf(msg.setPath.path, sizeof(msg.setPath.path), "%s", path.c_str());

	return pair.PushToWorker(&msg);
}

bool GmodMachine::DumpDTB(const std::string& path)
{
	if (!IsReady())
		return false;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::DUMP_DTB;

	std::snprintf(msg.setPath.path, sizeof(msg.setPath.path), "%s", path.c_str());

	return pair.PushToWorker(&msg);
}

bool GmodMachine::StartResume()
{
	if (!IsReady())
		return false;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::START_RESUME;

	return pair.PushToWorker(&msg);
}

bool GmodMachine::Pause()
{
	if (!IsReady())
		return false;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::PAUSE;

	return pair.PushToWorker(&msg);
}

bool GmodMachine::Restart()
{
	if (!IsReady())
		return false;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::RESTART;

	return pair.PushToWorker(&msg);
}

bool GmodMachine::Shutdown()
{
	if (!IsReady())
		return false;

	MachineMessage msg = {};

	msg.typeToSubprocess = MachineToSubprocess::Type::SHUTDOWN;

	return pair.PushToWorker(&msg);
}

const char* hu_tao = R"(⢀⢀⢀⠄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⢆⢔⠤⡢⢔⠤⡢⡰⡂⠀⠀⠀⢠⠔⡤⢢⠔⡤⢢⠔⡤⠢⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⢀⢀⠀⡄
⠠⡁⠆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠐⡕⣅⠳⡘⢆⠳⡘⣌⢝⠀⠀⢀⢕⠱⡊⢖⠱⡊⢖⠕⡜⡅⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢅⠌⡂⠘
⢁⠀⠀⠀⠐⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡣⡪⡪⡑⡕⢍⢎⢔⢕⠂⠀⡰⡑⡍⢎⢕⢍⢪⢊⢎⠪⠀⠀⠀⠀⠀⠀⠀⢀⠀⡀⢀⠀⠀⠀⠀⠀⠀⠀⠂⢨⠀⠅
⠀⠀⠀⠈⠀⠀⠈⠀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢘⢔⢕⠱⡩⡪⢢⠣⡱⠅⠀⢜⢌⢎⢪⠢⡣⡱⡡⡣⠃⠀⢀⠀⠁⠀⠂⠀⠄⠀⡀⠀⢀⠀⠁⠀⠂⠀⠠⠀⠀⠄⠑
⠀⠀⠁⢀⠀⠁⢀⠀⠀⠀⠁⠀⠠⠀⢀⠀⢀⠀⢀⠀⢀⠀⠠⠀⠀⠄⠀⠄⠀⠄⠀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡣⠱⡑⠕⡌⢣⠑⠕⢅⠐⢕⢅⡣⡱⡑⡜⢔⠕⡜⠀⠀⠀⠀⠈⠀⠀⠂⢀⠀⠀⠠⠀⠀⠂⠀⠂⠀⠀⠀⠠⠈⡐
⠀⠠⠀⠀⠀⡀⠀⠀⠈⠀⠐⠀⢀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠀⠀⠂⠀⠂⠀⠐⠀⡀⠐⠀⠠⠀⠀⠐⠀⠐⠀⠐⠀⠈⠀⡀⠠⠀⡀⠠⠈⠈⠈⠊⠘⠄⠀⠀⠈⠀⠠⠈⠀⠀⠀⠀⠂⠀⠄⠐⠀⠠⠀⠀⢂⠀⠂⡐⢀
⠀⠀⠀⠂⠀⠀⠀⠁⠀⠄⠀⠄⠀⠀⠈⠀⠈⠀⠈⠀⠀⠁⠀⠀⠀⠀⠁⠀⠠⠀⠐⠀⠀⠂⠀⠂⢀⠀⠂⠀⠂⠀⡀⠀⠂⠠⠀⠁⠀⠂⠐⠀⠐⠀⠂⢀⠀⡀⢀⠀⠀⠄⠀⢁⠀⠂⠀⠠⠀⠁⠀⠄⠀⠀⠁⠈⠀⢀⠀⠀⢀⠀⠀⠂⠄⠡⡀⠂⠀⠄
⠀⠀⠁⠀⠀⠁⠀⠂⠀⡀⠀⠀⠄⠀⠁⠀⠀⠀⠄⠀⠀⠄⠀⠐⠀⠂⠐⠀⠀⠂⢀⠈⢀⠀⠁⡀⢀⠀⠐⠀⠈⠀⠀⠐⠀⠠⠀⠈⠀⠀⠂⠈⠀⠀⠄⠀⠀⠀⠀⠀⠐⠀⠠⠀⠀⠀⠁⠀⠄⠈⠀⡀⠈⠀⠐⠀⠄⠀⠀⠂⠀⠀⠀⠂⠈⠄⡠⠀⠌⠐
⠀⠈⠀⠈⠀⠐⠀⢀⠀⠀⢀⠀⠀⠀⠀⡀⠈⠀⠀⠐⠀⠄⠈⠀⠄⠀⠂⠈⠀⠠⠀⠀⠀⠀⠀⠀⠀⠀⠀⠄⠐⠀⠠⠀⢀⠀⠀⠐⠀⠁⠀⠀⠀⠄⠀⠀⠐⠀⠐⠀⠀⠄⠀⢀⠀⠈⠀⠀⢀⠀⠀⠀⠀⠁⠀⡀⠀⠈⠀⠀⠐⠀⡀⠈⢀⠐⠠⠀⠄⠁
⠈⠀⠀⠂⠀⠄⠀⠀⠀⠀⠀⢀⠀⠈⠀⡀⢀⠈⠀⠂⠀⡀⠐⠀⠀⠠⠀⠀⠄⠀⠀⡀⠈⠀⠈⡀⠠⠀⠄⠠⠀⠄⠐⠠⠠⠀⠅⠨⠀⠊⠠⠁⠌⡀⠊⠠⠁⠌⡀⠡⢈⠀⡡⠀⠂⢂⠐⠁⠠⢀⠈⡀⠄⠐⠀⠀⠀⠄⠈⠀⠀⡀⠀⠀⠀⠀⠀⠑⠀⠂
⠀⠐⠀⠠⠀⠀⠀⠐⠀⠀⠁⠀⠀⠁⠀⠀⠀⠀⠀⠄⠀⠀⠀⠀⠄⢀⠠⠀⠄⢂⠐⠀⠡⡈⠐⠠⠈⠄⠡⡈⠐⡈⠄⡁⠂⠡⠈⠄⡁⠡⠈⢂⠐⡀⢁⠂⡁⠂⠄⡁⠐⠄⠠⠈⠄⡁⢈⠄⡁⠐⠄⠄⠨⠀⢅⠈⠄⠀⠀⢀⠀⠀⠀⠈⠀⠀⠁⠀⠀⠈
⠄⠀⠄⠀⠀⠀⠁⡀⠀⠁⠈⠀⠀⠁⠀⠐⠀⠐⠀⠄⠐⡀⠅⡈⠐⠠⡀⢁⠢⠀⠄⠑⠠⠀⠅⠨⡀⠑⠠⡀⠡⡀⠐⡀⠅⡈⢂⠐⡀⠅⡈⠄⢐⠀⢂⠐⡀⠡⠐⢈⠀⢊⠀⡁⢂⠐⠠⠠⡀⡁⢂⠈⡐⠀⡂⠐⡈⠄⢁⠀⠀⠀⠐⠀⠀⠄⠀⠐⠀⠀
⢀⠀⢀⠀⠈⠀⠀⠀⠀⠄⠀⠀⠂⡀⠔⠀⠊⠠⠁⠌⠠⠐⠠⡀⡁⢂⠐⠠⠀⡐⠈⠄⠡⠈⠄⠡⢀⠑⠠⠐⠠⡀⢈⠀⡂⢐⠀⡂⠔⢀⠢⠈⠄⢌⠠⠂⡐⠄⡁⡐⢀⢁⠐⡀⢂⠈⡐⠠⠀⠔⠀⢂⠈⠄⠠⠁⡐⢀⠂⠄⠀⠂⠀⠀⠄⠀⠀⡀⠠⠀
⠀⠀⠀⠀⡀⠈⠀⠈⡀⢀⠂⡁⠢⠀⢂⠁⡡⠈⠂⠨⡀⢁⠂⠄⠐⠠⠈⠄⠐⡀⠅⠨⠀⢅⠈⠢⡀⢊⠠⠑⡠⢐⠄⢈⠄⢂⠡⠐⡈⠄⢂⢁⠊⡠⠠⠁⠔⢈⠄⠌⠢⠠⡑⠐⢄⠅⡐⠌⡐⢈⠠⠁⡐⢈⠀⡂⠐⡀⠂⠢⠀⠀⠠⠀⢀⠀⠀⠀⠀⠀
⠂⠈⠀⠀⠀⢀⠐⡀⠂⠄⠂⡐⠀⠅⡐⠠⠀⠌⠠⢁⠠⠂⠐⡈⠄⠡⠈⠠⠈⠄⡈⡐⠡⡀⡑⠐⠄⡡⠐⡁⠄⡸⡂⠐⢄⠡⠐⡁⠔⢈⠄⠂⠔⠠⡈⢂⢁⠂⠔⠈⢂⠅⢌⠊⢄⢊⠐⠌⢄⠡⢂⠅⡰⠀⢂⠈⠄⠠⡁⠐⡁⠀⠀⠀⠀⠀⠐⠀⠠⠀
⠀⠀⡀⠐⢀⠂⡐⢀⠡⠈⠄⠠⠁⢂⠐⠠⢁⠈⠂⠄⢂⢈⠐⠠⠈⠄⢁⠐⡁⠐⠄⢌⠐⠄⡨⢈⠐⠄⢌⠐⢀⢯⡂⢁⠢⡈⢂⠔⢈⠄⢌⠈⡊⠔⡀⠢⠠⡈⢂⠁⠂⠔⠠⠑⢄⠢⠑⡑⢄⢑⠐⠔⠠⠃⠢⡀⢁⠂⠄⡁⢐⠀⠁⠀⠁⠀⠄⠀⡀⠀
⠈⠀⠠⠈⠄⠐⡀⠂⠄⠊⡀⠡⠈⠄⡈⢐⠀⠡⡈⠐⠄⠠⠊⡀⠑⠀⠂⠄⢌⠈⡂⠢⢈⠂⠔⠠⡁⢊⠄⡁⡸⡢⡳⠀⠢⡈⠔⠠⡁⠢⠐⡈⠔⡈⠔⡁⠢⡨⠠⡁⠌⠐⡁⢑⠠⢃⠜⡰⢀⠊⢌⠊⢌⠊⠔⠠⠂⡐⢀⠢⠀⢅⠀⠄⠐⠀⠀⠀⠀⠀
⠀⠌⠐⡈⠠⠁⢄⠈⡐⠄⠠⠁⡂⠐⠄⠄⡁⠢⡀⡑⡈⡐⢁⠌⠀⡡⠈⡂⠔⢈⠄⡑⢄⢁⠊⡐⠄⡡⢂⠀⡯⡪⡳⡌⠐⢄⠑⡐⠌⡐⢁⠔⢈⠔⠨⡐⢁⠢⠂⠢⠂⠈⠄⢂⠡⠂⢕⠨⠢⡈⠄⠣⡂⢅⠑⡑⠄⠂⠄⡑⢐⠄⢂⠀⠀⠀⡀⠁⠀⠁
⡡⢈⠀⠂⠌⠐⡀⠂⠄⠐⡁⠐⡈⠐⡀⢊⠠⠂⡐⠄⡐⢈⠄⠐⠀⠔⠐⠄⢊⠠⠂⠔⡀⢂⠡⠂⢌⠠⠂⡸⣕⢝⢎⢶⠈⢄⠑⠄⢊⠐⠄⢂⠢⢈⠢⠠⡁⢄⠑⡈⢂⠁⠨⡀⢂⠡⢂⠑⡡⠢⠈⡂⢕⠨⢂⠌⡨⠂⠂⢌⠂⡢⠐⡀⠀⠄⠀⠀⠄⠀
⠄⠠⠁⠌⠠⠁⢄⠈⢂⠈⠄⡈⠄⡨⠐⠠⠂⡡⠐⠄⠌⠄⢌⠀⢅⢈⠂⡑⠠⡁⢊⠐⡨⢀⢊⠐⠡⢀⡞⣕⢮⢣⡫⣚⢆⠠⢊⠐⡡⢈⠂⡡⠠⡁⢌⠂⡐⠄⠢⢈⠄⠂⡐⠠⢁⠂⡐⠡⡐⠡⡁⠐⠌⠢⠡⡨⠠⡑⡈⠠⠊⢄⠡⢂⠀⠀⢀⠀⠀⠀
⠌⠠⠁⠌⠠⠁⢄⠈⠄⠂⢂⠐⡐⠄⡁⢊⠐⠄⢊⠐⡡⢈⠄⢀⠂⢄⠡⡈⠐⢄⠡⠂⠔⠠⠂⡡⢡⠺⡜⣎⢮⢣⢳⢍⣞⡀⢄⠡⠂⢄⠅⡐⠐⠄⠢⢈⠄⢊⠐⢄⢈⠂⢘⡀⠄⡡⢈⠐⠄⡡⠨⡀⠑⡁⡊⡐⠔⡐⠄⢊⢈⢂⢑⢐⠀⠀⠀⠀⠈⠀
⠌⠠⠁⠌⢐⠀⡂⢐⠠⠁⢄⠡⠐⠄⢌⢀⠊⢄⠡⠂⠔⡀⢂⠐⠐⠄⠢⢈⠌⡐⢄⠑⡈⢂⠡⡰⠍⠫⠊⡊⢊⠑⡑⠑⠊⠆⠠⢊⠐⠔⠠⢊⢈⠂⡑⠠⢂⢁⠂⠢⠠⡁⠄⢧⡐⠀⠢⢁⠢⡀⢑⠄⠂⡈⠐⠌⠢⡨⢈⠂⢄⠡⢂⠡⡁⠀⠁⠀⠐⠀
⢂⠁⡐⢁⠂⠌⠠⢁⠄⢁⠂⡐⢁⠂⠢⢀⠅⠢⡀⠑⠄⢌⠠⢈⢈⢂⠑⠠⠂⠔⠠⡁⢌⡠⢦⢲⡒⡳⡳⣕⢭⡫⣪⡫⡳⡹⢦⠀⠕⡈⢂⠅⢄⠡⡈⢄⠡⠠⡁⡑⠐⠄⠂⢸⢎⡄⠑⡠⠂⢄⠡⡈⡂⠈⠄⢑⠁⡢⢈⠢⠠⡑⠄⢅⢌⠀⠀⠂⠀⠀
⠄⠌⠠⠂⠡⡈⠂⠔⢀⠂⠡⠐⢄⢁⠊⠠⠂⠢⡈⡈⢂⠂⡐⢀⠢⠠⠡⡁⡑⢈⢂⢴⢪⡕⡳⡕⣭⢫⡺⣜⢎⢞⡜⡮⣫⡺⡱⣣⠈⢄⠡⢂⠂⠢⠐⢄⠂⡡⠠⡈⢌⠈⠢⠈⡧⡻⣄⠂⢌⠠⠂⠔⡨⠀⡌⠠⠂⡐⠡⡂⠰⡈⢂⠢⢂⠀⠀⠄⠀⠁
⠊⡠⠑⡈⢂⠨⠈⢄⠁⠌⡐⠁⠔⢀⠅⢑⠈⡂⠄⢌⠐⠄⠨⡀⢂⠑⠐⢄⢈⡴⡣⡳⣱⣙⢎⢞⡜⡵⡱⣕⢝⢮⢺⢕⡵⣹⡪⡳⣥⠀⠕⡠⢁⢑⠁⡢⢈⠄⠂⠔⠠⡁⡑⠄⢹⡪⣎⠷⣄⠈⠢⡁⢌⠂⠸⣆⠈⢄⠈⠢⠐⠌⢄⠑⡰⠀⠀⡀⠠⡠
⢀⠢⠐⠠⢂⠈⠢⠀⠌⠠⡈⡈⢂⠡⠠⡁⠢⠈⠔⡀⢊⠄⠡⡀⢅⢈⢦⠲⡕⣕⢝⢎⢖⢕⠝⢎⢎⠚⢎⠪⡑⢍⠣⠩⠪⡪⢺⡕⣳⣣⠈⠔⡠⠡⡈⠔⠠⢊⠐⡁⠢⠂⢌⢂⠈⡞⢼⢕⢝⢶⣁⠐⢄⢑⠀⣫⣳⡀⠌⡐⢁⠊⡄⡑⢐⠀⡄⠰⡐⢌
⠠⠂⠡⡁⢂⢁⠑⠈⠄⠡⠐⡈⠄⠂⠢⠐⠡⡈⢂⠌⠠⠂⣡⢔⢎⢧⢓⢝⢜⢬⢪⢪⠲⡱⡙⢎⠭⡫⡹⣍⢯⡫⣫⣫⣛⢖⢗⢮⡣⣝⢷⡐⢄⠑⢄⠑⡁⠢⡁⠢⠈⡂⠢⠠⡁⠨⡦⣥⢪⣤⣩⣢⣀⠊⢄⠨⣎⢷⡀⠢⠠⡑⠄⡊⢨⠀⡊⢢⠈⢆
⢀⠑⡐⠄⠢⢀⠅⢈⠂⡁⢊⢀⠊⡈⢐⠁⠢⠂⠢⢈⢂⠁⡗⡵⠱⠕⠕⠁⡁⠁⠁⠁⠠⠀⠄⠠⠀⠄⡀⢀⠀⡁⠁⡁⣙⡩⣫⡳⣝⢮⡳⡵⣄⠑⢄⢑⢈⠢⡈⢌⢂⠈⡐⠡⡈⠄⠳⠱⠣⠚⠢⠣⠹⠙⠦⠄⢳⢕⡗⠀⢢⢈⠢⠂⡸⢀⠑⢌⠊⢔
⠀⠢⠐⡈⠂⠔⢈⠀⢂⠌⠠⠂⠡⠐⠀⢅⠑⠨⠠⡁⠢⡀⢹⡢⣆⣂⠈⠄⡠⣌⢔⠁⠐⡁⢨⣆⢷⡺⣤⡆⢐⠀⠂⢴⠠⡤⣥⢻⡜⣧⣛⢞⣜⢧⡀⠅⠢⡁⠢⠐⠄⡡⢀⠢⢈⠂⠢⠀⢄⢂⠄⠠⠀⠄⠠⠀⠁⠁⢁⠈⠔⡠⢑⠀⢎⠀⢕⢈⢊⠔
⢁⠑⡈⢄⢁⠊⡠⠀⠅⡠⠑⡈⢂⠑⡈⠠⡈⢂⠅⡐⠡⡐⠨⡞⡴⣍⠶⣀⠱⢔⢕⠀⢅⣐⡈⣼⡳⣝⢮⠀⡦⣡⠁⣎⢎⢶⢕⡯⡺⢮⢮⡳⣝⢮⡻⣆⠑⡈⡊⢌⠢⡀⣁⠂⢄⢁⠡⠉⢾⡹⡶⢀⠡⠀⡳⢌⠀⡁⠄⢠⠑⢄⠅⠐⡅⠨⢂⠅⡢⢑
⠀⠢⠐⡀⠢⠐⠠⠂⠐⠄⢌⠐⠄⢌⠠⠐⠠⡁⠔⡈⠔⠠⡈⢗⢵⡪⡳⣕⢕⡙⢟⣆⠠⡑⢔⡬⡣⣎⢧⡪⡂⠆⣼⢳⢯⡺⣕⢽⡹⣓⢷⡹⣎⢷⡹⣎⢷⣌⡐⢄⠱⠠⡈⠣⢀⠫⡂⡻⡊⢿⠂⡴⡁⢂⣝⡊⢠⠆⠐⡡⡈⠢⢀⠑⡌⠨⡂⠱⡠⡑
⢈⠐⡁⠌⠠⡁⠑⢄⡈⠢⡀⢊⠐⠄⢂⢁⠐⠄⢊⠠⢊⠐⠄⢹⡪⣎⢗⢵⡹⣜⢥⡫⡲⣌⠘⠪⠺⡔⢕⠍⣨⢞⣝⡳⡳⣝⢮⡳⣝⢵⡫⣞⢵⡫⡺⣎⢷⢜⠷⣦⣁⡑⠌⡢⠠⡑⢜⢎⡝⣕⢏⠢⣊⡾⡮⣪⡻⠀⢊⠄⡊⡂⢐⠱⠀⢕⠨⢂⢆⢘
⢀⠌⡀⠊⢄⢈⠂⢸⠆⠐⠄⡡⠈⡄⠡⡐⢀⠑⠠⡁⢂⢑⢈⠨⡮⣪⢳⢕⡵⡱⣣⡫⡺⡬⣫⢛⢞⢮⣪⡫⣎⠷⡵⣝⢮⡳⣝⢮⡳⣝⢮⡳⡵⣝⣝⢮⡳⣝⢯⢾⣜⢽⢦⡬⣢⣂⣢⣁⣝⣌⡣⣕⣏⢷⡹⣎⠗⢈⠢⡈⢆⠠⡁⠪⠀⢕⠨⡂⠢⡑
⠀⠢⢈⠂⠢⡀⠁⠫⢝⡄⠡⢐⢈⠐⡐⠐⢄⢈⠐⡈⠔⠠⠡⡀⢳⡱⡳⣕⢝⢮⡪⡞⡵⣹⡪⡳⣝⢮⢮⡺⣪⣛⢞⢮⡳⣝⢮⡳⣝⢮⡳⣝⢞⢮⡪⣗⢽⡪⣳⢻⣮⡳⣳⢝⢮⢮⢮⡺⡦⡻⣞⡵⣝⢷⣝⢮⠃⡐⠔⡨⠂⡐⢈⠂⠆⢑⢄⢑⠅⢪
⠠⢁⢂⠈⠢⠀⠌⠠⠀⡉⠂⠢⠠⡁⠌⢂⠢⢀⠢⠀⠡⡑⢐⠄⠌⢪⡳⣹⢪⣇⢟⡼⡱⣇⢟⡺⣎⢷⡱⣯⢪⣗⢝⡧⣻⢜⡧⣻⢜⡧⣻⢜⡧⣻⢪⡞⡵⣝⢵⣫⣞⢽⡪⣗⢽⡪⣗⢽⣪⡻⣮⡻⣞⢷⣝⠇⡐⢌⠢⡨⠠⠈⠔⢀⢓⠐⠤⠡⡊⠢
⠐⠄⢂⠡⠁⡐⠈⠄⠡⠀⠌⢀⠂⠔⢈⠄⢂⠡⡀⡑⠀⢄⠡⢈⠢⡈⢳⢕⡗⡵⣫⢺⡕⣽⢪⡳⣝⢮⡳⣕⢷⡹⣕⢯⡺⣕⢯⡺⣕⢯⡺⣕⢯⡺⣕⢯⡺⣎⢷⡱⣗⢽⡪⣗⢽⣪⡳⡳⡵⣫⢾⣝⡯⣷⡝⠰⢀⠎⡐⢀⠢⠡⢁⠢⠑⡄⢃⠑⢌⢊
⠐⡁⠢⠐⠠⠀⠅⠨⡀⠑⠀⡂⢐⠀⠂⠔⠁⠔⠠⡈⡐⠀⣟⠲⡦⣔⣈⢗⡝⣮⢳⣕⢝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⡵⣝⣝⢾⣕⢿⣮⡻⣮⢃⠑⠄⢃⠠⢁⠔⢁⠢⡁⠕⠰⠈⡪⢠⢑
⠂⡐⠠⠀⠅⠨⠀⠅⠠⠁⠨⠀⢄⠡⠈⠄⠑⢈⠂⠔⡈⢄⠀⠻⡸⣎⢮⡳⣹⡪⡳⣕⢽⣕⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢞⡮⡺⡵⣝⢷⣝⢿⠎⡠⠃⢌⠂⠔⡁⠂⠄⠥⡈⡊⡘⡂⠰⡁⢢
⠀⠄⢐⠁⠌⠠⠁⠌⠠⠁⡈⠂⠔⢈⠄⠡⢈⠄⠠⡁⠐⠄⡑⢄⢈⠪⡺⡪⢮⢝⢮⡪⡳⡮⣪⢗⡽⣪⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⡽⣌⠳⣝⢮⡳⠝⠮⢳⡝⣮⢳⠝⣮⢳⡝⣮⢳⣝⣝⢾⣝⢷⣝⢏⠐⢌⠐⢄⠑⠌⡀⢁⠊⡐⢌⠐⢌⠈⡆⠨⠢
⠐⡈⡀⠂⠌⠠⠁⠌⠂⢐⠈⢄⠑⡀⢂⠡⠐⡀⠢⢀⠑⢠⠀⠢⢀⠅⢌⡹⡕⡯⣪⢫⢞⢵⡣⣏⢾⢜⡵⣫⢞⡵⣫⢞⡵⣫⢞⡵⣫⢮⣛⢶⢲⣎⢟⣝⢗⢶⢕⡗⣯⢺⡕⣯⢺⣕⢧⣫⢷⣝⢷⡫⠠⢃⠄⡑⠠⢂⠑⢄⠡⡈⠄⢂⠱⠠⡡⢘⡀⠣
⠠⠠⠈⠄⠡⠈⠄⠡⢀⠡⠂⡐⢈⠄⠂⠄⠡⡀⠅⠠⡑⢈⢊⠢⡁⠐⠄⡉⠺⣜⢵⣙⢗⡵⡹⣎⢷⡹⣎⢷⡹⣎⢷⡹⣎⢷⡹⣎⢷⢵⡹⡳⣳⢝⡧⣫⢏⡷⣹⢎⡷⣹⢎⡗⣷⡹⣮⣳⡻⡞⢡⢂⠑⠔⡐⢈⠢⡀⠑⠌⡂⢢⠈⢂⠑⠔⡐⠄⠥⡈
⢀⠑⡈⠄⠡⠈⠄⡁⢐⠐⠄⢌⠠⠂⡁⡈⠂⠔⡈⠐⠄⡡⢑⠌⢜⠐⡅⡐⠄⢄⠑⠱⢝⡜⣧⢫⡎⡷⡹⣎⢷⡹⣎⢷⡹⣎⢷⡹⣎⢷⡹⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⡯⡾⢉⠔⠡⡠⠑⡈⢄⠡⠐⠌⡈⢂⢑⠠⢃⠢⡁⠊⢄⠑⢄⢑
⠠⢂⠐⡈⠄⠑⡀⠐⠄⡁⢂⠢⠐⠁⠄⡈⢈⠆⡨⢈⠂⢌⠢⡑⢌⢊⠔⢌⠢⢄⠑⠘⡶⢬⡘⢕⢝⢮⢳⢝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⣝⣮⠛⡩⢐⠡⡈⠢⡀⡑⠠⢂⠐⡁⡊⠠⠡⡨⡀⠕⡰⢈⠆⢄⠑⠄⢅
⠐⠠⠁⠄⡈⠄⠄⡈⠢⢀⠅⠠⡁⠑⠠⠈⠔⡐⠔⠠⠐⢅⢊⠔⡡⢂⠕⡨⠢⡡⡙⠄⣟⢜⣝⢕⢧⡳⡹⡕⡧⡻⣜⢵⡹⣎⢷⡹⣎⢷⡹⣎⢷⡹⣎⢷⡹⣎⢷⡹⣎⢷⠝⢊⠄⢊⠄⡡⠂⢌⠀⡂⠌⠐⠄⠡⡀⡑⡁⠂⠔⢰⠈⠔⠁⠊⠢⠱⠢⠢
⠡⢁⠊⠠⠐⡀⢐⠈⠔⠠⡈⠂⢌⠈⠄⠑⠌⢄⠑⠌⢐⠅⡢⢑⠌⡢⢑⠌⡢⢊⠜⠄⣯⢪⢎⢗⢵⡹⡪⣞⢜⡵⡱⣣⢫⢎⣗⢝⢮⡳⣝⢮⡳⣝⢮⡳⣝⢮⡳⠙⡨⢀⠢⢁⠌⠂⠔⠠⠑⠠⡁⢐⠈⠌⡐⢁⠐⡈⢌⠠⢑⠈⢆⠁⠐⠀⠂⠠⠀⠄)";

const char* getHuTao()
{
	return hu_tao;
}