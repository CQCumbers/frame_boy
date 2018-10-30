#include <iostream>
#include "ppu.h"

using namespace std;

Sprite::Sprite(Memory &mem, uint16_t addr) {
  y = mem.read(addr);
  x = mem.read(addr + 1);
  tile = mem.read(addr + 2);
  flags = mem.read(addr + 3);
}

// Drawing Functions

void PPU::get_sprites() {
  sprites.clear();
  if (!read1(lcdc, 1)) return;
  unsigned height = 8 + (read1(lcdc, 2) << 3);
  // fetch sprites from OAM RAM
  for (uint16_t i = 0xfe00; i < 0xfe9f; i += 4) {
    Sprite sprite(mem, i); // max 10 per line
    if (ly + 16 >= sprite.y && ly + 16 < sprite.y + height) {
      if (sprites.size() == 10) sprites.pop_front();
      sprites.push_back(sprite);
    }
  }
}

void PPU::draw_tile(uint16_t map, uint8_t x_offset, uint8_t y_offset) {
  // find correct tile in map
  uint8_t map_x = (x_offset >> 3) & 0x1f, map_y = (y_offset >> 3) & 0x1f;
  uint8_t tile = mem.read(map + (map_y << 5) + map_x);
  // find correct line in tile
  uint16_t bg_tiles = read1(lcdc, 4) << 11 ? 0x8000 : 0x8800;
  if (bg_tiles != 0x8000) tile ^= 0x80;
  uint8_t tile_x = 7 - (x_offset & 0x7), tile_y = y_offset & 0x7;
  uint16_t line = mem.read16(bg_tiles + (tile << 4) + (tile_y << 1));
  // find correct pixels in line
  for (unsigned i = 0; i < 4; ++i, --tile_x) {
    uint8_t pixel = (read1(line, tile_x + 8) << 1) | read1(line, tile_x);
    pixels[i] = pixel, palettes[i] = bgp;
  }
}

void PPU::draw_sprite(Sprite sprite) {  
  // find correct line in sprite
  bool height16 = read1(lcdc, 2);
  unsigned tile_x = x + 8 - sprite.x, tile_y = ly + 16 - sprite.y;
  // check mirroring and height
  if (!sprite.xf) tile_x = 7 - tile_x;
  if (sprite.yf) tile_y = 8 + (height16 << 3) - tile_y;
  if (height16) sprite.tile = write1(sprite.tile, 0, tile_y >= 8), tile_y &= 0x7;
  uint16_t line = mem.read16(0x8000 + (sprite.tile << 4) + (tile_y << 1));
  // find correct pixels in line
  for (unsigned i = 0; i < 4; ++i, tile_x += (sprite.xf ? 1 : -1)) {
    if (x + i >= sprite.x || x + 8 + i < sprite.x) continue;
    uint8_t pixel = (read1(line, tile_x + 8) << 1) | read1(line, tile_x);
    // check sprite rendering priority
    if (pixel != 0 && (!sprite.p || pixels[i] == 0)) {
      pixels[i] = pixel, palettes[i] = sprite.pal ? obp1 : obp0;
    }
  }
}

void PPU::draw() {
  if (read1(lcdc, 0)) {
    // draw background or window
    if (!read1(lcdc, 5) || ly < wy || x < wx) {
      uint16_t bg_map = 0x9800 + (read1(lcdc, 3) << 10);
      draw_tile(bg_map, scx + x, scy + ly);
    } else {
      uint16_t win_map = 0x9800 + (read1(lcdc, 6) << 10);
      draw_tile(win_map, x - wx, ly - wy);
    }
    // draw sprites
    for (const Sprite sprite: sprites) {
      if (!(x >= sprite.x || x + 11 < sprite.x)) draw_sprite(sprite);
    }
    // apply palette
    for (unsigned i = 0; i < 4; ++i, ++x) {
      lcd[ly * 160 + x] = (palettes[i] >> (pixels[i] << 1)) & 0x3;
    }
  } else {
    // draw blank screen
    for (unsigned i = 0; i < 4; ++i, ++x) {
      lcd[ly * 160 + x] = 0;
    }
  }
}

void PPU::check_lyc() {
  // check LYC interrupt
  bool lyc_equal = lyc == ly;
  stat = write1(stat, 2, lyc_equal);
  if (read1(stat, 6) && lyc_equal) IF = write1(IF, 1, true);
}

// Core Functions

PPU::PPU(Memory &mem_in): mem(mem_in) {
  // set initial register values
  lcdc = 0x91, stat = 0x81;
  ly = 0x90, dma = 0xff;
  bgp = 0xfc, IF = 0xe1;
  lcd.fill(0x0);
  // set r/w permission bitmasks
  mem.mask(0xff41, 0x1f);
  mem.mask(0xff44, 0x0);
}

void PPU::update(unsigned cpu_cycles) {
  check_lyc();
  // handle DMA OAM copy
  if (dma != 0xff) dma_src = (dma << 8) - 1, dma = 0xff, dma_i = 0;
  for (unsigned i = 0; dma_i < 161 && i < cpu_cycles; ++i, ++dma_i) {
    if (dma_i != 0) mem.write(0xfdff + dma_i, mem.read(dma_src + dma_i));
  }
  // catch up to CPU cycles
  if (read1(lcdc, 7)) {
    for (unsigned i = 0; i < cpu_cycles; ++i, ++cycles) {
      // change mode & draw lcd
      switch (stat & 0x3) {
        case 0: // H-BLANK
          if (cycles == 93) {
            cycles = 0, ++ly, check_lyc();
            unsigned mode = (ly == 144 ? 1 : 2);
            if (mode == 1) IF = write1(IF, 0, true);
            if (read1(stat, 3 + mode)) IF = write1(IF, 1, true);
            stat = (stat & 0xfc) | mode;
          }
          break;
        case 1: // V-BLANK
          if (cycles == 113) {
            if (ly == 154) {
              ly = -1, stat = (stat & 0xfc) | 2;
              if (read1(stat, 5)) IF = write1(IF, 1, true);
            }
            cycles = 0, ++ly, check_lyc();
          }
          break;
        case 2: // Using OAM
          if (cycles == 19) {
            cycles = x = 0, get_sprites();
            stat = (stat & 0xfc) | 3;
          }
          break;
        case 3: // Using VRAM
          if (cycles >= 3) draw();
          if (x == 160) {
            if (read1(stat, 3)) IF = write1(IF, 1, true);
            stat = stat & 0xfc;
          }
          break;
      }
    }
  } else if (cycles != 0) {
    // reset state when LCD off
    cycles = ly = x = 0;
    lcd.fill(0);
    stat = stat & 0xfc;
  }
}

uint8_t PPU::get_mode() const {
  return stat & 0x3;
}

const array<uint8_t, 160*144> &PPU::get_lcd() const {
  return lcd;
}
