#pragma once
#include "ram.h"
#include <string>

namespace Y86_64 {

class ProgramLoader {
public:
    void load(const std::string& input, RAM& ram);
};

}
