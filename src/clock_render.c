#include "clock_render.h"
#include "gfx_primitives.h"
#include "scene_core.h"
#include "backdrop.h"
#include "sky_events.h"
#include <fontconfig/fontconfig.h>
#include <limits.h>
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
static float digit_glyph_opt_x[10] = {0};
static float digit_glyph_left_void[10] = {0};
static float digit_glyph_right_void[10] = {0};
#define DIGIT_PROFILE_BINS 64
static float digit_left_profile[10][DIGIT_PROFILE_BINS];
static float digit_right_profile[10][DIGIT_PROFILE_BINS];
static bool digit_profile_valid[10] = {false};
static int digit_pair_gap_cache[10][10];
static int digit_pair_gap_nom = -1;

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

      // Measure optical center of mass and left/right edge profiles for
      // typographic centering and pair kerning (works for any font).
      double sum_a = 0.0, sum_ax = 0.0;
      double left_void_sum = 0.0, right_void_sum = 0.0;
      for (int b = 0; b < DIGIT_PROFILE_BINS; b++) {
        digit_left_profile[idx][b] = (float)crop_w;
        digit_right_profile[idx][b] = (float)crop_w;
      }
      if (fmt && SDL_LockSurface(surf)) {
        for (int y = 0; y < crop_h; y++) {
          const Uint8 *row = (const Uint8 *)surf->pixels + (y + min_y) * surf->pitch;
          int lx = crop_w, rx = -1;
          for (int x = 0; x < crop_w; x++) {
            Uint32 px;
            memcpy(&px, row + (x + min_x) * fmt->bytes_per_pixel, fmt->bytes_per_pixel);
            Uint8 r, g, b, a;
            SDL_GetRGBA(px, fmt, NULL, &r, &g, &b, &a);
            if (a >= 128) {
              if (lx == crop_w) lx = x;
              rx = x;
            }
            if (a > 0) {
              sum_a += a;
              sum_ax += a * (x + 0.5);
            }
          }
          if (lx < crop_w && rx >= 0) {
            float l_dist = (float)lx;
            float r_dist = (float)(crop_w - 1 - rx);
            left_void_sum += l_dist;
            right_void_sum += r_dist;
            int bin = (y * DIGIT_PROFILE_BINS) / crop_h;
            if (bin >= DIGIT_PROFILE_BINS) bin = DIGIT_PROFILE_BINS - 1;
            if (l_dist < digit_left_profile[idx][bin]) digit_left_profile[idx][bin] = l_dist;
            if (r_dist < digit_right_profile[idx][bin]) digit_right_profile[idx][bin] = r_dist;
          }
        }
        SDL_UnlockSurface(surf);
      }
      float left_void = (float)(left_void_sum / crop_h);
      float right_void = (float)(right_void_sum / crop_h);
      float com_shift = sum_a > 0.0 ? (float)(crop_w * 0.5 - sum_ax / sum_a) : 0.0f;
      float sb_shift = (right_void - left_void) * 0.5f;
      digit_glyph_left_void[idx] = left_void;
      digit_glyph_right_void[idx] = right_void;
      digit_glyph_opt_x[idx] = 0.5f * com_shift + 0.5f * sb_shift;
      digit_profile_valid[idx] = true;

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
      digit_glyph_opt_x[idx] = 0.0f;
      digit_glyph_left_void[idx] = 0.0f;
      digit_glyph_right_void[idx] = 0.0f;
      digit_profile_valid[idx] = false;
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


static double pair_optical_separation(int a, int b, double gap) {
  const int vpad = 4;  // +/- 4 of 64 bins (~6% of glyph height) for diagonal proximity
  double sum = 0.0;
  int cnt = 0;
  for (int i = 0; i < DIGIT_PROFILE_BINS; i++) {
    double best = 1e9;
    int lo = i - vpad < 0 ? 0 : i - vpad;
    int hi = i + vpad >= DIGIT_PROFILE_BINS ? DIGIT_PROFILE_BINS - 1 : i + vpad;
    for (int j = lo; j <= hi; j++) {
      float ra = digit_right_profile[a][j];
      float lb = digit_left_profile[b][j];
      if (ra < (float)digit_glyph_w[a] && lb < (float)digit_glyph_w[b]) {
        double d = ra + gap + lb;
        if (d < best) best = d;
      }
    }
    if (best < 1e8) {
      if (best < 1.0) best = 1.0;
      sum += 1.0 / (best * best);
      cnt++;
    }
  }
  return cnt > 0 ? sqrt(cnt / sum) : gap;
}


static int get_digit_pair_gap(int a, int b, int nominal_gap) {
  if (a < 0 || a > 9 || b < 0 || b > 9) return nominal_gap;
  if (!digit_profile_valid[a] || !digit_profile_valid[b]) return nominal_gap;

  // Ensure digit '0' is loaded as the optical spacing reference ("00").
  if (!digit_profile_valid[0]) {
    int dummy_w, dummy_h;
    get_digit_glyph_texture('0', &dummy_w, &dummy_h, NULL);
  }
  if (!digit_profile_valid[0]) return nominal_gap;

  if (digit_pair_gap_nom != nominal_gap) {
    for (int i = 0; i < 10; i++)
      for (int j = 0; j < 10; j++) digit_pair_gap_cache[i][j] = INT_MIN;
    digit_pair_gap_nom = nominal_gap;
  }
  if (digit_pair_gap_cache[a][b] != INT_MIN) return digit_pair_gap_cache[a][b];

  double target = pair_optical_separation(0, 0, (double)nominal_gap);
  const int vpad = 4;
  double min_touch = -1e9;
  for (int i = 0; i < DIGIT_PROFILE_BINS; i++) {
    int lo = i - vpad < 0 ? 0 : i - vpad;
    int hi = i + vpad >= DIGIT_PROFILE_BINS ? DIGIT_PROFILE_BINS - 1 : i + vpad;
    for (int j = lo; j <= hi; j++) {
      float ra = digit_right_profile[a][j];
      float lb = digit_left_profile[b][j];
      if (ra < (float)digit_glyph_w[a] && lb < (float)digit_glyph_w[b]) {
        double touch = -(double)(ra + lb);
        if (touch > min_touch) min_touch = touch;
      }
    }
  }
  double min_gap = min_touch + nominal_gap * 0.90;
  double max_tuck = -nominal_gap * 1.4;
  if (min_gap < max_tuck) min_gap = max_tuck;

  double lo = min_gap, hi = nominal_gap * 2.0;
  for (int it = 0; it < 18; it++) {
    double mid = 0.5 * (lo + hi);
    if (pair_optical_separation(a, b, mid) < target)
      lo = mid;
    else
      hi = mid;
  }
  int best = (int)lround(0.5 * (lo + hi));
  if (best < (int)ceil(min_gap)) best = (int)ceil(min_gap);
  digit_pair_gap_cache[a][b] = best;
  return best;
}


void blit_digits(RenderTarget *surface, SDL_Rect *rect, int spc, char digits[], Uint8 mod_r,
                 Uint8 mod_g, Uint8 mod_b) {
  if (digits[1]) {
    int nom_gap = spc + spc / 2;
    int w0 = 0, h0 = 0, y0 = 0, w1 = 0, h1 = 0, y1 = 0;
    SDL_Texture *t0 = get_digit_glyph_texture(digits[0], &w0, &h0, &y0);
    SDL_Texture *t1 = get_digit_glyph_texture(digits[1], &w1, &h1, &y1);
    if (!t0 || !t1) return;
    int d0 = digits[0] - '0';
    int d1 = digits[1] - '0';
    int gap = get_digit_pair_gap(d0, d1, nom_gap);
    int total_width = w0 + gap + w1;
    // Balance outer optical sidebearings across the card while clamping to
    // keep comfortable margins around protruding strokes (e.g. '1' beak + AM).
    float opt_shift = (digit_glyph_right_void[d1] - digit_glyph_left_void[d0]) * 0.35f;
    float x = (float)rect->x + (rect->w - total_width) * 0.5f + opt_shift;
    float min_margin = rect->w * 0.055f;
    if (x < rect->x + min_margin) x = rect->x + min_margin;
    if (x + total_width > rect->x + rect->w - min_margin)
      x = rect->x + rect->w - min_margin - total_width;
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
    int d0 = digits[0] - '0';
    float opt_shift = digit_glyph_opt_x[d0];
    SDL_FRect dst0 = {(float)rect->x + (rect->w - w0) * 0.5f + opt_shift,
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


typedef struct {
  Uint32 cp;
  SDL_Surface *surf;
  int w, h;
  int adv;
  int bmin_x, bmax_x;
  int *left_edge;
  int *right_edge;
} DateGlyph;

static DateGlyph render_date_glyph(TTF_Font *font, Uint32 cp, SDL_Color color,
                                   bool is_light_weight) {
  DateGlyph g = {cp, NULL, 0, 0, 0, 0, -1, NULL, NULL};
  TTF_GetGlyphMetrics(font, cp, NULL, NULL, NULL, NULL, &g.adv);
  SDL_Surface *raw = TTF_RenderGlyph_Blended(font, cp, color);
  if (!raw) return g;

  if (is_light_weight) {
    // Thicken Book/Regular glyphs by ~1.33x relative to previous pass:
    // full 1px + 0.33px horizontal, full 1px vertical, and diagonal fill.
    int tw = raw->w + 2;
    int th = raw->h + 1;
    SDL_Surface *thick = SDL_CreateSurface(tw, th, SDL_PIXELFORMAT_ARGB8888);
    if (thick) {
      SDL_FillSurfaceRect(thick, NULL, 0);
      SDL_SetSurfaceBlendMode(raw, SDL_BLENDMODE_BLEND);
      SDL_Rect r0 = {0, 0, raw->w, raw->h};
      SDL_BlitSurface(raw, NULL, thick, &r0);

      SDL_SetSurfaceAlphaMod(raw, 255);
      SDL_Rect rx = {1, 0, raw->w, raw->h};
      SDL_BlitSurface(raw, NULL, thick, &rx);

      SDL_SetSurfaceAlphaMod(raw, 85);
      SDL_Rect rx2 = {2, 0, raw->w, raw->h};
      SDL_BlitSurface(raw, NULL, thick, &rx2);

      SDL_SetSurfaceAlphaMod(raw, 255);
      SDL_Rect ry = {0, 1, raw->w, raw->h};
      SDL_BlitSurface(raw, NULL, thick, &ry);

      SDL_SetSurfaceAlphaMod(raw, 210);
      SDL_Rect rxy = {1, 1, raw->w, raw->h};
      SDL_BlitSurface(raw, NULL, thick, &rxy);

      SDL_SetSurfaceAlphaMod(raw, 60);
      SDL_Rect rx2y = {2, 1, raw->w, raw->h};
      SDL_BlitSurface(raw, NULL, thick, &rx2y);

      SDL_DestroySurface(raw);
      g.surf = thick;
    } else {
      g.surf = raw;
    }
  } else {
    g.surf = raw;
  }

  g.w = g.surf->w;
  g.h = g.surf->h;
  g.bmin_x = g.w;
  g.bmax_x = -1;
  g.left_edge = (int *)malloc((size_t)g.h * sizeof(int));
  g.right_edge = (int *)malloc((size_t)g.h * sizeof(int));
  if (!g.left_edge || !g.right_edge) {
    free(g.left_edge);
    free(g.right_edge);
    g.left_edge = NULL;
    g.right_edge = NULL;
    return g;
  }
  for (int y = 0; y < g.h; y++) {
    g.left_edge[y] = g.w;
    g.right_edge[y] = -1;
  }

  const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(g.surf->format);
  if (fmt && SDL_LockSurface(g.surf)) {
    for (int y = 0; y < g.h; y++) {
      const Uint8 *row = (const Uint8 *)g.surf->pixels + y * g.surf->pitch;
      for (int x = 0; x < g.w; x++) {
        Uint32 px;
        memcpy(&px, row + x * fmt->bytes_per_pixel, fmt->bytes_per_pixel);
        Uint8 r, gr, b, a;
        SDL_GetRGBA(px, fmt, NULL, &r, &gr, &b, &a);
        if (a >= 96) {
          if (g.left_edge[y] == g.w) g.left_edge[y] = x;
          g.right_edge[y] = x;
          if (x < g.bmin_x) g.bmin_x = x;
          if (x > g.bmax_x) g.bmax_x = x;
        }
      }
    }
    SDL_UnlockSurface(g.surf);
  }
  return g;
}

static void free_date_glyph(DateGlyph *g) {
  if (g->surf) SDL_DestroySurface(g->surf);
  free(g->left_edge);
  free(g->right_edge);
  g->surf = NULL;
  g->left_edge = NULL;
  g->right_edge = NULL;
}

static double date_pair_optical_sep(const DateGlyph *a, const DateGlyph *b, double dx, int vpad) {
  if (!a->right_edge || !b->left_edge) return -1.0;
  int h = a->h < b->h ? a->h : b->h;
  double sum = 0.0;
  int cnt = 0;
  for (int y = 0; y < h; y++) {
    double best = 1e9;
    int lo = y - vpad < 0 ? 0 : y - vpad;
    int hi = y + vpad >= h ? h - 1 : y + vpad;
    for (int yy = lo; yy <= hi; yy++) {
      if (a->right_edge[yy] >= 0 && b->left_edge[yy] < b->w) {
        double d = (dx + b->left_edge[yy]) - a->right_edge[yy];
        if (d < best) best = d;
      }
    }
    if (best < 1e8) {
      if (best < 1.0) best = 1.0;
      sum += 1.0 / (best * best);
      cnt++;
    }
  }
  return cnt > 0 ? sqrt(cnt / sum) : -1.0;
}

static SDL_Surface *render_date_surface_with_spacing(TTF_Font *font, const char *text,
                                                   SDL_Color color) {
  if (!font || !text || !*text) return NULL;
  int font_h = TTF_GetFontHeight(font);
  if (font_h < 1) font_h = 1;
  bool is_light_weight = TTF_GetFontWeight(font) < TTF_FONT_WEIGHT_MEDIUM;

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

  double tracking = font_h * 0.085;
  if (tracking < 2.0) tracking = 2.0;
  int vpad = font_h / 16;
  if (vpad < 2) vpad = 2;
  double min_clear = tracking * 0.72;
  if (min_clear < 1.5) min_clear = 1.5;

  // Use "nn" at target tracking as the optical spacing reference.
  DateGlyph ref_n = render_date_glyph(font, 'n', color, is_light_weight);
  double ref_w = (ref_n.bmax_x >= ref_n.bmin_x) ? (double)(ref_n.bmax_x - ref_n.bmin_x + 1)
                                                : (double)ref_n.adv;
  double target_sep = date_pair_optical_sep(&ref_n, &ref_n, ref_w + tracking, vpad);
  if (target_sep < 1.0) target_sep = tracking;
  free_date_glyph(&ref_n);

  DateGlyph glyphs[128];
  double pos_x[128];
  for (int i = 0; i < n_cp; i++) {
    glyphs[i] = render_date_glyph(font, codepoints[i], color, is_light_weight);
    pos_x[i] = 0.0;
  }

  for (int i = 0; i < n_cp; i++) {
    if (i == 0) {
      pos_x[i] = (glyphs[i].bmax_x >= glyphs[i].bmin_x) ? -(double)glyphs[i].bmin_x : 0.0;
      continue;
    }
    const DateGlyph *prev = &glyphs[i - 1];
    const DateGlyph *curr = &glyphs[i];

    if (curr->cp == ' ') {
      double prev_right = (prev->bmax_x >= 0) ? (pos_x[i - 1] + prev->bmax_x + 1)
                                              : (pos_x[i - 1] + prev->adv);
      double word_gap = curr->adv * 0.95;
      if (word_gap < font_h * 0.28) word_gap = font_h * 0.28;
      pos_x[i] = prev_right + word_gap;
      continue;
    }
    if (prev->cp == ' ') {
      pos_x[i] = pos_x[i - 1] - ((curr->bmax_x >= curr->bmin_x) ? (double)curr->bmin_x : 0.0);
      continue;
    }

    if (curr->cp == ',' && prev->right_edge && curr->bmax_x >= curr->bmin_x) {
      // Tuck comma tightly to the lower-right contour of the preceding glyph.
      int y_start = (prev->h * 55) / 100;
      int low_max = -1;
      for (int y = y_start; y < prev->h; y++) {
        if (prev->right_edge[y] > low_max) low_max = prev->right_edge[y];
      }
      double prev_right_low = low_max >= 0 ? (double)low_max : (double)prev->bmax_x;
      double comma_gap = tracking * 0.45;
      if (comma_gap < 1.5) comma_gap = 1.5;
      pos_x[i] = pos_x[i - 1] + prev_right_low + 1.0 + comma_gap - curr->bmin_x;
      continue;
    }

    // Solve horizontal step optically against target_sep with collision guard.
    int h = prev->h < curr->h ? prev->h : curr->h;
    double min_dx = -1e9;
    if (prev->right_edge && curr->left_edge) {
      for (int y = 0; y < h; y++) {
        int lo = y - vpad < 0 ? 0 : y - vpad;
        int hi = y + vpad >= h ? h - 1 : y + vpad;
        for (int yy = lo; yy <= hi; yy++) {
          if (prev->right_edge[yy] >= 0 && curr->left_edge[yy] < curr->w) {
            double need = (double)prev->right_edge[yy] - curr->left_edge[yy] + min_clear;
            if (need > min_dx) min_dx = need;
          }
        }
      }
    }
    if (min_dx < -1e8) min_dx = prev->adv;

    double max_dx = (prev->bmax_x >= 0 && curr->bmax_x >= curr->bmin_x)
                        ? (double)(prev->bmax_x - curr->bmin_x + 1) + tracking * 1.35
                        : (double)prev->adv + tracking;
    if (max_dx < min_dx) max_dx = min_dx;

    double lo = min_dx, hi = max_dx;
    for (int it = 0; it < 16; it++) {
      double mid = 0.5 * (lo + hi);
      double sep = date_pair_optical_sep(prev, curr, mid, vpad);
      if (sep < 0.0 || sep < target_sep)
        lo = mid;
      else
        hi = mid;
    }
    pos_x[i] = pos_x[i - 1] + 0.5 * (lo + hi);
  }

  double max_right = 1.0;
  for (int i = 0; i < n_cp; i++) {
    if (glyphs[i].bmax_x >= 0) {
      double r = pos_x[i] + glyphs[i].w;
      if (r > max_right) max_right = r;
    }
  }
  int total_w = (int)ceil(max_right) + 2;
  int max_h = font_h + (is_light_weight ? 1 : 0);
  if (total_w < 1) total_w = 1;
  if (max_h < 1) max_h = 1;

  SDL_Surface *dst = SDL_CreateSurface(total_w, max_h, SDL_PIXELFORMAT_ARGB8888);
  if (dst) {
    SDL_FillSurfaceRect(dst, NULL, 0);
    for (int i = 0; i < n_cp; i++) {
      if (glyphs[i].surf && glyphs[i].cp != ' ') {
        int blit_h = glyphs[i].h > max_h ? max_h : glyphs[i].h;
        SDL_Rect src = {0, 0, glyphs[i].w, blit_h};
        SDL_Rect drect = {(int)lround(pos_x[i]), 0, glyphs[i].w, blit_h};
        SDL_SetSurfaceBlendMode(glyphs[i].surf, SDL_BLENDMODE_BLEND);
        SDL_BlitSurface(glyphs[i].surf, &src, dst, &drect);
      }
    }
  }
  for (int i = 0; i < n_cp; i++) free_date_glyph(&glyphs[i]);
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
    digit_glyph_opt_x[i] = 0.0f;
    digit_glyph_left_void[i] = 0.0f;
    digit_glyph_right_void[i] = 0.0f;
    digit_profile_valid[i] = false;
  }
  digit_pair_gap_nom = -1;
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


static bool find_font_via_fontconfig(const char *pattern_str, const char *required_family,
                                     char *out, size_t out_size) {
  if (!FcInit()) return false;
  FcPattern *pat = FcNameParse((const FcChar8 *)pattern_str);
  if (!pat) return false;
  FcConfigSubstitute(NULL, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);
  FcResult result;
  FcPattern *match = FcFontMatch(NULL, pat, &result);
  FcPatternDestroy(pat);
  if (!match) return false;

  bool family_ok = (required_family == NULL);
  if (!family_ok) {
    FcChar8 *matched_family = NULL;
    for (int i = 0; FcPatternGetString(match, FC_FAMILY, i, &matched_family) == FcResultMatch;
         i++) {
      if (matched_family && strcasecmp((const char *)matched_family, required_family) == 0) {
        family_ok = true;
        break;
      }
    }
  }

  FcChar8 *file = NULL;
  bool found = false;
  if (family_ok && FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch && file) {
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

  // Prefer DejaVu Sans Bold (available on virtually all Linux/Arch/Pi systems),
  // then fall back to bold sans-serif and regular sans-serif.
  static const struct {
    const char *family;
    const char *pattern;
  } preferred_fonts[] = {
      {"DejaVu Sans", "DejaVu Sans:weight=bold:lang=en"},
      {"DejaVu Sans", "DejaVu Sans:style=Bold"},
      {"Liberation Sans", "Liberation Sans:weight=bold:lang=en"},
      {"Noto Sans", "Noto Sans:weight=bold:lang=en"},
  };
  for (size_t i = 0; i < sizeof(preferred_fonts) / sizeof(preferred_fonts[0]); i++) {
    char candidate[1024];
    if (find_font_via_fontconfig(preferred_fonts[i].pattern, preferred_fonts[i].family, candidate,
                                 sizeof(candidate)) &&
        font_supports_date_text(candidate)) {
      snprintf(resolved_date_font, sizeof(resolved_date_font), "%s", candidate);
      return resolved_date_font;
    }
  }

  static const char *fc_fallbacks[] = {
      "sans:weight=bold:lang=en", "sans-serif:weight=bold",
      "sans:lang=en",             "sans-serif",
      "DejaVu Sans"};
  for (size_t i = 0; i < sizeof(fc_fallbacks) / sizeof(fc_fallbacks[0]); i++) {
    char candidate[1024];
    if (find_font_via_fontconfig(fc_fallbacks[i], NULL, candidate, sizeof(candidate)) &&
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
  if (find_font_via_fontconfig("Gluqlo", "Gluqlo", fc_gluqlo, sizeof(fc_gluqlo)) &&
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
