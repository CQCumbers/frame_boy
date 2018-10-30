#include "timer.h"

using namespace std;

// Core Functions

Timer::Timer(Memory &mem_in): mem(mem_in) {
  div = clock >> 8;
}

void Timer::update(unsigned cpu_cycles) {
  // reset timer when DIV written
  if (div != clock >> 8) clock = 0;
  uint8_t on = read1(tac, 2), freq = tac & 0x3;
  // catch up to CPU cycles
  for (unsigned i = 0; i < cpu_cycles; ++i) {
    clock += 4, div = clock >> 8;
    if (tima_scheduled) {
      tima = tma, IF = write1(IF, 2, true);
      tima_scheduled = false;
    }
    bool bit = on && (clock >> freq_to_bit[freq]) & 0x1;
    if (!bit && last_bit) ++tima;
    tima_scheduled = (tima == 0xff), last_bit = bit;
  }
}
