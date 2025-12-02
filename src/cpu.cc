#include "cpu.h"

#include <limits>

namespace Y86_64 {

namespace {
constexpr uint64_t kWordBytes = 8;

bool did_add_overflow(int64_t lhs, int64_t rhs, int64_t result) {
    const bool lhs_pos = lhs >= 0;
    const bool rhs_pos = rhs >= 0;
    const bool res_pos = result >= 0;
    return (lhs_pos && rhs_pos && !res_pos) || (!lhs_pos && !rhs_pos && res_pos);
}

bool did_sub_overflow(int64_t lhs, int64_t rhs, int64_t result) {
    const bool lhs_pos = lhs >= 0;
    const bool rhs_pos = rhs >= 0;
    const bool res_pos = result >= 0;
    return (lhs_pos && !rhs_pos && !res_pos) || (!lhs_pos && rhs_pos && res_pos);
}

int status_priority(Y86Stat status) {
    switch (status) {
        case Y86Stat::AOK:
            return 0;
        case Y86Stat::INS:
            return 1;
        case Y86Stat::ADR:
            return 2;
        case Y86Stat::HLT:
            return 3;
    }
    return 0;
}

} // namespace

CPU::CPU(Bus& system_bus) : bus_(system_bus) {
    reset();
}

void CPU::reset() {
    registers_.fill(0);
    cc_ = ConditionCodes{};
    stat_ = Y86Stat::AOK;
    pc_ = 0;
    stage_ = StageState{};
    stage_.mem_ok = true;
}

uint64_t CPU::read_register(Register reg) const {
    const auto idx = static_cast<uint8_t>(reg);
    if (idx >= kRegisterCount) {
        return 0;
    }
    return registers_[idx];
}

void CPU::run_cycle() {
    if (stat_ != Y86Stat::AOK) {
        return;
    }

    stage_ = StageState{};
    stage_.inst_pc = pc_;

    fetch();
    decode();
    execute();
    memory();
    write_back();
    update_pc();
}

void CPU::fetch() {
    stage_.fetch_ok = false;
    stage_.rA = static_cast<uint8_t>(Register::RNONE);
    stage_.rB = static_cast<uint8_t>(Register::RNONE);

    uint8_t inst_byte = 0;
    if (!read_byte(pc_, inst_byte)) {
        return;
    }

    stage_.icode = inst_byte >> 4;
    stage_.ifun = inst_byte & 0xF;
    stage_.valP = pc_ + 1;

    if (!is_valid_instruction(stage_.icode) || !is_valid_ifun(stage_.icode, stage_.ifun)) {
        set_status(Y86Stat::INS);
        return;
    }

    if (instruction_uses_reg_ids(stage_.icode)) {
        uint8_t reg_byte = 0;
        if (!read_byte(stage_.valP, reg_byte)) {
            return;
        }
        stage_.rA = reg_byte >> 4;
        stage_.rB = reg_byte & 0xF;
        stage_.valP += 1;
    }

    if (instruction_uses_valC(stage_.icode)) {
        if (!read_u64(stage_.valP, stage_.valC)) {
            return;
        }
        stage_.valP += kWordBytes;
    }

    stage_.fetch_ok = true;
}

void CPU::decode() {
    if (!stage_.fetch_ok) {
        return;
    }

    stage_.decode_ok = true;
    stage_.valA = 0;
    stage_.valB = 0;

    const auto opcode = static_cast<Opcode>(stage_.icode);
    auto require_reg = [&](uint8_t id, bool allow_none = false) -> bool {
        if (allow_none && id == static_cast<uint8_t>(Register::RNONE)) {
            return true;
        }
        if (!is_valid_register(id)) {
            set_status(Y86Stat::INS);
            stage_.decode_ok = false;
            return false;
        }
        return true;
    };

    switch (opcode) {
        case Opcode::CMOVXX:
            if (!require_reg(stage_.rA) || !require_reg(stage_.rB)) {
                return;
            }
            stage_.valA = registers_[stage_.rA];
            break;
        case Opcode::IRMOVQ:
            if (!require_reg(stage_.rB)) {
                return;
            }
            break;
        case Opcode::RMMOVQ:
            if (!require_reg(stage_.rA) || !require_reg(stage_.rB)) {
                return;
            }
            stage_.valA = registers_[stage_.rA];
            stage_.valB = registers_[stage_.rB];
            break;
        case Opcode::MRMOVQ:
            if (!require_reg(stage_.rA) || !require_reg(stage_.rB)) {
                return;
            }
            stage_.valB = registers_[stage_.rB];
            break;
        case Opcode::OPQ:
            if (!require_reg(stage_.rA) || !require_reg(stage_.rB)) {
                return;
            }
            stage_.valA = registers_[stage_.rA];
            stage_.valB = registers_[stage_.rB];
            break;
        case Opcode::JXX:
            break;
        case Opcode::CALL:
            stage_.valA = stage_.valP;
            stage_.valB = registers_[static_cast<uint8_t>(Register::RSP)];
            break;
        case Opcode::RET:
            stage_.valA = registers_[static_cast<uint8_t>(Register::RSP)];
            stage_.valB = stage_.valA;
            break;
        case Opcode::PUSHQ:
            if (!require_reg(stage_.rA)) {
                return;
            }
            stage_.valA = registers_[stage_.rA];
            stage_.valB = registers_[static_cast<uint8_t>(Register::RSP)];
            break;
        case Opcode::POPQ:
            if (!require_reg(stage_.rA)) {
                return;
            }
            stage_.valA = registers_[static_cast<uint8_t>(Register::RSP)];
            stage_.valB = stage_.valA;
            break;
        case Opcode::IADDQ:
            if (!require_reg(stage_.rB)) {
                return;
            }
            stage_.valB = registers_[stage_.rB];
            break;
        case Opcode::NOP:
        case Opcode::HALT:
        default:
            break;
    }
}

void CPU::execute() {
    if (!stage_.decode_ok) {
        return;
    }

    stage_.execute_ok = true;
    stage_.cnd = true;

    const auto opcode = static_cast<Opcode>(stage_.icode);
    switch (opcode) {
        case Opcode::CMOVXX:
            stage_.cnd = evaluate_condition(stage_.ifun);
            stage_.valE = stage_.valA;
            break;
        case Opcode::IRMOVQ:
            stage_.valE = stage_.valC;
            break;
        case Opcode::RMMOVQ:
        case Opcode::MRMOVQ:
            stage_.valE = stage_.valB + stage_.valC;
            break;
        case Opcode::OPQ: {
            bool valid = false;
            stage_.valE = perform_alu(stage_.ifun, stage_.valB, stage_.valA, valid);
            if (!valid) {
                set_status(Y86Stat::INS);
                stage_.execute_ok = false;
                return;
            }
            update_cc(stage_.ifun, stage_.valB, stage_.valA, stage_.valE);
            break;
        }
        case Opcode::JXX:
            stage_.cnd = evaluate_condition(stage_.ifun);
            break;
        case Opcode::CALL:
        case Opcode::PUSHQ:
            stage_.valE = stage_.valB - kWordBytes;
            break;
        case Opcode::RET:
        case Opcode::POPQ:
            stage_.valE = stage_.valB + kWordBytes;
            break;
        case Opcode::IADDQ:
            stage_.valE = stage_.valB + stage_.valC;
            update_cc(0, stage_.valB, stage_.valC, stage_.valE);
            break;
        case Opcode::HALT:
            set_status(Y86Stat::HLT);
            break;
        case Opcode::NOP:
        default:
            break;
    }
}

void CPU::memory() {
    if (!stage_.execute_ok) {
        stage_.mem_ok = false;
        return;
    }

    stage_.mem_ok = true;
    const auto opcode = static_cast<Opcode>(stage_.icode);
    switch (opcode) {
        case Opcode::RMMOVQ:
            stage_.mem_ok = write_u64(stage_.valE, stage_.valA);
            break;
        case Opcode::MRMOVQ:
            stage_.mem_ok = read_u64(stage_.valE, stage_.valM);
            break;
        case Opcode::PUSHQ:
        case Opcode::CALL:
            stage_.mem_ok = write_u64(stage_.valE, stage_.valA);
            break;
        case Opcode::POPQ:
        case Opcode::RET:
            stage_.mem_ok = read_u64(stage_.valA, stage_.valM);
            break;
        default:
            break;
    }
}

void CPU::write_back() {
    if (!stage_.decode_ok) {
        return;
    }

    const auto opcode = static_cast<Opcode>(stage_.icode);
    switch (opcode) {
        case Opcode::CMOVXX:
            if (stage_.cnd) {
                set_register(stage_.rB, stage_.valE);
            }
            break;
        case Opcode::IRMOVQ:
            set_register(stage_.rB, stage_.valE);
            break;
        case Opcode::OPQ:
            set_register(stage_.rB, stage_.valE);
            break;
        case Opcode::IADDQ:
            set_register(stage_.rB, stage_.valE);
            break;
        case Opcode::MRMOVQ:
            set_register(stage_.rA, stage_.valM);
            break;
        case Opcode::POPQ:
            set_register(static_cast<uint8_t>(Register::RSP), stage_.valE);
            set_register(stage_.rA, stage_.valM);
            break;
        case Opcode::RET:
            set_register(static_cast<uint8_t>(Register::RSP), stage_.valE);
            break;
        case Opcode::PUSHQ:
        case Opcode::CALL:
            set_register(static_cast<uint8_t>(Register::RSP), stage_.valE);
            break;
        default:
            break;
    }
}

void CPU::update_pc() {
    if (!stage_.fetch_ok || !stage_.mem_ok) {
        return;
    }

    const auto opcode = static_cast<Opcode>(stage_.icode);
    switch (opcode) {
        case Opcode::HALT:
            pc_ = stage_.inst_pc;
            break;
        case Opcode::JXX:
            pc_ = stage_.cnd ? stage_.valC : stage_.valP;
            break;
        case Opcode::CALL:
            pc_ = stage_.valC;
            break;
        case Opcode::RET:
            pc_ = stage_.valM;
            break;
        default:
            pc_ = stage_.valP;
            break;
    }
}

bool CPU::instruction_uses_reg_ids(uint8_t icode) const {
    switch (static_cast<Opcode>(icode)) {
        case Opcode::CMOVXX:
        case Opcode::IRMOVQ:
        case Opcode::RMMOVQ:
        case Opcode::MRMOVQ:
        case Opcode::OPQ:
        case Opcode::PUSHQ:
        case Opcode::POPQ:
        case Opcode::IADDQ:
            return true;
        default:
            return false;
    }
}

bool CPU::instruction_uses_valC(uint8_t icode) const {
    switch (static_cast<Opcode>(icode)) {
        case Opcode::IRMOVQ:
        case Opcode::RMMOVQ:
        case Opcode::MRMOVQ:
        case Opcode::JXX:
        case Opcode::CALL:
        case Opcode::IADDQ:
            return true;
        default:
            return false;
    }
}

bool CPU::is_valid_instruction(uint8_t icode) const {
    switch (static_cast<Opcode>(icode)) {
        case Opcode::HALT:
        case Opcode::NOP:
        case Opcode::CMOVXX:
        case Opcode::IRMOVQ:
        case Opcode::RMMOVQ:
        case Opcode::MRMOVQ:
        case Opcode::OPQ:
        case Opcode::JXX:
        case Opcode::CALL:
        case Opcode::RET:
        case Opcode::PUSHQ:
        case Opcode::POPQ:
        case Opcode::IADDQ:
            return true;
        default:
            return false;
    }
}

bool CPU::is_valid_ifun(uint8_t icode, uint8_t ifun) const {
    const auto opcode = static_cast<Opcode>(icode);
    switch (opcode) {
        case Opcode::HALT:
        case Opcode::NOP:
        case Opcode::IRMOVQ:
        case Opcode::RMMOVQ:
        case Opcode::MRMOVQ:
        case Opcode::CALL:
        case Opcode::RET:
        case Opcode::PUSHQ:
        case Opcode::POPQ:
        case Opcode::IADDQ:
            return ifun == 0;
        case Opcode::CMOVXX:
            return ifun <= 0x6;
        case Opcode::OPQ:
            return ifun <= 0x3;
        case Opcode::JXX:
            return ifun <= 0x6;
        default:
            return false;
    }
}

bool CPU::is_valid_register(uint8_t id) const {
    return id < kRegisterCount;
}

bool CPU::evaluate_condition(uint8_t ifun) const {
    const bool sf_xor_of = cc_.sf ^ cc_.of;
    switch (ifun) {
        case 0x0:
            return true;
        case 0x1:
            return sf_xor_of || cc_.zf;
        case 0x2:
            return sf_xor_of;
        case 0x3:
            return cc_.zf;
        case 0x4:
            return !cc_.zf;
        case 0x5:
            return !sf_xor_of;
        case 0x6:
            return !sf_xor_of && !cc_.zf;
        default:
            return false;
    }
}

uint64_t CPU::perform_alu(uint8_t ifun, uint64_t lhs, uint64_t rhs, bool& valid) {
    valid = true;
    switch (ifun) {
        case 0x0:
            return lhs + rhs;
        case 0x1:
            return lhs - rhs;
        case 0x2:
            return lhs & rhs;
        case 0x3:
            return lhs ^ rhs;
        default:
            valid = false;
            return 0;
    }
}

void CPU::update_cc(uint8_t op_ifun, uint64_t lhs, uint64_t rhs, uint64_t result) {
    if (stat_ != Y86Stat::AOK) {
        return;
    }

    cc_.zf = (result == 0);
    cc_.sf = (static_cast<int64_t>(result) < 0);

    bool overflow = false;
    const auto lhs_signed = static_cast<int64_t>(lhs);
    const auto rhs_signed = static_cast<int64_t>(rhs);
    const auto res_signed = static_cast<int64_t>(result);

    if (op_ifun == 0x0) {
        overflow = did_add_overflow(lhs_signed, rhs_signed, res_signed);
    } else if (op_ifun == 0x1) {
        overflow = did_sub_overflow(lhs_signed, rhs_signed, res_signed);
    }

    cc_.of = overflow;
}

bool CPU::read_byte(uint64_t addr, uint8_t& value) {
    const BusResult result = bus_.read(addr);
    if (result.status_code != Y86Stat::AOK) {
        set_status(result.status_code);
        return false;
    }
    value = result.data;
    return true;
}

bool CPU::write_byte(uint64_t addr, uint8_t value) {
    const BusResult result = bus_.write(addr, value);
    if (result.status_code != Y86Stat::AOK) {
        set_status(result.status_code);
        return false;
    }
    return true;
}

bool CPU::read_u64(uint64_t addr, uint64_t& value) {
    value = 0;
    for (uint64_t offset = 0; offset < kWordBytes; ++offset) {
        uint8_t byte = 0;
        if (!read_byte(addr + offset, byte)) {
            return false;
        }
        value |= static_cast<uint64_t>(byte) << (offset * 8);
    }
    return true;
}

bool CPU::write_u64(uint64_t addr, uint64_t value) {
    for (uint64_t offset = 0; offset < kWordBytes; ++offset) {
        const uint8_t byte = static_cast<uint8_t>((value >> (offset * 8)) & 0xFF);
        if (!write_byte(addr + offset, byte)) {
            return false;
        }
    }
    return true;
}

void CPU::set_register(uint8_t id, uint64_t value) {
    if (!is_valid_register(id)) {
        return;
    }
    registers_[id] = value;
}

void CPU::set_status(Y86Stat candidate) {
    if (candidate == stat_) {
        return;
    }

    if (status_priority(candidate) >= status_priority(stat_)) {
        stat_ = candidate;
    }
}

} // namespace Y86_64
