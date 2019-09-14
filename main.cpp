#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "gameboy.h"
#include <SDL2/SDL.h>
#include <iostream>

using namespace std;

// Static Tables

const array<uint32_t, 4> colors = {0xff9bbc0f, 0xff8bac0f, 0xff306230,
                                   0xff0f380f};
const map<SDL_Keycode, Input> bindings = {{SDLK_x, Input::a},
                                          {SDLK_z, Input::b},
                                          {SDLK_BACKSPACE, Input::select},
                                          {SDLK_RETURN, Input::start},
                                          {SDLK_RIGHT, Input::right},
                                          {SDLK_LEFT, Input::left},
                                          {SDLK_UP, Input::up},
                                          {SDLK_DOWN, Input::down}};

// Global State

Gameboy *gameboy;
array<uint32_t, 160 * 144> pixels;
SDL_Renderer *renderer;
SDL_Window *window;
SDL_Texture *texture;
SDL_AudioDeviceID dev;

// Core Functions

void loop() {
  if (gameboy == nullptr)
    return;
  // handle keyboard input
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_QUIT)
      exit(0);
    if (event.type == SDL_KEYDOWN) {
      if (!bindings.count(event.key.keysym.sym))
        continue;
      gameboy->input(bindings.at(event.key.keysym.sym), true);
    } else if (event.type == SDL_KEYUP) {
      if (!bindings.count(event.key.keysym.sym))
        continue;
      gameboy->input(bindings.at(event.key.keysym.sym), false);
    }
  }

  // generate screen texture
  const array<uint8_t, 160 * 144> &lcd = gameboy->get_lcd();
  for (unsigned i = 0; i < 160 * 144; ++i)
    pixels[i] = colors[lcd[i]];

  // draw screen texture
  SDL_UpdateTexture(texture, nullptr, &pixels[0], 160 * 4);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);

  // queue audio buffer
  gameboy->update();
  const vector<int16_t> &audio = gameboy->get_audio();
  SDL_QueueAudio(dev, audio.data(), 2 * audio.size());
}

void cleanup() {
  delete gameboy;
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_CloseAudioDevice(dev);
  SDL_Quit();
}

extern "C" {
void save(const char *save) { gameboy->save(save); }
}

extern "C" {
void load(const char *filename, const char *save) {
  delete gameboy;
  gameboy = new Gameboy(filename, save);
}
}

extern "C" {
int main() {
  // setup SDL video
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_CreateWindowAndRenderer(160, 144, 0, &window, &renderer);
  SDL_SetRenderDrawColor(renderer, 0x9b, 0xbc, 0x0f, 0xff);
  SDL_RenderClear(renderer);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, 160, 144);

  // setup SDL audio
  SDL_AudioSpec spec;
  SDL_zero(spec);
  spec.freq = 44100;
  spec.format = AUDIO_S16;
  spec.channels = 2;
  spec.samples = 512;
  spec.callback = nullptr;
  dev = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
  SDL_PauseAudioDevice(dev, 0);

  // call cleanup at exit
  atexit(cleanup);

  // setup main loop
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(loop, -1, 1);
#else
  SDL_SetWindowSize(window, 160 * 4, 144 * 4);
  load("roms/zelda.gb", "roms/zelda.sav");
  unsigned next_loop = SDL_GetTicks();
  while (true) {
    loop();
    next_loop += 16;
    int delay = next_loop - SDL_GetTicks();
    if (delay > 0)
      SDL_Delay(delay);
  }
#endif
}
}
