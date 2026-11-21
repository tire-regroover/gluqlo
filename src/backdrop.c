#include "backdrop.h"
#include "gfx_primitives.h"
#include "scene_core.h"
#include "ships_battles.h"
#include "hill_events.h"
#include "sky_events.h"

SDL_Texture *backdrop_base = NULL;

static const int MAX_RAIN_DROPS = 1200;
static int backdrop_base_w = 0, backdrop_base_h = 0;

void invalidate_backdrop(void) {
  if (backdrop_base) {
    SDL_DestroyTexture(backdrop_base);
    backdrop_base = NULL;
  }
}

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
