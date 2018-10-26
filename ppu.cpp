#include "ppu.h"
#include <iostream>

using namespace std;

// Core Functions
// How to make sure CPU cannot access VRAM during mode 3?

PPU::PPU(Memory &mem_in): mem(mem_in) {
  // based on Sameboy values
  mem.write(IF, 0xe1);
  mem.write(LCDC, 0x91);
  mem.write(STAT, 0x81);
  mode = mem.read(STAT) & 0x3;
  mem.write(LY, 0x90);
  mem.write(DMA, 0xff);
  mem.write(BGP, 0xfc);
  lcd.fill(0x0);
}

inline uint16_t PPU::getbit(uint16_t val, unsigned index) {
  return (val >> index) & 0x1;
}

void PPU::get_sprites() {
  sprite_h = mem.readbit(LCDC, 2) ? 16 : 8; sprites.clear();
  for (uint16_t i = OAM; i < OAM + 160; i += 4) {
    Sprite sprite; sprite.y = mem.read(i);
    if ((y + 16) >= sprite.y && (y + 16) < (sprite.y + sprite_h)) {
      sprite.x = mem.read((uint16_t)(i + 1));
      sprite.tile = mem.read((uint16_t)(i + 2));
      sprite.flags = mem.read((uint16_t)(i + 3));
      sprites.push_back(sprite);
      if (sprites.size() > 10) sprites.pop_front();
    }
  }
}

void PPU::draw_tile(uint16_t map, uint8_t x_offset, uint8_t y_offset) {
  // calculate which bg_map tile
  uint8_t map_x = (x_offset >> 3) & 0x1f;
  uint8_t map_y = (y_offset >> 3) & 0x1f;
  uint8_t tile = mem.read((uint16_t)(map + (map_y << 5) + map_x));
  if (!mem.readbit(LCDC, 4)) tile ^= 0x80;
  // calculate which line of bg_tile
  uint8_t tile_y = y_offset & 0x7;
  uint16_t line = mem.read16((uint16_t)(bg_tiles + tile * 16 + tile_y * 2));
  for (int i = 0; i < 4; ++i, ++x_offset, ++x) {
    // read pixel from line of bg_tile
    // first bit from lower byte, second bit from upper byte
    uint8_t tile_x = 7 - (x_offset & 0x7);
    // store palette index (BGP = 0) in upper nibble of lcd byte
    lcd[y * 160 + x] = getbit(line, tile_x + 8) * 2 + getbit(line, tile_x);
  }
}

void PPU::draw_sprite(Sprite s) {  
  uint8_t tile_y = y + 16 - s.y;
  // handle vertical mirroring
  if (getbit(s.flags, 6)) tile_y = sprite_h - tile_y;
  // handle double height sprites
  if (sprite_h == 16) {
    s.tile = (tile_y < 8 ? s.tile & 0xfe : s.tile | 0x1);
    tile_y &= 0x7;
  }
  uint16_t line = mem.read16((uint16_t)(SP_TILES + s.tile * 16 + tile_y * 2));
  for (int i = 0; i < 4; ++i, ++x) {
    if (x + 8 < s.x || x >= s.x) continue;
    uint8_t tile_x = x + 8 - s.x;
    // handle horizontal mirroring
    if (!getbit(s.flags, 5)) tile_x = 7 - tile_x;
    uint8_t pixel = (getbit(line, tile_x + 8) << 1) | getbit(line, tile_x);
    // handle rendering priority
    if (pixel != 0 && (!getbit(s.flags, 7) || (lcd[y * 160 + x] & 0x3) == 0)) {
      lcd[y * 160 + x] = ((getbit(s.flags, 4) + 1) << 4) + pixel;
    }
  }
}

void PPU::draw() {
  // cout << "X: " << dec << (unsigned)x << endl;
  uint8_t initial_x = x;
  // draw blank screen
  if (!mem.readbit(LCDC, 0)) {
    while(x < initial_x + 4) {
      lcd[y * 160 + x++] = 0x0;
    }
    return;
  }
  // draw background or window
  if (!mem.readbit(LCDC, 5) || y < wy || x < wx) {
    draw_tile(bg_map, scx + x, scy + y);
  } else {
    x -= (x - wx) & 0x3;
    draw_tile(win_map, x - wx, y - wy);
  }
  // draw sprites
  if (mem.readbit(LCDC, 1)) {
    for (const Sprite s: sprites) {
      x = initial_x; draw_sprite(s);
    }
  }
  // apply palette
  for (unsigned i = y * 160 + initial_x; i < y * 160 + x; ++i) {
    uint8_t palette = BGP + (lcd[i] >> 4), pixel = lcd[i] & 0xf;
    lcd[i] = (mem.read(palette) >> (pixel * 2)) & 0x3;
  }
}

void PPU::update(unsigned cpu_cycles) {
  mem.writebit(STAT, 7, true);
  // DMA OAM copy
  if (mem.read(DMA) != 0xff) {
    uint16_t dma_src = mem.read(DMA) << 8; mem.write(DMA, 0xff);
    for (uint16_t i = 0; i < 160; ++i) {
      mem.write((uint16_t)(OAM + i), mem.read((uint16_t)(dma_src + i)));
    }
  }
  // reset state when LCD off
  if (!mem.readbit(LCDC, 7) && cycles != 0) {
    cycles = 0; mode = 0x0; mem.write(LY, 0x0);
    bool lyc = (mem.read(LYC) == mem.read(LY));
    mem.write(STAT, (mem.read(STAT) & 0xf8) | (lyc << 2) | mode);
    lcd.fill(0x0); return;
  }
  // catch up to CPU cycles
  for (unsigned i = 0; i < cpu_cycles; ++i) {
    ++cycles;
    //cout << dec << (unsigned)mode << " " << (unsigned)cycles << endl;
    switch (mode) {
      case 0x0: // H-BLANK
        if (cycles == 94) {
          bool lyc = (mem.read(LYC) == mem.read(LY));
          mem.write(LY, mem.read(LY) + 1); cycles = 0;
          mode = ((mem.read(LY) == 144) ? 0x1 : 0x2);
          if (mem.readbit(STAT, 3 + mode)) mem.writebit(IF, 1, true);
          if (mode == 0x1) mem.writebit(IF, 0, true);
          mem.write(STAT, (mem.read(STAT) & 0xf8) | (lyc << 2) | mode);
        }
        break;
      case 0x1: // V-BLANK
        if (cycles == 114) {
          bool lyc = (mem.read(LYC) == mem.read(LY));
          mem.write(LY, (mem.read(LY) + 1) % 0x99);
          cycles = 0; mode = ((mem.read(LY) == 0x0) ? 0x2 : 0x1);
          if (mode == 0x2 && mem.readbit(STAT, 5)) mem.writebit(IF, 1, true);
          mem.write(STAT, (mem.read(STAT) & 0xf8) | (lyc << 2) | mode);
        }
        break;
      case 0x2: // Using OAM
        if (cycles == 20) {
          x = 0; y = mem.read(LY); get_sprites();
          scx = mem.read(SCX); scy = mem.read(SCY);
          wx = mem.read(WX); wy = mem.read(WY);
          bg_map = mem.readbit(LCDC, 3) ? 0x9c00 : 0x9800;
          win_map = mem.readbit(LCDC, 6) ? 0x9c00 : 0x9800;
          bg_tiles = mem.readbit(LCDC, 4) ? 0x8000 : 0x8800;

          cycles = 0; mode = 0x3;
          bool lyc = (mem.read(LYC) == y);
          mem.write(STAT, (mem.read(STAT) & 0xf8) | (lyc << 2) | mode);
        }
        break;
      case 0x3: // Using VRAM
        if (cycles > 3) draw();
        if (x >= 160) {
          mode = 0x0; bool lyc = (mem.read(LYC) == mem.read(LY));
          if (mem.readbit(STAT, 3)) mem.writebit(IF, 1, true);
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
