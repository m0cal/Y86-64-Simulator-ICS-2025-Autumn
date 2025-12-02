#pragma once

#include <vector>
#include <iostream>
#include "json.hpp" 

using json = nlohmann::json;

namespace Y86_64 {

class CPU;
class RAM;

class Logger {
private:
    json trace_log; 

public:
    Logger();

    void trace(const CPU& cpu, const RAM& ram);

    void report();
};

} 