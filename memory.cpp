#include <fstream>
#include "memory.h"

using namespace std;

// Memory Access Functions

Memory::Memory(const string &filename) {
  ifstream file(filename, ios::binary);
  file.read((char *)mem.data(), mem.size());
}

void Memory::setmode(uint8_t mode_in) {
  mode = mode_in;
}

// Memory Access Functions

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

bool Memory::readbit(uint16_t addr, unsigned index) const {
  return (mem[addr] >> index) & 0x1;
}

bool Memory::readbit(uint8_t addr, unsigned index) const {
  return (mem[0xff00 + addr] >> index) & 0x1;
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

void Memory::writebit(uint16_t addr, unsigned index, bool value) {
  if (value) { mem[addr] |= 0x1 << index; }
  else { mem[addr] &= ~(0x1 << index); }
}

void Memory::writebit(uint8_t addr, unsigned index, bool value) {
  if (value) { mem[0xff00 + addr] |= 0x1 << index; }
  else { mem[0xff00 + addr] &= ~(0x1 << index); }
}
