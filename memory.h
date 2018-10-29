#ifndef MEMORY_H
#define MEMORY_H

#include <array>
#include <map>
#include <string>

class Memory {
  private:
    // Internal State
    std::array<uint8_t, 0x10000> mem;
    std::map<uint16_t, uint8_t> masks;

  public:
    // Core Functions
    Memory(const std::string &filename);
    void mask(uint16_t addr, uint8_t mask);

    // Memory Access Functions
    uint8_t& ref(uint16_t addr);
    uint8_t& refh(uint8_t addr);
    uint8_t read(uint16_t addr) const;
    uint8_t readh(uint8_t addr) const;
    uint16_t read16(uint16_t addr) const;
    uint16_t read16h(uint8_t addr) const;
    bool read1(uint16_t addr, unsigned index) const;
    bool read1h(uint8_t addr, unsigned index) const;
    void write(uint16_t addr, uint8_t value);
    void writeh(uint8_t addr, uint8_t value);
    void write16(uint16_t addr, uint16_t value);
    void write16h(uint8_t addr, uint16_t value);
    void write1(uint16_t addr, unsigned index, bool value);
    void write1h(uint8_t addr, unsigned index, bool value);
};

// Utility Functions
bool read1(unsigned num, unsigned index);
unsigned write1(unsigned num, unsigned index, bool val);

#endif
