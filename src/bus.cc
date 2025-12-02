#include "bus.h"

#include <stdexcept>

namespace Y86_64 {

namespace {
constexpr Y86Stat kAddressError = Y86Stat::ADR;
}

void Bus::register_device(Device& device, uint64_t start_addr, uint64_t end_addr) {
    if (start_addr >= end_addr) {
        throw std::invalid_argument("Bus::register_device requires start < end");
    }

    mappings_.push_back(Mapping{.device = &device, .start = start_addr, .end = end_addr});
}

const Bus::Mapping* Bus::find_mapping(uint64_t addr) const {
    for (const auto& mapping : mappings_) {
        if (addr >= mapping.start && addr < mapping.end) {
            return &mapping;
        }
    }
    return nullptr;
}

BusResult Bus::read(uint64_t addr) const {
    const Mapping* mapping = find_mapping(addr);
    if (mapping == nullptr) {
        return BusResult{.data = 0, .status_code = kAddressError};
    }

    const uint64_t relative = addr - mapping->start;
    return BusResult{.data = mapping->device->read(relative), .status_code = Y86Stat::AOK};
}

BusResult Bus::write(uint64_t addr, uint8_t data) {
    const Mapping* mapping = find_mapping(addr);
    if (mapping == nullptr) {
        return BusResult{.data = 0, .status_code = kAddressError};
    }

    const uint64_t relative = addr - mapping->start;
    mapping->device->write(relative, data);
    return BusResult{.data = data, .status_code = Y86Stat::AOK};
}

} // namespace Y86_64
