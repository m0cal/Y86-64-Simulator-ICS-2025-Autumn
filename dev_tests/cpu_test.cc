#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cpu.h"

namespace {

using namespace Y86_64;

class VectorRAM : public Device {
public:
	explicit VectorRAM(std::size_t size_bytes) : data_(size_bytes, 0) {}

	uint8_t read(uint64_t addr) override {
		REQUIRE(addr < data_.size());
		return data_[static_cast<std::size_t>(addr)];
	}

	void write(uint64_t addr, uint8_t value) override {
		REQUIRE(addr < data_.size());
		data_[static_cast<std::size_t>(addr)] = value;
	}

	void load(uint64_t addr, const std::vector<uint8_t>& bytes) {
		REQUIRE(addr + bytes.size() <= data_.size());
		std::copy(bytes.begin(), bytes.end(), data_.begin() + addr);
	}

	uint64_t size() const { return static_cast<uint64_t>(data_.size()); }

	uint64_t load_qword(uint64_t addr) const {
		REQUIRE(addr + 8 <= data_.size());
		uint64_t value = 0;
		for (int i = 0; i < 8; ++i) {
			value |= static_cast<uint64_t>(data_[static_cast<std::size_t>(addr + i)]) << (i * 8);
		}
		return value;
	}

private:
	std::vector<uint8_t> data_;
};

std::filesystem::path resolve_repo_path(const std::string& relative) {
	auto path = std::filesystem::current_path();
	for (int i = 0; i < 10; ++i) {
		const auto candidate = (path / relative).lexically_normal();
		if (std::filesystem::exists(candidate)) {
			return candidate;
		}
		if (!path.has_parent_path()) {
			break;
		}
		path = path.parent_path();
	}

	FAIL("Unable to resolve path for " << relative << " from " << std::filesystem::current_path());
	return std::filesystem::current_path();
}

std::vector<uint8_t> parse_hex_bytes(const std::string& text) {
	std::string filtered;
	filtered.reserve(text.size());
	for (char ch : text) {
		if (std::isxdigit(static_cast<unsigned char>(ch))) {
			filtered.push_back(ch);
		}
	}

	std::vector<uint8_t> bytes;
	bytes.reserve(filtered.size() / 2);
	for (std::size_t i = 0; i + 1 < filtered.size(); i += 2) {
		const auto byte_str = filtered.substr(i, 2);
		bytes.push_back(static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16)));
	}
	return bytes;
}

uint64_t parse_address(const std::string& token) {
	std::string trimmed;
	trimmed.reserve(token.size());
	for (char ch : token) {
		if (!std::isspace(static_cast<unsigned char>(ch))) {
			trimmed.push_back(ch);
		}
	}
	return std::stoull(trimmed, nullptr, 16);
}

void load_yo_program(VectorRAM& ram, const std::string& relative) {
	const auto full_path = resolve_repo_path(relative);
	INFO("Loading program from " << full_path.string());

	std::ifstream input(full_path);
	REQUIRE(input.is_open());

	std::string line;
	while (std::getline(input, line)) {
		const auto colon = line.find(':');
		const auto pipe = line.find('|', colon == std::string::npos ? 0 : colon);
		if (colon == std::string::npos || pipe == std::string::npos) {
			continue;
		}

		const auto addr_str = line.substr(0, colon);
		const auto hex_blob = line.substr(colon + 1, pipe - colon - 1);
		const auto bytes = parse_hex_bytes(hex_blob);
		if (bytes.empty()) {
			continue;
		}

		const uint64_t address = parse_address(addr_str);
		ram.load(address, bytes);
	}
}

void run_until_halt(CPU& cpu, int max_cycles = 2048) {
	while (cpu.stat() == Y86Stat::AOK && max_cycles-- > 0) {
		cpu.run_cycle();
	}
	REQUIRE(max_cycles >= 0);
	REQUIRE(cpu.stat() == Y86Stat::HLT);
}

} // namespace

TEST_CASE("prog1 halts with expected result", "[cpu][prog1]") {
	VectorRAM ram(512);
	Bus bus;
	bus.register_device(ram, 0, ram.size());

	load_yo_program(ram, "test/prog1.yo");

	CPU cpu(bus);
	cpu.reset();
	run_until_halt(cpu);

	REQUIRE(cpu.pc() == 0x19);
	REQUIRE(cpu.read_register(CPU::Register::RAX) == 13);
	REQUIRE(cpu.read_register(CPU::Register::RDX) == 10);
}

TEST_CASE("prog2 reaches halt without extra padding", "[cpu][prog2]") {
	VectorRAM ram(512);
	Bus bus;
	bus.register_device(ram, 0, ram.size());

	load_yo_program(ram, "test/prog2.yo");

	CPU cpu(bus);
	cpu.reset();
	run_until_halt(cpu);

	REQUIRE(cpu.pc() == 0x18);
	REQUIRE(cpu.read_register(CPU::Register::RAX) == 13);
	REQUIRE(cpu.read_register(CPU::Register::RDX) == 10);
}

TEST_CASE("prog5 handles load-use hazard", "[cpu][prog5]") {
	VectorRAM ram(1024);
	Bus bus;
	bus.register_device(ram, 0, ram.size());

	load_yo_program(ram, "test/prog5.yo");

	CPU cpu(bus);
	cpu.reset();
	run_until_halt(cpu, 4096);

	REQUIRE(cpu.pc() == 0x34);
	REQUIRE(cpu.read_register(CPU::Register::RAX) == 13);
	REQUIRE(cpu.read_register(CPU::Register::RBX) == 10);
	REQUIRE(cpu.read_register(CPU::Register::RCX) == 3);
	REQUIRE(cpu.read_register(CPU::Register::RDX) == 128);
	REQUIRE(ram.load_qword(128) == 3);
}
