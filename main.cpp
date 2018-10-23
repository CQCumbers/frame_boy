#include <iostream>
#include "cpu.h"
#include "ppu.h"

using namespace std;

int main() {
  // initialize hardware
  Memory mem("roms/cpu_instrs.gb");
  CPU cpu(mem); PPU ppu(mem);

  // run to breakpoint, then step
  cpu.print();
  /*for (int i = 0; i < 512; ++i) {*/
  while (cin.ignore()) {
    uint8_t SD = 0x01; uint8_t SC = 0x02;
    while (true) {//(mem.read(cpu.get_pc()) != 0xe8) {
      ppu.update(cpu.execute());
      if (mem.read(SC) >> 7) {
        mem.write(SC, SD);
        cout << (char)mem.read(SD) << flush;
      }
    }
    cpu.print();
    ppu.update(cpu.execute());
    cpu.print();
  }
  while (cin.ignore()) {
    cpu.print();
    ppu.update(cpu.execute());
    cout << "LY value: " << hex << (unsigned)mem.read((uint8_t)0x44) << endl;
  }
}
