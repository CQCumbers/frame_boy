#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <SDL2/SDL.h>
#endif

#include <iostream>
#include "gameboy.h"

using namespace std;

#ifdef __EMSCRIPTEN__
struct Context {
  Gameboy gameboy;
  array<uint8_t, 160*144*4> pixels;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
};

void loop(void *arg) {
  Context *ctx = (Context*)arg;
  Gameboy &gameboy = ctx->gameboy;
  SDL_Renderer *renderer = ctx->renderer;
  SDL_Texture *texture = ctx->texture;

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
  SDL_RenderClear(renderer);

  array<uint8_t, 160*144*4> &pixels = ctx->pixels;
  const array<uint8_t, 160*144> &lcd = gameboy.update();

  for (unsigned i = 0; i < 160 * 144; ++i) {
    for (unsigned j = 0; j < 3; ++j) {
      pixels[(i << 2) | j] = 0xff - lcd[i] * 0x55;
    }
    pixels[(i << 2) | 3] = 0xff;
  }

  SDL_UpdateTexture(texture, nullptr, &pixels[0], 160 * 4);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

int main() {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_CreateWindowAndRenderer(160 * 3, 144 * 3, 0, &window, &renderer);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);

  SDL_Texture *texture = SDL_CreateTexture(
    renderer, SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING, 160, 144
  );

  Context ctx = {
    Gameboy("roms/cpu_instrs.gb"),
    array<uint8_t, 160*144*4>(),
    renderer, texture
  };
  emscripten_set_main_loop_arg(loop, &ctx, -1, 1);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

#else
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

int main() {
  // initialize hardware
  Gameboy gb("roms/dr-mario.gb");

  // run to breakpoint
  uint8_t SB = 0x01, SC = 0x02;
  while (true) {
    show(gb.update());
    if (gb.mem.read1h(SC, 7)) {
      gb.mem.write1h(SC, 7, false);
      cout << (char)gb.mem.readh(SB) << flush;
    }
  }

  // step one instruction
  uint8_t LCDC = 0x40, STAT = 0x41, LY = 0x44, IE = 0xff;
  while (cin.ignore()) {
    gb.print();
    cout << hex << "FF40: " << (unsigned)gb.mem.read(LCDC) << " "
      << (unsigned)gb.mem.read(STAT) << " " << (unsigned)gb.mem.read(LY) << endl;
    cout << hex << "IE: " << (unsigned)gb.mem.read(IE) << endl;
    gb.step();
  }
}

#endif
