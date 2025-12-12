#pragma once
#include <vector>
#include <cstdint>
#include <iostream>   // std::cout, std::endl
#include <exception>  // std::exception
#include <cstdlib>    // exit()
#include "bus.h"


namespace Y86_64{
  
  struct Sprite{
    uint64_t addr;
    uint8_t height;
    uint8_t width;
    uint8_t x;
    uint8_t y;
  };

  class PPU : public Device { 
    public: 
      explicit PPU(Bus& system_bus, bool render_enabled = true)
          : memory(kPpuMemorySize, 0),
            front_buffer_(kFrameBufferBytes, 0),
            back_buffer_(kFrameBufferBytes, 0),
            bus(system_bus),
            render_enabled_(render_enabled) {}
      
      // Toggle terminal rendering. Useful for automated tests where extra
      // stdout would interfere with expected output.
      void set_render_enabled(bool enable) { render_enabled_ = enable; }
      
      uint8_t read(uint64_t addr) override {
        if (addr >= memory.size()) return 0;
        return memory[addr];
      }
      
      void write(uint64_t addr, uint8_t data) override {
        if (addr < memory.size()) memory[addr] = data;
      }

      // Expose framebuffer for testing/inspection (front buffer reflects last presented frame).
      const std::vector<uint8_t>& frame_buffer_for_test() const { return front_buffer_; }

      void update();

    private:
      static constexpr int kScreenWidth = 120;
      static constexpr int kScreenHeight = 30;
      static constexpr int kFrameBufferBytes = (kScreenWidth * kScreenHeight + 7) / 8;
      static constexpr int kSpriteCount = 16;
      static constexpr int kSpriteStride = 12; // 8 bytes addr + 4 bytes meta
      static constexpr int kPpuMemorySize = kSpriteCount * kSpriteStride;

      std::vector<uint8_t> memory;
      std::vector<Sprite> sprites;
      std::vector<uint8_t> front_buffer_;
      std::vector<uint8_t> back_buffer_;
      Bus& bus;
      bool render_enabled_;
      void parse_sprites();
      void draw_sprites();
      void present_frame();
      void render_to_terminal() const;
      void set_pixel(int x, int y);
  };
}
