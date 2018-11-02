#include <algorithm>
#include <iostream>
#include "ppu.h"

using namespace std;

// Sprite Functions

Sprite::Sprite(Memory &mem, uint16_t addr) {
  y = mem.ref(addr);
  x = mem.ref(addr + 1);
  tile = mem.ref(addr + 2);
  flags = mem.ref(addr + 3);
}

bool Sprite::operator <(const Sprite &r) const {
  return x > r.x;
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
      if (sprites.size() < 10) sprites.push_front(sprite);
    }
    stable_sort(sprites.begin(), sprites.end());
  }
}

void PPU::draw_tile(uint16_t map, uint8_t x_offset, uint8_t y_offset, unsigned start) {
  // find correct tile in map
  uint8_t map_x = (x_offset >> 3) & 0x1f, map_y = (y_offset >> 3) & 0x1f;
  uint8_t tile = mem.ref(map + (map_y << 5) + map_x);
  // find correct line in tile
  uint16_t bg_tiles = read1(lcdc, 4) << 11 ? 0x8000 : 0x8800;
  if (bg_tiles != 0x8000) tile ^= 0x80;
  uint8_t tile_x = 7 - (x_offset & 0x7), tile_y = y_offset & 0x7;
  uint8_t line = mem.ref(bg_tiles + (tile << 4) + (tile_y << 1));
  uint8_t lineh = mem.ref(bg_tiles + (tile << 4) + (tile_y << 1) + 1);
  // find correct pixels in line
  for (unsigned i = start; i < 4; ++i, --tile_x) {
    uint8_t pixel = (read1(lineh, tile_x) << 1) | read1(line, tile_x);
    pixels[i] = pixel, palettes[i] = bgp;
  }
}

void PPU::draw_sprite(Sprite sprite) {  
  // find correct line in sprite
  bool height16 = read1(lcdc, 2);
  unsigned tile_x = x + 8 - sprite.x, tile_y = ly + 16 - sprite.y;
  // check mirroring and height
  if (!sprite.xf) tile_x = 7 - tile_x;
  if (sprite.yf) tile_y = 7 + (height16 << 3) - tile_y;
  if (height16) sprite.tile = write1(sprite.tile, 0, tile_y >= 8), tile_y &= 0x7;
  uint8_t line = mem.ref(0x8000 + (sprite.tile << 4) + (tile_y << 1));
  uint8_t lineh = mem.ref(0x8001 + (sprite.tile << 4) + (tile_y << 1));
  // find correct pixels in line
  for (unsigned i = 0; i < 4; ++i, tile_x += (sprite.xf ? 1 : -1)) {
    if (x + i >= sprite.x || x + 8 + i < sprite.x) continue;
    uint8_t pixel = (read1(lineh, tile_x) << 1) | read1(line, tile_x);
    // check sprite rendering priority
    if (pixel != 0 && (!sprite.p || pixels[i] == 0)) {
      pixels[i] = pixel, palettes[i] = sprite.pal ? obp1 : obp0;
    }
  }
}

void PPU::draw() {
  // draw background or blank
  pixels.fill(0), palettes.fill(bgp);
  if (read1(lcdc, 0)) {
    uint16_t bg_map = read1(lcdc, 3) ? 0x9c00 : 0x9800;
    draw_tile(bg_map, scx + x, scy + ly, 0);
    draw_tile(bg_map, scx + x + 1, scy + ly, 1);
    draw_tile(bg_map, scx + x + 2, scy + ly, 2);
    draw_tile(bg_map, scx + x + 3, scy + ly, 3);
  }
  // draw window
  bool win = read1(lcdc, 5) && ly >= wy && x + 7 >= wx;
  if (win) {
    uint16_t win_map = read1(lcdc, 6) ? 0x9c00 : 0x9800;
    ly -= wy, draw_tile(win_map, x + 7 - wx, ly, 0);
    draw_tile(win_map, x + 8 - wx, ly, 1);
    draw_tile(win_map, x + 9 - wx, ly, 2);
    draw_tile(win_map, x + 10 - wx, ly, 3);
  }
  // draw sprites
  for (const Sprite sprite: sprites) {
    if (!(x >= sprite.x || x + 11 < sprite.x)) draw_sprite(sprite);
  }
  if (win) ly += wy;
  // apply palette
  for (unsigned i = 0; i < 4; ++i, ++x) {
    lcd[ly * 160 + x] = (palettes[i] >> (pixels[i] << 1)) & 0x3;
  }
}

void PPU::check_lyc() {
  bool lyc_equal = lyc == ly;
  stat = write1(stat, 2, lyc_equal);
  if (read1(stat, 6) && lyc_equal) IF = write1(IF, 1, true);
}

// Core Functions

PPU::PPU(Memory &mem_in): mem(mem_in) {
  // set initial register values
  lcdc = 0x91, stat = 0x81;
  ly = 0x90, bgp = 0xfc,
  IF = 0xe1, lcd.fill(0x0);
  // set r/w permission bitmasks
  mem.wmask(0xff41, 0x78);
  mem.wmask(0xff44, 0x0);
  mem.rmask(0xff46, 0x0);
  // create on-write hooks
  mem.hook(0xff46, [&](uint8_t val) {
    dma_src = (val << 8) - 1, dma_i = 0;
  });
  mem.hook(0xff45, [&](uint8_t val) {
    stat = write1(stat, 2, ly == val);
    if (read1(stat, 6) && ly == val) IF = write1(IF, 1, true);
  });
}

void PPU::update(unsigned cpu_cycles) {
  // handle DMA OAM copy
  for (unsigned i = 0; dma_i < 161 && i < cpu_cycles; ++i, ++dma_i) {
    if (dma_i != 0) mem.ref(0xfdff + dma_i) = mem.ref(dma_src + dma_i);
  }
  // change mode & draw lcd
  if (read1(lcdc, 7)) {
    for (unsigned i = 0; i < cpu_cycles; ++i, ++cycles) {
      switch (stat & 0x3) {
        case 0: // H-BLANK
          if (cycles == 93) {
            cycles = 0, ++ly, check_lyc();
            unsigned mode = (ly == 144 ? 1 : 2);
            if (mode == 1) IF = write1(IF, 0, true);
            else mem.mask(Range(0xfe00, 0xfe9f), 0x0);
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
            mem.mask(Range(0x8000, 0x9fff), 0x0);
            stat = (stat & 0xfc) | 3;
          }
          break;
        case 3: // Using VRAM
          if (cycles >= 3) draw();
          if (x == 160) {
            if (read1(stat, 3)) IF = write1(IF, 1, true);
            mem.mask(Range(0xfe00, 0xfe9f), 0xff);
            mem.mask(Range(0x8000, 0x9fff), 0xff);
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
