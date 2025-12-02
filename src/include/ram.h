#pragma once
#include <vector>
#include <cstdint>
#include "bus.h" 

namespace Y86_64 {

class RAM : public Device {
    std::vector<uint8_t> memory;
public:
    explicit RAM(size_t size) : memory(size, 0) {}
    
    uint8_t read(uint64_t addr) override {
        if (addr >= memory.size()) return 0;
        return memory[addr];
    }
    
    void write(uint64_t addr, uint8_t data) override {
        if (addr < memory.size()) memory[addr] = data;
    }

    uint8_t read(uint64_t addr) const {
        if (addr >= memory.size()) return 0;
        return memory[addr];
    }
};

}