#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#include "bus.h"
#include "cpu.h"
#include "logger.h"
#include "ram.h" 

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
    
    try {
        bus.register_device(ram, 0, 4096);
    } catch (...) {}

    load_program_from_stdin(ram);

    CPU cpu(bus);
    Logger logger;

    int max_cycles = 10000;
    int cycle = 0;
    
    bool is_running = true;
    while (is_running && cycle < max_cycles) {
        if (cpu.stat() == Y86Stat::AOK) {
            cpu.run_cycle();
            logger.trace(cpu, ram);
        } else {
            is_running = false;
        }
        cycle++;
    }

    logger.report();
    return 0;
}