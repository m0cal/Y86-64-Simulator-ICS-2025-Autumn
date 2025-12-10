#pragma once
#include <cstdint>
#include <termios.h>
#include <unistd.h>

#include "bus.h"

namespace Y86_64 {

// Simple 1-byte joystick mapped at 0x2000. Bits follow docs:
// bit7:A_UP, bit6:A_DOWN, bit5:B_UP, bit4:B_DOWN, bit3:START, bit2:RESET, bit1:RESV, bit0:RESV.
class Joystick : public Device {
public:
    Joystick();
    ~Joystick() override;

    uint8_t read(uint64_t addr) override;
    void write(uint64_t addr, uint8_t data) override;

    // Poll /dev/tty for keypresses and update state bits for this frame.
    void update();

private:
    int fd_{-1};
    termios orig_{};
    int orig_flags_{0};
    uint8_t state_{0};
    bool termios_ok_{false};
    bool fcntl_ok_{false};

    void set_raw_mode();
    void restore_mode();
};

} // namespace Y86_64
