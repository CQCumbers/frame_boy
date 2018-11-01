#include <fstream>
#include <cassert>
#include "memory.h"

using namespace std;

// Utility Functions

bool read1(unsigned num, unsigned index) {
  return (num >> index) & 0x1;
}

unsigned write1(unsigned num, unsigned index, bool val) {
  if (val) return num | (0x1 << index);
  else return num & ~(0x1 << index);
}

// Range Functions

Range::Range(uint16_t addr):
  start(addr), end(addr) { }

Range::Range(uint16_t start, uint16_t end):
  start(start), end(end) { }

bool Range::operator <(const Range &r) const {
  return end < r.start;
}

bool Range::operator ==(const Range &r) const {
  return start >= r.start && end <= r.end;
}

// Core Functions

Memory::Memory(const string &filename) {
  // read cart into memory
  ifstream file(filename, ios::binary);
  assert(file.good());

  file.seekg(0, ios::end);
  streampos size = file.tellg();
  file.seekg(0, ios::beg);
  cart.resize(size);
  file.read(reinterpret_cast<char*>(&cart[0]), size);
  
  copy_n(&cart[0], 0x8000, &mem[0]);
  // set r/w permission bitmasks
  wmask(Range(0x0, 0x7fff), 0x0);
  mask(Range(0xa000, 0xbfff), 0x0);
  // create on-write hooks
  if (cart_type >= 1 && cart_type <= 3) { // MBC 1 
    hook(Range(0x0, 0x1fff), [&](uint8_t val) {
      if ((val & 0xf) != 0xa) wmask(Range(0xa000, 0xbfff), 0x0);
      else wmask(Range(0xa000, 0xbfff), 0xff);
    });
    hook(Range(0x2000, 0x3fff), [&](uint8_t val) {
      if ((val &= 0x1f) == 0) val |= 1;
      swap_rom(val);
    });
  }
}

void Memory::rmask(Range addr, uint8_t mask) {
  rmasks[addr] = mask;
}

void Memory::wmask(Range addr, uint8_t mask) {
  wmasks[addr] = mask;
}

void Memory::mask(Range addr, uint8_t mask) {
  rmask(addr, mask), wmask(addr, mask);
}

void Memory::hook(Range addr, std::function<void(uint8_t)> hook) {
  hooks[addr] = hook;
}

void Memory::swap_rom(unsigned bank) {
  bank &= (0x2 << rom_size) - 1;
  copy_n(&cart[bank * 0x4000], 0x4000, &mem[0x4000]);
  last_rom = bank;
}

// Memory Access Functions

// for access without masks & hooks
uint8_t& Memory::ref(uint16_t addr) {
  return mem[addr];
}

uint8_t& Memory::refh(uint8_t addr) {
  return ref(0xff00 + addr);
}

uint8_t Memory::read(uint16_t addr) const {
  if (!rmasks.count(addr)) return mem[addr];
  return mem[addr] | ~rmasks.at(addr);
}

uint8_t Memory::readh(uint8_t addr) const {
  return read(0xff00 + addr);
}

uint16_t Memory::read16(uint16_t addr) const {
  return (read(addr + 1) << 8) | read(addr);
}

uint16_t Memory::read16h(uint8_t addr) const {
  return read16(0xff00 + addr);
}

void Memory::write(uint16_t addr, uint8_t val) {
  if (hooks.count(addr)) hooks[addr](val);
  if (!wmasks.count(addr)) mem[addr] = val;
  else mem[addr] = (val & wmasks.at(addr)) | (mem[addr] & ~wmasks.at(addr));
}

void Memory::writeh(uint8_t addr, uint8_t val) {
  write(0xff00 + addr, val);
}

void Memory::write16(uint16_t addr, uint16_t val) {
  write(addr + 1, val >> 8);
  write(addr, val & 0xff);
}

void Memory::write16h(uint8_t addr, uint16_t val) {
  write16(0xff00 + addr, val);
}
