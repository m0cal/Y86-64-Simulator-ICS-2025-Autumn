#pragma once
#include <cstind>
#include "y86_stat.h"

namespace Y86_64{

  struct BusResult{
    uint8_t data;
    Y86Stat status_code;
  };

}

