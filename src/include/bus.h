#pragma once

#include <cstdint>
#include <vector>

#include "bus_result.h"
#include "device.h"

namespace Y86_64 {

class Bus {
public:
	void register_device(Device& device, uint64_t start_addr, uint64_t end_addr);

	[[nodiscard]] BusResult read(uint64_t addr) const;
	[[nodiscard]] BusResult write(uint64_t addr, uint8_t data);

private:
	struct Mapping {
		Device* device;
		uint64_t start;
		uint64_t end; // exclusive
	};

	[[nodiscard]] const Mapping* find_mapping(uint64_t addr) const;

	std::vector<Mapping> mappings_;
};

} // namespace Y86_64
