#include "gameboy.h"
#include <SDL/SDL.h>
#include <emscripten.h>

// Static Tables

const std::array<uint32_t, 4> colors = {0xff0fbc9b, 0xff0fac8b, 0xff306230,
                                        0xff0f380f};
const std::map<SDL_Keycode, Input> bindings = {{SDLK_x, Input::a},
                                               {SDLK_z, Input::b},
                                               {SDLK_BACKSPACE, Input::select},
                                               {SDLK_RETURN, Input::start},
                                               {SDLK_RIGHT, Input::right},
                                               {SDLK_LEFT, Input::left},
                                               {SDLK_UP, Input::up},
                                               {SDLK_DOWN, Input::down}};

// Global State

Gameboy *gameboy;
std::vector<int16_t> audio;
SDL_Surface *screen;

// Core Functions

void audio_callback([[maybe_unused]] void *args, uint8_t *out, int bytes) {
  if (gameboy == nullptr) return;
  // copy audio buffer to SDL
  if (audio.size() < static_cast<size_t>(bytes / 2)) audio.resize(bytes / 2);
  std::copy_n(&audio[0], bytes / 2, reinterpret_cast<uint16_t *>(out));

  // move unused samples to front
  std::vector<int16_t> temp(audio.begin() + bytes / 2, audio.end());
  audio = std::move(temp);
}

void loop() {
  if (gameboy == nullptr) return;
  // handle keyboard input
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_KEYDOWN) {
      if (!bindings.count(event.key.keysym.sym)) continue;
      gameboy->input(bindings.at(event.key.keysym.sym), true);
    } else if (event.type == SDL_KEYUP) {
      if (!bindings.count(event.key.keysym.sym)) continue;
      gameboy->input(bindings.at(event.key.keysym.sym), false);
    }
  }

  // generate screen texture
  SDL_LockSurface(screen);
  const std::array<uint8_t, 160 * 144> &lcd = gameboy->get_lcd();
  uint32_t *pixels = static_cast<uint32_t *>(screen->pixels);
  for (unsigned i = 0; i < 160 * 144; ++i)
    pixels[i] = colors[lcd[i]];
  SDL_UnlockSurface(screen);

  // queue audio buffer
  gameboy->update();
  const std::vector<int16_t> &frame_audio = gameboy->read_audio();
  audio.insert(audio.end(), std::make_move_iterator(frame_audio.begin()),
               std::make_move_iterator(frame_audio.end()));
}

extern "C" void save() {
  gameboy->save("ram.sav");
}

extern "C" void load() {
  delete gameboy;
  gameboy = new Gameboy("rom.gb", "ram.sav");
}

extern "C" int main() {
  // setup SDL video
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  screen = SDL_SetVideoMode(160, 144, 32, 0);

  // setup SDL audio
  SDL_AudioSpec spec;
  memset(&spec, 0, sizeof(spec));
  spec.freq = 44100;
  spec.format = AUDIO_S16;
  spec.channels = 2;
  spec.samples = 1024;
  spec.callback = audio_callback;

  SDL_OpenAudio(&spec, nullptr);
  SDL_PauseAudio(0);

  // create main loop callback
  emscripten_set_main_loop(loop, -1, 1);
}
