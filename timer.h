#ifndef TIMER_H
#define TIMER_H

#include "memory.h"

class Timer {
  private:
    // Internal State
    Memory &mem;
    uint16_t clock = 0xabcc;
    bool last_bit = false, tima_scheduled = false;
    const std::array<uint8_t, 4> freq_to_bit = { 9, 3, 5, 7 };
    uint8_t &IF = mem.refh(0x0f);
    uint8_t &div = mem.refh(0x04), &tima = mem.refh(0x05);
    uint8_t &tma = mem.refh(0x06), &tac = mem.refh(0x07);
    
  public:
    // Core Functions
    Timer(Memory &mem_in);
    void update(unsigned cpu_cycles);
};

#endif
