#pragma once
#include <cstdint>
#include <vector>

namespace Y86_64 {

class RAM {
public:
    explicit RAM(size_t size = 4096);

    uint8_t read(uint64_t addr) const;
    void write(uint64_t addr, uint8_t data);

private:
    std::vector<uint8_t> memory;
};

}
