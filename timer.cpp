#include "timer.h"

using namespace std;

// Static Tables

const array<uint8_t, 4> Timer::freq_bits = {9, 3, 5, 7};

// Core Functions

Timer::Timer(Memory &mem_in) : mem(mem_in) {
  div = clock >> 8;
  // reset timer on DIV write
  mem.wmask(0xff04, 0x0);
  mem.hook(0xff04, [&](uint8_t) { clock = 0; });
  mem.hook(0xff07, [&](uint8_t val) {
    on = read1(val, 2);
    freq_bit = freq_bits[val & 0x3];
  });
}

void Timer::update(unsigned cpu_cycles) {
  // catch up to CPU cycles
  for (unsigned i = 0; i < cpu_cycles; ++i) {
    clock += 4;
    if (tima_scheduled) {
      tima = tma;
      IF = write1(IF, 2, true);
      tima_scheduled = false;
    }
    bool bit = on && read1(clock, freq_bit);
    if (last_bit && !bit)
      ++tima;
    tima_scheduled = (tima == 0xff);
    last_bit = bit;
  }
  div = clock >> 8;
}
