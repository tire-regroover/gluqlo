#include "clock_render.h"
#include "gfx_primitives.h"
#include "scene_core.h"
#include "backdrop.h"
#include "sky_events.h"
#include <fontconfig/fontconfig.h>
#include <time.h>
#include <unistd.h>

#ifndef FONT
#define FONT "gluqlo.ttf"
#endif

static char resolved_clock_font[1024] = "";
static char resolved_date_font[1024] = "";

static int date_y = 0;
SDL_Texture *date_texture_cache = NULL;
static int date_texture_w = 0, date_texture_h = 0;
static int date_surface_yday = -1;
static int past_yday = -1;
static int burnin_x = 0, burnin_y = 0;

SDL_Rect hourBackground;
SDL_Rect minBackground;
static SDL_Rect bgrect;
static SDL_Texture *bg_texture = NULL;
static SDL_Texture *ampm_textures[2] = {NULL, NULL};
static int ampm_w[2] = {0, 0}, ampm_h[2] = {0, 0};

static SDL_Texture *digit_glyph_tex[10] = {NULL};
static int digit_glyph_w[10] = {0};
static int digit_glyph_h[10] = {0};
static int digit_glyph_y_off[10] = {0};

static SDL_Texture *card_cache_tex[2] = {NULL, NULL};
static char card_cache_digits[2][3] = {"", ""};
static SDL_Texture *flip_card_tex = NULL;

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


static SDL_Surface *render_date_surface_with_spacing(TTF_Font *font, const char *text,
                                                   SDL_Color color) {
  if (!font || !text || !*text) return NULL;
  int font_h = TTF_GetFontHeight(font);
  int extra_space = (int)(font_h * 0.10f);
  if (extra_space < 2) extra_space = 2;

  Uint32 codepoints[128];
  int n_cp = 0;
  const char *p = text;
  size_t len = strlen(text);
  while (len > 0 && n_cp < (int)(sizeof(codepoints) / sizeof(codepoints[0]))) {
    Uint32 cp = SDL_StepUTF8(&p, &len);
    if (cp == 0) break;
    codepoints[n_cp++] = cp;
  }
  if (n_cp == 0) return NULL;

  int total_w = 0;
  int max_h = font_h;
  for (int i = 0; i < n_cp; i++) {
    int adv = 0;
    TTF_GetGlyphMetrics(font, codepoints[i], NULL, NULL, NULL, NULL, &adv);
    int kerning = 0;
    if (i > 0) TTF_GetGlyphKerning(font, codepoints[i - 1], codepoints[i], &kerning);
    total_w += adv + kerning;
    if (i + 1 < n_cp) total_w += extra_space;
  }
  total_w += 1;  // room for subtle 1px weight-reinforcement pass
  if (total_w < 1) total_w = 1;
  if (max_h < 1) max_h = 1;

  SDL_Surface *dst = SDL_CreateSurface(total_w, max_h, SDL_PIXELFORMAT_ARGB8888);
  if (!dst) return NULL;
  SDL_FillSurfaceRect(dst, NULL, 0);

  bool is_light_weight = TTF_GetFontWeight(font) < TTF_FONT_WEIGHT_MEDIUM;
  int pen_x = 0;
  for (int i = 0; i < n_cp; i++) {
    int kerning = 0;
    if (i > 0) TTF_GetGlyphKerning(font, codepoints[i - 1], codepoints[i], &kerning);
    pen_x += kerning;

    int adv = 0;
    TTF_GetGlyphMetrics(font, codepoints[i], NULL, NULL, NULL, NULL, &adv);
    SDL_Surface *glyph = TTF_RenderGlyph_Blended(font, codepoints[i], color);
    if (glyph) {
      int blit_h = glyph->h > max_h ? max_h : glyph->h;
      SDL_Rect src = {0, 0, glyph->w, blit_h};
      SDL_Rect drect = {pen_x, 0, glyph->w, blit_h};
      SDL_SetSurfaceBlendMode(glyph, SDL_BLENDMODE_BLEND);
      SDL_BlitSurface(glyph, &src, dst, &drect);
      if (is_light_weight) {
        // Thicken Book/Regular glyphs: full 1px horizontal pass, moderate 1px
        // vertical pass, and a diagonal fill pass for balanced stroke weight.
        SDL_SetSurfaceAlphaMod(glyph, 255);
        SDL_Rect drect_x = {pen_x + 1, 0, glyph->w, blit_h};
        SDL_BlitSurface(glyph, &src, dst, &drect_x);

        int blit_h_y = glyph->h > max_h - 1 ? max_h - 1 : glyph->h;
        if (blit_h_y > 0) {
          SDL_Rect src_y = {0, 0, glyph->w, blit_h_y};
          SDL_SetSurfaceAlphaMod(glyph, 160);
          SDL_Rect drect_y = {pen_x, 1, glyph->w, blit_h_y};
          SDL_BlitSurface(glyph, &src_y, dst, &drect_y);

          SDL_SetSurfaceAlphaMod(glyph, 130);
          SDL_Rect drect_xy = {pen_x + 1, 1, glyph->w, blit_h_y};
          SDL_BlitSurface(glyph, &src_y, dst, &drect_xy);
        }
      }
      SDL_DestroySurface(glyph);
    }
    pen_x += adv + extra_space;
  }
  return dst;
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
    SDL_Surface *new_date_surface = render_date_surface_with_spacing(font_date, date, DATE_COLOR);
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


void destroy_clock_caches(void) {
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

  static const char *fc_patterns[] = {
      "sans:weight=medium:lang=en", "sans:weight=semibold:lang=en",
      "sans:lang=en",               "sans-serif:weight=medium",
      "sans-serif",                 "DejaVu Sans",
      "Liberation Sans",            "Noto Sans"};
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
  char local_asset_font[1024] = "";
  if (base_path) {
    snprintf(local_font, sizeof(local_font), "%sgluqlo.ttf", base_path);
    snprintf(local_asset_font, sizeof(local_asset_font), "%sassets/gluqlo.ttf", base_path);
  }

  const char *clock_font_paths[] = {
      FONT,
      local_font[0] ? local_font : NULL,
      local_asset_font[0] ? local_asset_font : NULL,
      "./gluqlo.ttf",
      "./assets/gluqlo.ttf",
  };
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


void layout_clock(double display_scale_factor) {
  int out_w = width, out_h = height;
  SDL_GetCurrentRenderOutputSize(renderer, &out_w, &out_h);
  if (out_w < 1) out_w = 1;
  if (out_h < 1) out_h = 1;
  screen->w = out_w;
  screen->h = out_h;

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
  float date_font_size = (float)((is_horizontal ? scaled_h : scaled_w) / 16.0);
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
