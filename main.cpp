#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "gameboy.h"
#include <SDL2/SDL.h>
#include <iostream>

using namespace std;

array<uint32_t, 4> colors = {0xff9bbc0f, 0xff8bac0f, 0xff306230, 0xff0f380f};

struct Context {
  Gameboy gameboy;
  array<uint32_t, 160 * 144> pixels;
  map<SDL_Keycode, Input> bindings;
  SDL_Renderer *renderer;
  SDL_Window *window;
  SDL_Texture *texture;
  SDL_AudioDeviceID dev;
};

Context *gctx = nullptr;

void loop(void *arg) {
  // read variables from context
  Context *ctx = static_cast<Context *>(arg);
  Gameboy &gb = ctx->gameboy;
  array<uint32_t, 160 * 144> &pixels = ctx->pixels;
  map<SDL_Keycode, Input> &bindings = ctx->bindings;
  SDL_Renderer *renderer = ctx->renderer;
  SDL_Texture *texture = ctx->texture;
  SDL_AudioDeviceID dev = ctx->dev;

  // handle keyboard input
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_KEYDOWN) {
      if (!bindings.count(event.key.keysym.sym))
        continue;
      gb.input(bindings.at(event.key.keysym.sym), true);
    } else if (event.type == SDL_KEYUP) {
      if (!bindings.count(event.key.keysym.sym))
        continue;
      gb.input(bindings.at(event.key.keysym.sym), false);
    }
  }

  // generate audio buffer
  gb.update();
  const vector<float> &audio = gb.get_audio();
  SDL_QueueAudio(dev, audio.data(), 4 * static_cast<unsigned>(audio.size()));
  gb.clear_audio();

  // generate screen texture
  const array<uint8_t, 160 * 144> &lcd = gb.get_lcd();
  for (unsigned i = 0; i < 160 * 144; ++i)
    pixels[i] = colors[lcd[i]];

  // draw screen texture
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
  SDL_RenderClear(renderer);
  SDL_UpdateTexture(texture, nullptr, &pixels[0], 160 * 4);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

extern "C" {
void save() {
  Gameboy &gb = gctx->gameboy;
  gb.save("ram.sav");
}
}

extern "C" {
void play() {
  if (gctx) {
    // teardown SDL
    SDL_DestroyRenderer(gctx->renderer);
    SDL_DestroyWindow(gctx->window);
    SDL_CloseAudioDevice(gctx->dev);
    SDL_Quit();
    gctx = nullptr;
  }

  // setup SDL
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_CreateWindowAndRenderer(160 * 4, 144 * 4, 0, &window, &renderer);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);

  SDL_AudioSpec spec;
  SDL_AudioDeviceID dev;

  SDL_zero(spec);
  spec.freq = 44100;
  spec.format = AUDIO_F32;
  spec.channels = 2;
  spec.samples = 4096;
  spec.callback = nullptr;

  dev = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
  SDL_PauseAudioDevice(dev, 0);

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, 160, 144);

  // generate context object
  Context ctx = {Gameboy("rom.gb", "ram.sav"),
                 array<uint32_t, 160 * 144>(),
                 map<SDL_Keycode, Input>(),
                 renderer,
                 window,
                 texture,
                 dev};
  gctx = &ctx;

  // map SDL keys to joypad buttons
  ctx.bindings = {{SDLK_x, Input::a},
                  {SDLK_z, Input::b},
                  {SDLK_BACKSPACE, Input::select},
                  {SDLK_RETURN, Input::start},
                  {SDLK_RIGHT, Input::right},
                  {SDLK_LEFT, Input::left},
                  {SDLK_UP, Input::up},
                  {SDLK_DOWN, Input::down}};

  // create main loop
#ifdef __EMSCRIPTEN__
  emscripten_cancel_main_loop();
  emscripten_set_main_loop_arg(loop, &ctx, -1, 1);
#else
  while (true)
    loop(&ctx);
#endif
}
}

int main() { play(); }
