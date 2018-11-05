#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <SDL2/SDL.h>
#include <iostream>
#include "gameboy.h"

using namespace std;

array<uint32_t, 4> colors = {
  0xff9bbc0f, 0xff8bac0f, 0xff306230, 0xff0f380f
};

struct Context {
  Gameboy gameboy;
  array<uint32_t, 160*144> pixels;
  map<SDL_Keycode, Input> bindings;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  SDL_AudioDeviceID dev;
};

void loop(void *arg) {
  // read variables from context
  Context *ctx = (Context*)arg;
  Gameboy &gb = ctx->gameboy;
  array<uint32_t, 160*144> &pixels = ctx->pixels;
  map<SDL_Keycode, Input> &bindings = ctx->bindings;
  SDL_Renderer *renderer = ctx->renderer;
  SDL_Texture *texture = ctx->texture;
  SDL_AudioDeviceID dev = ctx->dev;

  // handle keyboard input
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_KEYDOWN) {
      if (!bindings.count(event.key.keysym.sym)) continue;
      gb.input(bindings.at(event.key.keysym.sym), true);
    } else if (event.type == SDL_KEYUP) {
      if (!bindings.count(event.key.keysym.sym)) continue;
      gb.input(bindings.at(event.key.keysym.sym), false);
    }
  }

  // generate screen texture
  // generate audio
  while (gb.ppu.get_mode() == 1) {
    gb.step();
    const vector<uint8_t> &audio = gb.get_audio();
    SDL_QueueAudio(dev, audio.data(), audio.size());
    gb.clear_audio();
  }
  while (gb.ppu.get_mode() != 1) {
    gb.step();
    const vector<uint8_t> &audio = gb.get_audio();
    SDL_QueueAudio(dev, audio.data(), audio.size());
    gb.clear_audio();
  }
  const array<uint8_t, 160*144> &lcd = gb.get_lcd();
  for (unsigned i = 0; i < 160*144; ++i) pixels[i] = colors[lcd[i]];

  // draw screen texture
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
  SDL_RenderClear(renderer);
  SDL_UpdateTexture(texture, nullptr, &pixels[0], 160 * 4);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

int main() {
  // setup SDL
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_CreateWindowAndRenderer(160 * 3, 144 * 3, 0, &window, &renderer);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
  SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "best");

  SDL_AudioSpec spec;
  SDL_AudioDeviceID dev;

  SDL_zero(spec);
  spec.freq = 2097152 / 64;
  spec.format = AUDIO_U8;
  spec.channels = 2;
  spec.samples = 4096;
  spec.callback = nullptr;

  dev = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
  SDL_PauseAudioDevice(dev, 0);

  SDL_Texture *texture = SDL_CreateTexture(
    renderer, SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING, 160, 144
  );

  // generate context object
  Context ctx = {
    Gameboy("roms/kirby.gb"),
    array<uint32_t, 160*144>(),
    map<SDL_Keycode, Input>(),
    renderer, texture, dev
  };

  // map SDL keys to joypad buttons
  ctx.bindings = {
    {SDLK_x, Input::a},
    {SDLK_z, Input::b},
    {SDLK_BACKSPACE, Input::select},
    {SDLK_RETURN, Input::start},
    {SDLK_RIGHT, Input::right},
    {SDLK_LEFT, Input::left},
    {SDLK_UP, Input::up},
    {SDLK_DOWN, Input::down}
  };
  
  // create main loop
  #ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(loop, &ctx, -1, 1);
  #else
  while (true) loop(&ctx);
  #endif

  // teardown SDL
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void show(const array<uint8_t, 160*144> &lcd) {
  array<string, 4> pixels = {" ", ".", "o", "M"};
  for (int j = 0; j < 160; ++j) {
    cout << "-";
  }
  cout << endl;
  for (int i = 0; i < 144; ++i) {
    for (int j = 0; j < 160; ++j) {
      cout << pixels[lcd[160 * i + j]] << flush;
    }
    cout << endl;
  }
}

int old_main() {
  // initialize hardware
  Gameboy gb("roms/zelda.gb");

  // run to breakpoint
  uint8_t &sb = gb.mem.refh(0x01);
  uint8_t &sc = gb.mem.refh(0x02);
  while (true) {
    gb.step();
    //show(gb.get_lcd());
    if (read1(sc, 7)) {
      cout << (char)sb << flush;
      sc = write1(sc, 7, false);
    }
  }

  // step one instruction
  uint8_t &lcdc = gb.mem.refh(0x40);
  uint8_t &stat = gb.mem.refh(0x41);
  uint8_t &ly = gb.mem.refh(0x44);
  uint8_t &ie = gb.mem.refh(0xff);
  while (cin.ignore()) {
    gb.print();
    cout << hex << "FF40: " << (unsigned)lcdc << " "
      << (unsigned)stat << " " << (unsigned)ly << endl;
    cout << hex << "IE: " << (unsigned)ie << endl;
    gb.step();
  }
}
