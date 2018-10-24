#ifndef MEMORY_H
#define MEMORY_H

#include <array>
#include <string>

class Memory {
  private:
    // Internal State
    uint8_t mode;
    std::array<uint8_t, 0x10000> mem;

  public:
    // Core Functions
    Memory(const std::string &filename);
    void setmode(uint8_t mode_in);

    // Memory Access Functions
    uint8_t read(uint16_t addr) const;
    uint8_t read(uint8_t addr) const;
    uint16_t read16(uint16_t addr) const;
    uint16_t read16(uint8_t addr) const;
    bool readbit(uint16_t addr, unsigned index) const;
    bool readbit(uint8_t addr, unsigned index) const;
    void write(uint16_t addr, uint8_t value);
    void write(uint8_t addr, uint8_t value);
    void write16(uint16_t addr, uint16_t value);
    void write16(uint8_t addr, uint16_t value);
    void writebit(uint16_t addr, unsigned index, bool value);
    void writebit(uint8_t addr, unsigned index, bool value);
};

#endif
