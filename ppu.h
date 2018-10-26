#ifndef PPU_H
#define PPU_H

#include <deque>
#include "memory.h"

struct Sprite { uint8_t y, x, tile, flags; };

class PPU {
  private:
    // Internal State
    Memory &mem;
    std::deque<Sprite> sprites;
    std::array<uint8_t, 160 * 144> lcd;
    unsigned cycles = 0;
    uint16_t bg_tiles, bg_map, win_map;
    uint8_t x, y, scx, scy, wx, wy, sprite_h;
    static const uint16_t OAM = 0xFE00, SP_TILES = 0x8000;
    static const uint8_t LCDC = 0x40, STAT = 0x41,
      SCY = 0x42, SCX = 0x43, LY = 0x44, LYC = 0x45,
      DMA = 0x46, BGP = 0x47, OBP0 = 0x48, OBP1 = 0x49,
      WX = 0x4a, WY = 0x4b, IF = 0x0f;

    // Drawing Functions
    uint16_t getbit(uint16_t val, unsigned index);
    void get_sprites();
    void draw_sprite(Sprite s);
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
