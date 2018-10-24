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
  Memory mem("roms/cpu_instrs.gb");
  CPU cpu(mem); PPU ppu(mem); Timer timer(mem);

  // run to breakpoint, then step
  uint8_t SD = 0x01; uint8_t SC = 0x02;
  bool shown = false;
  while (true) {
    unsigned cycles = cpu.execute();
    ppu.update(cycles);
    timer.update(cycles);
    if (mem.read(SC) >> 7) {
      mem.write(SC, SD);
      cout << (char)mem.read(SD) << flush;
    }
    if (!shown && ppu.mode == 0x01) { show(ppu); shown = true; }
    if (ppu.mode != 0x01) { shown = false; }
  }

  while (cin.ignore()) {
    cpu.print();
    cout << "TIMA value: " << hex << (unsigned)mem.read((uint8_t)0x05) << endl;
    cout << "TAC value: " << hex << (unsigned)mem.read((uint8_t)0x07) << endl;
    unsigned cycles = cpu.execute();
    ppu.update(cycles);
    timer.update(cycles);
  }
}
