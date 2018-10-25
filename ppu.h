#ifndef PPU_H
#define PPU_H

#include "memory.h"

class PPU {
  private:
    // Internal State
    Memory &mem;
    std::array<uint8_t, 160 * 144> lcd;
    unsigned cycles = 0;
    uint16_t dma_src, bg_tiles, bg_map, win_map;
    uint8_t x, y, scx, scy, wx, wy;
    static const uint8_t LCDC = 0x40, STAT = 0x41,
      SCY = 0x42, SCX = 0x43, LY = 0x44, LYC = 0x45,
      DMA = 0x46, BGP = 0x47, OBP0 = 0x48, OBP1 = 0x49,
      WX = 0x4a, WY = 0x4b, IF = 0x0f;
    static const uint16_t OAM = 0xfe00;

    // Drawing Functions
    void draw_tile(uint16_t map, uint8_t x_offset, uint8_t y_offset);
    void draw();

  public:
    // Core Functions
    PPU(Memory &mem_in);
    void update(unsigned cpu_cycles);
    const std::array<uint8_t, 160 * 144> &get_lcd() const;
    uint8_t mode;
};

#endif
