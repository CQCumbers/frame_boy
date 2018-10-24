#include <iostream>
#include "cpu.h"
#include "ppu.h"
#include "timer.h"

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
  Memory mem("roms/instr_timing.gb");
  CPU cpu(mem); PPU ppu(mem); Timer timer(mem);

  // run to breakpoint, then step
  cpu.print();
  bool printed = false;
  uint8_t SD = 0x01; uint8_t SC = 0x02;
  while (true) {//mem.read(uint8_t(0x07)) == 0) {//mem.read(cpu.get_pc()) != 0xf0) {
    unsigned cycles = cpu.execute();
    ppu.update(cycles);
    timer.update(cycles);
    if (mem.read(SC) >> 7) {
      mem.write(SC, SD);
      cout << (char)mem.read(SD) << flush;
    }
    if(!printed && ppu.mode == 0x1) { show(ppu); printed = true; }
    if (ppu.mode != 0x1) { printed = false; }
  }

  while (cin.ignore()) {
    cpu.print();
    cout << "IF value: " << hex << (unsigned)mem.read((uint8_t)0xf) << endl;
    cout << "TIMA value: " << hex << (unsigned)mem.read((uint8_t)0x05) << endl;
    cout << "TAC value: " << hex << (unsigned)mem.read((uint8_t)0x07) << endl;
    unsigned cycles = cpu.execute();
    ppu.update(cycles);
    timer.update(cycles);
  }
}
