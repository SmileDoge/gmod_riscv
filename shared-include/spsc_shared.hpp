// spsc_shared.hpp
#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <thread>
#include <chrono>
#include <type_traits>
#include <iostream>

#ifdef _WIN32
#include "Windows.h"
#endif

namespace shm_spsc {

    constexpr size_t CACHELINE = 64;

    // compute padding (in number of elements) to cover at least one cacheline
    inline size_t compute_padding_bytes(size_t item_size) {
        return (CACHELINE - 1) / item_size + 1;
    }

    // Round up 'n' to multiple of 'align' (align power-of-two not required)
    inline size_t align_up(size_t n, size_t align) {
        return (n + align - 1) / align * align;
    }

    // If you want page alignment size to round up total, helper:
    inline size_t system_allocation_granularity() {
#if defined(_WIN32)
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return static_cast<size_t>(si.dwPageSize);
#else
        return static_cast<size_t>(sysconf(_SC_PAGESIZE));
#endif
    }

    // Compute number of bytes needed for a single queue's slots (including padding and slack)
    // capacity = requested logical capacity (how many elements user wants to be able to store)
    // item_size = sizeof(T)
    inline size_t compute_total_bytes_for_slots(size_t capacity, size_t item_size) {
        if (capacity < 1) capacity = 1;
        size_t internal_capacity = capacity + 1; // rigtorp uses slack element
        size_t kPadding = compute_padding_bytes(item_size);
        size_t total_elements = internal_capacity + 2 * kPadding;
        return total_elements * item_size;
    }

    // Persistent header stored at queue offset (POD layout, fixed-size members)
    struct RingHeader {
        // We store atomics as uint32_t compatible members.
        // Use std::atomic<uint32_t> directly in shared memory.
        alignas(CACHELINE) std::atomic<uint32_t> head; // consumer reads & publishes
        alignas(CACHELINE) std::atomic<uint32_t> tail; // producer updates & publishes
        uint32_t capacity; // internal capacity (capacity_ in rigtorp) - i.e. capacity + 1
        uint32_t item_size; // bytes per item
        uint8_t reserved[CACHELINE - sizeof(uint32_t) * 2 - sizeof(uint32_t) * 2]; // pad up a cacheline (safe)
        // NOTE: reserved size chosen to keep header at least one cacheline + atomics aligned.
    };

    // Compute total shared memory size required for one queue including a small header
    // We store a small header (RingHeader) per queue, and the slots blob after it.
    // We align slots to CACHELINE boundary to ensure padding semantics are easy.
    inline size_t compute_shm_size_for_queue(size_t capacity, size_t item_size) {
        size_t slots_bytes = compute_total_bytes_for_slots(capacity, item_size);
        //size_t header_bytes = align_up(sizeof(uint32_t) * 4 + 64 /*reserve*/, CACHELINE);
        size_t header_bytes = align_up(sizeof(RingHeader), CACHELINE);
        size_t total = header_bytes + align_up(slots_bytes, CACHELINE);
        // round up to allocation granularity for mapping safety
        size_t gran = system_allocation_granularity();
        return align_up(total, gran);
    }

    // SPSC view that uses basePtr + offset as buffer; does NOT allocate memory.
    // Offsets in Shared memory must be <= 4GB (uint32_t). If you need larger, change to uint64_t.
    class SPSC {
    public:
        // basePtr - base of mapped region (uint8_t*)
        // header_offset - offset (in bytes) from basePtr to RingHeader
        // slots_offset - offset from basePtr to slots array (raw bytes)
        SPSC(uint8_t* basePtr = nullptr, uint32_t header_offset = 0, uint32_t slots_offset = 0) noexcept
            : base_(basePtr), header_offset_(header_offset), slots_offset_(slots_offset), header_(nullptr), slots_(nullptr) {
            if (base_) attach(basePtr, header_offset, slots_offset);
        }

        // attach to already mapped shared memory
        void attach(uint8_t* basePtr, uint32_t header_offset, uint32_t slots_offset) noexcept {
            base_ = basePtr;
            header_offset_ = header_offset;
            slots_offset_ = slots_offset;
            header_ = reinterpret_cast<RingHeader*>(base_ + header_offset_);
            slots_ = base_ + slots_offset_;
        }

        // initialize header (do this once from creator process)
        void init_header(uint32_t capacity, uint32_t item_size) noexcept {
            assert(header_);
            // store capacity as internal capacity (capacity + 1 slack)
            new (&header_->head) std::atomic<uint32_t>(0u);
            new (&header_->tail) std::atomic<uint32_t>(0u);
            uint32_t internal_capacity = capacity + 1;
            header_->item_size = item_size;
            header_->capacity = internal_capacity;
            header_->head.store(0u, std::memory_order_relaxed);
            header_->tail.store(0u, std::memory_order_relaxed);
            // zero slots area (optional)
            std::memset(slots_, 0, compute_total_bytes_for_slots(capacity, item_size));
        }

        // check lockfree for std::atomic<uint32_t>
        static bool atomics_lock_free() {
            return std::atomic<uint32_t>{}.is_lock_free();
        }

        // Try push: copy item bytes (item_size must match header_->item_size)
        // Returns true on success, false if full or mismatch
        bool try_push(const void* item, size_t item_size) noexcept {
            if (!header_) return false;
            if (item_size != header_->item_size) return false;

            uint32_t tail = header_->tail.load(std::memory_order_relaxed);
            uint32_t head = header_->head.load(std::memory_order_acquire);
            uint32_t cap = header_->capacity;
            if ((uint32_t)(tail - head) >= cap - 1) return false; // full (cap is internal capacity)
            uint32_t idx = tail % (cap - 1);
            //if (idx >= cap) idx -= cap; // wrap in [0, cap-1]
            uint32_t kPad = static_cast<uint32_t>(compute_padding_bytes(item_size));
            uint32_t slot_index = idx + kPad; // first usable is slots_[kPad]
            uint64_t slot_byte_offset = static_cast<uint64_t>(slot_index) * item_size;
            uint8_t* dst = slots_ + slot_byte_offset;
            std::memcpy(dst, item, item_size);
            // publish
            header_->tail.store(tail + 1, std::memory_order_release);
            return true;
        }

        // blocking push (simple spin + sleep). timeout_ms <0 = infinite
        bool push_wait(const void* item, size_t item_size, int timeout_ms = -1) noexcept {
            auto start = std::chrono::steady_clock::now();
            while (true) {
                if (try_push(item, item_size)) return true;
                if (timeout_ms >= 0) {
                    auto now = std::chrono::steady_clock::now();
                    int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());
                    if (elapsed >= timeout_ms) return false;
                }
                std::this_thread::yield();
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }

        // Try pop: read into out buffer (expects item_size) returns true if got item
        bool try_pop(void* out, size_t item_size) noexcept {
            if (!header_) return false;
            if (item_size != header_->item_size) return false;

            uint32_t head = header_->head.load(std::memory_order_relaxed);
            uint32_t tail = header_->tail.load(std::memory_order_acquire);
            uint32_t cap = header_->capacity;
            if (tail == head) return false; // empty
            uint32_t idx = head % (cap - 1);
            //if (idx >= cap) idx -= cap;
            uint32_t kPad = static_cast<uint32_t>(compute_padding_bytes(item_size));
            uint32_t slot_index = idx + kPad;
            uint8_t* src = slots_ + static_cast<size_t>(slot_index) * item_size;
            std::memcpy(out, src, item_size);
            // destroy? we just copy bytes (POD assumed)
            header_->head.store(head + 1, std::memory_order_release);
            return true;
        }

        // blocking pop with timeout
        bool pop_wait(void* out, size_t item_size, int timeout_ms = -1) noexcept {
            auto start = std::chrono::steady_clock::now();
            while (true) {
                if (try_pop(out, item_size)) return true;
                if (timeout_ms >= 0) {
                    auto now = std::chrono::steady_clock::now();
                    int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());
                    if (elapsed >= timeout_ms) return false;
                }
                std::this_thread::yield();
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }

        bool is_empty() const noexcept {
            if (!header_) return true;

            return header_->head.load(std::memory_order_acquire) == header_->tail.load(std::memory_order_acquire);
        }

        uint32_t size() const noexcept {
            if (!header_) return 0;

            uint32_t head = header_->head.load(std::memory_order_acquire);
            uint32_t tail = header_->tail.load(std::memory_order_acquire);
            uint32_t cap = capacity();

            return (tail - head) % cap;
        }

        // helpers to read capacity/item_size
        uint32_t capacity() const noexcept {
            return header_ ? (header_->capacity - 1) : 0u;
        }
        uint32_t item_size() const noexcept {
            return header_ ? header_->item_size : 0u;
        }

        // compute offsets helpers (static) - given base offsets compute where header and slots should be placed.
        // header_offset must be CACHELINE-aligned for best results.
        static uint32_t compute_header_offset(uint32_t base_offset) {
            return static_cast<uint32_t>(align_up(base_offset, CACHELINE));
        }
        static uint32_t compute_slots_offset(uint32_t header_offset) {
            return static_cast<uint32_t>(align_up(header_offset + static_cast<uint32_t>(sizeof(RingHeader)), CACHELINE));
        }

    private:
        uint8_t* base_;
        uint32_t header_offset_;
        uint32_t slots_offset_;
        RingHeader* header_;
        uint8_t* slots_;
    };

    //--------------- Convenience wrapper: pair of queues ------------------
    //
    // Layout suggestion in shared memory:
    // [ SharedPairHeader ] (CACHELINE aligned)
    // [ Queue A header ] (CACHELINE aligned)
    // [ Queue A slots ]  (CACHELINE aligned)
    // [ Queue B header ] (CACHELINE aligned)
    // [ Queue B slots ]  (CACHELINE aligned)
    //
    // We'll provide a helper struct which computes offsets and exposes simple API:
    // PushToWorker / PopFromWorker / PushToMain / PopFromMain
    //

    struct SharedPairHeader {
        uint32_t magic;
        uint32_t version;
        uint32_t q0_header_offset; // offsets from base
        uint32_t q0_slots_offset;
        uint32_t q1_header_offset;
        uint32_t q1_slots_offset;
        uint8_t reserved[64 - 6 * 4]; // pad to cacheline-ish
    };

    class SharedSPSCPair {
    public:
        // base: pointer to mapped region
        // capacity and item_size are logical parameters for both queues (same for both)
        static size_t compute_required_size(size_t capacity, size_t item_size) {
            size_t header_area = align_up(sizeof(SharedPairHeader), CACHELINE);
            size_t qbytes = compute_shm_size_for_queue(capacity, item_size);
            // two queues
            size_t total = header_area + 2 * qbytes;
            return align_up(total, system_allocation_granularity());
        }

        // Create layout in an already allocated base region; initialize headers
        static void format_region(uint8_t* base, size_t capacity, size_t item_size) {
            // place SharedPairHeader at base
            auto* hdr = reinterpret_cast<SharedPairHeader*>(base);
            hdr->magic = 0x53505343; // 'SPSC'
            hdr->version = 1;
            uint32_t base_off = static_cast<uint32_t>(align_up(sizeof(SharedPairHeader), CACHELINE));

            // q0
            uint32_t q0_header_off = SPSC::compute_header_offset(base_off);
            uint32_t q0_slots_off = SPSC::compute_slots_offset(q0_header_off);
            size_t q0_slots_bytes = compute_total_bytes_for_slots(capacity, item_size);
            uint32_t next = static_cast<uint32_t>(align_up(q0_slots_off + static_cast<uint32_t>(q0_slots_bytes), CACHELINE));

            // q1
            uint32_t q1_header_off = SPSC::compute_header_offset(next);
            uint32_t q1_slots_off = SPSC::compute_slots_offset(q1_header_off);
            // done

            hdr->q0_header_offset = q0_header_off;
            hdr->q0_slots_offset = q0_slots_off;
            hdr->q1_header_offset = q1_header_off;
            hdr->q1_slots_offset = q1_slots_off;

            // initialize headers
            SPSC q0(base, q0_header_off, q0_slots_off);
            q0.init_header(static_cast<uint32_t>(capacity), static_cast<uint32_t>(item_size));
            SPSC q1(base, q1_header_off, q1_slots_off);
            q1.init_header(static_cast<uint32_t>(capacity), static_cast<uint32_t>(item_size));

        }

        SharedSPSCPair() : base_(nullptr) {

        }

        // attach to existing region (both sides call)
        SharedSPSCPair(uint8_t* base) : base_(base) {
            assert(base_);
            auto* hdr = reinterpret_cast<SharedPairHeader*>(base_);
            assert(hdr->magic == 0x53505343);
            q0_.attach(base_, hdr->q0_header_offset, hdr->q0_slots_offset);
            q1_.attach(base_, hdr->q1_header_offset, hdr->q1_slots_offset);
        }

        // API: main -> worker (q0), worker -> main (q1)
        bool PushToWorker(const void* item) { return q0_.try_push(item, q0_.item_size()); }
        bool IsEmptyToWorker() { return q0_.is_empty(); }
        uint32_t SizeToWorker() { return q0_.size(); }
        bool PopFromWorker(void* out) { return q1_.try_pop(out, q1_.item_size()); }

        bool PushToMain(const void* item) { return q1_.try_push(item, q1_.item_size()); }
        bool IsEmptyToMain() { return q1_.is_empty(); }
        uint32_t SizeToMain() { return q1_.size(); }
        bool PopFromMain(void* out) { return q0_.try_pop(out, q0_.item_size()); }

        // blocking variants
        bool PushToWorkerWait(const void* item, int timeout_ms = -1) { return q0_.push_wait(item, q0_.item_size(), timeout_ms); }
        bool PopFromWorkerWait(void* out, int timeout_ms = -1) { return q1_.pop_wait(out, q1_.item_size(), timeout_ms); }

        bool PushToMainWait(const void* item, int timeout_ms = -1) { return q1_.push_wait(item, q1_.item_size(), timeout_ms); }
        bool PopFromMainWait(void* out, int timeout_ms = -1) { return q0_.pop_wait(out, q0_.item_size(), timeout_ms); }

        // expose compute helper
        static size_t RequiredRegionBytes(size_t capacity, size_t item_size) {
            return compute_required_size(capacity, item_size);
        }

    private:
        uint8_t* base_;
        SPSC q0_; // main->worker
        SPSC q1_; // worker->main
    };

} // namespace shm_spsc
