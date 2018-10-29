#ifndef JOYPAD_H
#define JOYPAD_H

#include "memory.h"

class Joypad {
  private:
    // Internal State
    Memory &mem;
    bool last_pressed = false;
    uint8_t &IF = mem.refh(0x0f), &p1 = mem.refh(0x00);
    
  public:
    // Input Variables
    union {
      struct { uint8_t a: 1, b: 1, select: 1, start: 1, : 4; };
      uint8_t buttons = 0xff;
    };
    union {
      struct { uint8_t right: 1, left: 1, up: 1, down: 1, : 4; };
      uint8_t directions = 0xff;
    };

    // Registers
    // Core Functions
    Joypad(Memory &mem_in);
    void update();
};

#endif
