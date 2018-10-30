#include <iostream>
#include <strings.h>
#include "cpu.h"

using namespace std;

// Arithmetic Functions

inline uint8_t CPU::add(uint8_t a, uint8_t b) {
  uint8_t res = a + b;
  f.n = false; f.z = (res == 0); f.c = res < a;
  f.h = (a & 0xf) + (b & 0xf) > 0xf;
  return res;
}

inline uint16_t CPU::add(uint16_t a, uint16_t b) {
  uint16_t res = a + b;
  f.n = false; f.c = res < a;
  f.h = (a & 0xfff) + (b & 0xfff) > 0xfff;
  return res;
}

inline uint16_t CPU::offset(uint16_t a, int8_t b) {
  f.z = f.n = false; 
  f.c = (a & 0xff) + (b & 0xff) > 0xff;
  f.h = (a & 0xf) + (b & 0xf) > 0xf;
  return a + b;
}

inline uint8_t CPU::add_carry(uint8_t a, uint8_t b) {
  uint8_t carry = f.c, res = a + b + f.c;
  f.n = false; f.z = (res == 0);
  f.c = a + b + f.c > 0xff;
  f.h = (a & 0xf) + (b & 0xf) + carry > 0xf;
  return res;
}

inline uint8_t CPU::subtract(uint8_t a, uint8_t b) {
  f.n = true; f.z = (a - b == 0); f.c = b > a;
  f.h = (uint8_t)((a & 0xf) - (b & 0xf)) > 0xf;
  return a - b;
}

inline uint8_t CPU::subtract_carry(uint8_t a, uint8_t b) {
  uint8_t carry = f.c, res = a - b - f.c;
  f.n = true; f.z = (res == 0);
  f.c = (uint16_t)(a - b - f.c) > 0xff;
  f.h = (uint8_t)((a & 0xf) - (b & 0xf) - carry) > 0xf;
  return res;
}

inline uint8_t CPU::binary_and(uint8_t a, uint8_t b) {
  f.n = f.c = false; f.h = true;
  f.z = (a & b) == 0;
  return a & b;
}

inline uint8_t CPU::binary_xor(uint8_t a, uint8_t b) {
  f.n = f.h = f.c = false;
  f.z = (a ^ b) == 0;
  return a ^ b;
}

inline uint8_t CPU::binary_or(uint8_t a, uint8_t b) {
  f.n = f.h = f.c = false;
  f.z = (a | b) == 0;
  return a | b;
}

inline uint8_t CPU::increment(uint8_t a) {
  f.n = false;
  f.z = (a == 0xff);
  f.h = (a & 0xf) == 0xf;
  return a + 1;
}

inline uint8_t CPU::decrement(uint8_t a) {
  f.n = true;
  f.z = (a == 0x1);
  f.h = (a & 0xf) == 0x0;
  return a - 1;
}

// Rotate, Shift, & Bit Operation Functions

inline uint8_t CPU::rotate_left(uint8_t a) {
  uint8_t res = (a << 1) | f.c;
  f.n = f.h = false;
  f.z = (res == 0); f.c = a >> 7;
  return res;
}

inline uint8_t CPU::rotate_left_carry(uint8_t a) {
  uint8_t res = (a << 1) | (a >> 7);
  f.n = f.h = false;
  f.z = (res == 0); f.c = a >> 7;
  return res;
}

inline uint8_t CPU::rotate_right(uint8_t a) {
  uint8_t res = (a >> 1) | (f.c << 7);
  f.n = f.h = false;
  f.z = (res == 0); f.c = a & 0x1;
  return res;
}

inline uint8_t CPU::rotate_right_carry(uint8_t a) {
  uint8_t res = (a >> 1) | (a << 7);
  f.n = f.h = false;
  f.z = (res == 0); f.c = a & 0x1;
  return res;
}

inline uint8_t CPU::shift_left(uint8_t a) {
  uint8_t res = a << 1;
  f.n = f.h = false;
  f.z = (res == 0); f.c = a >> 7;
  return res;
}

inline uint8_t CPU::shift_right(uint8_t a) {
  uint8_t res = a >> 1 | (0x1 << 7);
  f.n = f.h = false;
  f.z = (res == 0); f.c = a & 0x1;
  return res;
}

inline uint8_t CPU::shift_right_logic(uint8_t a) {
  uint8_t res = a >> 1;
  f.n = f.h = false;
  f.z = (res == 0); f.c = a & 0x1;
  return res;
}

inline uint8_t CPU::swap(uint8_t a) {
  f.n = f.h = f.c = 0;
  f.z = (a == 0);
  return (a & 0x0f) << 4 | (a & 0xf0) >> 4;
}

inline void CPU::bit(uint8_t n, uint8_t a) {
  f.n = false; f.h = true;
  f.z = !(a >> n & 0x1);
}

inline uint8_t CPU::reset(uint8_t n, uint8_t a) {
  return a & ~(0x1 << n); 
}

inline uint8_t CPU::set(uint8_t n, uint8_t a) {
  return a | (0x1 << n);
}

// Core Functions

void CPU::check_interrupts() {
  // call appropriate interrupt, lower bit priority
  uint8_t interrupt = ffs(IF & IE & 0x1f);
  if (interrupt != 0) {
    if (ime) {
      if (halt) ++cycles;
      sp -= 2; mem.write16(sp, pc);
      pc = 0x40 + 0x8 * (interrupt - 1);
      IF = write1(IF, interrupt - 1, false);
      ime = false; cycles += 5;
    }
    if (halt) halt = false;
  }
}

unsigned CPU::execute() {
  check_interrupts();
  unsigned initial_cycles = cycles;
  uint16_t u = 0x0; ++cycles;
  if (halt) return cycles - initial_cycles;
  if (ime_scheduled) { ime = true; ime_scheduled = false; }

  switch (mem.read(pc++)) {
    // 8-Bit Load & Store Instructions
    case 0x40: // LD B B
      break;
    case 0x41: // LD B C
      b = c;
      break;
    case 0x42: // LD B D
      b = d;
      break;
    case 0x43: // LD B E
      b = e;
      break;
    case 0x44: // LD B H
      b = h;
      break;
    case 0x45: // LD B L
      b = l;
      break;
    case 0x47: // LD B A
      b = a;
      break;
    case 0x48: // LD C B
      c = b;
      break;
    case 0x49: // LD C C
      break;
    case 0x4a: // LD C D
      c = d;
      break;
    case 0x4b: // LD C E
      c = e;
      break;
    case 0x4c: // LD C H
      c = h;
      break;
    case 0x4d: // LD C L
      c = l;
      break;
    case 0x4f: // LD C A
      c = a;
      break;
    case 0x50: // LD D B
      d = b;
      break;
    case 0x51: // LD D C
      d = c;
      break;
    case 0x52: // LD D D
      break;
    case 0x53: // LD D E
      d = e;
      break;
    case 0x54: // LD D H
      d = h;
      break;
    case 0x55: // LD D L
      d = l;
      break;
    case 0x57: // LD D A
      d = a;
      break;
    case 0x58: // LD E B
      e = b;
      break;
    case 0x59: // LD E C
      e = c;
      break;
    case 0x5a: // LD E D
      e = d;
      break;
    case 0x5b: // LD E E
      break;
    case 0x5c: // LD E H
      e = h;
      break;
    case 0x5d: // LD E L
      e = l;
      break;
    case 0x5f: // LD E A
      e = a;
      break;
    case 0x60: // LD H B
      h = b;
      break;
    case 0x61: // LD H C
      h = c;
      break;
    case 0x62: // LD H D
      h = d;
      break;
    case 0x63: // LD H E
      h = e;
      break;
    case 0x64: // LD H H
      break;
    case 0x65: // LD H L
      h = l;
      break;
    case 0x67: // LD H A
      h = a;
      break;
    case 0x68: // LD L B
      l = b;
      break;
    case 0x69: // LD L C
      l = c;
      break;
    case 0x6a: // LD L D
      l = d;
      break;
    case 0x6b: // LD L E
      l = e;
      break;
    case 0x6c: // LD L H
      l = h;
      break;
    case 0x6d: // LD L L
      break;
    case 0x6f: // LD L A
      l = a;
      break;
    case 0x78: // LD A B
      a = b;
      break;
    case 0x79: // LD A C
      a = c;
      break;
    case 0x7a: // LD A D
      a = d;
      break;
    case 0x7b: // LD A E
      a = e;
      break;
    case 0x7c: // LD A H
      a = h;
      break;
    case 0x7d: // LD A L
      a = l;
      break;
    case 0x7f: // LD A A
      break;
    case 0x06: // LD B n
      b = mem.read(pc++);
      ++cycles;
      break;
    case 0x0e: // LD C n
      c = mem.read(pc++);
      ++cycles;
      break;
    case 0x16: // LD D n
      d = mem.read(pc++);
      ++cycles;
      break;
    case 0x1e: // LD E n
      e = mem.read(pc++);
      ++cycles;
      break;
    case 0x26: // LD H n
      h = mem.read(pc++);
      ++cycles;
      break;
    case 0x2e: // LD L n
      l = mem.read(pc++);
      ++cycles;
      break;
    case 0x3e: // LD A n
      a = mem.read(pc++);
      ++cycles;
      break;
    case 0x70: // LD (HL) B
      mem.write(hl, b);
      ++cycles;
      break;
    case 0x71: // LD (HL) C
      mem.write(hl, c);
      ++cycles;
      break;
    case 0x72: // LD (HL) D
      mem.write(hl, d);
      ++cycles;
      break;
    case 0x73: // LD (HL) E
      mem.write(hl, e);
      ++cycles;
      break;
    case 0x74: // LD (HL) H
      mem.write(hl, h);
      ++cycles;
      break;
    case 0x75: // LD (HL) L
      mem.write(hl, l);
      ++cycles;
      break;
    case 0x77: // LD (HL) A
      mem.write(hl, a);
      ++cycles;
      break;
    case 0x36: // LD (HL) n
      mem.write(hl, mem.read(pc++));
      cycles += 2;
      break;
    case 0x46: // LD B (HL)
      b = mem.read(hl);
      ++cycles;
      break;
    case 0x4e: // LD C (HL)
      c = mem.read(hl);
      ++cycles;
      break;
    case 0x56: // LD D (HL)
      d = mem.read(hl);
      ++cycles;
      break;
    case 0x5e: // LD E (HL)
      e = mem.read(hl);
      ++cycles;
      break;
    case 0x66: // LD H (HL)
      h = mem.read(hl);
      ++cycles;
      break;
    case 0x6e: // LD L (HL)
      l = mem.read(hl);
      ++cycles;
      break;
    case 0x7e: // LD A (HL)
      a = mem.read(hl);
      ++cycles;
      break;
    case 0x02: // LD (BC) A
      mem.write(bc, a);
      ++cycles;
      break;
    case 0x12: // LD (DE) A
      mem.write(de, a);
      ++cycles;
      break;
    case 0xea: // LD (nn) A
      mem.write(mem.read16(pc), a); pc += 2; 
      cycles += 3;
      break;
    case 0xe0: // LDH (n) A
      mem.writeh(mem.read(pc++), a);
      cycles += 2;
      break;
    case 0xe2: // LD (C) A
      mem.writeh(c, a);
      ++cycles;
      break;
    case 0x0a: // LD A (BC)
      a = mem.read(bc);
      ++cycles;
      break;
    case 0x1a: // LD A (DE)
      a = mem.read(de);
      ++cycles;
      break;
    case 0xfa: // LD A (nn)
      a = mem.read(mem.read16(pc)); pc += 2;
      cycles += 3;
      break;
    case 0xf0: // LDH A (n)
      a = mem.readh(mem.read(pc++));
      cycles += 2;
      break;
    case 0xf2: // LD A (C)
      a = mem.readh(c);
      ++cycles;
      break;
    case 0x22: // LD (HL+) A
      mem.write(hl++, a);
      ++cycles;
      break;
    case 0x32: // LD (HL-) A
      mem.write(hl--, a);
      ++cycles;
      break;
    case 0x2a: // LD A (HL+)
      a = mem.read(hl++);
      ++cycles;
      break;
    case 0x3a: // LD A (HL-)
      a = mem.read(hl--);
      ++cycles;
      break;
    // 16-Bit Load & Store Instructions
    case 0x01: // LD BC nn
      bc = mem.read16(pc); pc += 2;
      cycles += 2;
      break;
    case 0x11: // LD DE nn
      de = mem.read16(pc); pc += 2;
      cycles += 2;
      break;
    case 0x21: // LD HL nn
      hl = mem.read16(pc); pc += 2;
      cycles += 2;
      break;
    case 0x31: // LD SP nn
      sp = mem.read16(pc); pc += 2;
      cycles += 2;
      break;
    case 0x08: // LD (nn) SP
      mem.write16(mem.read16(pc), sp); pc += 2;
      cycles += 4;
      break;
    case 0xf8: // LD HL SP+e
      hl = offset(sp, mem.read(pc++));
      cycles += 2;
      break;
    case 0xf9: // LD SP HL
      sp = hl;
      ++cycles;
      break;
    case 0xc1: // POP BC
      bc = mem.read16(sp); sp += 2;
      cycles += 2;
      break;
    case 0xd1: // POP DE
      de = mem.read16(sp); sp += 2;
      cycles += 2;
      break;
    case 0xe1: // POP HL
      hl = mem.read16(sp); sp += 2;
      cycles += 2;
      break;
    case 0xf1: // POP AF
      af = mem.read16(sp) & 0xfff0; sp += 2;
      cycles += 2;
      break;
    case 0xc5: // PUSH BC
      sp -= 2; mem.write16(sp, bc);
      cycles += 3;
      break;
    case 0xd5: // PUSH DE
      sp -= 2; mem.write16(sp, de);
      cycles += 3;
      break;
    case 0xe5: // PUSH HL
      sp -= 2; mem.write16(sp, hl);
      cycles += 3;
      break;
    case 0xf5: // PUSH AF
      sp -= 2; mem.write16(sp, af);
      cycles += 3;
      break;
    // 8-Bit Arithmetic Instructions
    case 0x80: // ADD B
      a = add(a, b);
      break;
    case 0x81: // ADD C
      a = add(a, c);
      break;
    case 0x82: // ADD D
      a = add(a, d);
      break;
    case 0x83: // ADD E
      a = add(a, e);
      break;
    case 0x84: // ADD H
      a = add(a, h);
      break;
    case 0x85: // ADD L
      a = add(a, l);
      break;
    case 0x87: // ADD A
      a = add(a, a);
      break;
    case 0x86: // ADD (HL)
      a = add(a, mem.read(hl));
      ++cycles;
      break;
    case 0xc6: // ADD n
      a = add(a, mem.read(pc++));
      ++cycles;
      break;
    case 0x88: // ADC B
      a = add_carry(a, b);
      break;
    case 0x89: // ADC C
      a = add_carry(a, c);
      break;
    case 0x8a: // ADC D
      a = add_carry(a, d);
      break;
    case 0x8b: // ADC E
      a = add_carry(a, e);
      break;
    case 0x8c: // ADC H
      a = add_carry(a, h);
      break;
    case 0x8d: // ADC L
      a = add_carry(a, l);
      break;
    case 0x8f: // ADC A
      a = add_carry(a, a);
      break;
    case 0x8e: // ADC (HL)
      a = add_carry(a, mem.read(hl));
      ++cycles;
      break;
    case 0xce: // ADC n
      a = add_carry(a, mem.read(pc++));
      ++cycles;
      break;
    case 0x90: // SUB B
      a = subtract(a, b);
      break;
    case 0x91: // SUB C
      a = subtract(a, c);
      break;
    case 0x92: // SUB D
      a = subtract(a, d);
      break;
    case 0x93: // SUB E
      a = subtract(a, e);
      break;
    case 0x94: // SUB H
      a = subtract(a, h);
      break;
    case 0x95: // SUB L
      a = subtract(a, l);
      break;
    case 0x97: // SUB A
      a = subtract(a, a);
      break;
    case 0x96: // SUB (HL)
      a = subtract(a, mem.read(hl));
      ++cycles;
      break;
    case 0xd6: // SUB n
      a = subtract(a, mem.read(pc++));
      ++cycles;
      break;
    case 0x98: // SBC B
      a = subtract_carry(a, b);
      break;
    case 0x99: // SBC C
      a = subtract_carry(a, c);
      break;
    case 0x9a: // SBC D
      a = subtract_carry(a, d);
      break;
    case 0x9b: // SBC E
      a = subtract_carry(a, e);
      break;
    case 0x9c: // SBC H
      a = subtract_carry(a, h);
      break;
    case 0x9d: // SBC L
      a = subtract_carry(a, l);
      break;
    case 0x9f: // SBC A
      a = subtract_carry(a, a);
      break;
    case 0x9e: // SBC (HL)
      a = subtract_carry(a, mem.read(hl));
      ++cycles;
      break;
    case 0xde: // SBC n
      a = subtract_carry(a, mem.read(pc++));
      ++cycles;
      break;
    case 0xa0: // AND B
      a = binary_and(a, b);
      break;
    case 0xa1: // AND C
      a = binary_and(a, c);
      break;
    case 0xa2: // AND D
      a = binary_and(a, d);
      break;
    case 0xa3: // AND E
      a = binary_and(a, e);
      break;
    case 0xa4: // AND H
      a = binary_and(a, h);
      break;
    case 0xa5: // AND L
      a = binary_and(a, l);
      break;
    case 0xa7: // AND A
      a = binary_and(a, a);
      break;
    case 0xa6: // AND (HL)
      a = binary_and(a, mem.read(hl));
      ++cycles;
      break;
    case 0xe6: // AND n
      a = binary_and(a, mem.read(pc++));
      ++cycles;
      break;
    case 0xa8: // XOR B 
      a = binary_xor(a, b);
      break;
    case 0xa9: // XOR C 
      a = binary_xor(a, c);
      break;
    case 0xaa: // XOR D 
      a = binary_xor(a, d);
      break;
    case 0xab: // XOR E 
      a = binary_xor(a, e);
      break;
    case 0xac: // XOR H 
      a = binary_xor(a, h);
      break;
    case 0xad: // XOR L 
      a = binary_xor(a, l);
      break;
    case 0xaf: // XOR A 
      a = binary_xor(a, a);
      break;
    case 0xae: // XOR (HL)
      a = binary_xor(a, mem.read(hl));
      ++cycles;
      break;
    case 0xee: // XOR n
      a = binary_xor(a, mem.read(pc++));
      ++cycles;
      break;
    case 0xb0: // OR B
      a = binary_or(a, b);
      break;
    case 0xb1: // OR C
      a = binary_or(a, c);
      break;
    case 0xb2: // OR D
      a = binary_or(a, d);
      break;
    case 0xb3: // OR E
      a = binary_or(a, e);
      break;
    case 0xb4: // OR H
      a = binary_or(a, h);
      break;
    case 0xb5: // OR L
      a = binary_or(a, l);
      break;
    case 0xb7: // OR A
      a = binary_or(a, a);
      break;
    case 0xb6: // OR (HL)
      a = binary_or(a, mem.read(hl));
      ++cycles;
      break;
    case 0xf6: // OR n
      a = binary_or(a, mem.read(pc++));
      ++cycles;
      break;
    case 0xb8: // CP B
      subtract(a, b);
      break;
    case 0xb9: // CP C
      subtract(a, c);
      break;
    case 0xba: // CP D
      subtract(a, d);
      break;
    case 0xbb: // CP E
      subtract(a, e);
      break;
    case 0xbc: // CP H
      subtract(a, h);
      break;
    case 0xbd: // CP L
      subtract(a, l);
      break;
    case 0xbf: // CP A
      subtract(a, a);
      break;
    case 0xbe: // CP (HL)
      subtract(a, mem.read(hl));
      ++cycles;
      break;
    case 0xfe: // CP n
      subtract(a, mem.read(pc++));
      ++cycles;
      break;
    case 0x04: // INC B
      b = increment(b);
      break;
    case 0x0c: // INC C
      c = increment(c);
      break;
    case 0x14: // INC D
      d = increment(d);
      break;
    case 0x1c: // INC E
      e = increment(e);
      break;
    case 0x24: // INC H
      h = increment(h);
      break;
    case 0x2c: // INC L
      l = increment(l);
      break;
    case 0x3c: // INC A
      a = increment(a);
      break;
    case 0x34: // INC (HL)
      mem.write(hl, increment(mem.read(hl)));
      cycles += 2;
      break;
    case 0x05: // DEC B
      b = decrement(b);
      break;
    case 0x0d: // DEC C
      c = decrement(c);
      break;
    case 0x15: // DEC D
      d = decrement(d);
      break;
    case 0x1d: // DEC E
      e = decrement(e);
      break;
    case 0x25: // DEC H
      h = decrement(h);
      break;
    case 0x2d: // DEC L
      l = decrement(l);
      break;
    case 0x3d: // DEC A
      a = decrement(a);
      break;
    case 0x35: // DEC (HL)
      mem.write(hl, decrement(mem.read(hl)));
      cycles += 2;
      break;
    case 0x27: // DAA
      // see https://www.reddit.com/r/EmuDev/comments/4ycoix
      if (f.h || (!f.n && (a & 0xf) > 0x9)) u = 0x6;
      if (f.c || (!f.n && a > 0x99)) { u |= 0x60; f.c = true; }
      a = f.n ? a - u : a + u;
      f.h = false; f.z = (a == 0);
      break;
    case 0x2f: // CPL
      f.n = f.h = true; a = ~a;
      break;
    case 0x37: // SCF
      f.n = f.h = false; f.c = true;
      break;
    case 0x3f: // CCF
      f.n = f.h = false; f.c = !f.c;
      break;
    // 16-Bit Arithmetic Instructions
    case 0x09: // ADD HL BC
      hl = add(hl, bc);
      ++cycles;
      break;
    case 0x19: // ADD HL DE
      hl = add(hl, de);
      ++cycles;
      break;
    case 0x29: // ADD HL HL
      hl = add(hl, hl);
      ++cycles;
      break;
    case 0x39: // ADD HL SP
      hl = add(hl, sp);
      ++cycles;
      break;
    case 0xe8: // ADD SP e
      sp = offset(sp, mem.read(pc++));
      cycles += 3;
      break;
    case 0x03: // INC BC
      ++bc; ++cycles;
      break;
    case 0x13: // INC DE
      ++de; ++cycles;
      break;
    case 0x23: // INC HL
      ++hl; ++cycles;
      break;
    case 0x33: // INC SP
      ++sp; ++cycles;
      break;
    case 0x0b: // DEC BC
      --bc; ++cycles;
      break;
    case 0x1b: // DEC DE
      --de; ++cycles;
      break;
    case 0x2b: // DEC HL
      --hl; ++cycles;
      break;
    case 0x3b: // DEC SP
      --sp; ++cycles;
      break;
    // Rotate, Shift, & Bit Operation Instructions
    case 0x07: // RLCA
      a = rotate_left_carry(a); f.z = false;
      break;
    case 0x0f: // RRCA
      a = rotate_right_carry(a); f.z = false;
      break;
    case 0x17: // RLA
      a = rotate_left(a); f.z = false;
      break;
    case 0x1f: // RRA
      a = rotate_right(a); f.z = false;
      break;
    case 0xcb: // CB op
      execute_cb();
      break;
    // Control Flow Instructions
    case 0xc3: // JP nn
      pc = mem.read16(pc);
      cycles += 3;
      break;
    case 0xe9: // JP HL
      pc = hl;
      break;
    case 0xc2: // JP NZ nn
      if (!f.z) { pc = mem.read16(pc); cycles += 3; }
      else { pc += 2; cycles += 2; }
      break;
    case 0xd2: // JP NC nn
      if (!f.c) { pc = mem.read16(pc); cycles += 3; }
      else { pc += 2; cycles += 2; }
      break;
    case 0xca: // JP Z nn
      if (f.z) { pc = mem.read16(pc); cycles += 3; }
      else { pc += 2; cycles += 2; }
      break;
    case 0xda: // JP C nn
      if (f.c) { pc = mem.read16(pc); cycles += 3; }
      else { pc += 2; cycles += 2; }
      break;
    case 0x18: // JR r
      pc += (int8_t)mem.read(pc);
      ++pc; cycles += 2;
      break;
    case 0x20: // JR NZ r
      if (!f.z) {
        pc += (int8_t)mem.read(pc);
        ++pc; cycles += 2;
      } else { ++pc; ++cycles; }
      break;
    case 0x30: // JR NC r
      if (!f.c) {
        pc += (int8_t)mem.read(pc);
        ++pc; cycles += 2;
      } else { ++pc; ++cycles; }
      break;
    case 0x28: // JR Z r
      if (f.z) {
        pc += (int8_t)mem.read(pc);
        ++pc; cycles += 2;
      } else { ++pc; ++cycles; }
      break;
    case 0x38: // JR C r
      if (f.c) {
        pc += (int8_t)mem.read(pc); 
        ++pc; cycles += 2; 
      } else { ++pc; ++cycles; }
      break;
    case 0xcd: // CALL nn
      sp -= 2; mem.write16(sp, pc + 2);
      pc = mem.read16(pc);
      cycles += 5;
      break;
    case 0xc4: // CALL NZ nn
      if (!f.z) { 
        sp -= 2; mem.write16(sp, pc + 2);
        pc = mem.read16(pc); cycles += 5;
      } else { pc += 2; cycles += 2; }
      break;
    case 0xd4: // CALL NC nn
      if (!f.c) { 
        sp -= 2; mem.write16(sp, pc + 2);
        pc = mem.read16(pc); cycles += 5;
      } else { pc += 2; cycles += 2; }
      break;
    case 0xcc: // CALL Z nn
      if (f.z) { 
        sp -= 2; mem.write16(sp, pc + 2);
        pc = mem.read16(pc); cycles += 5;
      } else { pc += 2; cycles += 2; }
      break;
    case 0xdc: // CALL C nn
      if (f.c) { 
        sp -= 2; mem.write16(sp, pc + 2);
        pc = mem.read16(pc); cycles += 5;
      } else { pc += 2; cycles += 2; }
      break;
    case 0xc9: // RET
      pc = mem.read16(sp); sp += 2;
      cycles += 3;
      break;
    case 0xc0: // RET NZ
      if (!f.z) {
        pc = mem.read16(sp); sp += 2;
        cycles += 4;
      } else { ++cycles; }
      break;
    case 0xd0: // RET NC
      if (!f.c) {
        pc = mem.read16(sp); sp += 2;
        cycles += 4;
      } else { ++cycles; }
      break;
    case 0xc8: // RET Z
      if (f.z) {
        pc = mem.read16(sp); sp += 2;
        cycles += 4;
      } else { ++cycles; }
      break;
    case 0xd8: // RET C
      if (f.c) {
        pc = mem.read16(sp); sp += 2;
        cycles += 4;
      } else { ++cycles; }
      break;
    case 0xd9: // RETI
      pc = mem.read16(sp); sp += 2;
      ime = true; cycles += 3;
      break;
    case 0xc7: // RST 0x00
      sp -= 2; mem.write16(sp, pc);
      pc = 0x00; cycles += 3;
      break;
    case 0xcf: // RST 0x08
      sp -= 2; mem.write16(sp, pc);
      pc = 0x08; cycles += 3;
      break;
    case 0xd7: // RST 0x10
      sp -= 2; mem.write16(sp, pc);
      pc = 0x10; cycles += 3;
      break;
    case 0xdf: // RST 0x18
      sp -= 2; mem.write16(sp, pc);
      pc = 0x18; cycles += 3;
      break;
    case 0xe7: // RST 0x20
      sp -= 2; mem.write16(sp, pc);
      pc = 0x20; cycles += 3;
      break;
    case 0xef: // RST 0x28
      sp -= 2; mem.write16(sp, pc);
      pc = 0x28; cycles += 3;
      break;
    case 0xf7: // RST 0x30
      sp -= 2; mem.write16(sp, pc);
      pc = 0x30; cycles += 3;
      break;
    case 0xff: // RST 0x38
      sp -= 2; mem.write16(sp, pc);
      pc = 0x38; cycles += 3;
      break;
    // Miscellaneous Instructions
    case 0x76: // HALT
      halt = true;
      break;
    case 0x10: // STOP
      stop = true; ++pc;
      break;
    case 0xf3: // DI
      ime = false;
      break;
    case 0xfb: // EI
      ime_scheduled = true;
      break;
    case 0x00: // NOP
      break;
    default:
      cout << "Unimplemented opcode " << hex
        << (unsigned)mem.read(--pc) << endl;
  }

  return cycles - initial_cycles;
}

void CPU::execute_cb() {
  ++cycles;
  switch(mem.read(pc++)) {
    case 0x00: // RLC B
      b = rotate_left_carry(b);
      break;
    case 0x01: // RLC C
      c = rotate_left_carry(c);
      break;
    case 0x02: // RLC D
      d = rotate_left_carry(d);
      break;
    case 0x03: // RLC E
      e = rotate_left_carry(e);
      break;
    case 0x04: // RLC H
      h = rotate_left_carry(h);
      break;
    case 0x05: // RLC L
      l = rotate_left_carry(l);
      break;
    case 0x06: // RLC (HL)
      mem.write(hl, rotate_left_carry(mem.read(hl)));
      cycles += 2;
      break;
    case 0x07: // RLC A
      a = rotate_left_carry(a);
      break;
    case 0x08: // RRC B
      b = rotate_right_carry(b);
      break;
    case 0x09: // RRC C
      c = rotate_right_carry(c);
      break;
    case 0x0a: // RRC D
      d = rotate_right_carry(d);
      break;
    case 0x0b: // RRC E
      e = rotate_right_carry(e);
      break;
    case 0x0c: // RRC H
      h = rotate_right_carry(h);
      break;
    case 0x0d: // RRC L
      l = rotate_right_carry(l);
      break;
    case 0x0e: // RRC (HL)
      mem.write(hl, rotate_right_carry(mem.read(hl)));
      cycles += 2;
      break;
    case 0x0f: // RRC A
      a = rotate_right_carry(a);
      break;
    case 0x10: // RL B
      b = rotate_left(b);
      break;
    case 0x11: // RL C
      c = rotate_left(c);
      break;
    case 0x12: // RL D
      d = rotate_left(d);
      break;
    case 0x13: // RL E
      e = rotate_left(e);
      break;
    case 0x14: // RL H
      h = rotate_left(h);
      break;
    case 0x15: // RL L
      l = rotate_left(l);
      break;
    case 0x16: // RL (HL)
      mem.write(hl, rotate_left(mem.read(hl)));
      cycles += 2;
      break;
    case 0x17: // RL A
      a = rotate_left(a);
      break;
    case 0x18: // RR B
      b = rotate_right(b);
      break;
    case 0x19: // RR C
      c = rotate_right(c);
      break;
    case 0x1a: // RR D
      d = rotate_right(d);
      break;
    case 0x1b: // RR E
      e = rotate_right(e);
      break;
    case 0x1c: // RR H
      h = rotate_right(h);
      break;
    case 0x1d: // RR L
      l = rotate_right(l);
      break;
    case 0x1e: // RR (HL)
      mem.write(hl, rotate_right(mem.read(hl)));
      cycles += 2;
      break;
    case 0x1f: // RR A
      a = rotate_right(a);
      break;
    case 0x20: // SLA B
      b = shift_left(b);
      break;
    case 0x21: // SLA C
      c = shift_left(c);
      break;
    case 0x22: // SLA D
      d = shift_left(d);
      break;
    case 0x23: // SLA E
      e = shift_left(e);
      break;
    case 0x24: // SLA H
      h = shift_left(h);
      break;
    case 0x25: // SLA L
      l = shift_left(l);
      break;
    case 0x26: // SLA (HL)
      mem.write(hl, shift_left(mem.read(hl)));
      cycles += 2;
      break;
    case 0x27: // SLA A
      a = shift_left(a);
      break;
    case 0x28: // SRA B
      b = shift_right(b);
      break;
    case 0x29: // SRA C
      c = shift_right(c);
      break;
    case 0x2a: // SRA D
      d = shift_right(d);
      break;
    case 0x2b: // SRA E
      e = shift_right(e);
      break;
    case 0x2c: // SRA H
      h = shift_right(h);
      break;
    case 0x2d: // SRA L
      l = shift_right(l);
      break;
    case 0x2e: // SRA (HL)
      mem.write(hl, shift_right(mem.read(hl)));
      cycles += 2;
      break;
    case 0x2f: // SRA A
      a = shift_right(a);
      break;
    case 0x30: // SWAP B
      b = swap(b);
      break;
    case 0x31: // SWAP C
      c = swap(c);
      break;
    case 0x32: // SWAP D
      d = swap(d);
      break;
    case 0x33: // SWAP E
      e = swap(e);
      break;
    case 0x34: // SWAP H
      h = swap(h);
      break;
    case 0x35: // SWAP L
      l = swap(l);
      break;
    case 0x36: // SWAP (HL)
      mem.write(hl, swap(mem.read(hl)));
      cycles += 2;
      break;
    case 0x37: // SWAP A
      a = swap(a);
      break;
    case 0x38: // SRL B
      b = shift_right_logic(b);
      break;
    case 0x39: // SRL C
      c = shift_right_logic(c);
      break;
    case 0x3a: // SRL D
      d = shift_right_logic(d);
      break;
    case 0x3b: // SRL E
      e = shift_right_logic(e);
      break;
    case 0x3c: // SRL H
      h = shift_right_logic(h);
      break;
    case 0x3d: // SRL L
      l = shift_right_logic(l);
      break;
    case 0x3e: // SRL (HL)
      mem.write(hl, shift_right_logic(mem.read(hl)));
      cycles += 2;
      break;
    case 0x3f: // SRL A
      a = shift_right_logic(a);
      break;
    case 0x40: // BIT 0 B
      bit(0, b);
      break;
    case 0x41: // BIT 0 C
      bit(0, c);
      break;
    case 0x42: // BIT 0 D
      bit(0, d);
      break;
    case 0x43: // BIT 0 E
      bit(0, e);
      break;
    case 0x44: // BIT 0 H
      bit(0, h);
      break;
    case 0x45: // BIT 0 L
      bit(0, l);
      break;
    case 0x46: // BIT 0 (HL)
      bit(0, mem.read(hl));
      ++cycles;
      break;
    case 0x47: // BIT 0 A
      bit(0, a);
      break;
    case 0x48: // BIT 1 B
      bit(1, b);
      break;
    case 0x49: // BIT 1 C
      bit(1, c);
      break;
    case 0x4a: // BIT 1 D
      bit(1, d);
      break;
    case 0x4b: // BIT 1 E
      bit(1, e);
      break;
    case 0x4c: // BIT 1 H
      bit(1, h);
      break;
    case 0x4d: // BIT 1 L
      bit(1, l);
      break;
    case 0x4e: // BIT 1 (HL)
      bit(1, mem.read(hl));
      ++cycles;
      break;
    case 0x4f: // BIT 1 A
      bit(1, a);
      break;
    case 0x50: // BIT 2 B
      bit(2, b);
      break;
    case 0x51: // BIT 2 C
      bit(2, c);
      break;
    case 0x52: // BIT 2 D
      bit(2, d);
      break;
    case 0x53: // BIT 2 E
      bit(2, e);
      break;
    case 0x54: // BIT 2 H
      bit(2, h);
      break;
    case 0x55: // BIT 2 L
      bit(2, l);
      break;
    case 0x56: // BIT 2 (HL)
      bit(2, mem.read(hl));
      ++cycles;
      break;
    case 0x57: // BIT 0 A
      bit(2, a);
      break;
    case 0x58: // BIT 3 B
      bit(3, b);
      break;
    case 0x59: // BIT 3 C
      bit(3, c);
      break;
    case 0x5a: // BIT 3 D
      bit(3, d);
      break;
    case 0x5b: // BIT 3 E
      bit(3, e);
      break;
    case 0x5c: // BIT 3 H
      bit(3, h);
      break;
    case 0x5d: // BIT 3 L
      bit(3, l);
      break;
    case 0x5e: // BIT 3 (HL)
      bit(3, mem.read(hl));
      ++cycles;
      break;
    case 0x5f: // BIT 3 A
      bit(3, a);
      break;
    case 0x60: // BIT 4 B
      bit(4, b);
      break;
    case 0x61: // BIT 4 C
      bit(4, c);
      break;
    case 0x62: // BIT 4 D
      bit(4, d);
      break;
    case 0x63: // BIT 4 E
      bit(4, e);
      break;
    case 0x64: // BIT 4 H
      bit(4, h);
      break;
    case 0x65: // BIT 4 L
      bit(4, l);
      break;
    case 0x66: // BIT 4 (HL)
      bit(4, mem.read(hl));
      ++cycles;
      break;
    case 0x67: // BIT 4 A
      bit(4, a);
      break;
    case 0x68: // BIT 5 B
      bit(5, b);
      break;
    case 0x69: // BIT 5 C
      bit(5, c);
      break;
    case 0x6a: // BIT 5 D
      bit(5, d);
      break;
    case 0x6b: // BIT 5 E
      bit(5, e);
      break;
    case 0x6c: // BIT 5 H
      bit(5, h);
      break;
    case 0x6d: // BIT 5 L
      bit(5, l);
      break;
    case 0x6e: // BIT 5 (HL)
      bit(5, mem.read(hl));
      ++cycles;
      break;
    case 0x6f: // BIT 5 A
      bit(5, a);
      break;
    case 0x70: // BIT 6 B
      bit(6, b);
      break;
    case 0x71: // BIT 6 C
      bit(6, c);
      break;
    case 0x72: // BIT 6 D
      bit(6, d);
      break;
    case 0x73: // BIT 6 E
      bit(6, e);
      break;
    case 0x74: // BIT 6 H
      bit(6, h);
      break;
    case 0x75: // BIT 6 L
      bit(6, l);
      break;
    case 0x76: // BIT 6 (HL)
      bit(6, mem.read(hl));
      ++cycles;
      break;
    case 0x77: // BIT 6 A
      bit(6, a);
      break;
    case 0x78: // BIT 7 B
      bit(7, b);
      break;
    case 0x79: // BIT 7 C
      bit(7, c);
      break;
    case 0x7a: // BIT 7 D
      bit(7, d);
      break;
    case 0x7b: // BIT 7 E
      bit(7, e);
      break;
    case 0x7c: // BIT 7 H
      bit(7, h);
      break;
    case 0x7d: // BIT 7 L
      bit(7, l);
      break;
    case 0x7e: // BIT 7 (HL)
      bit(7, mem.read(hl));
      ++cycles;
      break;
    case 0x7f: // BIT 7 A
      bit(7, a);
      break;
    case 0x80: // RES 0 B
      b = reset(0, b);
      break;
    case 0x81: // RES 0 C
      c = reset(0, c);
      break;
    case 0x82: // RES 0 D
      d = reset(0, d);
      break;
    case 0x83: // RES 0 E
      e = reset(0, e);
      break;
    case 0x84: // RES 0 H
      h = reset(0, h);
      break;
    case 0x85: // RES 0 L
      l = reset(0, l);
      break;
    case 0x86: // RES 0 (HL)
      mem.write(hl, reset(0, mem.read(hl)));
      cycles += 2;
      break;
    case 0x87: // RES 0 A
      a = reset(0, a);
      break;
    case 0x88: // RES 1 B
      b = reset(1, b);
      break;
    case 0x89: // RES 1 C
      c = reset(1, c);
      break;
    case 0x8a: // RES 1 D
      d = reset(1, d);
      break;
    case 0x8b: // RES 1 E
      e = reset(1, e);
      break;
    case 0x8c: // RES 1 H
      h = reset(1, h);
      break;
    case 0x8d: // RES 1 L
      l = reset(1, l);
      break;
    case 0x8e: // RES 1 (HL)
      mem.write(hl, reset(1, mem.read(hl)));
      cycles += 2;
      break;
    case 0x8f: // RES 1 A
      a = reset(1, a);
      break;
    case 0x90: // RES 2 B
      b = reset(2, b);
      break;
    case 0x91: // RES 2 C
      c = reset(2, c);
      break;
    case 0x92: // RES 2 D
      d = reset(2, d);
      break;
    case 0x93: // RES 2 E
      e = reset(2, e);
      break;
    case 0x94: // RES 2 H
      h = reset(2, h);
      break;
    case 0x95: // RES 2 L
      l = reset(2, l);
      break;
    case 0x96: // RES 2 (HL)
      mem.write(hl, reset(2, mem.read(hl)));
      cycles += 2;
      break;
    case 0x97: // RES 0 A
      a = reset(2, a);
      break;
    case 0x98: // RES 3 B
      b = reset(3, b);
      break;
    case 0x99: // RES 3 C
      c = reset(3, c);
      break;
    case 0x9a: // RES 3 D
      d = reset(3, d);
      break;
    case 0x9b: // RES 3 E
      e = reset(3, e);
      break;
    case 0x9c: // RES 3 H
      h = reset(3, h);
      break;
    case 0x9d: // RES 3 L
      l = reset(3, l);
      break;
    case 0x9e: // RES 3 (HL)
      mem.write(hl, reset(3, mem.read(hl)));
      cycles += 2;
      break;
    case 0x9f: // RES 3 A
      a = reset(3, a);
      break;
    case 0xa0: // RES 4 B
      b = reset(4, b);
      break;
    case 0xa1: // RES 4 C
      c = reset(4, c);
      break;
    case 0xa2: // RES 4 D
      d = reset(4, d);
      break;
    case 0xa3: // RES 4 E
      e = reset(4, e);
      break;
    case 0xa4: // RES 4 H
      h = reset(4, h);
      break;
    case 0xa5: // RES 4 L
      l = reset(4, l);
      break;
    case 0xa6: // RES 4 (HL)
      mem.write(hl, reset(4, mem.read(hl)));
      cycles += 2;
      break;
    case 0xa7: // RES 4 A
      a = reset(4, a);
      break;
    case 0xa8: // RES 5 B
      b = reset(5, b);
      break;
    case 0xa9: // RES 5 C
      c = reset(5, c);
      break;
    case 0xaa: // RES 5 D
      d = reset(5, d);
      break;
    case 0xab: // RES 5 E
      e = reset(5, e);
      break;
    case 0xac: // RES 5 H
      h = reset(5, h);
      break;
    case 0xad: // RES 5 L
      l = reset(5, l);
      break;
    case 0xae: // RES 5 (HL)
      mem.write(hl, reset(5, mem.read(hl)));
      cycles += 2;
      break;
    case 0xaf: // RES 5 A
      a = reset(5, a);
      break;
    case 0xb0: // RES 6 B
      b = reset(6, b);
      break;
    case 0xb1: // RES 6 C
      c = reset(6, c);
      break;
    case 0xb2: // RES 6 D
      d = reset(6, d);
      break;
    case 0xb3: // RES 6 E
      e = reset(6, e);
      break;
    case 0xb4: // RES 6 H
      h = reset(6, h);
      break;
    case 0xb5: // RES 6 L
      l = reset(6, l);
      break;
    case 0xb6: // RES 6 (HL)
      mem.write(hl, reset(6, mem.read(hl)));
      cycles += 2;
      break;
    case 0xb7: // RES 6 A
      a = reset(6, a);
      break;
    case 0xb8: // RES 7 B
      b = reset(7, b);
      break;
    case 0xb9: // RES 7 C
      c = reset(7, c);
      break;
    case 0xba: // RES 7 D
      d = reset(7, d);
      break;
    case 0xbb: // RES 7 E
      e = reset(7, e);
      break;
    case 0xbc: // RES 7 H
      h = reset(7, h);
      break;
    case 0xbd: // RES 7 L
      l = reset(7, l);
      break;
    case 0xbe: // RES 7 (HL)
      mem.write(hl, reset(7, mem.read(hl)));
      cycles += 2;
      break;
    case 0xbf: // RES 7 A
      a = reset(7, a);
      break;
    case 0xc0: // SET 0 B
      b = set(0, b);
      break;
    case 0xc1: // SET 0 C
      c = set(0, c);
      break;
    case 0xc2: // SET 0 D
      d = set(0, d);
      break;
    case 0xc3: // SET 0 E
      e = set(0, e);
      break;
    case 0xc4: // SET 0 H
      h = set(0, h);
      break;
    case 0xc5: // SET 0 L
      l = set(0, l);
      break;
    case 0xc6: // SET 0 (HL)
      mem.write(hl, set(0, mem.read(hl)));
      cycles += 2;
      break;
    case 0xc7: // SET 0 A
      a = set(0, a);
      break;
    case 0xc8: // SET 1 B
      b = set(1, b);
      break;
    case 0xc9: // SET 1 C
      c = set(1, c);
      break;
    case 0xca: // SET 1 D
      d = set(1, d);
      break;
    case 0xcb: // SET 1 E
      e = set(1, e);
      break;
    case 0xcc: // SET 1 H
      h = set(1, h);
      break;
    case 0xcd: // SET 1 L
      l = set(1, l);
      break;
    case 0xce: // SET 1 (HL)
      mem.write(hl, set(1, mem.read(hl)));
      cycles += 2;
      break;
    case 0xcf: // SET 1 A
      a = set(1, a);
      break;
    case 0xd0: // SET 2 B
      b = set(2, b);
      break;
    case 0xd1: // SET 2 C
      c = set(2, c);
      break;
    case 0xd2: // SET 2 D
      d = set(2, d);
      break;
    case 0xd3: // SET 2 E
      e = set(2, e);
      break;
    case 0xd4: // SET 2 H
      h = set(2, h);
      break;
    case 0xd5: // SET 2 L
      l = set(2, l);
      break;
    case 0xd6: // SET 2 (HL)
      mem.write(hl, set(2, mem.read(hl)));
      cycles += 2;
      break;
    case 0xd7: // SET 0 A
      a = set(2, a);
      break;
    case 0xd8: // SET 3 B
      b = set(3, b);
      break;
    case 0xd9: // SET 3 C
      c = set(3, c);
      break;
    case 0xda: // SET 3 D
      d = set(3, d);
      break;
    case 0xdb: // SET 3 E
      e = set(3, e);
      break;
    case 0xdc: // SET 3 H
      h = set(3, h);
      break;
    case 0xdd: // SET 3 L
      l = set(3, l);
      break;
    case 0xde: // SET 3 (HL)
      mem.write(hl, set(3, mem.read(hl)));
      cycles += 2;
      break;
    case 0xdf: // SET 3 A
      a = set(3, a);
      break;
    case 0xe0: // SET 4 B
      b = set(4, b);
      break;
    case 0xe1: // SET 4 C
      c = set(4, c);
      break;
    case 0xe2: // SET 4 D
      d = set(4, d);
      break;
    case 0xe3: // SET 4 E
      e = set(4, e);
      break;
    case 0xe4: // SET 4 H
      h = set(4, h);
      break;
    case 0xe5: // SET 4 L
      l = set(4, l);
      break;
    case 0xe6: // SET 4 (HL)
      mem.write(hl, set(4, mem.read(hl)));
      cycles += 2;
      break;
    case 0xe7: // SET 4 A
      a = set(4, a);
      break;
    case 0xe8: // SET 5 B
      b = set(5, b);
      break;
    case 0xe9: // SET 5 C
      c = set(5, c);
      break;
    case 0xea: // SET 5 D
      d = set(5, d);
      break;
    case 0xeb: // SET 5 E
      e = set(5, e);
      break;
    case 0xec: // SET 5 H
      h = set(5, h);
      break;
    case 0xed: // SET 5 L
      l = set(5, l);
      break;
    case 0xee: // SET 5 (HL)
      mem.write(hl, set(5, mem.read(hl)));
      cycles += 2;
      break;
    case 0xef: // SET 5 A
      a = set(5, a);
      break;
    case 0xf0: // SET 6 B
      b = set(6, b);
      break;
    case 0xf1: // SET 6 C
      c = set(6, c);
      break;
    case 0xf2: // SET 6 D
      d = set(6, d);
      break;
    case 0xf3: // SET 6 E
      e = set(6, e);
      break;
    case 0xf4: // SET 6 H
      h = set(6, h);
      break;
    case 0xf5: // SET 6 L
      l = set(6, l);
      break;
    case 0xf6: // SET 6 (HL)
      mem.write(hl, set(6, mem.read(hl)));
      cycles += 2;
      break;
    case 0xf7: // SET 6 A
      a = set(6, a);
      break;
    case 0xf8: // SET 7 B
      b = set(7, b);
      break;
    case 0xf9: // SET 7 C
      c = set(7, c);
      break;
    case 0xfa: // SET 7 D
      d = set(7, d);
      break;
    case 0xfb: // SET 7 E
      e = set(7, e);
      break;
    case 0xfc: // SET 7 H
      h = set(7, h);
      break;
    case 0xfd: // SET 7 L
      l = set(7, l);
      break;
    case 0xfe: // SET 7 (HL)
      mem.write(hl, set(7, mem.read(hl)));
      cycles += 2;
      break;
    case 0xff: // SET 7 A
      a = set(7, a);
      break;
  }
}

// Debug Functions

void CPU::print() const {
  cout << "PC: " << hex << pc << "\n"
    << "op: " << hex << (unsigned)mem.read(pc) << "\n"
    << " F: "
    << (f.z ? "Z " : " ")
    << (f.n ? "N " : " ")
    << (f.h ? "H " : " ")
    << (f.c ? "C " : " ") << "\n"
    << "AF: " << hex << af << "\n"
    << "BC: " << hex << bc << "\n"
    << "DE: " << hex << de << "\n"
    << "HL: " << hex << hl << "\n"
    << "SP: " << hex << sp << "\n";
}

uint16_t CPU::get_pc() const {
  return pc;
}
