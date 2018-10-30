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
  ifstream file(filename, ios::binary);
  assert(file.good());
  file.read(reinterpret_cast<char*>(mem.data()), mem.size());
  wmask_range(0x0, 0x7fff, 0x0);
}

void Memory::rmask(uint16_t addr, uint8_t mask) {
  rmasks[addr] = mask;
}

void Memory::rmask_range(uint16_t start, uint16_t end, uint8_t mask) {
  rmasks[Range(start, end)] = mask;
}

void Memory::wmask(uint16_t addr, uint8_t mask) {
  wmasks[addr] = mask;
}

void Memory::wmask_range(uint16_t start, uint16_t end, uint8_t mask) {
  wmasks[Range(start, end)] = mask;
}

void Memory::mask_range(uint16_t start, uint16_t end, uint8_t mask) {
  rmask_range(start, end, mask);
  wmask_range(start, end, mask);
}

// Memory Access Functions

// for fast r/w without masks
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

bool Memory::read1(uint16_t addr, unsigned index) const {
  return (mem[addr] >> index) & 0x1;
}

bool Memory::read1h(uint8_t addr, unsigned index) const {
  if (!rmasks.count(addr) || !read1(rmasks.at(addr), index)) {
    return read1(0xff00 + addr, index);
  }
  return true;
}

void Memory::write(uint16_t addr, uint8_t val) {
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

void Memory::write1(uint16_t addr, unsigned index, bool val) {
  if (!wmasks.count(addr) || read1(wmasks.at(addr), index)) {
    if (val) mem[addr] |= 0x1 << index;
    else mem[addr] &= ~(0x1 << index);
  }
}

void Memory::write1h(uint8_t addr, unsigned index, bool val) {
  write1(0xff00 + addr, index, val);
}
