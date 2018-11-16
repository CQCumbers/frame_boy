#ifndef MEMORY_H
#define MEMORY_H

#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>

class Range {
private:
  // Internal State
  uint16_t start, end;

public:
  // Core Functions
  Range(uint16_t addr);
  Range(uint16_t start, uint16_t end);
  bool operator<(const Range &r) const;
  bool operator==(const Range &r) const;
};

class Memory {
private:
  // Internal State
  std::vector<uint8_t> rom, ram;
  std::array<uint8_t, 0x10000> mem;
  std::map<Range, uint8_t> rmasks, wmasks;
  std::map<Range, std::function<void(uint8_t)>> hooks;
  uint8_t &cart_type = ref(0x147);
  uint8_t &rom_size = ref(0x148);
  uint8_t &ram_size = ref(0x149);
  unsigned ram_bank = 0;

  // MBC State
  uint8_t mbc = 0;
  bool clock = false;
  bool rumble = false;
  bool ram_mode = false;
  unsigned bank = 0;
  void swap_rom(unsigned bank);
  void swap_ram(unsigned bank);

public:
  // Core Functions
  explicit Memory(const std::string &filename, const std::string &save);
  void rmask(Range addr, uint8_t mask);
  void wmask(Range addr, uint8_t mask);
  void mask(Range addr, uint8_t mask);
  void hook(Range addr, std::function<void(uint8_t)> hook);
  void save(const std::string &save);

  // Memory Access Functions
  uint8_t &ref(uint16_t addr);
  uint8_t &refh(uint8_t addr);
  uint8_t read(uint16_t addr) const;
  uint8_t readh(uint8_t addr) const;
  uint16_t read16(uint16_t addr) const;
  uint16_t read16h(uint8_t addr) const;
  void write(uint16_t addr, uint8_t val);
  void writeh(uint8_t addr, uint8_t val);
  void write16(uint16_t addr, uint16_t val);
  void write16h(uint8_t addr, uint16_t val);
};

// Utility Functions

inline bool read1(unsigned num, unsigned index) { return (num >> index) & 0x1; }

inline unsigned write1(unsigned num, unsigned index, bool val) {
  if (val)
    return num | (0x1 << index);
  else
    return num & ~(0x1 << index);
}

inline uint8_t reset(uint8_t n, uint8_t a) { return a & ~(0x1 << n); }

inline uint8_t set(uint8_t n, uint8_t a) { return a | (0x1 << n); }

#endif
