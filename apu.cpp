#include <iostream>
#include "apu.h"

using namespace std;

// Static Tables

const array<uint8_t, 4> Channel::vol_codes = {4, 0, 1, 2};
const array<uint8_t, 8> Channel::noise_freqs = {4, 8, 16, 24, 32, 40, 48, 56};
const array<uint8_t, 4> Channel::duty_cycles = {0x8, 0x81, 0xe1, 0x7e};

// Channel Functions

Channel::Channel(CT type_in, Memory &mem_in): mem(mem_in), type(type_in) {
  mem.rmask(Range(addr, addr + 4), 0x0);
  mem.hook(addr + 4, [&](uint8_t val){ if (read1(val, 7)) enable(); });
  if (type == CT::wave) {
    mem.hook(addr + 1, [&](uint8_t val){ len = val; });
    mem.hook(addr + 2, [&](uint8_t val){ volume = vol_codes[(val >> 5) & 0x3]; });
  } else
    mem.hook(addr + 1, [&](uint8_t val){ len = val & 0x3f; });
}

void Channel::enable() {
  on = true, timer = 0, lsfr = 0xff;
  vol_len = nr2 & 0x7;
  if (type == CT::wave) wave_pt = 0;
  if (len == 0) len = (type != CT::wave ? 0x3f : 0xff);
  if (type != CT::wave) volume = nr2 >> 4;
  if (type == CT::square1) {
    sweep_freq = ((nr4 & 0x7) << 8) | nr3;
    sweep_len = (nr0 >> 4) & 0x7;
    sweep_on = sweep_len != 0 || (nr0 & 0x7) != 0;
  }
}

void Channel::update_frame(uint8_t frame_pt) {
  // update length counter
  if (!read1(frame_pt, 0) && read1(nr4, 6)
    && len > 0 && --len == 0) on = false;
  // update volume envelope
  if (frame_pt == 7 && type != CT::wave
      && vol_len > 0 && --vol_len == 0) {
    if (read1(nr2, 3) && volume < 0xf) ++volume;
    else if (!read1(nr2, 3) && volume > 0x0) --volume;
    vol_len = nr2 & 0x7;
  }
  // update sweep
  if ((frame_pt & 0x3) == 0x2 && type == CT::square1
      && sweep_on && sweep_len > 0 && --sweep_len == 0) {
    sweep_len = (nr0 >> 4) & 0x7;
    uint16_t freq = sweep_freq >> (nr0 & 0x7);
    if (read1(nr0, 4)) sweep_freq -= freq;
    else sweep_freq += freq;
    if (sweep_freq < 0x800) {
      nr3 = sweep_freq & 0xff;
      nr4 = (nr4 & 0xf8) | (sweep_freq >> 8);
    } else on = false;
  }
}

void Channel::update_wave() {
  // advance 1 sample in waveform
  switch (type) {
    case CT::square1: case CT::square2: {
      uint16_t freq = ((nr4 & 0x7) << 8) | nr3;
      timer = (0x800 - freq) << 1;
      wave_pt = (wave_pt + 1) & 0x7;
      uint8_t duty = nr1 >> 6;
      output = read1(duty_cycles[duty], wave_pt) * volume * on;
      break;
    } case CT::wave: {
      uint16_t freq = ((nr4 & 0x7) << 8) | nr3;
      timer = (0x800 - freq);
      wave_pt = (wave_pt + 1) & 0x1f;
      uint8_t sample = mem.refh(0x30 + (wave_pt >> 1));
      if (read1(wave_pt, 0)) sample >>= 4; else sample &= 0xf;
      output = (sample >> volume) * on;
      break;
    } case CT::noise: {
      uint8_t div_code = nr3 & 0x7;
      bool bit = read1(lsfr, 0) ^ read1(lsfr, 1);
      timer = noise_freqs[div_code] << (nr3 >> 4);
      lsfr = (bit << 14) | (lsfr >> 1);
      if (read1(nr3, 3)) lsfr = write1(lsfr, 6, bit);
      output = !read1(lsfr, 0) * volume * on;
      break;
    }
  }
}

// Core Functions

APU::APU(Memory &mem_in): mem(mem_in) {
  nr50 = 0x77, nr51 = 0xf3, nr52 = 0xf1;
}

void APU::update(unsigned cpu_cycles) {
  // update frame sequencer
  bool bit = read1(div, 5);
  if (last_bit && !bit) {
    frame_pt = (frame_pt + 1) & 0x7;
    for (Channel &channel : channels)
      channel.update_frame(frame_pt);
  }
  last_bit = bit;
  // update wave generator
  for (unsigned i = 0; i < cpu_cycles * 2; ++i) {
    for (Channel &channel : channels) {
      if (channel.timer != 0) --channel.timer;
      else channel.update_wave();
    }
    ++sample &= 0x3f;
    if (sample != 0) continue;
    uint8_t left_out = 0, right_out = 0;
    for (const Channel &channel : channels) {
      const uint8_t &ch_out = channel.get_output();
      if (read1(nr51, 4 + static_cast<uint8_t>(channel.get_type())))
        left_out += ch_out - (ch_out * left_out) / 256;
      if (read1(nr51, static_cast<uint8_t>(channel.get_type())))
        right_out += ch_out - (ch_out * right_out) / 256;
    }
    audio.push_back(left_out * ((nr50 >> 4) & 0x7) / 8);
    audio.push_back(right_out * (nr50 & 0x7) / 8);
  }
}
