#ifndef APU_H
#define APU_H

#include "memory.h"

// Channel Types
enum class CT {
  square1, square2, wave, noise
};

class Channel {
  private:
    // Internal State
    Memory &mem;
    bool on = false, sweep_on = false;
    uint16_t timer = 0, len = 0;
    uint8_t wave_pt = 0, volume = 0xf;
    uint16_t vol_len = 0, lsfr = 0;
    uint16_t sweep_len = 0, sweep_freq = 0;
    uint8_t waveform();
    void enable();
    
    // Registers
    uint8_t &nr0, &nr1, &nr2, &nr3, &nr4;

  public:
    // Core Functions
    CT type;
    Channel(CT type_in, Memory &mem, uint16_t addr);
    uint8_t update_cycle();
    void update_frame(uint8_t frame_pt);
};

class APU {
  private:
    // Internal State
    Memory &mem;
    uint8_t sample = 0, frame_pt = 0;
    bool last_bit = 0;
    std::array<Channel, 4> channels;
    std::vector<uint8_t> audio;

    // Registers
    uint8_t &div = mem.refh(0x04);
    uint8_t &nr50 = mem.refh(0x24);
    uint8_t &nr51 = mem.refh(0x25);
    uint8_t &nr52 = mem.refh(0x26);
  
  public:
    // Core Functions
    APU(Memory &mem_in);
    void update(unsigned cpu_cycles);
    const std::vector<uint8_t> &get_audio() const { return audio; }
    void clear_audio() { audio.clear(); }
};

#endif
