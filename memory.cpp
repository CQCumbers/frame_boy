#include <fstream>
#include "memory.h"

using namespace std;

// Memory Access Functions

Memory::Memory(const string &filename) {
  ifstream file(filename, ios::binary);
  file.read((char *)mem.data(), mem.size());
}

uint8_t Memory::read(uint16_t addr) const {
  return mem[addr];
}

uint8_t Memory::read(uint8_t addr) const {
  return mem[0xff00 + addr];
}

uint16_t Memory::read16(uint16_t addr) const {
  return ((uint16_t)mem[addr + 1] << 8) | mem[addr];
}

uint16_t Memory::read16(uint8_t addr) const {
  return ((uint16_t)mem[0xff00 + addr + 1] << 8) | mem[0xff00 + addr];
}

void Memory::write(uint16_t addr, uint8_t value) {
  mem[addr] = value;
}

void Memory::write(uint8_t addr, uint8_t value) {
  mem[0xff00 + addr] = value;
}

void Memory::write16(uint16_t addr, uint16_t value) {
  mem[addr + 1] = (uint8_t)(value >> 8);
  mem[addr] = (uint8_t)(value & 0xff);
}

void Memory::write16(uint8_t addr, uint16_t value) {
  mem[0xff00 + addr + 1] = (uint8_t)(value >> 8);
  mem[0xff00 + addr] = (uint8_t)(value & 0xff);
}
