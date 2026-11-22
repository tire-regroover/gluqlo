// Render clock digit typesetting contact sheets offscreen using src/clock_render.c.
// Outputs BMPs to dev/captures/ (convert to PNG with python3/zlib):
//   clock_digits_00_59.bmp  - all 60 minute cards (00..59)
//   clock_hours_ampm.bmp    - all 24 12-hour cards (1..12 AM & PM)
//   clock_344am.bmp         - full 1280x720 layout at 3:44 AM
#include "../../src/gluqlo.h"
#include "../../src/clock_render.h"
#include "../../src/scene_core.h"
#include <stdio.h>
#include <stdlib.h>

SDL_Renderer *renderer = NULL;
RenderTarget *screen = NULL;
TTF_Font *font_time = NULL;
TTF_Font *font_mode = NULL;
TTF_Font *font_date = NULL;
const SDL_Color FONT_COLOR = {0xd4, 0xd9, 0xe4, 0xff};
const SDL_Color DATE_COLOR = {0xd4, 0xd9, 0xe4, 0xff};
const SDL_Color BACKGROUND_COLOR = {0x14, 0x17, 0x22, 0xff};
const SDL_Color CARD_BORDER_COLOR = {0x38, 0x3e, 0x4c, 0xff};

bool twentyfourh = false;
bool leadingzero = false;
bool animate = true;
bool profile_rendering = false;
int width = 1280;
int height = 720;
int past_h = 3, past_m = 44;

void render_backdrop(RenderTarget *, Uint32) {}
void render_foreground_event(RenderTarget *, Uint32, int) {}
void render_rare_screen_fade(RenderTarget *, Uint32) {}
Uint32 scene_time(void) { return 0; }
SceneWeather get_scene_weather(Uint32) {
  SceneWeather w = {};
  return w;
}
void thickLineRGBA(RenderTarget *, int, int, int, int, int, Uint8, Uint8, Uint8, Uint8) {}
void pixelRGBA(RenderTarget *, int, int, Uint8, Uint8, Uint8, Uint8) {}

#include "../../src/clock_render.c"

static void render_card_sheet(const char *out_bmp, int cols, int rows, int card, int pad,
                              int count, bool hours_mode) {
  int W = cols * (card + pad) + pad;
  int H = rows * (card + pad) + pad;
  width = W;
  height = H;
  SDL_Surface *out = SDL_CreateSurface(W, H, SDL_PIXELFORMAT_RGBA8888);
  renderer = SDL_CreateSoftwareRenderer(out);
  RenderTarget rt = {renderer, W, H};
  screen = &rt;

  // Set up card-relative fonts & card background texture directly for `card`.
  destroy_clock_caches();
  if (font_time) TTF_CloseFont(font_time);
  if (font_mode) TTF_CloseFont(font_mode);
  const char *clock_font = resolve_clock_font_path();
  font_time = TTF_OpenFont(clock_font, (float)(card / 1.06));
  font_mode = TTF_OpenFont(clock_font, (float)(card / 9.9));

  int radius = (int)(card * 0.0952);
  SDL_Surface *bg_surf = SDL_CreateSurface(card, card, SDL_PIXELFORMAT_RGBA8888);
  SDL_Rect bg_r = {0, 0, card, card};
  fill_rounded_box_b(bg_surf, &bg_r, radius, CARD_BORDER_COLOR, 232);
  int bw = card / 120;
  if (bw < 1) bw = 1;
  SDL_Rect cr = {bw, bw, card - 2 * bw, card - 2 * bw};
  fill_rounded_box_b(bg_surf, &cr, radius > bw ? radius - bw : 1, BACKGROUND_COLOR, 210);
  bg_texture = SDL_CreateTextureFromSurface(renderer, bg_surf);
  SDL_SetTextureBlendMode(bg_texture, SDL_BLENDMODE_BLEND);
  SDL_DestroySurface(bg_surf);

  SDL_SetRenderDrawColor(renderer, 14, 16, 24, 255);
  SDL_RenderClear(renderer);

  for (int i = 0; i < count; i++) {
    int col = i % cols;
    int row = i / cols;
    SDL_Rect rect = {pad + col * (card + pad), pad + row * (card + pad), card, card};
    char buf[3];
    if (hours_mode) {
      int h = (i % 12) + 1;
      snprintf(buf, sizeof(buf), "%d", h);
    } else {
      snprintf(buf, sizeof(buf), "%02d", i);
    }
    // Clear per-card cache key so each cell renders fresh.
    card_cache_digits[0][0] = '\0';
    render_digits(screen, &rect, 0, buf, buf, 20, 19);
    if (hours_mode) render_ampm(screen, &rect, i >= 12 ? 1 : 0);
  }

  SDL_RenderPresent(renderer);
  SDL_SaveBMP(out, out_bmp);
  destroy_clock_caches();
  SDL_DestroyRenderer(renderer);
  SDL_DestroySurface(out);
  renderer = NULL;
  screen = NULL;
}

int main(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
  if (!TTF_Init()) return 1;

  render_card_sheet("dev/captures/clock_digits_00_59.bmp", 10, 6, 180, 10, 60, false);
  render_card_sheet("dev/captures/clock_hours_ampm.bmp", 6, 4, 180, 10, 24, true);

  // Full 1280x720 layout at 3:44 AM
  width = 1280;
  height = 720;
  SDL_Surface *surf = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);
  renderer = SDL_CreateSoftwareRenderer(surf);
  RenderTarget rt = {renderer, width, height};
  screen = &rt;
  layout_clock(1.0);
  SDL_SetRenderDrawColor(renderer, 16, 20, 32, 255);
  SDL_RenderClear(renderer);
  char hbuf[3] = "3";
  char mbuf[3] = "44";
  render_digits(screen, &hourBackground, 0, hbuf, hbuf, 20, 19);
  render_ampm(screen, &hourBackground, 0);
  render_digits(screen, &minBackground, 1, mbuf, mbuf, 20, 19);
  SDL_RenderPresent(renderer);
  SDL_SaveBMP(surf, "dev/captures/clock_344am.bmp");

  destroy_clock_caches();
  if (font_time) TTF_CloseFont(font_time);
  if (font_mode) TTF_CloseFont(font_mode);
  if (font_date) TTF_CloseFont(font_date);
  SDL_DestroyRenderer(renderer);
  SDL_DestroySurface(surf);
  TTF_Quit();
  SDL_Quit();
  return 0;
}
