#include "timer.h"

using namespace std;

// Core Functions

Timer::Timer(Memory &mem_in): mem(mem_in) {
  clock = 0xabcc;
  last_bit = tima_scheduled = false;
  mem.write(DIV, clock >> 8);
}

void Timer::update(unsigned cpu_cycles) {
  // reset timer when DIV written
  if (mem.read(DIV) != (clock >> 8)) clock = 0;
  // catch up to CPU cycles
  for (unsigned i = 0; i < cpu_cycles; ++i) {
    clock += 4;
    mem.write(DIV, clock >> 8);
    if (tima_scheduled) {
      mem.write(TIMA, mem.read(TMA));
      mem.writebit(IF, 2, true);
      tima_scheduled = false;
    }
    uint8_t on = mem.readbit(TAC, 2), freq = mem.read(TAC) & 0x3;
    bool bit = (clock >> freq_to_bit[freq]) & on & 0x1;
    if (!bit && last_bit) mem.write(TIMA, mem.read(TIMA) + 1);
    if (mem.read(TIMA) == 0xff) tima_scheduled = true;
    last_bit = bit;
  }
}
