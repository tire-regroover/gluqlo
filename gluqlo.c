/*
 * Gluqlo L-Space: a flip clock that lives inside a night landscape
 * https://github.com/tire-regroover/gluqlo (branch l-space)
 *
 * L-Space is a fork of Gluqlo (Fliqlo for Linux) by Alexander Kovalenko,
 * https://github.com/alexanderk23/gluqlo, which is itself based on noflipqlo by
 * Jacek Kuźniarski. The flip cards, the flip animation and the licence come
 * from those projects. What is new in L-Space is everything behind and around
 * the cards: a procedural backdrop with seasons and weather, and hundreds of
 * scheduled scenery events (things that cross the sky, happen on the hills, or
 * take over the whole screen) including ship battles with nine weapon types.
 * See README.md and NOTES.md for the details and for how to add an event.
 *
 * Copyright (c) 2010-2012 Kuźniarski Jacek
 * Copyright (c) 2014 Alexander Kovalenko
 * Copyright (c) 2026 tire-regroover (L-Space scenery, weather and events)
 *
 * L-Space was written with the help of AI coding assistants: GitHub Copilot
 * using agents from Anthropic Claude, Google Gemini, OpenAI GPT, Hangzhou
 * DeepSeek and Alibaba Qwen. Their output was directed and reviewed by
 * tire-regroover.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <errno.h>
#include <fontconfig/fontconfig.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef FONT
#define FONT "gluqlo.ttf"
#endif

static char resolved_clock_font[1024] = "";
static char resolved_date_font[1024] = "";

const char *TITLE = "Gluqlo L-Space";
const int DEFAULT_WIDTH = 1024;
const int DEFAULT_HEIGHT = 768;

bool twentyfourh = true;
bool leadingzero = false;
bool fullscreen = false;
bool animate = true;
bool anykeyclose = false;

int past_h = -1, past_m = -1, past_yday = -1;
int burnin_x = 0, burnin_y = 0;
int date_y = 0;
static SDL_Texture *date_texture_cache = NULL;
static int date_texture_w = 0, date_texture_h = 0;
static int date_surface_yday = -1;

int width = DEFAULT_WIDTH;
int height = DEFAULT_HEIGHT;
static const Uint32 AMBIENT_FRAME_INTERVAL = 100;
static const int MAX_RAIN_DROPS = 1200;
static bool profile_rendering = false;

TTF_Font *font_time = NULL;
TTF_Font *font_mode = NULL;
TTF_Font *font_date = NULL;

const SDL_Color FONT_COLOR = {0xda, 0xda, 0xe2, 255};
const SDL_Color DATE_COLOR = {0xc6, 0xc2, 0xcf, 255};
const SDL_Color BACKGROUND_COLOR = {0x12, 0x14, 0x1d, 255};
const SDL_Color CARD_BORDER_COLOR = {0x37, 0x3d, 0x4e, 255};

// SDL3 RenderTarget abstraction: keeps existing draw functions clean and lets
// both the main GPU renderer and offscreen software harnesses share one path.
typedef struct {
  SDL_Renderer *renderer;
  int w;
  int h;
} RenderTarget;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static RenderTarget screen_target = {NULL, DEFAULT_WIDTH, DEFAULT_HEIGHT};
static RenderTarget *screen = &screen_target;
static Uint32 user_event_type = 0;

SDL_Rect hourBackground;
SDL_Rect minBackground;

SDL_Rect bgrect;
static SDL_Texture *bg_texture = NULL;
static SDL_Texture *backdrop_base = NULL;
static SDL_Texture *ampm_textures[2] = {NULL, NULL};
static int ampm_w[2] = {0, 0}, ampm_h[2] = {0, 0};

// Cached digit glyph textures ('0'..'9') cropped to their ink bounding box
// for fast GPU-accelerated card & flip rendering with accurate centering.
static SDL_Texture *digit_glyph_tex[10] = {NULL};
static int digit_glyph_w[10] = {0};
static int digit_glyph_h[10] = {0};
static int digit_glyph_y_off[10] = {0};

// Pre-rendered static card textures (hour, minute) + scratch flip card texture.
static SDL_Texture *card_cache_tex[2] = {NULL, NULL};
static char card_cache_digits[2][3] = {"", ""};
static SDL_Texture *flip_card_tex = NULL;


#include "src/gfx_primitives.c"

// draw rounded box onto an SDL_Surface (used once at startup for the card
// texture) see
// http://lists.libsdl.org/pipermail/sdl-libsdl.org/2006-December/058868.html
void fill_rounded_box_b(SDL_Surface *dst, SDL_Rect *coords, int r, SDL_Color color, Uint8 alpha) {
  const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(dst->format);
  Uint32 pixcolor = SDL_MapRGBA(fmt, NULL, color.r, color.g, color.b, alpha);

  int i, j;
  int rpsqrt2 = (int)(r / sqrt(2));
  int yd = dst->pitch / fmt->bytes_per_pixel;
  int w = coords->w / 2 - 1;
  int h = coords->h / 2 - 1;
  int xo = coords->x + w;
  int yo = coords->y + h;

  w -= r;
  h -= r;

  if (w <= 0 || h <= 0) return;

  SDL_LockSurface(dst);
  Uint32 *pixels = (Uint32 *)(dst->pixels);

  int sy = (yo - h) * yd;
  int ey = (yo + h) * yd;
  int sx = xo - w;
  int ex = xo + w;

  for (i = sy; i <= ey; i += yd)
    for (j = sx - r; j <= ex + r; j++) pixels[i + j] = pixcolor;

  int d = -r;
  int x2m1 = -1;
  int y = r;

  for (int x = 0; x <= rpsqrt2; x++) {
    x2m1 += 2;
    d += x2m1;
    if (d >= 0) {
      y--;
      d -= y * 2;
    }

    for (i = sx - x; i <= ex + x; i++) {
      pixels[sy - y * yd + i] = pixcolor;
    }

    for (i = sx - y; i <= ex + y; i++) {
      pixels[sy - x * yd + i] = pixcolor;
    }

    for (i = sx - y; i <= ex + y; i++) {
      pixels[ey + x * yd + i] = pixcolor;
    }

    for (i = sx - x; i <= ex + x; i++) {
      pixels[ey + y * yd + i] = pixcolor;
    }
  }

  SDL_UnlockSurface(dst);
}


void render_ampm(RenderTarget *surface, SDL_Rect *rect, int pm) {
  int idx = pm ? 1 : 0;
  if (!ampm_textures[idx]) {
    char mode[3];
    snprintf(mode, 3, "%cM", pm ? 'P' : 'A');
    SDL_Surface *ampm = TTF_RenderText_Blended(font_mode, mode, 0, FONT_COLOR);
    if (!ampm) return;
    ampm_w[idx] = ampm->w;
    ampm_h[idx] = ampm->h;
    ampm_textures[idx] = SDL_CreateTextureFromSurface(surface->renderer, ampm);
    SDL_DestroySurface(ampm);
    if (!ampm_textures[idx]) return;
  }
  int offset = (int)(rect->h * 0.127);
  SDL_FRect dst = {(float)(rect->x + rect->h * 0.07),
                   (float)(rect->y + (pm ? rect->h - offset - ampm_h[idx] : offset)),
                   (float)ampm_w[idx], (float)ampm_h[idx]};
  SDL_RenderTexture(surface->renderer, ampm_textures[idx], NULL, &dst);
}


static double hash_noise(int x, int y, Uint32 seed) {
  Uint32 value = (Uint32)x * 374761393u + (Uint32)y * 668265263u + seed * 2246822519u;
  value = (value ^ (value >> 13)) * 1274126177u;
  value ^= value >> 16;
  return value / 4294967295.0;
}


static double smooth_noise(double x, double y, Uint32 seed) {
  int x0 = (int)floor(x);
  int y0 = (int)floor(y);
  double fx = x - x0;
  double fy = y - y0;
  fx = fx * fx * (3.0 - 2.0 * fx);
  fy = fy * fy * (3.0 - 2.0 * fy);
  double top = hash_noise(x0, y0, seed) * (1.0 - fx) + hash_noise(x0 + 1, y0, seed) * fx;
  double bottom = hash_noise(x0, y0 + 1, seed) * (1.0 - fx) + hash_noise(x0 + 1, y0 + 1, seed) * fx;
  return top * (1.0 - fy) + bottom * fy;
}


static Uint8 blend_channel(Uint8 from, Uint8 to, double amount) {
  if (amount < 0.0) amount = 0.0;
  if (amount > 1.0) amount = 1.0;
  return (Uint8)(from + (to - from) * amount);
}


static double smooth_step(double edge0, double edge1, double value) {
  double t = (value - edge0) / (edge1 - edge0);
  if (t < 0.0) t = 0.0;
  if (t > 1.0) t = 1.0;
  return t * t * (3.0 - 2.0 * t);
}


// Height of the three ground ridges as a fraction of the screen height at
// horizontal position nx (0..1). Effects use it to sit things on the ground.
enum { RIDGE_FAR, RIDGE_MIDDLE, RIDGE_NEAR };

static double ridge_height(int layer, double nx) {
  switch (layer) {
    case RIDGE_FAR:
      return 0.68 + (smooth_noise(nx * 3.6, 1.0, 907) - 0.5) * 0.13;
    case RIDGE_MIDDLE:
      return 0.79 + (smooth_noise(nx * 5.2 + 8.0, 2.0, 1013) - 0.5) * 0.10;
    default:
      return 0.88 + (smooth_noise(nx * 8.0 - 3.0, 3.0, 1123) - 0.5) * 0.09;
  }
}


#include "src/scene_core.c"
#include "src/ships_battles.c"
#include "src/hill_events.c"
#include "src/sky_events.c"

static int backdrop_base_w = 0, backdrop_base_h = 0;

void render_backdrop(RenderTarget *surface, Uint32 ticks) {
  int scale = surface->h < surface->w ? surface->h : surface->w;
  if (scale < 1) scale = 1;
  if (backdrop_base && (backdrop_base_w != surface->w || backdrop_base_h != surface->h)) {
    SDL_DestroyTexture(backdrop_base);
    backdrop_base = NULL;
  }

  if (!backdrop_base) {
    int bw = (surface->w + 3) / 4;
    int bh = (surface->h + 3) / 4;
    SDL_Surface *base_surf = SDL_CreateSurface(bw, bh, SDL_PIXELFORMAT_RGBA8888);
    if (!base_surf) {
      fprintf(stderr, "Unable to allocate backdrop surface: %s\n", SDL_GetError());
      return;
    }
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(base_surf->format);
    SDL_LockSurface(base_surf);

    double drift = ticks * 0.000018;
    for (int by = 0; by < bh; by++) {
      int y = by * 4;
      double ny = (double)(y + 2) / surface->h;
      double sky_blend = ny / 0.68;
      Uint32 *row = (Uint32 *)((Uint8 *)base_surf->pixels + by * base_surf->pitch);
      for (int bx = 0; bx < bw; bx++) {
        int x = bx * 4;
        double nx = (double)(x + 2) / surface->w;
        double fine = smooth_noise(nx * 25.0 + drift * 0.35, ny * 21.0, 41);
        Uint8 red = blend_channel(5, 35, sky_blend);
        Uint8 green = blend_channel(8, 37, sky_blend);
        Uint8 blue = blend_channel(21, 67, sky_blend);

        if (ny < 0.62) {
          double broad = smooth_noise(nx * 3.2 + drift * 0.25, ny * 3.8, 101);
          double middle = smooth_noise(nx * 8.5 - drift * 0.42, ny * 10.0 + drift * 0.12, 211);
          double detail = smooth_noise(nx * 23.0 + drift * 0.75, ny * 28.0 - drift * 0.2, 307);
          double turbulence = broad * 0.48 + middle * 0.34 + detail * 0.18;
          double band = 0.27 + (smooth_noise(nx * 2.3 + drift * 0.08, 1.4, 419) - 0.5) * 0.24;
          double band_width = 0.24 + broad * 0.06;
          double band_edge = 1.0 - fabs(ny - band) / band_width;
          double nebula = band_edge > 0.0 ? (turbulence - 0.36) * 3.4 * band_edge : 0.0;
          if (nebula > 0.0) {
            double hue = smooth_noise(nx * 4.0 - drift * 0.08, ny * 5.5, 523);
            Uint8 nebula_red = blend_channel(62, 151, hue);
            Uint8 nebula_green = blend_channel(49, 83, hue);
            Uint8 nebula_blue = blend_channel(145, 226, 1.0 - hue * 0.45);
            double light = nebula * (0.24 + 0.30 * turbulence);
            red = blend_channel(red, nebula_red, light);
            green = blend_channel(green, nebula_green, light);
            blue = blend_channel(blue, nebula_blue, light);
          }

          double cloud_mass =
              smooth_noise(nx * 4.2 + drift * 0.42, ny * 6.0 + drift * 0.08, 631) * 0.62 +
              smooth_noise(nx * 13.0 + drift * 0.7, ny * 19.0 - drift * 0.16, 719) * 0.38;
          double cloud_band =
              0.48 + 0.035 * (smooth_noise(nx * 3.0 + drift * 0.14, 2.0, 811) - 0.5);
          double cloud_envelope = 1.0 - fabs(ny - cloud_band) / 0.14;
          double cloud_density =
              cloud_envelope > 0.0 ? (cloud_mass - 0.50) * 3.2 * cloud_envelope : 0.0;
          if (cloud_density > 0.0) {
            double amount = cloud_density * 0.42;
            red = blend_channel(red, (Uint8)(123 + 32 * fine), amount);
            green = blend_channel(green, (Uint8)(139 + 29 * fine), amount);
            blue = blend_channel(blue, (Uint8)(174 + 34 * fine), amount);
          }
        }

        double far_edge = ridge_height(RIDGE_FAR, nx);
        double middle_edge = ridge_height(RIDGE_MIDDLE, nx);
        double near_edge = ridge_height(RIDGE_NEAR, nx);
        double ground_noise = smooth_noise(nx * 28.0, ny * 32.0, 1237);
        double far_cover = smooth_step(far_edge - 0.014, far_edge + 0.014, ny);
        red = blend_channel(red, (Uint8)(35 + 17 * ground_noise), far_cover);
        green = blend_channel(green, (Uint8)(44 + 21 * ground_noise), far_cover);
        blue = blend_channel(blue, (Uint8)(60 + 24 * ground_noise), far_cover);
        double middle_cover = smooth_step(middle_edge - 0.012, middle_edge + 0.012, ny);
        red = blend_channel(red, (Uint8)(20 + 10 * ground_noise), middle_cover);
        green = blend_channel(green, (Uint8)(31 + 15 * ground_noise), middle_cover);
        blue = blend_channel(blue, (Uint8)(39 + 17 * ground_noise), middle_cover);
        double near_cover = smooth_step(near_edge - 0.010, near_edge + 0.010, ny);
        red = blend_channel(red, (Uint8)(9 + 8 * ground_noise), near_cover);
        green = blend_channel(green, (Uint8)(17 + 10 * ground_noise), near_cover);
        blue = blend_channel(blue, (Uint8)(25 + 12 * ground_noise), near_cover);

        row[bx] = SDL_MapRGBA(fmt, NULL, red, green, blue, 255);
      }
    }
    SDL_UnlockSurface(base_surf);

    backdrop_base = SDL_CreateTextureFromSurface(surface->renderer, base_surf);
    SDL_DestroySurface(base_surf);
    if (backdrop_base) {
      SDL_SetTextureScaleMode(backdrop_base, SDL_SCALEMODE_NEAREST);
      SDL_SetTextureBlendMode(backdrop_base, SDL_BLENDMODE_NONE);
      backdrop_base_w = surface->w;
      backdrop_base_h = surface->h;
    } else {
      fprintf(stderr, "Unable to cache backdrop texture: %s\n", SDL_GetError());
    }
  }

  if (backdrop_base) {
    SDL_FRect src_f = {0, 0, (float)surface->w / 4.0f, (float)surface->h / 4.0f};
    SDL_FRect dst_f = {0, 0, (float)surface->w, (float)surface->h};
    SDL_RenderTexture(surface->renderer, backdrop_base, &src_f, &dst_f);
  }

  SceneWeather weather = get_scene_weather(ticks);
  double warm_blend = smooth_step(0.72, 0.95, weather.sunshine);
  static const Uint8 season_tint[4][3] = {{40, 68, 43}, {72, 77, 43}, {92, 48, 23}, {40, 56, 83}};
  double tint_red = 0.0, tint_green = 0.0, tint_blue = 0.0;
  for (int i = 0; i < 4; i++) {
    tint_red += season_tint[i][0] * weather.season[i];
    tint_green += season_tint[i][1] * weather.season[i];
    tint_blue += season_tint[i][2] * weather.season[i];
  }
  double cloud_mix = weather.cloud * 0.55;
  tint_red = tint_red * (1.0 - cloud_mix) + 48 * cloud_mix;
  tint_green = tint_green * (1.0 - cloud_mix) + 59 * cloud_mix;
  tint_blue = tint_blue * (1.0 - cloud_mix) + 78 * cloud_mix;
  double palette_mix = 0.70 + weather.fog * 0.10 + weather.aurora * 0.10;
  tint_red = tint_red * (1.0 - palette_mix) + weather.palette[0] * palette_mix;
  tint_green = tint_green * (1.0 - palette_mix) + weather.palette[1] * palette_mix;
  tint_blue = tint_blue * (1.0 - palette_mix) + weather.palette[2] * palette_mix;
  double warmth = weather.sunshine * 0.24;
  tint_red = tint_red * (1.0 - warmth) + 235 * warmth;
  tint_green = tint_green * (1.0 - warmth) + 151 * warmth;
  tint_blue = tint_blue * (1.0 - warmth) + 78 * warmth;
  Uint8 warm_red = 255;
  Uint8 warm_green = (Uint8)(130 + weather.palette[1] * 0.25);
  Uint8 warm_blue = (Uint8)(28 + weather.palette[2] * 0.25);
  tint_red = tint_red * (1.0 - warm_blend) + warm_red * warm_blend;
  tint_green = tint_green * (1.0 - warm_blend) + warm_green * warm_blend;
  tint_blue = tint_blue * (1.0 - warm_blend) + warm_blue * warm_blend;
  Uint8 tint_alpha = (Uint8)(19 + weather.cloud * 12 + weather.fog * 18 + weather.lightning * 12 +
                             weather.aurora * 12 + weather.sunshine * 33);
  boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, (Uint8)tint_red, (Uint8)tint_green,
          (Uint8)tint_blue, tint_alpha);
  render_fullscreen_event(surface, ticks, scale, &weather);
  render_weather_effects(surface, ticks, &weather, scale);
  render_alien_weather(surface, ticks, scale);
  render_world_jump(surface, ticks, scale);
  if (weather.sunshine > 0.01) {
    int sun_x = surface->w * 3 / 4;
    int sun_y = surface->h * 27 / 100;
    int glow_radius = scale / 11;
    Uint8 glow_alpha = (Uint8)(weather.sunshine * 10);
    filledCircleRGBA(surface, sun_x, sun_y, glow_radius, 211, 171, 112, glow_alpha);
    filledCircleRGBA(surface, sun_x, sun_y, glow_radius / 2, 239, 205, 146,
                     (Uint8)(weather.sunshine * 20));
    filledCircleRGBA(surface, sun_x, sun_y, glow_radius / 5, 255, 231, 183,
                     (Uint8)(weather.sunshine * 105));
  }

  Uint32 star_seed = 0x4f1bbcdcu;
  int star_count = (int)(surface->w * surface->h / 2800.0 * (1.0 - weather.sunshine * 0.8));
  for (int i = 0; i < star_count; i++) {
    star_seed = star_seed * 1664525u + 1013904223u;
    int sx = star_seed % surface->w;
    star_seed = star_seed * 1664525u + 1013904223u;
    int sy = star_seed % (surface->h * 54 / 100);
    double twinkle = 0.5 + 0.5 * sin(ticks / (700.0 + i % 13 * 91) + i * 1.73);
    Uint8 alpha = (Uint8)(35 + twinkle * 95);
    if (i % 31 == 0) filledCircleRGBA(surface, sx, sy, 3, 126, 158, 226, 13);
    pixelRGBA(surface, sx, sy, 205, 214, 235, alpha);
    if (i % 31 == 0) filledCircleRGBA(surface, sx, sy, 1, 237, 232, 222, alpha);
  }

  static const int bird_group_x[] = {8, 36, 65, 91};
  static const int bird_group_y[] = {29, 43, 34, 48};
  for (size_t group = 0; group < sizeof(bird_group_x) / sizeof(bird_group_x[0]); group++) {
    int bird_span = scale / 85;
    if (bird_span < 4) bird_span = 4;
    int formation_width = 27 * bird_span;
    int exit_margin = bird_span * 2;
    int route_length = surface->w + formation_width + exit_margin * 2;
    int travel = (int)((Uint64)ticks * route_length / 90000);
    int route_start = route_length * bird_group_x[group] / 100;
    int group_x = (route_start + travel) % route_length - exit_margin;
    int group_y = surface->h * bird_group_y[group] / 100 +
                  (int)(sin(ticks / 1700.0 + group * 2.1) * scale * 0.008);
    static const int formation_x[] = {0, -3, -6, -9, -12, -15, -18, -21, -24, -27};
    static const int formation_y[] = {0, 3, -3, 5, -6, 1, 7, -4, 4, -1};
    for (int bird = 0; bird < 10; bird++) {
      int bird_x = group_x + formation_x[bird] * bird_span;
      int bird_y = group_y + formation_y[bird] * bird_span;
      int flap = (int)((0.5 + 0.5 * sin(ticks / 170.0 + group * 1.8 + bird)) * (bird_span + 1));
      draw_bird(surface, bird_x, bird_y, bird_span, flap);
    }
  }

  // Battles, flyby and rare sky events never overlap (see GROUP_SKY).
  render_fleet_battle(surface, ticks, scale);
  render_background_battle(surface, ticks, scale);
  render_battle_flyby(surface, ticks, scale);
  render_dogfight_effect(surface, ticks, scale);
  render_rare_sky_event(surface, ticks, scale);
  render_long_scene(surface, ticks, scale);
  render_hill_event(surface, ticks, scale);

  int autumn = weather.season[2] * 16;
  for (int i = 0; i < autumn; i++) {
    unsigned int seed = (unsigned int)i * 2246822519u + 3266489917u;
    int x = (seed + ticks / (32 + i % 5 * 9)) % surface->w;
    int y = (seed / 11 + ticks * (2 + i % 3) / 55) % surface->h;
    int sway = (int)(sin(ticks / 370.0 + i * 1.7) * scale * 0.012);
    Uint8 alpha = (Uint8)(55 + 90 * weather.season[2]);
    thickLineRGBA(surface, x, y, x + sway + 2, y + 3, 2, (Uint8)(154 + i % 3 * 20),
                  (Uint8)(76 + i % 4 * 13), 43, alpha);
  }

  unsigned int rain_seed = 0x7f4a7c15u;
  double storm_strength = smooth_step(0.25, 0.75, weather.lightning);
  int rain_count =
      (int)(surface->w * surface->h / 1650.0 * weather.rain * (1.0 + storm_strength * 2.0));
  if (rain_count > MAX_RAIN_DROPS) rain_count = MAX_RAIN_DROPS;
  for (int i = 0; i < rain_count; i++) {
    rain_seed = rain_seed * 1664525u + 1013904223u;
    int start_x = rain_seed % surface->w;
    rain_seed = rain_seed * 1664525u + 1013904223u;
    int start_y = rain_seed % surface->h;
    int length = 5 + (rain_seed >> 24) % 10;
    int speed = 5 + i % 8;
    double travel = ticks * speed / 150.0;
    int y = (start_y + (int)travel) % (surface->h + length);
    int x = (start_x + (int)(travel * (0.16 + weather.wind * 0.22))) % surface->w;
    // The streak leans the same way the drop travels.
    int x_end = x - (int)(length * (0.16 + weather.wind * 0.22) + 0.5);
    int y_start = y - length;
    Uint8 alpha = (Uint8)(38 + (rain_seed >> 16) % 48 + weather.rain * 35);
    thickLineRGBA(surface, x, y, x_end, y_start, 1, 154, 174, 207, alpha);
  }

  int snow_count = (int)(surface->w * surface->h / 3400.0 * weather.snow);
  for (int i = 0; i < snow_count; i++) {
    unsigned int seed = (unsigned int)i * 1664525u + 1013904223u;
    int x = (seed + ticks / (12 + i % 7 * 4)) % surface->w;
    int y = (seed / 13 + ticks / (18 + i % 5 * 7)) % surface->h;
    filledCircleRGBA(surface, x, y, i % 9 == 0 ? 2 : 1, 202, 216, 231,
                     (Uint8)(85 + weather.snow * 80));
  }
  Uint8 warm_alpha = (Uint8)(warm_blend * (75 + weather.sunshine * 15));
  boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, warm_red, warm_green, warm_blue,
          warm_alpha);
}


static SDL_Texture *get_digit_glyph_texture(char ch, int *w, int *h, int *y_off) {
  if (ch < '0' || ch > '9') return NULL;
  int idx = ch - '0';
  if (!digit_glyph_tex[idx]) {
    SDL_Surface *surf = TTF_RenderGlyph_Blended(font_time, (Uint32)ch, FONT_COLOR);
    if (!surf) {
      fprintf(stderr, "Unable to render clock digit '%c': %s\n", ch, SDL_GetError());
      return NULL;
    }
    int min_x = surf->w, max_x = -1;
    int min_y = surf->h, max_y = -1;
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(surf->format);
    if (fmt && SDL_LockSurface(surf)) {
      for (int y = 0; y < surf->h; y++) {
        const Uint8 *row = (const Uint8 *)surf->pixels + y * surf->pitch;
        for (int x = 0; x < surf->w; x++) {
          Uint32 px;
          memcpy(&px, row + x * fmt->bytes_per_pixel, fmt->bytes_per_pixel);
          Uint8 r, g, b, a;
          SDL_GetRGBA(px, fmt, NULL, &r, &g, &b, &a);
          if (a > 0) {
            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
          }
        }
      }
      SDL_UnlockSurface(surf);
    }
    if (max_x >= min_x && max_y >= min_y) {
      int crop_w = max_x - min_x + 1;
      int crop_h = max_y - min_y + 1;
      SDL_Surface *cropped = SDL_CreateSurface(crop_w, crop_h, surf->format);
      if (cropped) {
        SDL_Rect src_rect = {min_x, min_y, crop_w, crop_h};
        SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);
        SDL_BlitSurface(surf, &src_rect, cropped, NULL);
        digit_glyph_w[idx] = crop_w;
        digit_glyph_h[idx] = crop_h;
        digit_glyph_y_off[idx] = min_y - (surf->h - 1 - max_y);
        digit_glyph_tex[idx] = SDL_CreateTextureFromSurface(renderer, cropped);
        SDL_DestroySurface(cropped);
      } else {
        digit_glyph_w[idx] = surf->w;
        digit_glyph_h[idx] = surf->h;
        digit_glyph_y_off[idx] = 0;
        digit_glyph_tex[idx] = SDL_CreateTextureFromSurface(renderer, surf);
      }
    } else {
      digit_glyph_w[idx] = surf->w;
      digit_glyph_h[idx] = surf->h;
      digit_glyph_y_off[idx] = 0;
      digit_glyph_tex[idx] = SDL_CreateTextureFromSurface(renderer, surf);
    }
    SDL_DestroySurface(surf);
    if (!digit_glyph_tex[idx]) return NULL;
    SDL_SetTextureBlendMode(digit_glyph_tex[idx], SDL_BLENDMODE_BLEND);
  }
  *w = digit_glyph_w[idx];
  *h = digit_glyph_h[idx];
  if (y_off) *y_off = digit_glyph_y_off[idx];
  return digit_glyph_tex[idx];
}


void blit_digits(RenderTarget *surface, SDL_Rect *rect, int spc, char digits[], Uint8 mod_r,
                 Uint8 mod_g, Uint8 mod_b) {
  if (digits[1]) {
    int gap = spc + spc / 2;
    int w0 = 0, h0 = 0, y0 = 0, w1 = 0, h1 = 0, y1 = 0;
    SDL_Texture *t0 = get_digit_glyph_texture(digits[0], &w0, &h0, &y0);
    SDL_Texture *t1 = get_digit_glyph_texture(digits[1], &w1, &h1, &y1);
    if (!t0 || !t1) return;
    int total_width = w0 + gap + w1;
    float x = (float)(rect->x + (rect->w - total_width) / 2);
    SDL_FRect dst0 = {x, (float)(rect->y + (rect->h - h0 + y0) / 2), (float)w0, (float)h0};
    SDL_SetTextureColorMod(t0, mod_r, mod_g, mod_b);
    SDL_RenderTexture(surface->renderer, t0, NULL, &dst0);
    SDL_FRect dst1 = {x + w0 + gap, (float)(rect->y + (rect->h - h1 + y1) / 2), (float)w1,
                      (float)h1};
    SDL_SetTextureColorMod(t1, mod_r, mod_g, mod_b);
    SDL_RenderTexture(surface->renderer, t1, NULL, &dst1);
  } else {
    int w0 = 0, h0 = 0, y0 = 0;
    SDL_Texture *t0 = get_digit_glyph_texture(digits[0], &w0, &h0, &y0);
    if (!t0) return;
    SDL_FRect dst0 = {(float)(rect->x + (rect->w - w0) / 2),
                      (float)(rect->y + (rect->h - h0 + y0) / 2), (float)w0, (float)h0};
    SDL_SetTextureColorMod(t0, mod_r, mod_g, mod_b);
    SDL_RenderTexture(surface->renderer, t0, NULL, &dst0);
  }
}


static void draw_card_divider(RenderTarget *surface, SDL_Rect *background) {
  if (!animate) return;
  bool is_h = surface->h < surface->w;
  float div_h = (float)((int)((is_h ? surface->h : surface->w) * 0.005));
  if (div_h < 1.0f) div_h = 1.0f;
  float div_y = (float)(background->y + (background->h - (int)div_h) / 2);
  SDL_SetRenderDrawBlendMode(surface->renderer, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(surface->renderer, 7, 9, 15, 255);
  SDL_FRect r1 = {(float)background->x, div_y, (float)background->w, div_h};
  SDL_RenderFillRect(surface->renderer, &r1);
  SDL_SetRenderDrawColor(surface->renderer, 0x35, 0x3a, 0x49, 255);
  SDL_FRect r2 = {(float)background->x, div_y + div_h, (float)background->w, 1.0f};
  SDL_RenderFillRect(surface->renderer, &r2);
}


void render_digits(RenderTarget *surface, SDL_Rect *background, int card_idx, char digits[],
                   char prevdigits[], int maxsteps, int step) {
  bool is_h = surface->h < surface->w;
  int spc = (int)(is_h ? surface->h * .0125 : surface->w * .0125);
  SDL_FRect bg_dst = {(float)background->x, (float)background->y, (float)background->w,
                      (float)background->h};

  if (step == maxsteps - 1) {
    if (!card_cache_tex[card_idx] || strcmp(card_cache_digits[card_idx], digits) != 0) {
      if (!card_cache_tex[card_idx]) {
        card_cache_tex[card_idx] =
            SDL_CreateTexture(surface->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                              background->w, background->h);
        if (card_cache_tex[card_idx])
          SDL_SetTextureBlendMode(card_cache_tex[card_idx], SDL_BLENDMODE_BLEND);
      }
      if (card_cache_tex[card_idx]) {
        SDL_Texture *prev_target = SDL_GetRenderTarget(surface->renderer);
        SDL_SetRenderTarget(surface->renderer, card_cache_tex[card_idx]);
        SDL_SetRenderDrawColor(surface->renderer, 0, 0, 0, 0);
        SDL_SetRenderDrawBlendMode(surface->renderer, SDL_BLENDMODE_NONE);
        SDL_RenderClear(surface->renderer);
        SDL_FRect local_rect = {0, 0, (float)background->w, (float)background->h};
        SDL_SetTextureBlendMode(bg_texture, SDL_BLENDMODE_NONE);
        SDL_RenderTexture(surface->renderer, bg_texture, NULL, &local_rect);
        SDL_SetTextureBlendMode(bg_texture, SDL_BLENDMODE_BLEND);
        SDL_Rect local_i = {0, 0, background->w, background->h};
        blit_digits(surface, &local_i, spc, digits, 255, 255, 255);
        SDL_SetRenderTarget(surface->renderer, prev_target);
        snprintf(card_cache_digits[card_idx], sizeof(card_cache_digits[card_idx]), "%s", digits);
      }
    }
    if (card_cache_tex[card_idx]) {
      SDL_RenderTexture(surface->renderer, card_cache_tex[card_idx], NULL, &bg_dst);
    } else {
      SDL_RenderTexture(surface->renderer, bg_texture, NULL, &bg_dst);
      blit_digits(surface, background, spc, digits, 255, 255, 255);
    }
    draw_card_divider(surface, background);
    return;
  }

  // Flip animation frame: draw static card background + top half of new digits
  SDL_RenderTexture(surface->renderer, bg_texture, NULL, &bg_dst);
  SDL_Rect top_clip = {background->x, background->y, background->w, background->h / 2};
  SDL_SetRenderClipRect(surface->renderer, &top_clip);
  blit_digits(surface, background, spc, digits, 255, 255, 255);
  SDL_SetRenderClipRect(surface->renderer, NULL);

  int halfsteps = maxsteps / 2;
  int upperhalf = (step + 1) <= halfsteps;
  if (upperhalf) {
    SDL_Rect bot_clip = {background->x, background->y + background->h / 2, background->w,
                         background->h - background->h / 2};
    SDL_SetRenderClipRect(surface->renderer, &bot_clip);
    blit_digits(surface, background, spc, prevdigits, 255, 255, 255);
    SDL_SetRenderClipRect(surface->renderer, NULL);
  }

  double scale, intensity;
  if (upperhalf) {
    scale = 1.0 - (1.0 * step) / (halfsteps - 1);
    intensity = 1.0 - (1.0 * step) / (halfsteps - 1);
  } else {
    scale = ((1.0 * step) - halfsteps + 1) / halfsteps;
    intensity = ((1.0 * step) - halfsteps + 1) / halfsteps;
  }
  Uint8 mod = (Uint8)(255.0 * intensity);

  // Render flipping flap onto reusable GPU target texture, then draw scaled
  // half!
  if (!flip_card_tex) {
    flip_card_tex = SDL_CreateTexture(surface->renderer, SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET, background->w, background->h);
    if (flip_card_tex) SDL_SetTextureBlendMode(flip_card_tex, SDL_BLENDMODE_BLEND);
  }
  if (flip_card_tex) {
    SDL_Texture *prev_target = SDL_GetRenderTarget(surface->renderer);
    SDL_SetRenderTarget(surface->renderer, flip_card_tex);
    SDL_SetRenderDrawColor(surface->renderer, 0, 0, 0, 0);
    SDL_SetRenderDrawBlendMode(surface->renderer, SDL_BLENDMODE_NONE);
    SDL_RenderClear(surface->renderer);
    SDL_FRect local_rect = {0, 0, (float)background->w, (float)background->h};
    SDL_SetTextureBlendMode(bg_texture, SDL_BLENDMODE_NONE);
    SDL_RenderTexture(surface->renderer, bg_texture, NULL, &local_rect);
    SDL_SetTextureBlendMode(bg_texture, SDL_BLENDMODE_BLEND);
    SDL_Rect local_i = {0, 0, background->w, background->h};
    blit_digits(surface, &local_i, spc, upperhalf ? prevdigits : digits, mod, mod, mod);
    SDL_SetRenderTarget(surface->renderer, prev_target);

    float half_h = (float)(background->h / 2);
    SDL_FRect src_half = {0.0f, upperhalf ? 0.0f : half_h, (float)background->w, half_h};
    float scaled_h = (float)(background->h * scale);
    float dst_half_h = scaled_h * 0.5f;
    SDL_FRect dst_half = {(float)background->x,
                          (float)background->y + (upperhalf ? ((background->h - scaled_h) * 0.5f)
                                                            : (background->h * 0.5f)),
                          (float)background->w, dst_half_h};
    SDL_RenderTexture(surface->renderer, flip_card_tex, &src_half, &dst_half);
  }

  draw_card_divider(surface, background);
}


void render_clock(int maxsteps, int step, bool ambient_frame) {
  static Uint64 last_profile_report = 0;
  char buffer[3], buffer2[3];
  struct tm _time_val;
  struct tm *_time = &_time_val;
  time_t rawtime;
  static const int offset_x[] = {-1, 0, 1, 1, 1, 0, -1, -1};
  static const int offset_y[] = {0, -1, -1, 0, 1, 1, 0, -1};
  int phase, new_burnin_x, new_burnin_y;
  Uint64 frame_start = profile_rendering ? SDL_GetTicks() : 0;
  Uint64 backdrop_elapsed = 0;

  time(&rawtime);
  localtime_r(&rawtime, _time);
  phase = (rawtime / 300) % 8;
  new_burnin_x = offset_x[phase] * (screen->w / 80);
  new_burnin_y = offset_y[phase] * (screen->h / 80);
  if (new_burnin_x != burnin_x || new_burnin_y != burnin_y) {
    hourBackground.x += new_burnin_x - burnin_x;
    hourBackground.y += new_burnin_y - burnin_y;
    minBackground.x += new_burnin_x - burnin_x;
    minBackground.y += new_burnin_y - burnin_y;
    burnin_x = new_burnin_x;
    burnin_y = new_burnin_y;
  }
  Uint32 now = scene_time();
  // Render the live backdrop every frame (including during card flips) so
  // sky, hill, weather, and battle events continue animating smoothly.
  {
    Uint64 backdrop_start = profile_rendering ? SDL_GetTicks() : 0;
    render_backdrop(screen, now);
    if (profile_rendering) backdrop_elapsed = SDL_GetTicks() - backdrop_start;
  }

  bool time_changed = _time->tm_hour != past_h || _time->tm_min != past_m;
  bool redraw = ambient_frame || !time_changed;
  int render_steps = redraw ? 20 : maxsteps;
  int render_step = redraw ? 19 : step;
  bool card_drawn[2] = {false, false};

  // draw hours
  {
    int h = twentyfourh ? _time->tm_hour : (_time->tm_hour + 11) % 12 + 1;
    int prev_h = twentyfourh ? past_h : (past_h + 11) % 12 + 1;
    bool hour_static = redraw || _time->tm_hour == past_h;
    int hour_steps = hour_static ? 20 : render_steps;
    int hour_step = hour_static ? 19 : render_step;
    if (leadingzero) {
      snprintf(buffer, 3, "%02d", h);
      snprintf(buffer2, 3, "%02d", hour_static ? h : prev_h);
    } else {
      snprintf(buffer, 3, "%d", h);
      snprintf(buffer2, 3, "%d", hour_static ? h : prev_h);
    }
    render_digits(screen, &hourBackground, 0, buffer, buffer2, hour_steps, hour_step);
    if (!twentyfourh) render_ampm(screen, &hourBackground, _time->tm_hour >= 12);
    card_drawn[0] = true;
  }

  // draw minutes
  {
    bool min_static = redraw || _time->tm_min == past_m;
    snprintf(buffer, 3, "%02d", _time->tm_min);
    snprintf(buffer2, 3, "%02d", min_static ? _time->tm_min : past_m);
    render_digits(screen, &minBackground, 1, buffer, buffer2, min_static ? 20 : render_steps,
                  min_static ? 19 : render_step);
    card_drawn[1] = true;
  }

  if (_time->tm_yday != date_surface_yday) {
    char date[128];
    strftime(date, sizeof(date), "%A, %B", _time);
    snprintf(date + strlen(date), sizeof(date) - strlen(date), " %d, %d", _time->tm_mday,
             _time->tm_year + 1900);
    SDL_Surface *new_date_surface = TTF_RenderText_Blended(font_date, date, 0, DATE_COLOR);
    if (!new_date_surface) {
      fprintf(stderr, "Unable to render date: %s\n", SDL_GetError());
    } else {
      if (date_texture_cache) SDL_DestroyTexture(date_texture_cache);
      date_texture_w = new_date_surface->w;
      date_texture_h = new_date_surface->h;
      date_texture_cache = SDL_CreateTextureFromSurface(renderer, new_date_surface);
      SDL_DestroySurface(new_date_surface);
      if (date_texture_cache) {
        SDL_SetTextureBlendMode(date_texture_cache, SDL_BLENDMODE_BLEND);
        date_surface_yday = _time->tm_yday;
      }
    }
  }
  if (date_texture_cache && date_surface_yday == _time->tm_yday) {
    SDL_FRect date_coords = {(float)((screen->w - date_texture_w) / 2 + burnin_x),
                             (float)(date_y + burnin_y), (float)date_texture_w,
                             (float)date_texture_h};
    SDL_RenderTexture(renderer, date_texture_cache, NULL, &date_coords);
    past_yday = _time->tm_yday;
  }

  SceneWeather weather = get_scene_weather(now);
  SDL_Rect *clock_cards[] = {&hourBackground, &minBackground};
  for (size_t card_index = 0; card_index < sizeof(clock_cards) / sizeof(clock_cards[0]);
       card_index++) {
    SDL_Rect *card = clock_cards[card_index];
    if (!card_drawn[card_index]) continue;
    SDL_SetRenderClipRect(renderer, card);
    int drops = (int)(card->w / 42.0 * weather.rain);
    for (int i = 0; i < drops; i++) {
      Uint32 seed = (Uint32)(i + card_index * 97) * 2246822519u + 3266489917u;
      int drop_x = card->x + seed % card->w;
      int length = 4 + (seed >> 24) % 7;
      int speed = 7 + (seed >> 12) % 7;
      int drop_y = card->y + (int)((seed + now * speed / 90) % card->h);
      Uint8 alpha = (Uint8)(24 + (seed >> 18) % 30);
      thickLineRGBA(screen, drop_x, drop_y, drop_x - 1, drop_y - length, 1, 151, 174, 211, alpha);
    }
    int flakes = (int)(card->w / 50.0 * weather.snow);
    for (int i = 0; i < flakes; i++) {
      Uint32 seed = (Uint32)(i + card_index * 61) * 1664525u + 1013904223u;
      int flake_x = card->x + seed % card->w;
      int flake_y = card->y + (int)((seed + now * (2 + (seed >> 16) % 4) / 80) % card->h);
      pixelRGBA(screen, flake_x, flake_y, 208, 221, 236, 110);
    }
  }
  SDL_SetRenderClipRect(renderer, NULL);
  int scene_scale = screen->h < screen->w ? screen->h : screen->w;
  render_foreground_event(screen, now, scene_scale);
  render_rare_screen_fade(screen, now);

  // present frame
  SDL_RenderPresent(renderer);
  if (profile_rendering) {
    Uint64 frame_elapsed = SDL_GetTicks() - frame_start;
    if (frame_elapsed >= 50 && frame_start - last_profile_report >= 1000) {
      fprintf(stderr, "Gluqlo slow frame: %llu ms (backdrop: %llu ms)\n",
              (unsigned long long)frame_elapsed, (unsigned long long)backdrop_elapsed);
      last_profile_report = frame_start;
    }
  }

  if (render_step == render_steps - 1) {
    past_h = _time->tm_hour;
    past_m = _time->tm_min;
  }
}


void render_animation() {
  if (!animate) {
    render_clock(20, 19, false);
    return;
  }

  const int DURATION = 260;
  const int FRAME_COUNT = 20;
  Uint64 start_tick = SDL_GetTicks();
  for (int frame = 0; frame < FRAME_COUNT; frame++) {
    render_clock(FRAME_COUNT, frame, false);
    Uint64 elapsed = SDL_GetTicks() - start_tick;
    Uint64 next_frame = (Uint64)DURATION * (frame + 1) / FRAME_COUNT;
    if (elapsed < next_frame) SDL_Delay((Uint32)(next_frame - elapsed));
  }
}


Uint32 SDLCALL update_time(void *userdata, SDL_TimerID timerID, Uint32 interval) {
  (void)userdata;
  (void)timerID;
  SDL_Event e;
  SDL_Event pending_event;
  int pending = SDL_PeepEvents(&pending_event, 1, SDL_PEEKEVENT, user_event_type, user_event_type);
  if (pending < 0) {
    fprintf(stderr, "Unable to check pending clock events: %s\n", SDL_GetError());
    return interval;
  }
  if (pending == 0) {
    SDL_zero(e);
    e.type = user_event_type;
    e.user.code = 0;
    e.user.data1 = NULL;
    e.user.data2 = NULL;
    if (!SDL_PushEvent(&e)) {
      fprintf(stderr, "Unable to queue clock event: %s\n", SDL_GetError());
    }
  }

  return interval;
}


// Returns the value that follows option argv[*i] and advances *i, or NULL (with
// a message) if it is missing.
static const char *option_value(int argc, char **argv, int *i) {
  if (*i + 1 >= argc) {
    fprintf(stderr, "Option %s needs a value\n", argv[*i]);
    return NULL;
  }
  return argv[++*i];
}


static bool parse_int_in_range(const char *text, long min, long max, long *value) {
  char *end = NULL;
  errno = 0;
  long parsed = strtol(text, &end, 0);
  if (errno != 0 || end == text || *end != '\0' || parsed < min || parsed > max) return false;
  *value = parsed;
  return true;
}


static void destroy_clock_caches(void) {
  for (int i = 0; i < 10; i++) {
    if (digit_glyph_tex[i]) {
      SDL_DestroyTexture(digit_glyph_tex[i]);
      digit_glyph_tex[i] = NULL;
    }
  }
  for (int i = 0; i < 2; i++) {
    if (card_cache_tex[i]) {
      SDL_DestroyTexture(card_cache_tex[i]);
      card_cache_tex[i] = NULL;
    }
    card_cache_digits[i][0] = '\0';
    if (ampm_textures[i]) {
      SDL_DestroyTexture(ampm_textures[i]);
      ampm_textures[i] = NULL;
    }
  }
  if (flip_card_tex) {
    SDL_DestroyTexture(flip_card_tex);
    flip_card_tex = NULL;
  }
  if (bg_texture) {
    SDL_DestroyTexture(bg_texture);
    bg_texture = NULL;
  }
}


static bool is_readable_file(const char *path) {
  return path && path[0] != '\0' && access(path, R_OK) == 0;
}


static bool find_font_via_fontconfig(const char *pattern_str, char *out, size_t out_size) {
  if (!FcInit()) return false;
  FcPattern *pat = FcNameParse((const FcChar8 *)pattern_str);
  if (!pat) return false;
  FcConfigSubstitute(NULL, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);
  FcResult result;
  FcPattern *match = FcFontMatch(NULL, pat, &result);
  FcPatternDestroy(pat);
  if (!match) return false;
  FcChar8 *file = NULL;
  bool found = false;
  if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch && file) {
    const char *path = (const char *)file;
    if (is_readable_file(path)) {
      snprintf(out, out_size, "%s", path);
      found = true;
    }
  }
  FcPatternDestroy(match);
  return found;
}


static bool font_supports_date_text(const char *path) {
  TTF_Font *probe = TTF_OpenFont(path, 16.0f);
  if (!probe) return false;
  // gluqlo.ttf only contains '0'-'9', 'A', 'M', 'P'; reject fonts missing full
  // alphabet.
  static const char *required = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,";
  bool ok = true;
  for (const char *p = required; *p; p++) {
    if (!TTF_FontHasGlyph(probe, (Uint32)(unsigned char)*p)) {
      ok = false;
      break;
    }
  }
  TTF_CloseFont(probe);
  return ok;
}


static bool font_supports_clock_text(const char *path) {
  TTF_Font *probe = TTF_OpenFont(path, 16.0f);
  if (!probe) return false;
  static const char *required = "0123456789AMP";
  bool ok = true;
  for (const char *p = required; *p; p++) {
    if (!TTF_FontHasGlyph(probe, (Uint32)(unsigned char)*p)) {
      ok = false;
      break;
    }
  }
  TTF_CloseFont(probe);
  return ok;
}


static const char *resolve_date_font_path(void) {
  if (resolved_date_font[0] != '\0' && is_readable_file(resolved_date_font)) {
    return resolved_date_font;
  }

  const char *env_date_font = getenv("GLUQLO_DATE_FONT");
  if (is_readable_file(env_date_font) && font_supports_date_text(env_date_font)) {
    snprintf(resolved_date_font, sizeof(resolved_date_font), "%s", env_date_font);
    return resolved_date_font;
  }

  static const char *fc_patterns[] = {"sans:lang=en", "sans-serif", "DejaVu Sans",
                                      "Liberation Sans", "Noto Sans"};
  for (size_t i = 0; i < sizeof(fc_patterns) / sizeof(fc_patterns[0]); i++) {
    char candidate[1024];
    if (find_font_via_fontconfig(fc_patterns[i], candidate, sizeof(candidate)) &&
        font_supports_date_text(candidate)) {
      snprintf(resolved_date_font, sizeof(resolved_date_font), "%s", candidate);
      return resolved_date_font;
    }
  }
  return NULL;
}


static const char *resolve_clock_font_path(void) {
  if (resolved_clock_font[0] != '\0' && is_readable_file(resolved_clock_font)) {
    return resolved_clock_font;
  }

  const char *env_font = getenv("GLUQLO_FONT");
  if (is_readable_file(env_font) && font_supports_clock_text(env_font)) {
    snprintf(resolved_clock_font, sizeof(resolved_clock_font), "%s", env_font);
    return resolved_clock_font;
  }

  const char *base_path = SDL_GetBasePath();
  char local_font[1024] = "";
  if (base_path) {
    snprintf(local_font, sizeof(local_font), "%sgluqlo.ttf", base_path);
  }

  const char *clock_font_paths[] = {FONT, local_font[0] ? local_font : NULL, "./gluqlo.ttf"};
  for (size_t i = 0; i < sizeof(clock_font_paths) / sizeof(clock_font_paths[0]); i++) {
    if (is_readable_file(clock_font_paths[i]) && font_supports_clock_text(clock_font_paths[i])) {
      snprintf(resolved_clock_font, sizeof(resolved_clock_font), "%s", clock_font_paths[i]);
      return resolved_clock_font;
    }
  }

  char fc_gluqlo[1024];
  if (find_font_via_fontconfig("Gluqlo", fc_gluqlo, sizeof(fc_gluqlo)) &&
      font_supports_clock_text(fc_gluqlo)) {
    snprintf(resolved_clock_font, sizeof(resolved_clock_font), "%s", fc_gluqlo);
    return resolved_clock_font;
  }

  // Fallback to the resolved date font if gluqlo.ttf is not installed anywhere.
  const char *fallback = resolve_date_font_path();
  if (fallback && font_supports_clock_text(fallback)) {
    snprintf(resolved_clock_font, sizeof(resolved_clock_font), "%s", fallback);
    return resolved_clock_font;
  }
  return NULL;
}


static void layout_clock(double display_scale_factor) {
  int out_w = width, out_h = height;
  SDL_GetCurrentRenderOutputSize(renderer, &out_w, &out_h);
  if (out_w < 1) out_w = 1;
  if (out_h < 1) out_h = 1;
  screen_target.w = out_w;
  screen_target.h = out_h;

  int scaled_w = (int)(out_w * display_scale_factor);
  int scaled_h = (int)(out_h * display_scale_factor);
  bool is_horizontal = scaled_w > scaled_h;

  if (font_time) TTF_CloseFont(font_time);
  if (font_mode) TTF_CloseFont(font_mode);
  if (font_date) TTF_CloseFont(font_date);
  const char *clock_font_path = resolve_clock_font_path();
  const char *date_font_path = resolve_date_font_path();
  font_time = clock_font_path ? TTF_OpenFont(clock_font_path,
                                             (float)((is_horizontal ? scaled_h : scaled_w) / 1.68))
                              : NULL;
  font_mode = clock_font_path ? TTF_OpenFont(clock_font_path,
                                             (float)((is_horizontal ? scaled_h : scaled_w) / 16.5))
                              : NULL;
  if (!font_time || !font_mode) {
    fprintf(stderr, "TTF_OpenFont (clock font): %s\n",
            clock_font_path ? SDL_GetError() : "no suitable clock font found");
    exit(1);
  }
  float date_font_size = (float)((is_horizontal ? scaled_h : scaled_w) / 24.0);
  font_date = date_font_path ? TTF_OpenFont(date_font_path, date_font_size) : NULL;
  if (!font_date) {
    fprintf(stderr, "TTF_OpenFont (date font): %s\n",
            date_font_path ? SDL_GetError() : "no suitable date font found");
    exit(1);
  }

  destroy_clock_caches();
  if (date_texture_cache) {
    SDL_DestroyTexture(date_texture_cache);
    date_texture_cache = NULL;
  }
  date_surface_yday = -1;
  past_yday = -1;
  burnin_x = 0;
  burnin_y = 0;

  int rectsize, spacing, radius;
  int date_h = TTF_GetFontHeight(font_date);
  int date_gap = date_h / 2;

  if (is_horizontal) {
    rectsize = (int)(scaled_h * 0.6);
    spacing = (int)(scaled_w * .031);
    radius = (int)(rectsize * .0952);
  } else {
    spacing = (int)(scaled_h * .031);
    int max_rectsize = (scaled_h - date_h - date_gap * 2 - spacing) / 2;
    rectsize = scaled_w * 0.6 < max_rectsize ? (int)(scaled_w * 0.6) : max_rectsize;
    radius = (int)(rectsize * .0952);
  }
  if (rectsize < 4) rectsize = 4;

  int jitter_width = 1, jitter_height = 1;
  if (display_scale_factor != 1.0) {
    jitter_width = (int)((out_w - scaled_w) * 0.5);
    jitter_height = (int)((out_h - scaled_h) * 0.5);
  }

  hourBackground.w = rectsize;
  hourBackground.h = rectsize;
  minBackground.w = rectsize;
  minBackground.h = rectsize;

  if (is_horizontal) {
    hourBackground.x = (int)(0.5 * (scaled_w - (2 * rectsize) - spacing) + jitter_width);
    hourBackground.y = (scaled_h - rectsize - date_gap - date_h) / 2 + jitter_height;
    minBackground.x = hourBackground.x + rectsize + spacing;
    minBackground.y = hourBackground.y;
    date_y = hourBackground.y + rectsize + date_gap;
  } else {
    hourBackground.y =
        (scaled_h - (2 * rectsize) - spacing - date_gap - date_h) / 2 + jitter_height;
    hourBackground.x = (scaled_w - rectsize) / 2 + jitter_width;
    minBackground.y = hourBackground.y + rectsize + spacing;
    minBackground.x = hourBackground.x;
    date_y = minBackground.y + rectsize + date_gap;
  }

  bgrect.x = 0;
  bgrect.y = 0;
  bgrect.w = rectsize;
  bgrect.h = rectsize;
  SDL_Surface *bg_surf = SDL_CreateSurface(rectsize, rectsize, SDL_PIXELFORMAT_RGBA8888);
  if (bg_surf) {
    fill_rounded_box_b(bg_surf, &bgrect, radius, CARD_BORDER_COLOR, 232);
    int border_width = rectsize / 120;
    if (border_width < 1) border_width = 1;
    SDL_Rect cardrect = {border_width, border_width, rectsize - 2 * border_width,
                         rectsize - 2 * border_width};
    int inner_radius = radius > border_width ? radius - border_width : 1;
    fill_rounded_box_b(bg_surf, &cardrect, inner_radius, BACKGROUND_COLOR, 190);
    bg_texture = SDL_CreateTextureFromSurface(renderer, bg_surf);
    SDL_DestroySurface(bg_surf);
    if (bg_texture) SDL_SetTextureBlendMode(bg_texture, SDL_BLENDMODE_BLEND);
  }
}


int main(int argc, char **argv) {
  char *wid_env;
  double display_scale_factor = 1;
  bool seed_given = false;
  Uint32 scene_seed = 0;
  profile_rendering = getenv("GLUQLO_PROFILE") != NULL;

  Uint32 wid = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp("--help", argv[i]) == 0 || strcmp("-help", argv[i]) == 0) {
      printf("Usage: %s [OPTION...]\nOptions:\n", argv[0]);
      printf("  -help\t\tDisplay this\n");
      printf("  -root, -f\tFullscreen\n");
      printf("  -noflip\tDisable the flip animation (change time in one frame)\n");
      printf("  -anykeyclose\tClose app when mouse move or any key pressed\n");
      printf("  -ampm\t\tUse 12-hour clock format (AM/PM)\n");
      printf("  -leadingzero\tAlways display hour with two digits\n");
      printf("  -w\t\tCustom width\n");
      printf("  -h\t\tCustom height\n");
      printf("  -r\t\tCustom resolution in WxH format\n");
      printf("  -s\t\tCustom display scale factor\n");
      printf(
          "  -effect NAME[:KIND]\tShow only one scheduled effect, repeating "
          "(NAME 'list' lists them)\n");
      printf(
          "  -seed N\tStart the scenery (season, weather, effects) at a fixed "
          "point instead of a random one\n");
      return 0;
    } else if (strcmp("-root", argv[i]) == 0 || strcmp("-f", argv[i]) == 0 ||
               strcmp("--fullscreen", argv[i]) == 0) {
      fullscreen = true;
    } else if (strcmp("-noflip", argv[i]) == 0) {
      animate = false;
    } else if (strcmp("-anykeyclose", argv[i]) == 0) {
      anykeyclose = true;
    } else if (strcmp("-ampm", argv[i]) == 0) {
      twentyfourh = false;
    } else if (strcmp("-leadingzero", argv[i]) == 0) {
      leadingzero = true;
    } else if (strcmp("-r", argv[i]) == 0 || strcmp("--resolution", argv[i]) == 0) {
      const char *value = option_value(argc, argv, &i);
      if (!value) return 1;
      int parsed_width, parsed_height;
      char trailing;
      if (sscanf(value, "%dx%d%c", &parsed_width, &parsed_height, &trailing) != 2 ||
          parsed_width < 1 || parsed_width > 16384 || parsed_height < 1 || parsed_height > 16384) {
        fprintf(stderr, "Invalid resolution -- %s (expected WxH, e.g. 1280x720)\n", value);
        return 1;
      }
      width = parsed_width;
      height = parsed_height;
    } else if (strcmp("-w", argv[i]) == 0 || strcmp("-h", argv[i]) == 0) {
      bool is_width = argv[i][1] == 'w';
      const char *value = option_value(argc, argv, &i);
      if (!value) return 1;
      long parsed;
      if (!parse_int_in_range(value, 1, 16384, &parsed)) {
        fprintf(stderr, "Invalid %s -- %s\n", is_width ? "width" : "height", value);
        return 1;
      }
      if (is_width)
        width = (int)parsed;
      else
        height = (int)parsed;
    } else if (strcmp("-s", argv[i]) == 0) {
      const char *value = option_value(argc, argv, &i);
      if (!value) return 1;
      char *end = NULL;
      double parsed = strtod(value, &end);
      if (end == value || *end != '\0' || !(parsed > 0.0 && parsed <= 16.0)) {
        fprintf(stderr, "Invalid scale factor -- %s (expected a number above 0)\n", value);
        return 1;
      }
      display_scale_factor = parsed;
    } else if (strcmp("-window-id", argv[i]) == 0) {
      const char *value = option_value(argc, argv, &i);
      if (!value) return 1;
      long parsed;
      if (!parse_int_in_range(value, 1, 0xffffffffL, &parsed)) {
        fprintf(stderr, "Invalid window id -- %s\n", value);
        return 1;
      }
      wid = (Uint32)parsed;
    } else if (strcmp("-seed", argv[i]) == 0) {
      const char *value = option_value(argc, argv, &i);
      if (!value) return 1;
      long parsed;
      if (!parse_int_in_range(value, 0, 0xffffffffL, &parsed)) {
        fprintf(stderr, "Invalid seed -- %s (expected a number)\n", value);
        return 1;
      }
      scene_seed = (Uint32)parsed;
      seed_given = true;
    } else if (strcmp("-effect", argv[i]) == 0) {
      const char *effect_value = option_value(argc, argv, &i);
      if (!effect_value) {
        fprintf(stderr, "Try -effect list for the available effects.\n");
        return 1;
      }
      char effect_arg[64];
      snprintf(effect_arg, sizeof(effect_arg), "%s", effect_value);
      if (strcmp(effect_arg, "list") == 0) {
        for (size_t effect = 0; effect < effect_count; effect++) {
          if (effect_table[effect]->kind_count > 0) {
            printf("%s (kinds 0-%d)\n", effect_table[effect]->name,
                   effect_table[effect]->kind_count - 1);
          } else {
            printf("%s\n", effect_table[effect]->name);
          }
        }
        return 0;
      }
      char *kind_separator = strchr(effect_arg, ':');
      if (kind_separator) *kind_separator = '\0';
      forced_effect = find_effect(effect_arg);
      if (!forced_effect) {
        printf("Unknown effect -- %s\n", effect_arg);
        printf("Try -effect list for the available effects.\n");
        return 1;
      }
      if (kind_separator) {
        char *kind_end = NULL;
        long requested = strtol(kind_separator + 1, &kind_end, 10);
        if (kind_end == kind_separator + 1 || *kind_end != '\0' || requested < 0 ||
            requested >= forced_effect->kind_count) {
          if (forced_effect->kind_count > 0) {
            printf("Effect %s takes a kind from 0 to %d\n", forced_effect->name,
                   forced_effect->kind_count - 1);
          } else {
            printf("Effect %s has no kinds\n", forced_effect->name);
          }
          return 1;
        }
        forced_kind = (int)requested;
      }
    } else {
      fprintf(stderr, "Invalid option -- %s\n", argv[i]);
      fprintf(stderr, "Try --help for more information.\n");
      return 1;
    }
  }

  // Each launch starts at a different point of the season/weather/effect
  // schedule. -effect starts at 0 so the chosen effect begins straight away.
  if (forced_effect)
    scene_offset = 0;
  else if (seed_given)
    scene_offset = scene_offset_from_seed(scene_seed);
  else
    scene_offset = random_scene_offset();

  if (wid == 0) {
    if ((wid_env = getenv("XSCREENSAVER_WINDOW")) != NULL) {
      wid = (Uint32)strtol(wid_env, NULL, 0);
    }
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    return 1;
  }
  atexit(SDL_Quit);

  if (!TTF_Init()) {
    fprintf(stderr, "Unable to init SDL_ttf: %s\n", SDL_GetError());
    return 1;
  }
  atexit(TTF_Quit);

  SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, TITLE);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, !fullscreen && wid == 0);
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
  if (fullscreen && wid == 0) {
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);
  }
  if (wid != 0) {
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER, (Sint64)wid);
  }
  window = SDL_CreateWindowWithProperties(props);
  SDL_DestroyProperties(props);
  if (!window) {
    fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
    return 1;
  }

  renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    return 1;
  }
  SDL_SetRenderVSync(renderer, 1);
  screen_target.renderer = renderer;

  if (fullscreen || wid) {
    SDL_HideCursor();
  }

  user_event_type = SDL_RegisterEvents(1);
  if (user_event_type == 0) {
    fprintf(stderr, "Unable to register timer event: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 1;
  }

  layout_clock(display_scale_factor);
  render_clock(20, 19, false);

  // main loop
  bool done = false;
  SDL_Event event;
  SDL_TimerID timer = SDL_AddTimer(AMBIENT_FRAME_INTERVAL, update_time, NULL);
  if (!timer) {
    fprintf(stderr, "Unable to start clock timer: %s\n", SDL_GetError());
    destroy_clock_caches();
    if (backdrop_base) SDL_DestroyTexture(backdrop_base);
    if (date_texture_cache) SDL_DestroyTexture(date_texture_cache);
    if (ship_target_tex) SDL_DestroyTexture(ship_target_tex);
    TTF_CloseFont(font_time);
    TTF_CloseFont(font_mode);
    TTF_CloseFont(font_date);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 1;
  }

  float mouse_x = -1.0f;
  float mouse_y = -1.0f;

  while (!done && SDL_WaitEvent(&event)) {
    if (event.type == user_event_type) {
      time_t rawtime;
      struct tm current_time;
      time(&rawtime);
      localtime_r(&rawtime, &current_time);
      bool time_changed = current_time.tm_hour != past_h || current_time.tm_min != past_m;
      if (time_changed) {
        render_animation();
      } else {
        render_clock(20, 19, true);
      }
      continue;
    }
    switch (event.type) {
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      case SDL_EVENT_WINDOW_RESIZED:
        layout_clock(display_scale_factor);
        render_clock(20, 19, true);
        break;

      case SDL_EVENT_KEY_DOWN:
        if (anykeyclose) {
          done = true;
          break;
        }
        switch (event.key.key) {
          case SDLK_ESCAPE:
          case SDLK_Q:
            done = true;
            break;
          default:
            break;
        }
        break;

      case SDL_EVENT_MOUSE_MOTION:
        if ((mouse_x < 0.0f) || (mouse_y < 0.0f)) {
          mouse_x = event.motion.x;
          mouse_y = event.motion.y;
        }
        if (((mouse_x != event.motion.x) || (mouse_y != event.motion.y)) && anykeyclose)
          done = true;
        break;

      case SDL_EVENT_QUIT:
        done = true;
        break;
    }
  }

  SDL_RemoveTimer(timer);

  destroy_clock_caches();
  if (backdrop_base) SDL_DestroyTexture(backdrop_base);
  if (ship_target_tex) SDL_DestroyTexture(ship_target_tex);
  if (date_texture_cache) SDL_DestroyTexture(date_texture_cache);

  TTF_CloseFont(font_time);
  TTF_CloseFont(font_mode);
  TTF_CloseFont(font_date);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  return 0;
}
