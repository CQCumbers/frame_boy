#ifndef PPU_H
#define PPU_H

#include "memory.h"

struct Sprite {
  uint16_t addr;
  uint8_t y, x, tile;
  union {
    struct {
      uint8_t : 4, pal : 1, xf : 1, yf : 1, p : 1;
    };
    uint8_t flags;
  };
  Sprite(Memory &mem, uint16_t addr_in);
  bool operator<(const Sprite &r) const;
};

class PPU {
private:
  // Internal State
  Memory &mem;
  std::vector<Sprite> sprites;
  std::array<uint8_t, 4> pixels;
  std::array<uint8_t, 4> palettes;
  std::array<uint8_t, 160 * 144> lcd;
  unsigned cycles = 0, dma_i = 161;
  uint16_t x = 0, dma_src = 0;

  // Cached Properties
  uint16_t bg_tiles = 0x8800;
  uint16_t bg_map = 0x9800;
  uint16_t win_map = 0x9800;
  uint8_t mode = 0;
  bool height16 = false;

  // Registers
  uint8_t &lcdc = mem.refh(0x40), &stat = mem.refh(0x41);
  uint8_t &scy = mem.refh(0x42), &scx = mem.refh(0x43);
  uint8_t &ly = mem.refh(0x44), &lyc = mem.refh(0x45);
  uint8_t &bgp = mem.refh(0x47), &obp0 = mem.refh(0x48);
  uint8_t &obp1 = mem.refh(0x49), &wy = mem.refh(0x4a);
  uint8_t &wx = mem.refh(0x4b), &IF = mem.refh(0x0f);

  // Drawing Functions
  void get_sprites();
  void draw_sprite(Sprite &sprite);
  void draw_tile(uint16_t map, uint8_t x, uint8_t y, unsigned i);
  void draw();
  void check_lyc() const;

public:
  // Core Functions
  explicit PPU(Memory &mem_in);
  void update(unsigned cpu_cycles);
  uint8_t get_mode() const { return stat & 0x3; }
  const std::array<uint8_t, 160 * 144> &get_lcd() const { return lcd; }
};

#endif
