#pragma once

#ifndef _OHTOAI_MEMORY_LEAK_CHECK_HH_
#define _OHTOAI_MEMORY_LEAK_CHECK_HH_

#include <new>
#include <stack>
#include <memory>
#include <functional>
#include <unordered_map>

namespace ohtoai {
    namespace mlc {
        namespace detail {
            template <typename T>
            struct UnhandledAllocator {
                using value_type = T;
                using pointer = T*;
                using const_pointer = const T*;
                using reference = T&;
                using const_reference = const T&;
                using size_type = size_t;
                using difference_type = ptrdiff_t;

                template <typename U>
                struct rebind {
                    using other = UnhandledAllocator<U>;
                };

                UnhandledAllocator() noexcept = default;
                template <typename U>
                UnhandledAllocator(const UnhandledAllocator<U>&) noexcept {}

                pointer address(reference x) const noexcept {
                    return std::addressof(x);
                }

                const_pointer address(const_reference x) const noexcept {
                    return std::addressof(x);
                }

                pointer allocate(size_t n) {
                    if (n > std::numeric_limits<std::size_t>::max() / sizeof(value_type)) {
                        throw std::bad_alloc();
                    }
                    void* p = std::malloc(n * sizeof(value_type));
                    if (!p) {
                        throw std::bad_alloc();
                    }
                    return static_cast<pointer>(p);
                }

                void deallocate(pointer p, size_t n) noexcept {
                    std::free(p);
                }
            };

            class ScopeMemoryLeakCheck {
            private:
                friend void* ::operator new(size_t size);
                friend void ::operator delete(void* p) noexcept;
                void* allocate(size_t size) {
                    void* p = malloc(size);
                    if (p == nullptr) {
                        throw std::bad_alloc();
                    }
                    memoryRecords[p] = size;
                    return p;
                }
                void deallocate(void* p) noexcept {
                    free(p);
                    memoryRecords.erase(p);
                }
            private:
                inline thread_local static std::stack<std::weak_ptr<ScopeMemoryLeakCheck>,
                    std::deque<std::weak_ptr<ScopeMemoryLeakCheck>,
                    UnhandledAllocator<std::weak_ptr<ScopeMemoryLeakCheck>>>> records;
                std::unordered_map<void*, size_t, std::hash<void*>, std::equal_to<void*>, UnhandledAllocator<std::pair<void* const, size_t>>> memoryRecords;
                inline thread_local static UnhandledAllocator<ScopeMemoryLeakCheck> allocator;
            public:
                static std::shared_ptr<ScopeMemoryLeakCheck> issueRecord() {
                    auto ptr = std::allocate_shared<ScopeMemoryLeakCheck>(allocator);
                    records.push(ptr);
                    return ptr;
                }

                static std::shared_ptr<ScopeMemoryLeakCheck> currentRecord() {
                    while (!records.empty()) {
                        if (auto ptr = records.top().lock()) {
                            return ptr;
                        }
                        else {
                            records.pop();
                        }
                    }
                    return {};
                }
                bool checkMemoryLeak(std::function<void(const decltype(memoryRecords)&)> callback = {}) const {
                    if (callback) {
                        callback(memoryRecords);
                    }
                    return memoryRecords.empty();
                }
                bool checkMemoryLeak(std::function<void(const ScopeMemoryLeakCheck*, const decltype(memoryRecords)&)> callback = {}) const {
                    if (callback) {
                        callback(this, memoryRecords);
                    }
                    return memoryRecords.empty();
                }

                ~ScopeMemoryLeakCheck() {
                    while (!records.empty() && records.top().expired()) {
                        records.pop();
                    }
                    if (!records.empty()) {
                        auto record = records.top().lock();
                        if (!record)
                            return;
                        record->memoryRecords.insert(memoryRecords.begin(), memoryRecords.end());
                    }
                }
            };
        }
        using detail::ScopeMemoryLeakCheck;
    }
}

void* operator new(size_t size) {
    if (auto record = ohtoai::mlc::ScopeMemoryLeakCheck::currentRecord()) {
        return record->allocate(size);
    }
    else {
        return ohtoai::mlc::detail::UnhandledAllocator<std::uint8_t>().allocate(size);
    }
}

void operator delete(void* p) noexcept {
    if (auto record = ohtoai::mlc::ScopeMemoryLeakCheck::currentRecord()) {
        return record->deallocate(p);
    }
    else {
        return ohtoai::mlc::detail::UnhandledAllocator<std::uint8_t>().deallocate(static_cast<std::uint8_t*>(p), 1);
    }
}

#endif // !_OHTOAI_MEMORY_LEAK_CHECK_HH_
