#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "apu.h"
#include "cpu.h"
#include "joypad.h"
#include "ppu.h"
#include "timer.h"

struct Gameboy {
  // Internal State
  Memory mem;
  CPU cpu;
  PPU ppu;
  APU apu;
  Timer timer;
  Joypad joypad;

  // Core Functions
  explicit Gameboy(const std::string &filename, const std::string &save);
  void step();
  void update();
  void input(Input input_enum, bool val);
  const std::array<uint8_t, 160 * 144> &get_lcd() const {
    return ppu.get_lcd();
  }
  const std::vector<int16_t> &read_audio() { return apu.read_audio(); }
  void save(const std::string &save) { mem.save(save); }

  // Debug Functions
  void print() const { cpu.print(); }
};

#endif
