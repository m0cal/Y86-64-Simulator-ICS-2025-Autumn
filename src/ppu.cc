#include "ppu.h"

#include <algorithm> // std::fill
#include <string>

namespace Y86_64 {

void PPU::update() {
  try {
    parse_sprites();
    draw_sprites();
  }
  catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    exit(0);
  }
}

void PPU::parse_sprites() {
  sprites.clear();
  sprites.reserve(kSpriteCount);

  for (int i = 0; i < kSpriteCount; ++i) {
    const int base = i * kSpriteStride;

    uint64_t addr = 0;
    for (int b = 0; b < 8; ++b) {
      addr |= static_cast<uint64_t>(memory[base + b]) << (8 * b);
    }

    const uint8_t height = memory[base + 8];
    const uint8_t width = memory[base + 9];
    const uint8_t x = memory[base + 10];
    const uint8_t y = memory[base + 11];

    // Width/height of zero means the sprite is disabled.
    if (width == 0 || height == 0) {
      continue;
    }

    sprites.push_back(Sprite{addr, height, width, x, y});
  }
}

void PPU::set_pixel(int x, int y) {
  if (x < 0 || y < 0 || x >= kScreenWidth || y >= kScreenHeight) {
    return;
  }

  const int idx = y * kScreenWidth + x;
  const int byte_idx = idx / 8;
  const int bit_idx = idx % 8;
  back_buffer_[byte_idx] |= static_cast<uint8_t>(1u << bit_idx);
}

void PPU::draw_sprites() {
  std::fill(back_buffer_.begin(), back_buffer_.end(), 0);

  for (const auto& sprite : sprites) {
    for (int row = 0; row < sprite.height; ++row) {
      for (int col = 0; col < sprite.width; ++col) {
        const int bit_index = row * sprite.width + col;
        const uint64_t byte_addr = sprite.addr + static_cast<uint64_t>(bit_index / 8);
        const auto result = bus.read(byte_addr);

        if (result.status_code != Y86Stat::AOK) {
          continue;
        }

        const uint8_t mask = static_cast<uint8_t>(1u << (bit_index % 8));
        if ((result.data & mask) == 0) {
          continue;
        }

        const int screen_x = sprite.x + col;
        const int screen_y = sprite.y + row;
        set_pixel(screen_x, screen_y);
      }
    }
  }

  present_frame();
}

void PPU::present_frame() {
  if (back_buffer_ == front_buffer_) {
    return;
  }

  front_buffer_ = back_buffer_;
  if (render_enabled_) {
    render_to_terminal();
  }
}

void PPU::render_to_terminal() const {
  // Basic terminal output using two-color palette (# / space).
  for (int y = 0; y < kScreenHeight; ++y) {
    std::string line;
    line.reserve(kScreenWidth);

    for (int x = 0; x < kScreenWidth; ++x) {
      const int idx = y * kScreenWidth + x;
      const bool on = (front_buffer_[idx / 8] >> (idx % 8)) & 0x1;
      line.push_back(on ? '#' : ' ');
    }

    std::cout << line << '\n';
  }

  std::cout << std::flush;
}

} // namespace Y86_64
