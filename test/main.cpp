#include "mlc.h"
#include <iostream>

int main() {
    auto memLeakCallback = [](const auto smlc, const auto& map) {
        std::cout << "mlc@" << smlc << " Memory leak detected: " << map.size() << std::endl;
        for (const auto& [p, size] : map) {
            std::cout << p << " [" << size << "]" << std::endl;
        }
    };

    using ohtoai::mlc::ScopeMemoryLeakCheck;

    auto record1 = ScopeMemoryLeakCheck::issueRecord();
    ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback);
    new int;
    ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback);
    new char;
    auto record2 = ScopeMemoryLeakCheck::issueRecord();
    new short;
    ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback);
    record1->checkMemoryLeak(memLeakCallback);
    record2 = nullptr;
    record1->checkMemoryLeak(memLeakCallback);
    auto p = new int[20];
    record1->checkMemoryLeak(memLeakCallback);
    delete[] p;
    record1->checkMemoryLeak(memLeakCallback);
    p = new int[20];
    auto record3 = ScopeMemoryLeakCheck::issueRecord();
    ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback);
    record1->checkMemoryLeak(memLeakCallback);
    delete[] p;
    ScopeMemoryLeakCheck::currentRecord()->checkMemoryLeak(memLeakCallback);
    record1->checkMemoryLeak(memLeakCallback);

	return 0;
}
