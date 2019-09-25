#include "apu.h"

// Static Tables

const std::array<uint8_t, 4> Channel::vol_code = {4, 0, 1, 2};
const std::array<uint8_t, 8> Channel::divisors = {4, 8, 16, 24, 32, 40, 48, 56};
const std::array<uint8_t, 4> Channel::duty_cycles = {0x01, 0x81, 0x87, 0x7e};

// Channel Functions

Channel::Channel(CT type_in, Memory &mem_in) : mem(mem_in), type(type_in) {
  // set r/w permission bitmasks
  mem.rmask(Range(addr, addr + 4), 0x0);
  // create on-write hooks
  mem.hook(addr + 4, [&](uint8_t val) {
    if (read1(val, 7)) enable();
  });
  if (type == CT::wave) {
    mem.hook(addr, [&](uint8_t val) {
      if (!read1(val, 7)) on = false;
    });
    mem.hook(addr + 1, [&](uint8_t val) { len = 0x100 - val; });
    mem.hook(addr + 2, [&](uint8_t val) { vol = vol_code[(val >> 5) & 0x3]; });
  } else
    mem.hook(addr + 1, [&](uint8_t val) { len = 0x40 - (val & 0x3f); });
}

void Channel::enable() {
  on = true, timer = 1, lsfr = 0xff;
  vol_len = nr2 & 0x7;
  if (type == CT::wave) wave_pt = 0;
  if (len == 0) len = (type != CT::wave ? 0x3f : 0xff);
  if (type != CT::wave) vol = nr2 >> 4;
  if (type == CT::square1) {
    sweep_freq = ((nr4 & 0x7) << 8) | nr3;
    sweep_len = (nr0 >> 4) & 0x7;
    sweep_on = sweep_len != 0 || (nr0 & 0x7) != 0;
    if ((nr0 & 0x7) != 0) update_sweep();
  }
}

void Channel::update_sweep() {
  uint16_t freq = sweep_freq >> (nr0 & 0x7);
  sweep_freq += read1(nr0, 3) ? -freq : freq;
  if (sweep_freq > 0x7ff) on = false;
}

void Channel::update_frame(uint8_t frame_pt) {
  // update length counter
  if (read1(frame_pt, 0) && read1(nr4, 6) && len > 0 && --len == 0) on = false;
  // update volume envelope
  if (frame_pt == 7 && type != CT::wave && vol_len > 0 && --vol_len == 0) {
    vol = read1(nr2, 3) ? vol + (vol < 0xf) : vol - (vol > 0);
    vol_len = nr2 & 0x7;
  }
  // update sweep
  if ((frame_pt & 0x3) == 0x2 && type == CT::square1 && sweep_on &&
      sweep_len > 0 && --sweep_len == 0) {
    sweep_len = (nr0 >> 4) & 0x7;
    update_sweep();
    if (!on || (nr0 & 0x7) == 0) return;
    update_sweep();
    nr3 = sweep_freq & 0xff;
    nr4 = (nr4 & 0xf8) | (sweep_freq >> 8);
  }
}

void Channel::update_wave() {
  // advance 1 sample in waveform
  switch (type) {
  case CT::square1:
  case CT::square2: {
    uint16_t freq = ((nr4 & 0x7) << 8) | nr3;
    timer = ((0x800 - freq) << 1) + 1;
    wave_pt = (wave_pt + 1) & 0x7;
    uint8_t duty = nr1 >> 6;
    output = on * vol * read1(duty_cycles[duty], wave_pt);
    break;
  }
  case CT::wave: {
    uint16_t freq = ((nr4 & 0x7) << 8) | nr3;
    timer = (0x800 - freq) + 1;
    wave_pt = (wave_pt + 1) & 0x1f;
    uint8_t wave_s = mem.refh(0x30 + (wave_pt >> 1));
    wave_s = read1(wave_pt, 0) ? wave_s >> 4 : wave_s & 0xf;
    output = on * (wave_s >> vol);
    break;
  }
  case CT::noise: {
    uint8_t div_code = nr3 & 0x7;
    bool bit = read1(lsfr, 0) ^ read1(lsfr, 1);
    timer = (divisors[div_code] << (nr3 >> 4)) + 1;
    lsfr = write1(lsfr >> 1, 14, bit);
    if (read1(nr3, 3)) lsfr = write1(lsfr, 6, bit);
    output = on * vol * !read1(lsfr, 0);
    break;
  }
  }
}

// Core Functions

APU::APU(Memory &mem_in) : mem(mem_in) {
  // create resampling buffers
  left_buffer = blip_new(4410), right_buffer = blip_new(4410);
  blip_set_rates(left_buffer, 2097152, 44100 * 1.01);
  blip_set_rates(right_buffer, 2097152, 44100 * 1.01);
  // set initial register values
  nr50 = 0x77, nr51 = 0xf3, nr52 = 0xf1;
  // create on-write hooks
  mem.hook(0xff24, [&](uint8_t val) {
    left_vol = ((val >> 4 & 0x7) + 1) * 16;
    right_vol = ((val & 0x7) + 1) * 16;
  });
  mem.hook(0xff25, [&](uint8_t val) {
    for (unsigned i = 0; i < 4; ++i) {
      channels[i].left_on = read1(val, 4 + i);
      channels[i].right_on = read1(val, i);
    }
  });
}

APU::~APU() {
  blip_delete(left_buffer);
  blip_delete(right_buffer);
}

const std::vector<int16_t> &APU::read_audio() {
  blip_end_frame(left_buffer, sample + 1);
  blip_end_frame(right_buffer, sample + 1);
  sample = 0;

  int size = blip_samples_avail(right_buffer);
  audio.resize(size * 2);
  blip_read_samples(left_buffer, &audio[0], size, true);
  blip_read_samples(right_buffer, &audio[1], size, true);
  return audio;
}

void APU::update(unsigned cpu_cycles) {
  // update frame sequencer
  bool bit = read1(div, 4);
  if (last_bit && !bit) {
    frame_pt = (frame_pt + 1) & 0x7;
    for (Channel &channel : channels)
      channel.update_frame(frame_pt);
  }
  last_bit = bit;
  // update wave generator
  for (unsigned i = 0; i < cpu_cycles * 2; ++i) {
    if ((sample = (sample + 1) & 0x7ff) == 0) {
      if (blip_samples_avail(right_buffer) > 4310) {
        blip_clear(left_buffer);
        blip_clear(right_buffer);
      }
      blip_end_frame(left_buffer, 0x800);
      blip_end_frame(right_buffer, 0x800);
    }

    int16_t left_delta = 0, right_delta = 0;
    for (Channel &channel : channels) {
      if (--channel.timer != 0) continue;
      channel.update_wave();
      int16_t delta = channel.get_output() - channel.last_out;
      channel.last_out += delta;
      if (channel.left_on) left_delta += delta;
      if (channel.right_on) right_delta += delta;
    }

    if (left_delta != 0)
      blip_add_delta(left_buffer, sample, left_delta * left_vol);
    if (right_delta != 0)
      blip_add_delta(right_buffer, sample, right_delta * right_vol);
  }
}
