#include "joypad.h"

using namespace std;

// Core Functions

Joypad::Joypad(Memory &mem_in): mem(mem_in) {
  p1 = 0xcf;
  mem.wmask(0xff00, 0x30);
}

void Joypad::update() {
  // set P1 using CPU output
  uint8_t read_buttons = (read1(p1, 5) ? 0xff : buttons);
  uint8_t read_directions = (read1(p1, 4) ? 0xff : directions);
  p1 = (p1 & 0xf0) | (read_buttons & read_directions & 0xf);
  // check joypad interrupt
  bool pressed = (read_buttons & read_directions) != 0xf;
  if (!last_pressed && pressed) IF = write1(IF, 4, true);
  last_pressed = pressed;
}

void Joypad::input(Input input_enum, bool val) {
  unsigned index = static_cast<unsigned>(input_enum);
  // joypad inputs are pulled up
  if (index < 4) buttons = write1(buttons, index, !val);
  else directions = write1(directions, index - 4, !val);
}
