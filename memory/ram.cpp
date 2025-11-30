#include "ram.h"

namespace Y86_64 {

RAM::RAM(size_t size) : memory(size, 0) {}

uint8_t RAM::read(uint64_t addr) const {
    return memory.at(addr); // 先用 at()，会抛异常便于调试
}

void RAM::write(uint64_t addr, uint8_t data) {
    memory.at(addr) = data;
}

}
