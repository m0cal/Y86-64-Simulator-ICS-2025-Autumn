#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#include "bus.h"
#include "cpu.h"
#include "logger.h"
#include "ram.h"
#include "ppu.h"
#include "joystick.h"
#include "timer.h"

using namespace Y86_64;

void load_program_from_stdin(RAM& ram) {
    std::string line;
    while (std::getline(std::cin, line)) {
      
        size_t pipe_pos = line.find('|');
        if (pipe_pos == std::string::npos) pipe_pos = line.length();
        
        std::string content = line.substr(0, pipe_pos);

        size_t colon_pos = content.find(':');
        if (colon_pos == std::string::npos) continue; 

        std::string data_part = content.substr(colon_pos + 1);
        std::string hex_code;
        for (char c : data_part) {
            if (!isspace(c)) hex_code += c;
        }

        if (hex_code.empty()) continue;

        std::string addr_part = content.substr(0, colon_pos);
        
        try {
            uint64_t addr = std::stoull(addr_part, nullptr, 16);

            for (size_t i = 0; i + 1 < hex_code.length(); i += 2) {
                std::string byte_str = hex_code.substr(i, 2);
                try {
                    uint8_t byte = (uint8_t)std::stoul(byte_str, nullptr, 16);
                    ram.write(addr + (i / 2), byte);
                } catch(...) {
                    break; 
                }
            }
        } catch (const std::exception& e) {
            continue;
        }
    }
}

int main() {
    Bus bus;
    RAM ram(4096);
    
    // Only RAM is needed to load program; attach others after stdin is consumed
    // to avoid interfering with terminal modes (joystick sets raw mode).
    try {
        bus.register_device(ram, 0, 4096);
    } catch (...) {}

    load_program_from_stdin(ram);

    PPU ppu(bus, /*render_enabled=*/true);
    Joystick joystick;
    Timer timer;

    try {
        bus.register_device(ppu, 0x3000, 0x30C0);
        bus.register_device(joystick, 0x2000, 0x2001);
        bus.register_device(timer, 0x4000, 0x4001);
    } catch (...) {}

    CPU cpu(bus);
    Logger logger;

    int max_cycles = 10000;
    int cycle = 0;
    
    bool is_running = true;
    while (is_running && cycle < max_cycles) {
        if (cpu.stat() == Y86Stat::AOK) {
            cpu.run_cycle();
            ppu.update();
            joystick.update();
            timer.update();
            logger.trace(cpu, ram);
        } else {
            is_running = false;
        }
        cycle++;
    }

    logger.report();
    return 0;
}