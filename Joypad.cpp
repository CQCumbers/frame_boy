#include "joypad.h"

using namespace std;

// Core Functions

Joypad::Joypad(Memory &mem_in): mem(mem_in) {
  mem.write(P1, 0xcf);
}

void Joypad::update(unsigned cpu_cycles) {
  for (unsigned i = 0; i < cpu_cycles; ++i) {
    mem.write(P1, (mem.read(P1) & 0x30) | 0xcf);
  }
}
