#ifndef JOYPAD_H
#define JOYPAD_H

#include "memory.h"

enum class Input { a, b, select, start, right, left, up, down };

class Joypad {
private:
  // Internal State
  Memory &mem;
  bool last_pressed = false;
  uint8_t &IF = mem.refh(0x0f), &p1 = mem.refh(0x00);
  uint8_t buttons = 0xff, directions = 0xff;

public:
  // Core Functions
  explicit Joypad(Memory &mem_in);
  void update();
  void input(Input input_enum, bool val);
};

#endif
