#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "timer.h"
#include "joypad.h"

struct Gameboy {
  // Internal State
  Memory mem;
  CPU cpu;
  PPU ppu;
  APU apu;
  Timer timer;
  Joypad joypad;

  // Core Functions
  explicit Gameboy(const std::string &filename);
  void step();
  void update();
  void input(Input input_enum, bool val);
  const std::array<uint8_t, 160*144> &get_lcd() const { return ppu.get_lcd(); }
  const std::vector<uint8_t> &get_audio() const { return apu.get_audio(); }
  void clear_audio() { apu.clear_audio(); }

  // Debug Functions
  void print() const { cpu.print(); }
};

#endif
