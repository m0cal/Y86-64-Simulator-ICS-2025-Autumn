#include "logger.h"
#include "cpu.h"
#include "ram.h" 

namespace Y86_64 {

int64_t to_signed(uint64_t val) {
    return static_cast<int64_t>(val);
}


Logger::Logger() {
    trace_log = json::array(); 
}

void Logger::trace(const CPU& cpu, const RAM& ram) {
    json frame;

    frame["PC"] = cpu.pc();
    int stat_val = 1; 
    switch (cpu.stat()) {
        case Y86Stat::AOK: stat_val = 1; break;
        case Y86Stat::HLT: stat_val = 2; break;
        case Y86Stat::ADR: stat_val = 3; break;
        case Y86Stat::INS: stat_val = 4; break;
    }
    frame["STAT"] = stat_val;

    const auto& regs = cpu.registers();
    auto& reg_json = frame["REG"];
    
    reg_json["rax"] = to_signed(regs[0]);
    reg_json["rcx"] = to_signed(regs[1]);
    reg_json["rdx"] = to_signed(regs[2]);
    reg_json["rbx"] = to_signed(regs[3]);
    reg_json["rsp"] = to_signed(regs[4]);
    reg_json["rbp"] = to_signed(regs[5]);
    reg_json["rsi"] = to_signed(regs[6]);
    reg_json["rdi"] = to_signed(regs[7]);
    reg_json["r8"]  = to_signed(regs[8]);
    reg_json["r9"]  = to_signed(regs[9]);
    reg_json["r10"] = to_signed(regs[10]);
    reg_json["r11"] = to_signed(regs[11]);
    reg_json["r12"] = to_signed(regs[12]);
    reg_json["r13"] = to_signed(regs[13]);
    reg_json["r14"] = to_signed(regs[14]);

    
    auto cc = cpu.condition_codes();
    frame["CC"]["ZF"] = cc.zf ? 1 : 0;
    frame["CC"]["SF"] = cc.sf ? 1 : 0;
    frame["CC"]["OF"] = cc.of ? 1 : 0;

    frame["MEM"] = json::object();
    
    for (uint64_t addr = 0; addr < 4096; addr += 8) {
        uint64_t val = 0;
        for(int i=0; i<8; ++i) {
            val |= (uint64_t)ram.read(addr + i) << (i * 8);
        }

        if (val != 0) {
            frame["MEM"][std::to_string(addr)] = to_signed(val);
        }
    }

    trace_log.push_back(frame);
}

void Logger::report() {
    std::cout << trace_log.dump(4) << std::endl;
}

}