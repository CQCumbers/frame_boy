#include "ppu.h"
#include <iostream>

using namespace std;

// Core Functions
// How to make sure CPU cannot access VRAM during mode 3?

PPU::PPU(Memory &mem_in): mem(mem_in) {
  mem.write(LCDC, 0x91);
  mem.write(STAT, 0x80);
  mode = mem.read(STAT) & 0x3;
  mem.write(BGP, 0xfc);
  lcd.fill(0x0);
}

void PPU::drawpixels() {
  // calculate which bg_map tile
  uint8_t map_y = ((scy + y) >> 3) & 0x1f;
  uint8_t map_x = ((scx + x) >> 3) & 0x1f;
  uint8_t tile = mem.read((uint16_t)(bg_map + (map_y << 5) + map_x));
  if (!mem.readbit(LCDC, 4)) tile ^= 0x80;
  // calculate which line of bg_map tile
  uint8_t tile_y = (scy + y) & 0x7;
  uint16_t line = mem.read16((uint16_t)(bg_tiles + tile * 16 + tile_y * 2));
  for (int i = 0; i < 4; ++i, ++x) {
    // read pixel from line of bg_map tile
    uint8_t tile_x = 7 - ((scx + x) & 0x7);
    uint8_t pixel = ((line >> tile_x) & 0x1) | ((line >> (7 + tile_x)) & 0x2);
    lcd[y * 160 + x] = (mem.read(BGP) >> (pixel << 1)) & 0x3;
  }
}

void PPU::update(unsigned cpu_cycles) {
  // reset state when LCD off
  if (!mem.readbit(LCDC, 7)) {
    cycles = 0; mode = 0x0;
    mem.write(LY, 0x0);
  }
  // catch up to CPU cycles
  for (int i = 0; i < cpu_cycles; ++i) {
    ++cycles;
    switch (mode) {
      case 0x0: // H-BLANK
        if (cycles == 94) {
          mem.write(LY, mem.read(LY) + 1); cycles = 0;
          mode = ((mem.read(LY) == 144) ? 0x1 : 0x2);
          if (mem.readbit(STAT, mode)) mem.writebit(IF, 1, true);
          if (mode == 0x1) mem.writebit(IF, 0, true);
          mem.write(STAT, (mem.read(STAT) & 0xfc) | mode);
        }
        break;
      case 0x1: // V-BLANK
        if (cycles == 114) {
          mem.write(LY, (mem.read(LY) + 1) % 0x99);
          cycles = 0; mode = ((mem.read(LY) == 0x0) ? 0x2 : 0x1);
          if (mode == 0x2 && mem.readbit(STAT, 0x2)) mem.writebit(IF, 1, true);
          mem.write(STAT, (mem.read(STAT) & 0xfc) | mode);
        }
        break;
      case 0x2: // Using OAM
        if (cycles == 20) {
          cycles = 0; mode = 0x3; x = 0; y = mem.read(LY);
          scx = mem.read(SCX), scy = mem.read(SCY);
          mem.write(STAT, (mem.read(STAT) & 0xfc) | mode);
          // set VRAM memory map
          bg_map = mem.readbit(LCDC, 3) ? 0x9c00 : 0x9800;
          win_map = mem.readbit(LCDC, 6) ? 0x9c00 : 0x9800;
          bg_tiles = mem.readbit(LCDC, 4) ? 0x8000 : 0x8800;
        }
        break;
      case 0x3: // Using VRAM
        if (cycles > 3) drawpixels();
        if (x == 160) {
          mode = x = 0; bool lyc = (mem.read(LYC) == mem.read(LY));
          if (mem.readbit(STAT, mode)) mem.writebit(IF, 1, true);
          if (lyc && mem.readbit(STAT, 6)) mem.writebit(IF, 1, true);
          mem.write(STAT, (mem.read(STAT) & 0xf8) | (lyc << 2) | mode);
        }
        break;
    }
  }
}

const array<uint8_t, 160 * 144> &PPU::get_lcd() const {
  return lcd;
}
