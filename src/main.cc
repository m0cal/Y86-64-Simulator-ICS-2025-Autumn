#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "bus.h"
#include "cpu.h"
#include "ram.h"

using namespace Y86_64;

namespace {

struct YoSegment {
  uint64_t address = 0;
  std::vector<uint8_t> bytes;
};

std::string trim(const std::string& text) {
  std::string result;
  result.reserve(text.size());
  for (char ch : text) {
    if (!std::isspace(static_cast<unsigned char>(ch))) {
      result.push_back(ch);
    }
  }
  return result;
}

std::vector<uint8_t> parse_hex_blob(const std::string& blob) {
  std::string filtered;
  filtered.reserve(blob.size());
  for (char ch : blob) {
    if (std::isxdigit(static_cast<unsigned char>(ch))) {
      filtered.push_back(ch);
    }
  }

  std::vector<uint8_t> bytes;
  bytes.reserve(filtered.size() / 2);
  for (std::size_t i = 0; i + 1 < filtered.size(); i += 2) {
    const auto high = filtered[i];
    const auto low = filtered[i + 1];
    const std::string byte_str{high, low};
    bytes.push_back(static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16)));
  }
  return bytes;
}

uint64_t parse_address(const std::string& token) {
  const std::string cleaned = trim(token);
  if (cleaned.empty()) {
    throw std::invalid_argument("Missing address token");
  }
  const bool has_prefix = cleaned.rfind("0x", 0) == 0 || cleaned.rfind("0X", 0) == 0;
  const std::string numeric = has_prefix ? cleaned.substr(2) : cleaned;
  if (numeric.empty()) {
    throw std::invalid_argument("Invalid address token");
  }
  return std::stoull(numeric, nullptr, 16);
}

std::vector<YoSegment> parse_yo_program(const std::string& content) {
  std::vector<YoSegment> segments;
  std::istringstream input(content);
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    const auto colon = line.find(':');
    const auto pipe = line.find('|', colon == std::string::npos ? 0 : colon);
    if (colon == std::string::npos || pipe == std::string::npos) {
      continue;
    }

    const auto addr_str = line.substr(0, colon);
    const auto hex_blob = line.substr(colon + 1, pipe - colon - 1);
    auto bytes = parse_hex_blob(hex_blob);
    if (bytes.empty()) {
      continue;
    }

    try {
      const uint64_t address = parse_address(addr_str);
      segments.push_back({address, std::move(bytes)});
    } catch (...) {
      continue;
    }
  }
  return segments;
}

int encode_status(Y86Stat stat) {
  switch (stat) {
    case Y86Stat::AOK:
      return 1;
    case Y86Stat::HLT:
      return 2;
    case Y86Stat::ADR:
      return 3;
    case Y86Stat::INS:
      return 4;
  }
  return 4;
}

struct Snapshot {
  uint64_t pc = 0;
  int stat = 0;
  int of = 0;
  int sf = 0;
  int zf = 0;
  std::vector<std::pair<std::string, int64_t>> registers;
  std::vector<std::pair<uint64_t, int64_t>> memory;
};

Snapshot capture_state(const CPU& cpu, const RAM& ram) {
  static constexpr std::array<std::pair<CPU::Register, const char*>, 15> kRegisterOrder = {
    {{CPU::Register::R10, "r10"},
     {CPU::Register::R11, "r11"},
     {CPU::Register::R12, "r12"},
     {CPU::Register::R13, "r13"},
     {CPU::Register::R14, "r14"},
     {CPU::Register::R8, "r8"},
     {CPU::Register::R9, "r9"},
     {CPU::Register::RAX, "rax"},
     {CPU::Register::RBP, "rbp"},
     {CPU::Register::RBX, "rbx"},
     {CPU::Register::RCX, "rcx"},
     {CPU::Register::RDI, "rdi"},
     {CPU::Register::RDX, "rdx"},
     {CPU::Register::RSI, "rsi"},
     {CPU::Register::RSP, "rsp"}}};

  Snapshot snapshot;
  snapshot.pc = cpu.pc();
  snapshot.stat = encode_status(cpu.stat());
  const auto cc = cpu.condition_codes();
  snapshot.of = cc.of ? 1 : 0;
  snapshot.sf = cc.sf ? 1 : 0;
  snapshot.zf = cc.zf ? 1 : 0;

  snapshot.registers.reserve(kRegisterOrder.size());
  for (const auto& entry : kRegisterOrder) {
    const auto raw = cpu.read_register(entry.first);
    snapshot.registers.emplace_back(entry.second, static_cast<int64_t>(raw));
  }

  constexpr uint64_t kWordBytes = 8;
  for (uint64_t addr = 0; addr + kWordBytes <= ram.size(); addr += kWordBytes) {
    uint64_t value = 0;
    for (uint64_t i = 0; i < kWordBytes; ++i) {
      value |= static_cast<uint64_t>(ram.peek(addr + i)) << (i * 8);
    }
    if (value != 0) {
      snapshot.memory.emplace_back(addr, static_cast<int64_t>(value));
    }
  }

  return snapshot;
}

void emit_states(const std::vector<Snapshot>& states) {
  std::cout << "[";
  for (std::size_t i = 0; i < states.size(); ++i) {
    if (i > 0) {
      std::cout << ",";
    }

    const auto& state = states[i];
    std::cout << "{";

    std::cout << "\"CC\":{";
    std::cout << "\"OF\":" << state.of << ",";
    std::cout << "\"SF\":" << state.sf << ",";
    std::cout << "\"ZF\":" << state.zf;
    std::cout << "},";

    std::cout << "\"MEM\":{";
    for (std::size_t j = 0; j < state.memory.size(); ++j) {
      if (j > 0) {
        std::cout << ",";
      }
      std::cout << "\"" << state.memory[j].first << "\":" << state.memory[j].second;
    }
    std::cout << "},";

    std::cout << "\"PC\":" << state.pc << ",";

    std::cout << "\"REG\":{";
    for (std::size_t j = 0; j < state.registers.size(); ++j) {
      if (j > 0) {
        std::cout << ",";
      }
      std::cout << "\"" << state.registers[j].first << "\":" << state.registers[j].second;
    }
    std::cout << "},";

    std::cout << "\"STAT\":" << state.stat;
    std::cout << "}";
  }
  std::cout << "]";
}

} // namespace

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  const std::string program_text((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
  if (program_text.empty()) {
    std::cout << "[]";
    return 0;
  }

  const auto segments = parse_yo_program(program_text);
  if (segments.empty()) {
    std::cout << "[]";
    return 0;
  }

  uint64_t required = 0;
  for (const auto& seg : segments) {
    required = std::max(required, seg.address + static_cast<uint64_t>(seg.bytes.size()));
  }

  constexpr uint64_t kDefaultMemory = 1ull << 20; // 1 MiB default arena
  constexpr uint64_t kSafetyMargin = 1ull << 13;   // extra space for stack/data
  const uint64_t memory_size = std::max(required + kSafetyMargin, kDefaultMemory);

  RAM ram(memory_size);
  for (const auto& seg : segments) {
    ram.load_bytes(seg.address, seg.bytes);
  }

  Bus bus;
  bus.register_device(ram, 0, ram.size());

  CPU cpu(bus);
  cpu.reset();

  std::vector<Snapshot> states;
  constexpr int kMaxCycles = 100000;
  for (int cycle = 0; cycle < kMaxCycles && cpu.stat() == Y86Stat::AOK; ++cycle) {
    cpu.run_cycle();
    states.push_back(capture_state(cpu, ram));
  }

  emit_states(states);
  return 0;
}
