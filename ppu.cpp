#include "ppu.h"

using namespace std;

// Core Functions

PPU::PPU(Memory &mem_in): mem(mem_in) {
  mem.write16(LCDC, 0x8191);
  mode = mem.read(STAT) & 0x3;
  mem.write(LY, 0x98);
  mem.write(BGP, 0xfc);
}

void PPU::update(unsigned cpu_cycles) {
  // reset state when LCD off
  if (!(mem.read(LCDC) >> 7)) {
    cycles = 0; mode = 0x0;
    mem.write(LY, 0x0);
  }
  // catch up to CPU cycles
  for (int i = 0; i < cpu_cycles; ++i) {
    ++cycles;
    switch (mode) {
      case 0x0: // H-BLANK
        if (cycles == 51) {
          mem.write(LY, mem.read(LY) + 1); cycles = 0;
          mode = ((mem.read(LY) == 0x90) ? 0x1 : 0x2);
          if (mode == 0x1) { mem.write(IF, mem.read(IF) | 0x1); }
          mem.write(STAT, (mem.read(STAT) & 0xfc) | mode);
        }
        break;
      case 0x1: // V-BLANK
        if (cycles == 114) {
          mem.write(LY, (mem.read(LY) + 1) % 0x99);
          cycles = 0; mode = ((mem.read(LY) == 0x0) ? 0x2 : 0x1);
          mem.write(STAT, (mem.read(STAT) & 0xfc) | mode);
        }
        break;
      case 0x2: // Using OAM
        if (cycles == 20) {
          cycles = 0; mode = 0x3;
          mem.write(STAT, (mem.read(STAT) & 0xfc) | mode);
        }
        break;
      case 0x3: // Using VRAM
        if (cycles == 43) {
          cycles = 0; mode = 0x0;
          bool lyc = (mem.read(LYC) == mem.read(LY));
          mem.write(STAT, (mem.read(STAT) & 0xf8) | lyc << 2 | mode);
        }
        break;
    }
  }
}
