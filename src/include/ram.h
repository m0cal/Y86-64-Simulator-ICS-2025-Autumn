#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "device.h"

namespace Y86_64 {

class RAM : public Device {
public:
	explicit RAM(uint64_t size_bytes) : data_(static_cast<std::size_t>(size_bytes), 0) {
		if (size_bytes == 0) {
			throw std::invalid_argument("RAM size must be greater than zero");
		}
	}

	uint8_t read(uint64_t addr) override {
		return data_.at(to_index(addr));
	}

	void write(uint64_t addr, uint8_t value) override {
		data_.at(to_index(addr)) = value;
	}

	void load_bytes(uint64_t addr, const std::vector<uint8_t>& bytes) {
		const auto start = to_index(addr);
		if (start + bytes.size() > data_.size()) {
			throw std::out_of_range("Program segment exceeds RAM size");
		}
		std::copy(bytes.begin(), bytes.end(), data_.begin() + static_cast<std::ptrdiff_t>(start));
	}

	[[nodiscard]] uint64_t size() const noexcept { return static_cast<uint64_t>(data_.size()); }

	[[nodiscard]] uint8_t peek(uint64_t addr) const noexcept {
		if (addr >= size()) {
			return 0;
		}
		return data_[static_cast<std::size_t>(addr)];
	}

	void clear() { std::fill(data_.begin(), data_.end(), 0); }

private:
	[[nodiscard]] std::size_t to_index(uint64_t addr) const {
		if (addr >= size()) {
			throw std::out_of_range("RAM access outside of configured range");
		}
		return static_cast<std::size_t>(addr);
	}

	std::vector<uint8_t> data_;
};

} // namespace Y86_64
