#ifndef JOYPAD_H
#define JOYPAD_H

#include "memory.h"

class Joypad {
  private:
    // Internal State
    Memory &mem;
    static const uint8_t P1 = 0x00, IF = 0x0f;
    
  public:
    // Core Functions
    Joypad(Memory &mem_in);
    void update(unsigned cpu_cycles);
};

#endif
