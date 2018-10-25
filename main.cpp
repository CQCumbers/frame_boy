#include <iostream>
#include "cpu.h"
#include "timer.h"
#include "joypad.h"
#include "ppu.h"

using namespace std;

void show(const PPU &ppu) {
  array<string, 4> pixels = {" ", ".", "o", "M"};
  const array<uint8_t, 160 * 144> &lcd = ppu.get_lcd();
  for (int j = 0; j < 160; ++j) {
    cout << "-";
  }
  cout << endl;
  for (int i = 0; i < 144; ++i) {
    for (int j = 0; j < 160; ++j) {
      cout << pixels[lcd[160 * i + j]] << flush;
    }
    cout << endl;
  }
}

int main() {
  // initialize hardware
  Memory mem("roms/dr-mario.gb");
  CPU cpu(mem); Timer timer(mem);
  Joypad joypad(mem); PPU ppu(mem);

  // run to breakpoint, then step
  //uint8_t SD = 0x01; uint8_t SC = 0x02;
  bool shown = true;
  while (cpu.get_pc() != 0x225e) { //0x2ad) {
    unsigned cycles = cpu.execute();
    timer.update(cycles);
    ppu.update(cycles);
    joypad.update(cycles);
    /*if (mem.read(SC) >> 7) {
      mem.write(SC, SD);
      cout << (char)mem.read(SD) << flush;
    }*/
    if (!shown && ppu.mode == 0x01) { show(ppu); shown = true; }
    if (ppu.mode != 0x01) { shown = false; }
  }

  show(ppu);

  uint8_t LCDC = 0x40, STAT = 0x41, LY = 0x44, IE = 0xff;
  while (cin.ignore()) {
    cpu.print();
    cout << hex << "FF40: " << (unsigned)mem.read(LCDC) << " "
      << (unsigned)mem.read(STAT) << " " << (unsigned)mem.read(LY) << endl;
    cout << hex << "IE: " << (unsigned)mem.read(IE) << endl;
    unsigned cycles = cpu.execute();
    timer.update(cycles);
    ppu.update(cycles);
    joypad.update(cycles);
  }
}
