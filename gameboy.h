#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "cpu.h"
#include "timer.h"
#include "joypad.h"
#include "ppu.h"

struct Gameboy {
  // Internal State
  Memory mem;
  CPU cpu;
  PPU ppu;
  Timer timer;
  Joypad joypad;

  // Core Functions
  Gameboy(const std::string &filename);
  void step();
  const std::array<uint8_t, 160*144> &update();
  void input(Input input_enum, bool val);

  // Debug Functions
  void print() const;
};

#endif
