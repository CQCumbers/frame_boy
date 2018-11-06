#ifndef TIMER_H
#define TIMER_H

#include "memory.h"

class Timer {
  private:
    // Static Tables
    static const std::array<uint8_t, 4> freq_bits;

    // Internal State
    Memory &mem;
    uint16_t clock = 0xabcc;
    bool last_bit = false, tima_scheduled = false, on = false;
    uint8_t freq_bit = 9;

    // Registers
    uint8_t &IF = mem.refh(0x0f);
    uint8_t &div = mem.refh(0x04);
    uint8_t &tima = mem.refh(0x05);
    uint8_t &tma = mem.refh(0x06);
    uint8_t &tac = mem.refh(0x07);
    
  public:
    // Core Functions
    explicit Timer(Memory &mem_in);
    void update(unsigned cpu_cycles);
};

#endif
