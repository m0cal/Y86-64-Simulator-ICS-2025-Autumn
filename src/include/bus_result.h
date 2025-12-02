#pragma once

#include <cstdint>

#include "y86_stat.h"

namespace Y86_64 {

struct BusResult {
    uint8_t data{0};
    Y86Stat status_code{Y86Stat::AOK};
};

}

