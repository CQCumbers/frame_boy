#include "ppu.h"
#include <algorithm>

// Sprite Functions

Sprite::Sprite(Memory &mem, uint16_t addr_in)
    : addr(addr_in), y(mem.ref(addr)), x(mem.ref(addr + 1)),
      tile(mem.ref(addr + 2)), flags(mem.ref(addr + 3)) {}

bool Sprite::operator<(const Sprite &r) const {
  return x > r.x || (x == r.x && addr > r.addr);
}

// Drawing Functions

void PPU::get_sprites() {
  sprites.clear();
  if (!read1(lcdc, 1)) return;
  unsigned height = 8 + (read1(lcdc, 2) << 3);
  // fetch sprites from OAM RAM
  for (uint16_t i = 0xfe00; i < 0xfe9f; i += 4) {
    Sprite sprite(mem, i);
    if (ly + 16 < sprite.y || ly + 16 >= sprite.y + height) continue;
    sprites.push_back(sprite);
    if (sprites.size() == 10) break;
  }
  // sort sprites by priority
  std::sort(sprites.begin(), sprites.end());
}

void PPU::draw_tile(uint16_t map, uint8_t x, uint8_t y, unsigned i) {
  // find correct tile in map
  uint8_t map_x = (x >> 3) & 0x1f, map_y = (y >> 3) & 0x1f;
  uint8_t tile = mem.ref(map + (map_y << 5) + map_x);
  tile ^= (bg_tiles >> 4) & 0x80;
  // find correct line in tile
  uint8_t tile_x = 7 - (x & 0x7), tile_y = y & 0x7;
  uint16_t addr = bg_tiles + (tile << 4) + (tile_y << 1);
  const uint8_t &line = mem.ref(addr), &lineh = mem.ref(addr + 1);
  pixels[i] = (read1(lineh, tile_x) << 1) | read1(line, tile_x);
}

void PPU::draw_sprite(const Sprite &sprite) {
  // find correct line in sprite
  unsigned tile_x = x + 8 - sprite.x, tile_y = ly + 16 - sprite.y;
  uint8_t tile = sprite.tile;
  // check mirroring and height
  if (!sprite.xf) tile_x = 7 - tile_x;
  if (sprite.yf) tile_y = 7 + (height16 << 3) - tile_y;
  uint16_t addr = 0x8000 + (tile << 4) + (tile_y << 1);
  const uint8_t &line = mem.ref(addr), &lineh = mem.ref(addr + 1);
  // find correct pixels in line
  int dir = sprite.xf ? 1 : -1;
  for (unsigned i = 0; i < 4; ++i, tile_x += dir) {
    uint8_t pixel = (read1(lineh, tile_x) << 1) | read1(line, tile_x);
    // check sprite rendering priority
    if (pixel != 0 && (!sprite.p || pixels[i] == 0)) {
      pixels[i] = pixel;
      palettes[i] = sprite.pal ? obp1 : obp0;
    }
  }
}

void PPU::draw() {
  // draw background or blank
  palettes.fill(bgp);
  if (read1(lcdc, 0)) {
    for (uint8_t i = 0, j = scx + x; i < 4; ++i, ++j)
      draw_tile(bg_map, j, scy + ly, i);
  }
  // draw window
  if (read1(lcdc, 5) && ly >= wy && x + 7 >= wx) {
    for (uint8_t i = 0, j = x + 7 - wx; i < 4; ++i, ++j)
      draw_tile(win_map, j, ly - wy, i);
  }
  // draw sprites
  for (Sprite &sprite : sprites) {
    if (x >= sprite.x || x + 11 < sprite.x) continue;
    draw_sprite(sprite);
  }
  // apply palette
  for (uint16_t i = 0, j = ly * 160 + x; i < 4; ++i, ++j) {
    lcd[j] = (palettes[i] >> (pixels[i] << 1)) & 0x3;
  }
  x += 4;
}

void PPU::check_lyc() const {
  bool lyc_equal = lyc == ly;
  stat = write1(stat, 2, lyc_equal);
  if (read1(stat, 6) && lyc_equal) IF = write1(IF, 1, true);
}

// Core Functions

PPU::PPU(Memory &mem_in) : mem(mem_in) {
  // set initial register values
  lcdc = 0x91, stat = 0x81;
  ly = 0x8f, bgp = 0xfc;
  IF = 0xe1;
  lcd.fill(0x0);
  // set r/w permission bitmasks
  mem.wmask(0xff41, 0x78);
  mem.wmask(0xff44, 0x0);
  mem.rmask(0xff46, 0x0);
  // create on-write hooks
  mem.hook(0xff46, [&](uint8_t val) {
    dma_src = (val << 8) - 1;
    dma_i = 0;
  });
  mem.hook(0xff45, [&](uint8_t val) {
    stat = write1(stat, 2, ly == val);
    if (read1(stat, 6) && ly == val) IF = write1(IF, 1, true);
  });
  mem.hook(0xff40, [&](uint8_t val) {
    bg_tiles = read1(val, 4) ? 0x8000 : 0x8800;
    bg_map = read1(val, 3) ? 0x9c00 : 0x9800;
    win_map = read1(val, 6) ? 0x9c00 : 0x9800;
    height16 = read1(val, 2);
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
      switch (mode) {
      case 0: { // H-BLANK
        if (cycles != 93) continue;
        cycles = 0, ++ly, check_lyc();
        mode = (ly == 144 ? 1 : 2);
        if (mode == 2) mem.mask(Range(0xfe00, 0xfe9f), 0x0);
        if (read1(stat, 3 + mode)) IF = write1(IF, 1, true);
        stat = (stat & 0xfc) | mode;
        continue;
      }
      case 1: // V-BLANK
        if (ly == 144 && cycles == 4) IF = write1(IF, 0, true);
        if (cycles != 113) continue;
        if (ly == 154) {
          ly = -1, mode = 2;
          stat = (stat & 0xfc) | 2;
          if (read1(stat, 5)) IF = write1(IF, 1, true);
        }
        cycles = 0, ++ly, check_lyc();
        continue;
      case 2: // Using OAM
        if (cycles != 19) continue;
        cycles = x = 0;
        get_sprites();
        mem.mask(Range(0x8000, 0x9fff), 0x0);
        stat = (stat & 0xfc) | 3, mode = 3;
        continue;
      case 3: // Using VRAM
        if (cycles >= 3) draw();
        if (x != 160) continue;
        if (read1(stat, 3)) IF = write1(IF, 1, true);
        mem.mask(Range(0xfe00, 0xfe9f), 0xff);
        mem.mask(Range(0x8000, 0x9fff), 0xff);
        stat = stat & 0xfc, mode = 0;
        continue;
      }
    }
  } else if (cycles != 0) {
    // reset state when LCD off
    cycles = ly = x = 0;
    lcd.fill(0);
    stat = stat & 0xfc, mode = 0;
  }
}
