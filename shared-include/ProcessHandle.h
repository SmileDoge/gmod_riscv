#pragma once

#include <string>
#include <vector>
#include <utility>
#include <stdexcept>
#include <system_error>
#include <cstdint>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#if defined(__linux__) && defined(__NR_pidfd_open)
#include <sys/syscall.h>
#include <unistd.h>
#endif
#endif

class Process {
public:
    // Default-constructible (empty), like std::thread()
    Process() noexcept = default;

    // Disable copy
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    // Enable move
    Process(Process&& other) noexcept;
    Process& operator=(Process&& other) noexcept;

    ~Process();

    // Convenience alias: creates (spawns) a new process (same as Spawn).
    static Process Create(const std::string& executable, const std::vector<std::string>& args = {}) {
        return Spawn(executable, args);
    }

    // Convenience alias: open existing by pid (same as Open).
    static Process CreateFromPid(uint32_t pid) {
        return Open(pid);
    }

    // Spawn a new process.
    // On Windows: executable can be full path or executable name; args appended to command line.
    // On POSIX: executable is passed to execvp (searches PATH). args[0] should be arg0 or omitted.
    static Process Spawn(const std::string& executable, const std::vector<std::string>& args = {});

    // Open an existing process by PID (does NOT make you the parent; On POSIX wait() won't be available
    // unless this object spawned the process). Throws on error.
    static Process Open(uint32_t pid);

    bool valid() const noexcept;

    uint32_t pid() const noexcept;

    // Wait for termination:
    // - On Windows: waits on process HANDLE (if we have it).
    // - On POSIX: only supported if this Process spawned the child (uses waitpid).
    // If waiting is not supported for this Process, throws runtime_error.
    int wait(); // returns exit code

    // Non-blocking check whether process exists/is running.
    bool is_running() const noexcept;

    // Forcefully terminate the process (SIGKILL on POSIX, TerminateProcess on Windows).
    void terminate();

    // Native handles access:
#ifdef _WIN32
    HANDLE native_handle() const noexcept;
#else
    pid_t native_pid() const noexcept;
    // returns pidfd >= 0 if available, or -1
    int native_pidfd() const noexcept;
#endif

private:
    // internal empty constructor kept but public default ctor is provided above
#ifdef _WIN32
    HANDLE proc_handle_ = nullptr;
    DWORD   pid_ = 0;
#else
    pid_t pid_ = -1;
    int pidfd_ = -1;
    bool spawned_child_ = false; // true if created via Spawn (so waitpid is valid)
#endif
};

// ================= Implementation =================

inline Process::Process(Process&& other) noexcept {
#ifdef _WIN32
    proc_handle_ = other.proc_handle_;
    pid_ = other.pid_;
    other.proc_handle_ = nullptr;
    other.pid_ = 0;
#else
    pid_ = other.pid_;
    pidfd_ = other.pidfd_;
    spawned_child_ = other.spawned_child_;
    other.pid_ = -1;
    other.pidfd_ = -1;
    other.spawned_child_ = false;
#endif
}

inline Process& Process::operator=(Process&& other) noexcept {
    if (this == &other) return *this;
#ifdef _WIN32
    if (proc_handle_) CloseHandle(proc_handle_);
    proc_handle_ = other.proc_handle_;
    pid_ = other.pid_;
    other.proc_handle_ = nullptr;
    other.pid_ = 0;
#else
    if (pidfd_ >= 0) ::close(pidfd_);
    pid_ = other.pid_;
    pidfd_ = other.pidfd_;
    spawned_child_ = other.spawned_child_;
    other.pid_ = -1;
    other.pidfd_ = -1;
    other.spawned_child_ = false;
#endif
    return *this;
}

inline Process::~Process() {
#ifdef _WIN32
    if (proc_handle_) {
        CloseHandle(proc_handle_);
        proc_handle_ = nullptr;
    }
#else
    if (pidfd_ >= 0) {
        ::close(pidfd_);
        pidfd_ = -1;
    }
    // We DO NOT send kill to pid on destruction.
#endif
}

#ifdef _WIN32

inline Process Process::Spawn(const std::string& executable, const std::vector<std::string>& args) {
    std::string cmd = executable;
    for (const auto& a : args) {
        cmd.push_back(' ');
        // naive quoting
        if (a.find(' ') != std::string::npos) {
            cmd.push_back('"');
            cmd += a;
            cmd.push_back('"');
        }
        else {
            cmd += a;
        }
    }

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    // CreateProcess may modify command line buffer; make a writable copy
    std::vector<char> cmdbuf(cmd.begin(), cmd.end());
    cmdbuf.push_back('\0');

    BOOL ok = CreateProcessA(
        nullptr,
        cmdbuf.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!ok) {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "CreateProcessA failed");
    }

    Process p;
    p.proc_handle_ = pi.hProcess;
    p.pid_ = pi.dwProcessId;
    // close thread handle we don't need
    if (pi.hThread) CloseHandle(pi.hThread);
    return p;
}

inline Process Process::Open(uint32_t pid) {
    if (pid == 0) throw std::invalid_argument("invalid pid");
    HANDLE h = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, (DWORD)pid);
    if (!h) {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "OpenProcess failed");
    }
    Process p;
    p.proc_handle_ = h;
    p.pid_ = (DWORD)pid;
    return p;
}

inline bool Process::valid() const noexcept { return proc_handle_ != nullptr; }
inline uint32_t Process::pid() const noexcept { return static_cast<uint32_t>(pid_); }

inline int Process::wait() {
    if (!proc_handle_) throw std::runtime_error("invalid process handle");
    DWORD r = WaitForSingleObject(proc_handle_, INFINITE);
    if (r == WAIT_FAILED) {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "WaitForSingleObject failed");
    }
    DWORD exitCode = 0;
    if (!GetExitCodeProcess(proc_handle_, &exitCode)) {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "GetExitCodeProcess failed");
    }
    return static_cast<int>(exitCode);
}

inline bool Process::is_running() const noexcept {
    if (!proc_handle_) return false;
    DWORD code = 0;
    if (!GetExitCodeProcess(proc_handle_, &code)) return false;
    return code == STILL_ACTIVE;
}

inline void Process::terminate() {
    if (!proc_handle_) throw std::runtime_error("invalid process handle");
    if (!TerminateProcess(proc_handle_, 1)) {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "TerminateProcess failed");
    }
}

inline HANDLE Process::native_handle() const noexcept { return proc_handle_; }

#else // POSIX

inline Process Process::Spawn(const std::string& executable, const std::vector<std::string>& args) {
    if (executable.empty()) throw std::invalid_argument("executable empty");

    pid_t pid = fork();
    if (pid < 0) {
        throw std::system_error(errno, std::system_category(), "fork failed");
    }
    if (pid == 0) {
        // child
        std::vector<char*> argv;
        argv.reserve(args.size() + 2);
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);

        execvp(executable.c_str(), argv.data());
        _exit(127); // exec failed
    }

    // parent
    Process p;
    p.pid_ = pid;
    p.spawned_child_ = true;

    // try pidfd_open if available (best-effort)
#if defined(__linux__) && defined(__NR_pidfd_open)
    int fd = static_cast<int>(syscall(__NR_pidfd_open, pid, 0));
    p.pidfd_ = (fd >= 0) ? fd : -1;
#else
    p.pidfd_ = -1;
#endif

    return p;
}

inline Process Process::Open(uint32_t pid_u) {
    pid_t pid = static_cast<pid_t>(pid_u);
    if (pid <= 0) throw std::invalid_argument("invalid pid");

    Process p;
    p.pid_ = pid;
    p.spawned_child_ = false;

#if defined(__linux__) && defined(__NR_pidfd_open)
    int fd = static_cast<int>(syscall(__NR_pidfd_open, pid, 0));
    p.pidfd_ = (fd >= 0) ? fd : -1;
#else
    p.pidfd_ = -1;
#endif

    return p;
}

inline bool Process::valid() const noexcept { return pid_ > 0; }
inline uint32_t Process::pid() const noexcept { return static_cast<uint32_t>(pid_); }

inline int Process::wait() {
    if (!valid()) throw std::runtime_error("invalid process");
    if (!spawned_child_) {
        throw std::runtime_error("wait() is supported only for processes spawned by this object on POSIX");
    }
    int status = 0;
    pid_t r = ::waitpid(pid_, &status, 0);
    if (r < 0) throw std::system_error(errno, std::system_category(), "waitpid failed");
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return status;
}

inline bool Process::is_running() const noexcept {
    if (!valid()) return false;
    if (kill(pid_, 0) == 0) return true;
    if (errno == ESRCH) return false;
    return true; // EPERM or other means it exists but we can't signal it
}

inline void Process::terminate() {
    if (!valid()) throw std::runtime_error("invalid process");
    if (kill(pid_, SIGKILL) != 0) throw std::system_error(errno, std::system_category(), "kill(SIGKILL) failed");
}

inline pid_t Process::native_pid() const noexcept { return pid_; }
inline int Process::native_pidfd() const noexcept { return pidfd_; }

#endif // _WIN32 / POSIX
