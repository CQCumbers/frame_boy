#ifndef PPU_H
#define PPU_H

#include "memory.h"

class PPU {
  private:
    // Internal State
    Memory &mem;
    uint8_t mode;
    unsigned cycles = 0;
    static const uint8_t LCDC = 0x40, STAT = 0x41, SCY = 0x42,
      SCX = 0x43, LY = 0x44, LYC = 0x45, DMA = 0x46, BGP = 0x47,
      OBP0 = 0x48, OBP1 = 0x49, WX = 0x4a, WY = 0x4b;

  public:
    // Core Functions
    PPU(Memory &mem_in);
    void update(unsigned cpu_cycles);
};

#endif
