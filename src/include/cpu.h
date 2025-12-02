#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "bus.h"
#include "y86_stat.h"

namespace Y86_64 {

class CPU {
public:
	static constexpr std::size_t kRegisterCount = 15;

	enum class Register : uint8_t {
		RAX = 0,
		RCX = 1,
		RDX = 2,
		RBX = 3,
		RSP = 4,
		RBP = 5,
		RSI = 6,
		RDI = 7,
		R8 = 8,
		R9 = 9,
		R10 = 10,
		R11 = 11,
		R12 = 12,
		R13 = 13,
		R14 = 14,
		RNONE = 0xF
	};

	struct ConditionCodes {
		bool zf = true;
		bool sf = false;
		bool of = false;
	};

	explicit CPU(Bus& system_bus);

	void reset();
	void run_cycle();

	[[nodiscard]] uint64_t pc() const noexcept { return pc_; }
	[[nodiscard]] Y86Stat stat() const noexcept { return stat_; }
	[[nodiscard]] ConditionCodes condition_codes() const noexcept { return cc_; }
	[[nodiscard]] const std::array<uint64_t, kRegisterCount>& registers() const noexcept { return registers_; }
	[[nodiscard]] uint64_t read_register(Register reg) const;

private:
	enum class Opcode : uint8_t {
		HALT = 0x0,
		NOP = 0x1,
		CMOVXX = 0x2,
		IRMOVQ = 0x3,
		RMMOVQ = 0x4,
		MRMOVQ = 0x5,
		OPQ = 0x6,
		JXX = 0x7,
		CALL = 0x8,
		RET = 0x9,
		PUSHQ = 0xA,
		POPQ = 0xB,
		IADDQ = 0xC
	};

	struct StageState {
		uint8_t icode = 0;
		uint8_t ifun = 0;
		uint8_t rA = static_cast<uint8_t>(Register::RNONE);
		uint8_t rB = static_cast<uint8_t>(Register::RNONE);
		uint64_t valC = 0;
		uint64_t valA = 0;
		uint64_t valB = 0;
		uint64_t valE = 0;
		uint64_t valM = 0;
		uint64_t valP = 0;
		uint64_t inst_pc = 0;
		bool cnd = true;
		bool fetch_ok = false;
		bool decode_ok = false;
		bool execute_ok = false;
		bool mem_ok = false;
	};

	void fetch();
	void decode();
	void execute();
	void memory();
	void write_back();
	void update_pc();

	[[nodiscard]] bool instruction_uses_reg_ids(uint8_t icode) const;
	[[nodiscard]] bool instruction_uses_valC(uint8_t icode) const;
	[[nodiscard]] bool is_valid_instruction(uint8_t icode) const;
	[[nodiscard]] bool is_valid_ifun(uint8_t icode, uint8_t ifun) const;
	[[nodiscard]] bool is_valid_register(uint8_t id) const;
	[[nodiscard]] bool evaluate_condition(uint8_t ifun) const;
	uint64_t perform_alu(uint8_t ifun, uint64_t lhs, uint64_t rhs, bool& valid);
	void update_cc(uint8_t op_ifun, uint64_t lhs, uint64_t rhs, uint64_t result);

	bool read_byte(uint64_t addr, uint8_t& value);
	bool write_byte(uint64_t addr, uint8_t value);
	bool read_u64(uint64_t addr, uint64_t& value);
	bool write_u64(uint64_t addr, uint64_t value);

	[[nodiscard]] uint64_t get_register(uint8_t id) const;
	void set_register(uint8_t id, uint64_t value);
	void set_status(Y86Stat candidate);

	Bus& bus_;
	std::array<uint64_t, kRegisterCount> registers_{};
	ConditionCodes cc_{};
	Y86Stat stat_{Y86Stat::AOK};
	uint64_t pc_{0};
	StageState stage_{};
};

} // namespace Y86_64
