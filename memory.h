#ifndef MEMORY_H
#define MEMORY_H

#include <array>
#include <map>
#include <string>

struct Range {
  uint16_t start, end;
  Range(uint16_t addr);
  Range(uint16_t start, uint16_t end);
  bool operator <(const Range &r) const;
  bool operator ==(const Range &r) const;
};

class Memory {
  private:
    // Internal State
    std::array<uint8_t, 0x10000> mem;
    std::map<Range, uint8_t> rmasks;
    std::map<Range, uint8_t> wmasks;
    uint8_t &cart_type = ref(0x147);
    uint8_t &rom_size = ref(0x148);
    uint8_t &ram_size = ref(0x149);

  public:
    // Core Functions
    Memory(const std::string &filename);
    void rmask(uint16_t addr, uint8_t mask);
    void rmask_range(uint16_t start, uint16_t end, uint8_t mask);
    void wmask(uint16_t addr, uint8_t mask);
    void wmask_range(uint16_t start, uint16_t end, uint8_t mask);
    void mask_range(uint16_t start, uint16_t end, uint8_t mask);

    // Memory Access Functions
    uint8_t& ref(uint16_t addr);
    uint8_t& refh(uint8_t addr);
    uint8_t read(uint16_t addr) const;
    uint8_t readh(uint8_t addr) const;
    uint16_t read16(uint16_t addr) const;
    uint16_t read16h(uint8_t addr) const;
    bool read1(uint16_t addr, unsigned index) const;
    bool read1h(uint8_t addr, unsigned index) const;
    void write(uint16_t addr, uint8_t val);
    void writeh(uint8_t addr, uint8_t val);
    void write16(uint16_t addr, uint16_t val);
    void write16h(uint8_t addr, uint16_t val);
    void write1(uint16_t addr, unsigned index, bool val);
    void write1h(uint8_t addr, unsigned index, bool val);
};

// Utility Functions
bool read1(unsigned num, unsigned index);
unsigned write1(unsigned num, unsigned index, bool val);

#endif
