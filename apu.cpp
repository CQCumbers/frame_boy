#include "apu.h"

using namespace std;

// Channel Functions

Channel::Channel(CT type_in, Memory &mem_in, uint16_t addr):
  mem(mem_in), nr0(mem.ref(addr)), nr1(mem.ref(addr + 1)),
  nr2(mem.ref(addr + 2)), nr3(mem.ref(addr + 3)),
  nr4(mem.ref(addr + 4)), type(type_in) { }

uint8_t Channel::waveform() {
  if (!on) return false;
  // get current volume from waveform, 0-8
  switch (type) {
    case CT::square1: case CT::square2: {
      std::array<uint8_t, 4> duty_cycles = {0x1, 0x81, 0x87, 0x7e};
      uint8_t duty = nr1 >> 6;
      return read1(duty_cycles[duty], wave_pt) << 3;
    } case CT::wave: {
      uint8_t sample = mem.refh(0x30 + (wave_pt >> 1));
      if (read1(wave_pt, 0)) sample >>= 4; else sample &= 0xf;
      return sample;
    } case CT::noise:
      return !read1(lsfr, 0) << 3;
  }
}

uint8_t Channel::update() {
  if (timer == 0) {
    // advance 1 sample in waveform
    switch (type) {
      case CT::square1: case CT::square2: {
        uint16_t freq = ((nr4 & 0x7) << 8) | nr3;
        timer = (0x800 - freq) << 2;
        wave_pt = (wave_pt + 1) & 0x7;
        break;
      } case CT::wave: {
        uint16_t freq = ((nr4 & 0x7) << 8) | nr3;
        timer = (0x800 - freq) << 1;
        wave_pt = (wave_pt + 1) & 0x1f;
        break;
      } case CT::noise: {
        std::array<uint8_t, 8> noise_freqs = {
          8, 16, 32, 48, 64, 80, 96, 112
        };
        uint8_t div_code = nr3 & 0x7;
        bool bit = read1(lsfr, 0) ^ read1(lsfr, 1);
        timer = noise_freqs[div_code];
        lsfr = (bit << 14) | (lsfr >> 1);
        if (read1(nr3, 3)) lsfr = write1(lsfr, 6, bit);
        break;
      }
    }
  }
  --timer;
  return waveform() << 5;
}

// Core Functions

APU::APU(Memory &mem_in): mem(mem_in), channels({
  Channel(CT::square1, mem, 0xff10),
  Channel(CT::square2, mem, 0xff15),
  Channel(CT::wave, mem, 0xff1a),
  Channel(CT::noise, mem, 0xff1f),
}) { }

void APU::update(unsigned cpu_cycles) {
  for (unsigned i = 0; i < cpu_cycles; ++i) {
    sample = (sample + 1) & 0x7;
    for (Channel channel : channels) {
      uint8_t out = channel.update();
      if (sample == 0) audio.push_back(out);
    }
  }
}

/*void Channel::read() {
  switch (type) { // don't keep all these variables, read from refs when necessary
    case square1:
      sweep_period = (nr0 >> 4) & 0x7;
      negate = read1(nr0, 3);
      shift = nr0 & 0x7;
      [[fallthrough]];
    case square2:
      duty = nr1 >> 6;
      length_load = nr1 & 0x3f;
      volume = nr2 >> 4;
      add_mode = read1(nr2, 3);
      period = nr2 & 0x7;
      frequency = ((nr4 & 0x7) << 8) | nr3;
      trigger = read1(nr4, 7);
      length_enable = read1(nr4, 6);
      break;
    case wave:
      length_load = nr1;
      volume = (nr2 >> 5) & 0x3;
      frequency = ((nr4 & 0x7) << 8) | nr3;
      trigger = read1(nr4, 7);
      length_enable = read1(nr4, 6);
      break;
    case noise:
      length_load = nr1 & 0x3f;
      volume = nr2 >> 4;
      add_mode = read1(nr2, 3);
      period = nr2 & 0x7;
      clock_shift = nr3 >> 4;
      width_mode = read1(nr3, 3);
      divisor_code = */
/*
 * {
      // apply frame sequencer changes
      bool bit = read1(div, 5);
      if (!bit && last_bit) {
        frame_seq = (frame_seq + 1) & 0x7;
        for (Channel channel : channels) {
          if (!read1(frame_seq, 0)) --channel.len; // calc length
          if (frame_seq == 7) channel.volume = // calc volume
          if ((frame_seq & 0x3) == 0x2) channel.sweep // clock sweep
        }
      }
      last_bit = bit;
      for (unsigned i = 0; i < cpu_cycles; ++i) {
        for (Channel channel : channels) {
          channel.timer = (channel.timer - 1) % freq;
          if (timer == 0) {
            channel.seq = (channel.seq + 1) & 0x7;
            channel.output = channel.waveform[channel.seq] * channel.volume;
          }
        }
      }
*/
