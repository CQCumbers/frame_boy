#include "memory.h"
#include <algorithm>
#include <string>

// Range Functions

Range::Range(uint16_t addr) : start(addr), end(addr) {}

Range::Range(uint16_t start, uint16_t end) : start(start), end(end) {}

bool Range::operator<(const Range &r) const {
  return end < r.start;
}

bool Range::operator==(const Range &r) const {
  return start >= r.start && end <= r.end;
}

// Core Functions

Memory::Memory(const std::string &filename, const std::string &save) {
  // open rom file
  FILE *file = fopen(filename.c_str(), "r");
  assert(file != nullptr);
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // resize & read rom
  rom.resize(size);
  fread(&rom[0], 1, size, file);
  std::copy_n(&rom[0], 0x8000, &mem[0]);
  rom.resize(0x8000 << rom_size);
  fclose(file);

  // resize & read ram
  std::array<unsigned, 6> ram_sizes = {0, 2, 8, 32, 128, 64};
  ram.resize(ram_sizes[ram_size] << 10);
  file = fopen(save.c_str(), "r");
  if (file != nullptr) {
    fread(&ram[0], 1, ram.size(), file);
    std::copy_n(&ram[0], 0x2000, &mem[0xa000]);
    fclose(file);
  }

  // set r/w permission bitmasks
  wmask(Range(0x0, 0x7fff), 0x0);
  mask(Range(0xa000, 0xbfff), 0x0);

  // setup memory bank controller
  Range type(cart_type);
  if (type == Range(0x1, 0x3))
    mbc = 1;
  else if (type == Range(0xf, 0x13))
    mbc = 3;
  else if (type == Range(0x19, 0x1e))
    mbc = 5;
  clock = (type == Range(0xf, 0x10));
  rumble = (type == Range(0x1c, 0x1e));
  if (mbc == 0) return;

  hook(Range(0x0, 0x1fff), [&](uint8_t val) {
    if ((val & 0xf) == 0xa) {
      if (ram.size() >= 0x2000)
        mask(Range(0xa000, 0xbfff), 0xff);
      else
        mask(Range(0xa000, 0xa000 + ram.size() - 1), 0xff);
    } else
      mask(Range(0xa000, 0xbfff), 0x0);
  });
  hook(Range(0x2000, 0x3fff), [&](uint8_t val) {
    if (mbc == 1)
      val &= 0x1f;
    else if (mbc == 3)
      val &= 0x7f;
    if (val == 0) val = 1;
    if (mbc == 1) {
      bank = (bank & 0x60) | val;
      if (ram_mode)
        swap_rom(bank & 0x1f), swap_ram(bank >> 5);
      else
        swap_rom(bank), swap_ram(0);
    } else
      swap_rom(val);
  });
  hook(Range(0x4000, 0x5fff), [&](uint8_t val) {
    if (mbc == 1) {
      val &= 0x3;
      bank = (val << 5) | (bank & 0x1f);
      if (ram_mode)
        swap_rom(bank & 0x1f), swap_ram(bank >> 5);
      else
        swap_rom(bank), swap_ram(0);
    } else {
      if (rumble) val &= 0x7;
      swap_ram(val);
    }
  });
  hook(Range(0x6000, 0x7fff), [&](uint8_t val) {
    if (mbc == 1) {
      ram_mode = read1(val, 0);
      if (ram_mode)
        swap_rom(bank & 0x1f), swap_ram(bank >> 5);
      else
        swap_rom(bank), swap_ram(0);
    }
  });
}

void Memory::rmask(Range addr, uint8_t mask) {
  rmasks[addr] = mask;
}

void Memory::wmask(Range addr, uint8_t mask) {
  wmasks[addr] = mask;
}

void Memory::mask(Range addr, uint8_t mask) {
  rmask(addr, mask);
  wmask(addr, mask);
}

void Memory::hook(Range addr, std::function<void(uint8_t)> hook) {
  hooks[addr] = hook;
}

void Memory::swap_rom(unsigned bank) {
  bank &= (0x2 << rom_size) - 1;
  std::copy_n(&rom[bank * 0x4000], 0x4000, &mem[0x4000]);
}

void Memory::swap_ram(unsigned bank) {
  bank &= (ram.size() >> 13) - 1;
  if (bank == ram_bank || ram.size() <= 0x2000) return;
  std::copy_n(&mem[0xa000], 0x2000, &ram[ram_bank * 0x2000]);
  std::copy_n(&ram[bank * 0x2000], 0x2000, &mem[0xa000]);
  ram_bank = bank;
}

void Memory::save(const std::string &save) {
  FILE *file = fopen(save.c_str(), "r");
  if (file == nullptr || mbc == 0) return;
  std::copy_n(&mem[0xa000], 0x2000, &ram[ram_bank * 0x2000]);
  fwrite(&ram[0], 1, ram.size(), file);
  fclose(file);
}

// Memory Access Functions

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

void Memory::write(uint16_t addr, uint8_t val) {
  if (hooks.count(addr)) hooks[addr](val);
  if (!wmasks.count(addr))
    mem[addr] = val;
  else
    mem[addr] = (val & wmasks.at(addr)) | (mem[addr] & ~wmasks.at(addr));
}

void Memory::writeh(uint8_t addr, uint8_t val) {
  write(0xff00 + addr, val);
}

void Memory::write16(uint16_t addr, uint16_t val) {
  write(addr + 1, val >> 8);
  write(addr, val & 0xff);
}
