#include "gameboy.h"

using namespace std;

// Core Functions

Gameboy::Gameboy(const string &filename):
  mem(filename), cpu(mem), ppu(mem),
  timer(mem), joypad(mem) { }

void Gameboy::step() {
  unsigned cycles = cpu.execute();
  timer.update(cycles);
  ppu.update(cycles);
  joypad.update();
}

const array<uint8_t, 160*144> &Gameboy::update() {
  while (ppu.get_mode() == 1) step();
  while (ppu.get_mode() != 1) step();
  return ppu.get_lcd();
}

// Debug Functions

void Gameboy::print() const {
  cpu.print();
}
