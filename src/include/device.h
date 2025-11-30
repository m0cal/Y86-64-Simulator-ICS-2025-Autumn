#pragma once
#include <cstdint>

namespace Y86_64 {

// Base class of all bus-mapped devices.
// Access uses *relative* address (Bus does absolute->relative mapping).
class Device {
public:
    virtual ~Device() = default;

    // Read a byte from the device using relative address.
    virtual uint8_t read(uint64_t addr) = 0;

    // Write a byte to the device using relative address.
    virtual void write(uint64_t addr, uint8_t data) = 0;
};

} // namespace Y86_64
