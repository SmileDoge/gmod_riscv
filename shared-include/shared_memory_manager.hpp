#pragma once

// shared_memory.hpp
// Cross-platform SharedMemory<T> (Windows / POSIX)
// C++17, header-only
//
// Behavior changes:
// - Tracks whether this instance actually created the shared object.
// - If created_by_me_ == true, Close() will remove the POSIX name via shm_unlink(name).
// - On Windows there is no explicit "unlink" API for named mappings; CloseHandle is used.

#include <string>
#include <cstdint>
#include <cstddef>
#include <system_error>
#include <utility>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#endif

template<typename T>
class SharedMemory
{
public:
    SharedMemory() noexcept;
    ~SharedMemory();

    // Non-copyable
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    // Movable
    SharedMemory(SharedMemory&& other) noexcept;
    SharedMemory& operator=(SharedMemory&& other) noexcept;

    // Create (or open) a shared memory object mapped for read/write.
    // If bytes == 0 -> sizeof(T)
    // Returns true on success. created_by_me_ will be true only if this call actually created the underlying object.
    bool CreateShared(const std::string& shared_path, size_t bytes = 0);

    // Open an existing shared memory object.
    // If bytes == 0 -> sizeof(T)
    bool OpenShared(const std::string& shared_path, size_t bytes = 0);

    // Close/unmap local resources. If this instance created the object, it will remove POSIX name (shm_unlink).
    void Close() noexcept;

    // Get base pointer (null if not mapped)
    T* GetBase() const noexcept;

    bool IsOpen() const noexcept { return mapped_ != nullptr; }

    T* operator->() noexcept { return GetBase(); }
    const T* operator->() const noexcept { return GetBase(); }

    T& operator*() noexcept { return *GetBase(); }
    const T& operator*() const noexcept { return *GetBase(); }

    explicit operator bool() const noexcept { return IsOpen() && GetBase() != nullptr; }

private:
    // stored name (normalized on POSIX)
    std::string name_;
    bool created_by_me_ = false;

#ifdef _WIN32
    HANDLE         mapping_handle_;
#else
    int            fd_;
    // keep track of size for munmap
    size_t         mapped_size_;
#endif
    void* mapped_;
};

// ---------------- Implementation ----------------

template<typename T>
inline SharedMemory<T>::SharedMemory() noexcept
#ifdef _WIN32
    : mapping_handle_(nullptr), mapped_(nullptr)
#else
    : fd_(-1), mapped_size_(0), mapped_(nullptr)
#endif
{
}

template<typename T>
inline SharedMemory<T>::~SharedMemory()
{
    Close();
}

template<typename T>
inline SharedMemory<T>::SharedMemory(SharedMemory&& other) noexcept
#ifdef _WIN32
    : name_(std::move(other.name_)), created_by_me_(other.created_by_me_), mapping_handle_(other.mapping_handle_), mapped_(other.mapped_)
#else
    : name_(std::move(other.name_)), created_by_me_(other.created_by_me_), fd_(other.fd_), mapped_size_(other.mapped_size_), mapped_(other.mapped_)
#endif
{
#ifdef _WIN32
    other.mapping_handle_ = nullptr;
    other.mapped_ = nullptr;
#else
    other.fd_ = -1;
    other.mapped_size_ = 0;
    other.mapped_ = nullptr;
#endif
    other.created_by_me_ = false;
}

template<typename T>
inline SharedMemory<T>& SharedMemory<T>::operator=(SharedMemory&& other) noexcept
{
    if (this == &other) return *this;
    Close();
    name_ = std::move(other.name_);
    created_by_me_ = other.created_by_me_;
#ifdef _WIN32
    mapping_handle_ = other.mapping_handle_;
    mapped_ = other.mapped_;
    other.mapping_handle_ = nullptr;
    other.mapped_ = nullptr;
#else
    fd_ = other.fd_;
    mapped_size_ = other.mapped_size_;
    mapped_ = other.mapped_;
    other.fd_ = -1;
    other.mapped_size_ = 0;
    other.mapped_ = nullptr;
#endif
    other.created_by_me_ = false;
    return *this;
}

template<typename T>
inline void SharedMemory<T>::Close() noexcept
{
    // POSIX: if we were the creator, unlink name_
#ifdef _WIN32
    if (mapped_) {
        UnmapViewOfFile(mapped_);
        mapped_ = nullptr;
    }
    if (mapping_handle_) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
    }
    // On Windows there's no explicit unlink API — once all handles are closed the object is gone.
    name_.clear();
    created_by_me_ = false;
#else
    if (mapped_) {
        munmap(mapped_, mapped_size_);
        mapped_ = nullptr;
        mapped_size_ = 0;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    if (created_by_me_ && !name_.empty()) {
        // best-effort unlink; ignore errors
        shm_unlink(name_.c_str());
    }
    name_.clear();
    created_by_me_ = false;
#endif
}

template<typename T>
inline T* SharedMemory<T>::GetBase() const noexcept
{
    return static_cast<T*>(mapped_);
}

#ifdef _WIN32

template<typename T>
inline bool SharedMemory<T>::CreateShared(const std::string& shared_path, size_t bytes)
{
    if (mapped_ || mapping_handle_) return false;
    if (bytes == 0) bytes = sizeof(T);

    name_ = shared_path;
    created_by_me_ = false;

    // CreateFileMappingA returns handle to existing mapping if name already exists.
    // GetLastError will tell if it already existed (ERROR_ALREADY_EXISTS).
    mapping_handle_ = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
        static_cast<DWORD>(bytes), name_.c_str());
    if (!mapping_handle_) {
        std::fprintf(stderr, "[SharedMemory::CreateShared] CreateFileMappingA failed: %lu\n", GetLastError());
        return false;
    }

    DWORD last = GetLastError();
    created_by_me_ = (last != ERROR_ALREADY_EXISTS);

    mapped_ = MapViewOfFile(mapping_handle_, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
    if (!mapped_) {
        std::fprintf(stderr, "[SharedMemory::CreateShared] MapViewOfFile failed: %lu\n", GetLastError());
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
        // on Windows we cannot "unlink" name explicitly; just fail and return false
        created_by_me_ = false;
        return false;
    }

    return true;
}

template<typename T>
inline bool SharedMemory<T>::OpenShared(const std::string& shared_path, size_t bytes)
{
    if (mapped_ || mapping_handle_) return false;
    if (bytes == 0) bytes = sizeof(T);

    name_ = shared_path;
    created_by_me_ = false;

    mapping_handle_ = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name_.c_str());
    if (!mapping_handle_) {
        std::fprintf(stderr, "[SharedMemory::OpenShared] OpenFileMappingA failed: %lu\n", GetLastError());
        return false;
    }

    mapped_ = MapViewOfFile(mapping_handle_, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
    if (!mapped_) {
        std::fprintf(stderr, "[SharedMemory::OpenShared] MapViewOfFile failed: %lu\n", GetLastError());
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
        return false;
    }
    return true;
}

#else // POSIX

// Helper: normalize name to start with '/'
static inline std::string posix_normalize_name(const std::string& name_in) {
    if (name_in.empty()) return std::string("/");
    if (name_in[0] == '/') return name_in;
    return std::string("/") + name_in;
}

template<typename T>
inline bool SharedMemory<T>::CreateShared(const std::string& shared_path, size_t bytes)
{
    if (mapped_ || fd_ >= 0) return false;
    if (bytes == 0) bytes = sizeof(T);

    // Normalize POSIX name to begin with '/'
    name_ = posix_normalize_name(shared_path);

    // Try to create exclusively first to detect if new
    int fd = shm_open(name_.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
    bool created = true;
    if (fd < 0) {
        if (errno == EEXIST) {
            // already exists, open without O_EXCL
            fd = shm_open(name_.c_str(), O_RDWR, 0666);
            created = false;
        }
        else {
            std::fprintf(stderr, "[SharedMemory::CreateShared] shm_open(O_CREAT|O_EXCL) failed for '%s': %s\n", name_.c_str(), std::strerror(errno));
            name_.clear();
            created_by_me_ = false;
            return false;
        }
    }

    if (fd < 0) {
        std::fprintf(stderr, "[SharedMemory::CreateShared] shm_open failed for '%s': %s\n", name_.c_str(), std::strerror(errno));
        name_.clear();
        created_by_me_ = false;
        return false;
    }

    // If created==true then this process is the owner and will be allowed to unlink later.
    created_by_me_ = created;

    if (created) {
        // set size
        if (ftruncate(fd, static_cast<off_t>(bytes)) != 0) {
            std::fprintf(stderr, "[SharedMemory::CreateShared] ftruncate failed for '%s': %s\n", name_.c_str(), std::strerror(errno));
            ::close(fd);
            // cleanup name to avoid leaking a created object if possible
            if (created_by_me_) shm_unlink(name_.c_str());
            name_.clear();
            created_by_me_ = false;
            return false;
        }
    }
    else {
        // If opened existing and user requested bytes larger than current size, we try ftruncate to requested size.
        if (bytes > 0) {
            struct stat st;
            if (fstat(fd, &st) == 0) {
                if (static_cast<size_t>(st.st_size) < bytes) {
                    // attempt to resize (may fail if other process expects smaller)
                    if (ftruncate(fd, static_cast<off_t>(bytes)) != 0) {
                        // not fatal — warn and continue using existing size
                        std::fprintf(stderr, "[SharedMemory::CreateShared] ftruncate on existing failed for '%s': %s\n", name_.c_str(), std::strerror(errno));
                    }
                }
            }
        }
    }

    void* ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        std::fprintf(stderr, "[SharedMemory::CreateShared] mmap failed for '%s': %s\n", name_.c_str(), std::strerror(errno));
        ::close(fd);
        // if we created the name but mmap failed, attempt to unlink so future Create won't conflict
        if (created_by_me_) shm_unlink(name_.c_str());
        name_.clear();
        created_by_me_ = false;
        return false;
    }

    fd_ = fd;
    mapped_ = ptr;
    mapped_size_ = bytes;
    return true;
}

template<typename T>
inline bool SharedMemory<T>::OpenShared(const std::string& shared_path, size_t bytes)
{
    if (mapped_ || fd_ >= 0) return false;
    if (bytes == 0) bytes = sizeof(T);

    name_ = posix_normalize_name(shared_path);
    created_by_me_ = false;

    int fd = shm_open(name_.c_str(), O_RDWR, 0666);
    if (fd < 0) {
        std::fprintf(stderr, "[SharedMemory::OpenShared] shm_open failed for '%s': %s\n", name_.c_str(), std::strerror(errno));
        name_.clear();
        return false;
    }

    // Determine mapping size. Prefer provided bytes, otherwise use current size.
    size_t map_size = bytes;
    if (bytes == sizeof(T)) {
        struct stat st;
        if (fstat(fd, &st) == 0 && st.st_size > 0) {
            map_size = static_cast<size_t>(st.st_size);
        }
    }

    void* ptr = mmap(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        std::fprintf(stderr, "[SharedMemory::OpenShared] mmap failed for '%s': %s\n", name_.c_str(), std::strerror(errno));
        ::close(fd);
        name_.clear();
        return false;
    }

    fd_ = fd;
    mapped_ = ptr;
    mapped_size_ = map_size;
    return true;
}

#endif // _WIN32 / POSIX