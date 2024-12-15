#pragma once

#ifndef _OHTOAI_MEMORY_LEAK_CHECK_HH_
#define _OHTOAI_MEMORY_LEAK_CHECK_HH_

#include <new>
#include <set>
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

                bool operator==(const UnhandledAllocator&) const noexcept {
                    return true;
                }
                bool operator!=(const UnhandledAllocator&) const noexcept {
                    return false;
                }
            };

            class ScopeMemoryLeakCheck {
            private:
                friend void* ::operator new(size_t size);
                friend void ::operator delete(void* p) noexcept;
                using handle_t = void*;
                handle_t allocate(size_t size) {
                    handle_t p = std::malloc(size);
                    if (p == nullptr) {
                        throw std::bad_alloc();
                    }
                    memoryInChecker.insert(p);
                    memoryRecords[p] = size;
                    return p;
                }
                void deallocate(handle_t p) noexcept {
                    free(p);
                    memoryInChecker.erase(p);
                    memoryRecords.erase(p);
                }
                mutable std::set<handle_t, std::less<handle_t>, UnhandledAllocator<handle_t>> memoryInChecker;
            private:
                inline thread_local static UnhandledAllocator<ScopeMemoryLeakCheck> allocator;
                inline thread_local static std::stack<std::weak_ptr<ScopeMemoryLeakCheck>,
                    std::deque<std::weak_ptr<ScopeMemoryLeakCheck>,
                    UnhandledAllocator<std::weak_ptr<ScopeMemoryLeakCheck>>>>
                    checkerRecords;
                inline thread_local static std::unordered_map<handle_t, size_t, std::hash<handle_t>,
                    std::equal_to<handle_t>, UnhandledAllocator<std::pair<const handle_t, size_t>>>
                    memoryRecords;
            public:
                static std::shared_ptr<ScopeMemoryLeakCheck> issueRecord() {
                    auto ptr = std::allocate_shared<ScopeMemoryLeakCheck>(allocator);
                    checkerRecords.push(ptr);
                    return ptr;
                }

                static std::shared_ptr<ScopeMemoryLeakCheck> currentRecord() {
                    while (!checkerRecords.empty()) {
                        if (auto ptr = checkerRecords.top().lock()) {
                            return ptr;
                        }
                        else {
                            checkerRecords.pop();
                        }
                    }
                    return {};
                }
                bool checkMemoryLeak(std::function<void(const decltype(memoryRecords)&)> callback = {}) const {
                    return checkMemoryLeak([callback](const ScopeMemoryLeakCheck*, const decltype(memoryRecords)& map) {
                        return callback(map);
                        });
                }
                bool checkMemoryLeak(std::function<void(const ScopeMemoryLeakCheck*, const decltype(memoryRecords)&)> callback = {}) const {
                    decltype(memoryInChecker) set{};
                    decltype(memoryRecords) tempRecords{};
                    for (const auto m : memoryInChecker) {
                        if (memoryRecords.find(m) != memoryRecords.end()) {
                            set.insert(m);
                            tempRecords[m] = memoryRecords.at(m);
                        }
                    }
                    memoryInChecker = set;
                    if (callback) {
                        callback(this, tempRecords);
                    }
                    return tempRecords.empty();
                }

                ~ScopeMemoryLeakCheck() {
                    while (!checkerRecords.empty() && checkerRecords.top().expired()) {
                        checkerRecords.pop();
                    }
                    if (!checkerRecords.empty()) {
                        auto record = checkerRecords.top().lock();
                        if (!record)
                            return;
                        record->memoryInChecker.insert(memoryInChecker.begin(), memoryInChecker.end());
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
