#include "timer.h"

using namespace std;

// Core Functions

Timer::Timer(Memory &mem_in): mem(mem_in) {
  div = clock >> 8;
  // reset timer on DIV write
  mem.wmask(0xff04, 0x0);
  mem.hook(0xff04, [&](uint8_t) { clock = 0; });
}

void Timer::update(unsigned cpu_cycles) {
  uint8_t on = read1(tac, 2), freq = tac & 0x3;
  // catch up to CPU cycles
  for (unsigned i = 0; i < cpu_cycles; ++i) {
    clock += 4, div = clock >> 8;
    if (tima_scheduled) {
      tima = tma, IF = write1(IF, 2, true);
      tima_scheduled = false;
    }
    bool bit = on && read1(clock, freq_to_bit[freq]);
    if (!bit && last_bit) ++tima;
    tima_scheduled = (tima == 0xff), last_bit = bit;
  }
}
