#include <catch2/catch_test_macros.hpp>

#include <thread>
#include <vector>

#include "bus.h"
#include "ppu.h"
#include "timer.h"

namespace {
using namespace Y86_64;

class StubRAM : public Device {
public:
    explicit StubRAM(std::size_t size) : mem_(size, 0) {}

    uint8_t read(uint64_t addr) override {
        REQUIRE(addr < mem_.size());
        return mem_[static_cast<std::size_t>(addr)];
    }

    void write(uint64_t addr, uint8_t data) override {
        REQUIRE(addr < mem_.size());
        mem_[static_cast<std::size_t>(addr)] = data;
    }

private:
    std::vector<uint8_t> mem_;
};

bool pixel_on(const std::vector<uint8_t>& fb, int x, int y, int width = 120) {
    const int idx = y * width + x;
    const int byte_idx = idx / 8;
    const int bit_idx = idx % 8;
    return (fb[byte_idx] >> bit_idx) & 0x1;
}

TEST_CASE("PPU renders bit-packed sprite", "[ppu]") {
    Bus bus;
    StubRAM ram(64);
    bus.register_device(ram, 0, 64);

    // Sprite bitmap: 3x5 triangle
    // Row0: 00001
    // Row1: 00011
    // Row2: 00111
    ram.write(0, 0x10); // bits 0..7
    ram.write(1, 0x73); // bits 8..15

    PPU ppu(bus, /*render_enabled=*/false);

    // Sprite 0 metadata at offset 0
    for (int i = 0; i < 8; ++i) {
        ppu.write(i, 0x00); // base addr = 0
    }
    ppu.write(8, 3);  // height
    ppu.write(9, 5);  // width
    ppu.write(10, 0); // x
    ppu.write(11, 0); // y

    ppu.update();
    const auto& fb = ppu.frame_buffer_for_test();

    REQUIRE(pixel_on(fb, 4, 0));
    REQUIRE(pixel_on(fb, 3, 1));
    REQUIRE(pixel_on(fb, 4, 1));
    REQUIRE(pixel_on(fb, 2, 2));
    REQUIRE(pixel_on(fb, 3, 2));
    REQUIRE(pixel_on(fb, 4, 2));

    // A pixel outside the shape should be off
    REQUIRE_FALSE(pixel_on(fb, 0, 0));
    REQUIRE_FALSE(pixel_on(fb, 1, 2));
}

TEST_CASE("Timer advances near 60Hz", "[timer]") {
    Timer timer;

    const uint8_t start = timer.read(0);
    timer.update();
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    timer.update();
    const uint8_t mid = timer.read(0);

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    timer.update();
    const uint8_t end = timer.read(0);

    // After ~50ms we expect at least 2 ticks
    REQUIRE(mid >= start);
    REQUIRE(end > start);
    REQUIRE(static_cast<int>(end - start) >= 2);
}

} // namespace
