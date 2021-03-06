#ifndef CPU_H
#define CPU_H

#include "memory.h"

struct Flags {
  uint8_t : 4, c : 1, h : 1, n : 1, z : 1;
};

class CPU {
private:
  // Internal State
  Memory &mem;
  unsigned cycles = 0;
  bool ime = false, ime_scheduled = false;
  bool halt = false, stop = false;

  // Registers
  union {
    struct {
      Flags f;
      uint8_t a;
    };
    uint16_t af = 0x01b0;
  };
  union {
    struct {
      uint8_t c;
      uint8_t b;
    };
    uint16_t bc = 0x0013;
  };
  union {
    struct {
      uint8_t e;
      uint8_t d;
    };
    uint16_t de = 0x00d8;
  };
  union {
    struct {
      uint8_t l;
      uint8_t h;
    };
    uint16_t hl = 0x014d;
  };
  uint16_t sp = 0xfffe, pc = 0x0100;
  uint8_t &IF = mem.refh(0x0f), &IE = mem.refh(0xff);

  // Arithmetic Functions
  uint8_t add(uint8_t a, uint8_t b);
  uint16_t add(uint16_t a, uint16_t b);
  uint16_t offset(uint16_t a, int8_t b);
  uint8_t add_carry(uint8_t a, uint8_t b);
  uint8_t subtract(uint8_t a, uint8_t b);
  uint8_t subtract_carry(uint8_t a, uint8_t b);
  uint8_t binary_and(uint8_t a, uint8_t b);
  uint8_t binary_xor(uint8_t a, uint8_t b);
  uint8_t binary_or(uint8_t a, uint8_t b);
  uint8_t increment(uint8_t a);
  uint8_t decrement(uint8_t a);

  // Rotate, Shift, & Bit Operation Functions
  uint8_t rotate_left(uint8_t a);
  uint8_t rotate_left_carry(uint8_t a);
  uint8_t rotate_right(uint8_t a);
  uint8_t rotate_right_carry(uint8_t a);
  uint8_t shift_left(uint8_t a);
  uint8_t shift_right(uint8_t a);
  uint8_t shift_right_logic(uint8_t a);
  uint8_t swap(uint8_t a);
  void bit(uint8_t n, uint8_t a);
  void check_interrupts();
  void execute_cb();

public:
  // Core Functions
  explicit CPU(Memory &mem_in) : mem(mem_in) {}
  unsigned execute();

  // Debug Functions
  void print() const;
  uint16_t get_pc() const { return pc; }
};

#endif
