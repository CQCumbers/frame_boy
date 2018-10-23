#ifndef TIMER_H
#define TIMER_H

#include "memory.h"

class Timer {
  private:
    // Internal State
    Memory &mem;
    uint16_t clock;
    bool last_bit, tima_scheduled;
    const std::array<uint8_t, 4> freq_to_bit = { 9, 3, 5, 7 };
    static const uint8_t DIV = 0x04, TIMA = 0x05,
      TMA = 0x06, TAC = 0x07, IF = 0x0f;
    
  public:
    // Core Functions
    Timer(Memory &mem_in);
    void update(unsigned cpu_cycles);
};

#endif
