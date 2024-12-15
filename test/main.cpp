#include "mlc.h"
#include <iostream>

int main() {
    auto memLeakCallback = [](const auto smlc, const auto& map) {
        std::cout << "mlc@" << smlc << " Memory leak detected: " << map.size() << std::endl;
        for (const auto& [p, size] : map) {
            std::cout << p << " with size " << size << std::endl;
        }
    };

    using ohtoai::mlc::ScopeMemoryLeakCheck;
    int* p = nullptr;
    p = new int;
    std::cout << "New Record" << std::endl;
    auto record = ScopeMemoryLeakCheck::issueRecord();

    ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback); // Memory leak detected: 0
    std::cout << "New Record" << std::endl;
    auto record2 = ScopeMemoryLeakCheck::issueRecord();
    p = new int;
    ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback); // Memory leak detected: 1
    record->checkMemoryLeak(memLeakCallback); // Memory leak detected: 0
    record2->checkMemoryLeak(memLeakCallback); // Memory leak detected: 1

    {
        std::cout << "New Record" << std::endl;
        auto record3 = ScopeMemoryLeakCheck::issueRecord();
        p = new int;
        p = new int;
        auto record4 = ScopeMemoryLeakCheck::issueRecord();
        p = new int;
        ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback); // Memory leak detected: 1
        record3->checkMemoryLeak(memLeakCallback); // Memory leak detected: 2
        record3 = nullptr; // record3 will be inserted to record4(top stack)
        ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback); // Memory leak detected: 3
    }
    ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback); // Memory leak detected: 4

	return 0;
}
