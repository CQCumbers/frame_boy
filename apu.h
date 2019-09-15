#ifndef APU_H
#define APU_H

#include "blip_buf.h"
#include "memory.h"

// Channel Types
enum class CT { square1, square2, wave, noise };

class Channel {
private:
  // Static Tables
  static const std::array<uint8_t, 4> vol_code;
  static const std::array<uint8_t, 8> divisors;
  static const std::array<uint8_t, 4> duty_cycles;

  // Internal State
  Memory &mem;
  const CT type;
  const uint16_t addr = 0xff10 + static_cast<uint16_t>(type) * 5;

  bool on = false, sweep_on = false;
  uint8_t wave_pt = 0, vol = 16, output = 0;
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
  uint16_t timer = 0;
  int16_t last_out = 0;
  bool left_on = true, right_on = true;
  Channel(CT type_in, Memory &mem_in);
  void update_sweep();
  void update_frame(uint8_t frame_pt);
  void update_wave();
  const uint8_t &get_output() const { return output; }
  CT get_type() const { return type; }
};

class APU {
private:
  // Internal State
  Memory &mem;
  uint16_t sample = 0;
  uint8_t frame_pt = 0;
  bool last_bit = 0;
  std::array<Channel, 4> channels = {{
      Channel(CT::square1, mem),
      Channel(CT::square2, mem),
      Channel(CT::wave, mem),
      Channel(CT::noise, mem),
  }};
  uint8_t left_vol = 128, right_vol = 128;
  blip_t *left_buffer, *right_buffer;
  std::vector<int16_t> audio;

  // Registers
  uint8_t &div = mem.refh(0x04);
  uint8_t &nr50 = mem.refh(0x24);
  uint8_t &nr51 = mem.refh(0x25);
  uint8_t &nr52 = mem.refh(0x26);

public:
  // Core Functions
  explicit APU(Memory &mem_in);
  ~APU();
  void update(unsigned cpu_cycles);
  const std::vector<int16_t> &get_audio();
};

#endif
