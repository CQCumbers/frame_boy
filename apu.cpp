#include "apu.h"

using namespace std;

// Channel Functions

Channel::Channel(CT type_in, Memory &mem_in, uint16_t addr):
  mem(mem_in), nr0(mem.ref(addr)), nr1(mem.ref(addr + 1)),
  nr2(mem.ref(addr + 2)), nr3(mem.ref(addr + 3)),
  nr4(mem.ref(addr + 4)), type(type_in) {
  if (type == CT::wave) mem.hook(addr + 1, [&](uint8_t val){ len = val; });
  else mem.hook(addr + 1, [&](uint8_t val){ len = val & 0x3f; });
}

uint8_t Channel::waveform() {
  if (!on) return 0x0;
  // get current volume from waveform, 0-f
  switch (type) {
    case CT::square1: case CT::square2: {
      std::array<uint8_t, 4> duty_cycles = {0x8, 0x81, 0xe1, 0x7e};
      uint8_t duty = nr1 >> 6;
      return read1(duty_cycles[duty], wave_pt) << 3;
    } case CT::wave: {
      uint8_t sample = mem.refh(0x30 + (wave_pt >> 1));
      if (read1(wave_pt, 0)) sample >>= 4; else sample &= 0xf;
      return sample;
    } case CT::noise:
      return !read1(lsfr, 0) << 2;
  }
}

void Channel::enable() {
  on = true, timer = 0;
  vol_len = nr2 & 0x7, lsfr = 0xff;
  if (vol_len == 0) vol_len = 8;
  if (type == CT::wave) wave_pt = 0;
  if (len == 0) len = (type != CT::wave ? 64 : 256);
  if (type == CT::wave) {
    array<uint8_t, 4> codes = {0x0, 0xf, 0x8, 0x4};
    volume = codes[(nr2 >> 5) & 0x3];
  } else volume = nr2 >> 4;
}

uint8_t Channel::update_cycle() {
  if (!on && read1(nr4, 7)) enable();
  if (timer == 0) {
    // advance 1 sample in waveform
    switch (type) {
      case CT::square1: case CT::square2: {
        uint16_t freq = ((nr4 & 0x7) << 8) | nr3;
        timer = (0x800 - freq) << 1;
        wave_pt = (wave_pt + 1) & 0x7;
        break;
      } case CT::wave: {
        uint16_t freq = ((nr4 & 0x7) << 8) | nr3;
        timer = (0x800 - freq);
        wave_pt = (wave_pt + 1) & 0x1f;
        break;
      } case CT::noise: {
        std::array<uint8_t, 8> noise_freqs = {4, 8, 16, 24, 32, 40, 48, 56};
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
  return waveform() << 4 * volume;
}

void Channel::update_frame(uint8_t frame_pt) {
  // update length counter
  if (!read1(frame_pt, 0) && read1(nr4, 6) && len > 0) {
    --len; if (len == 0) on = false;
  }
  // update volume envelope
  if (frame_pt == 7 && vol_len > 0 && type != CT::wave) {
    --vol_len;
    if (read1(nr2, 3) && volume < 0xf) ++volume;
    else if (!read1(nr2, 3) && volume > 0x0) --volume;
  }
}

// Core Functions

APU::APU(Memory &mem_in): mem(mem_in), channels({
  Channel(CT::square1, mem, 0xff10),
  Channel(CT::square2, mem, 0xff15),
  Channel(CT::wave, mem, 0xff1a),
  Channel(CT::noise, mem, 0xff1f),
}) { }

void APU::update(unsigned cpu_cycles) {
  // update frame sequencer
  bool bit = read1(div, 5);
  if (!bit && last_bit) {
    frame_pt = (frame_pt + 1) & 0x7;
    for (auto &channel : channels) {
      channel.update_frame(frame_pt);
    }
  }
  last_bit = bit;
  // update wave generator
  for (unsigned i = 0; i < cpu_cycles * 2; ++i) {
    uint16_t out = 0, ch_out = 0;
    for (auto &channel : channels) {
      ch_out = channel.update_cycle();
      out = ch_out + out - (ch_out * out) / 256;
    }
    sample = (sample + 1) & 0x3f;
    if (sample == 0) audio.push_back(out);
  }
}
