#ifndef APU_H
#define APU_H

#include "memory.h"

// Channel Types
enum class CT { square1, square2, wave, noise };

class Channel {
private:
  // Static Tables
  static const std::array<uint8_t, 4> vol_codes;
  static const std::array<uint8_t, 8> noise_freqs;
  static const std::array<uint8_t, 4> duty_cycles;

  // Internal State
  Memory &mem;
  const CT type;
  const uint16_t addr = 0xff10 + static_cast<uint16_t>(type) * 5;

  bool on = false, sweep_on = false;
  uint8_t wave_pt = 0, volume = 16, output = 0;
  uint16_t len = 0, vol_len = 0;
  uint16_t sweep_len = 0, sweep_freq = 0;
  uint16_t lsfr = 0;
  void enable();

  // Registers
  uint8_t &nr0 = mem.ref(addr);
  uint8_t &nr1 = mem.ref(addr + 1);
  uint8_t &nr2 = mem.ref(addr + 2);
  uint8_t &nr3 = mem.ref(addr + 3);
  uint8_t &nr4 = mem.ref(addr + 4);

public:
  // Core Functions
  uint16_t timer;
  Channel(CT type_in, Memory &mem);
  void update_frame(uint8_t frame_pt);
  void update_wave();
  const uint8_t &get_output() const { return output; }
  CT get_type() const { return type; }
};

class APU {
private:
  // Internal State
  Memory &mem;
  uint8_t sample = 0;
  uint8_t frame_pt = 0;
  bool last_bit = 0;
  std::array<Channel, 4> channels = {{
      Channel(CT::square1, mem),
      Channel(CT::square2, mem),
      Channel(CT::wave, mem),
      Channel(CT::noise, mem),
  }};
  std::vector<float> audio;
  float left_out = 0, right_out = 0;

  // Registers
  uint8_t &div = mem.refh(0x04);
  uint8_t &nr50 = mem.refh(0x24);
  uint8_t &nr51 = mem.refh(0x25);
  uint8_t &nr52 = mem.refh(0x26);

public:
  // Core Functions
  explicit APU(Memory &mem_in);
  void update(unsigned cpu_cycles);
  const std::vector<float> &get_audio() const { return audio; }
  void clear_audio() { audio.clear(); }
};

#endif
