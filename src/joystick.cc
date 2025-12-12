#include "joystick.h"

#include <fcntl.h>
#include <unistd.h>

#include <vector>

namespace Y86_64 {

namespace {
constexpr uint8_t kBitAUp = 1u << 7;
constexpr uint8_t kBitADown = 1u << 6;
constexpr uint8_t kBitBUp = 1u << 5;
constexpr uint8_t kBitBDown = 1u << 4;
constexpr uint8_t kBitStart = 1u << 3;
constexpr uint8_t kBitReset = 1u << 2;
}

Joystick::Joystick() : fd_(STDIN_FILENO) {
    set_raw_mode();
}

Joystick::~Joystick() {
    restore_mode();
}

uint8_t Joystick::read(uint64_t /*addr*/) {
    return state_;
}

void Joystick::write(uint64_t /*addr*/, uint8_t /*data*/) {
    // Joystick is input-only; writes are ignored.
}

void Joystick::set_raw_mode() {
    if (tcgetattr(fd_, &orig_) == 0) {
        termios raw = orig_;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(fd_, TCSANOW, &raw) == 0) {
            termios_ok_ = true;
        }
    }

    const int flags = fcntl(fd_, F_GETFL, 0);
    if (flags != -1 && fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == 0) {
        orig_flags_ = flags;
        fcntl_ok_ = true;
    }
}

void Joystick::restore_mode() {
    if (termios_ok_) {
        tcsetattr(fd_, TCSANOW, &orig_);
    }
    if (fcntl_ok_) {
        fcntl(fd_, F_SETFL, orig_flags_);
    }
}

void Joystick::update() {
    state_ = 0;

    std::vector<uint8_t> buf;
    uint8_t ch = 0;
    while (::read(fd_, &ch, 1) == 1) {
        buf.push_back(ch);
    }

    for (size_t i = 0; i < buf.size(); ++i) {
        const uint8_t c = buf[i];

        if (c == '\x1b' && i + 2 < buf.size() && buf[i + 1] == '[') {
            const uint8_t code = buf[i + 2];
            if (code == 'A') {
                state_ |= kBitBUp;
            } else if (code == 'B') {
                state_ |= kBitBDown;
            }
            i += 2;
            continue;
        }

        switch (c) {
            case 'w':
            case 'W':
                state_ |= kBitAUp;
                break;
            case 's':
            case 'S':
                state_ |= kBitADown;
                break;
            case 'e':
            case 'E':
                state_ |= kBitStart;
                break;
            case 'r':
            case 'R':
                state_ |= kBitReset;
                break;
            default:
                break;
        }
    }
}

} // namespace Y86_64
