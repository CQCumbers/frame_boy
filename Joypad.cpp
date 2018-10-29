#include "joypad.h"

using namespace std;

// Core Functions

Joypad::Joypad(Memory &mem_in): mem(mem_in) {
  p1 = 0xcf;
  mem.mask(0xff00, 0x30);
}

void Joypad::update() {
  // set P1 using CPU output
  uint8_t read_buttons = (read1(p1, 5) ? 0xff : buttons);
  uint8_t read_directions = (read1(p1, 4) ? 0xff : directions);
  p1 = (p1 & 0xf0) | (read_buttons & read_directions);
  // check joypad interrupt
  bool pressed = (read_buttons & read_directions) != 0xf;
  if (!last_pressed && pressed) IF = write1(IF, 4, true);
  last_pressed = pressed;
}
