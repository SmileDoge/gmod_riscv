#pragma once

#include "spsc_shared.hpp"
#include "shared_memory_manager.hpp"

#include <string>

#include <memory>

class SharedSPSC
{
public:
    SharedSPSC() = default;
    ~SharedSPSC() noexcept;

    bool Initialize(const std::string& shared_path, uint32_t item_size, uint32_t capacity, bool from_main = false);

    void Close() noexcept;

    bool PushToWorker(const void* item) noexcept;
    bool IsEmptyToWorker() noexcept;
    uint32_t SizeToWorker() noexcept;
    bool PopFromWorker(void* out) noexcept;

    bool PushToMain(const void* item) noexcept;
    bool IsEmptyToMain() noexcept;
    uint32_t SizeToMain() noexcept;
    bool PopFromMain(void* out) noexcept;

    bool IsInitialized() noexcept;
private:
    std::unique_ptr<shm_spsc::SharedSPSCPair> pair;

    SharedMemory<uint8_t> shared_memory;
};

inline SharedSPSC::~SharedSPSC() noexcept
{
    Close();
}

inline bool SharedSPSC::Initialize(const std::string& shared_path, uint32_t item_size, uint32_t capacity, bool from_main)
{
    if (shared_memory.IsOpen() || pair) return false;

    size_t bytes = shm_spsc::SharedSPSCPair::RequiredRegionBytes(capacity, item_size);

    if (from_main)
    {
        if (!shared_memory.CreateShared(shared_path, bytes))
            return false;

        shm_spsc::SharedSPSCPair::format_region(shared_memory.GetBase(), capacity, item_size);
    }
    else
    {
        if (!shared_memory.OpenShared(shared_path, bytes))
            return false;
    }

    pair = std::make_unique<shm_spsc::SharedSPSCPair>(shared_memory.GetBase());

    return true;
}

inline void SharedSPSC::Close() noexcept
{
    pair.reset();

    shared_memory.Close();
}

inline bool SharedSPSC::PushToWorker(const void* item) noexcept
{
    if (!pair) return false;

    return pair->PushToWorker(item);
}

inline bool SharedSPSC::IsEmptyToWorker() noexcept
{
    if (!pair) return true;

    return pair->IsEmptyToWorker();
}

inline uint32_t SharedSPSC::SizeToWorker() noexcept
{
    if (!pair) return 0;

    return pair->SizeToWorker();
}

inline bool SharedSPSC::PopFromWorker(void* out) noexcept
{
    if (!pair) return false;

    return pair->PopFromWorker(out);
}

inline bool SharedSPSC::PushToMain(const void* item) noexcept
{
    if (!pair) return false;

    return pair->PushToMain(item);
}

inline bool SharedSPSC::IsEmptyToMain() noexcept
{
    if (!pair) return true;

    return pair->IsEmptyToMain();
}

inline uint32_t SharedSPSC::SizeToMain() noexcept
{
    if (!pair) return 0;

    return pair->SizeToMain();
}

inline bool SharedSPSC::PopFromMain(void* out) noexcept
{
    if (!pair) return false;

    return pair->PopFromMain(out);
}

inline bool SharedSPSC::IsInitialized() noexcept
{
    return static_cast<bool>(pair);
}