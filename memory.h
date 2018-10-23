#ifndef MEMORY_H
#define MEMORY_H

#include <array>
#include <string>

class Memory {
  private:
    // Internal State
    std::array<uint8_t, 0x10000> mem;

  public:
    // Memory Access Functions
    Memory(const std::string &filename);
    uint8_t read(uint16_t addr) const;
    uint8_t read(uint8_t addr) const;
    uint16_t read16(uint16_t addr) const;
    uint16_t read16(uint8_t addr) const;
    void write(uint16_t addr, uint8_t value);
    void write(uint8_t addr, uint8_t value);
    void write16(uint16_t addr, uint16_t value);
    void write16(uint8_t addr, uint16_t value);
};

#endif
