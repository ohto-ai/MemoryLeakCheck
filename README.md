# MemoryLeakCheck

## Usage
```cpp
    auto memLeakCallback = [](const auto& map) {
        std::cout << "Memory leak detected: " << map.size() << std::endl;
        for (const auto& [p, size] : map) {
            std::cout << "Memory leak detected: " << p << " with size " << size << std::endl;
        }
    };
    using ohtoai::memory_leak_check::ScopeMemoryLeakCheck;

    int* p = nullptr;
    p = new int;
    auto record = ScopeMemoryLeakCheck::issueRecord();

    ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback); // Memory leak detected: 0

    auto record2 = ScopeMemoryLeakCheck::issueRecord();
    p = new int;
    ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback); // Memory leak detected: 1
    record->checkMemoryLeak(memLeakCallback); // Memory leak detected: 0
    record2->checkMemoryLeak(memLeakCallback); // Memory leak detected: 1
```
