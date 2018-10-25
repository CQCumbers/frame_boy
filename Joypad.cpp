#include "joypad.h"

using namespace std;

// Core Functions

Joypad::Joypad(Memory &mem_in): mem(mem_in) {
  mem.write(P1, 0xcf);
}

void Joypad::update(unsigned cpu_cycles) {
  mem.write(P1, (mem.read(P1) & 0x30) | 0xcf);
}
