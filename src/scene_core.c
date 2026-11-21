#include "scene_core.h"
#include "gfx_primitives.h"
#include <time.h>

double hash_noise(int x, int y, Uint32 seed) {
  Uint32 value = (Uint32)x * 374761393u + (Uint32)y * 668265263u + seed * 2246822519u;
  value = (value ^ (value >> 13)) * 1274126177u;
  value ^= value >> 16;
  return value / 4294967295.0;
}


double smooth_noise(double x, double y, Uint32 seed) {
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


Uint8 blend_channel(Uint8 from, Uint8 to, double amount) {
  if (amount < 0.0) amount = 0.0;
  if (amount > 1.0) amount = 1.0;
  return (Uint8)(from + (to - from) * amount);
}


double smooth_step(double edge0, double edge1, double value) {
  double t = (value - edge0) / (edge1 - edge0);
  if (t < 0.0) t = 0.0;
  if (t > 1.0) t = 1.0;
  return t * t * (3.0 - 2.0 * t);
}


// Height of the three ground ridges as a fraction of the screen height at
// horizontal position nx (0..1). Effects use it to sit things on the ground.
double ridge_height(int layer, double nx) {
  switch (layer) {
    case RIDGE_FAR:
      return 0.68 + (smooth_noise(nx * 3.6, 1.0, 907) - 0.5) * 0.13;
    case RIDGE_MIDDLE:
      return 0.79 + (smooth_noise(nx * 5.2 + 8.0, 2.0, 1013) - 0.5) * 0.10;
    default:
      return 0.88 + (smooth_noise(nx * 8.0 - 3.0, 3.0, 1123) - 0.5) * 0.09;
  }
}

/*
 * Scheduled effects
 *
 * Time is split into slots of `cycle` ms. Each slot gets a seed and may be
 * gated off (it only runs when seed % gate == 0). When a slot is active the
 * effect runs for `duration` ms (plus seeded jitter), either from the start
 * of the slot or at a seeded offset inside it, fading in and out.
 *
 * Effects that share a non-zero `group` never overlap: when their runs
 * collide, the one with the lowest `priority` number plays and the others
 * skip that slot entirely (so nothing pops in half way through). `accept`
 * lets an effect declare that a slot has nothing to show, which keeps empty
 * slots from blocking other effects.
 *
 * Effects call effect_window() and draw only if it returns true. With
 * `-effect NAME[:KIND]` only that effect runs, back to back, ignoring gates.
 *
 * To add an effect: write render_xxx() starting with effect_window(), add an
 * EffectSpec below, add it to effect_table[], and call it from
 * render_backdrop() (or render_clock() for overlays drawn over the cards).
 */

typedef struct {
  Uint32 seed;
  Uint32 duration;
  Uint64 start;  // absolute start time (ms)
} EffectRun;

// Rare sky kinds from RARE_SKY_CLASSIC_KINDS up are the rarer ones: only 1 slot
// in 7 shows them.
static bool rare_sky_accept(Uint32 slot, Uint32 seed) {
  int kind = (seed >> 24) % RARE_SKY_KINDS;
  if (kind < RARE_SKY_CLASSIC_KINDS) return true;
  return (slot * 2654435761u + 0x9e3779b9u) % 7 == 0;
}


// Fullscreen kinds 4 and 5 are deliberate quiet slots (they keep the effect
// rare).
static bool fullscreen_accept(Uint32 slot, Uint32 seed) {
  (void)slot;
  return (seed >> 5) % 6 < 4;
}


static const EffectSpec EFFECT_ALIEN_WEATHER = {
    "alien_weather", 300000, 0,    4, false,          2246822519u, 52000, 18000, 8,
    false,           5000,   7000, 5, GROUP_TAKEOVER, 1,           NULL};
static const EffectSpec EFFECT_WORLD_JUMP = {
    "world_jump", 360000, 0,   5, false,          2654435761u, 7800, 0, 0,
    false,        350,    450, 4, GROUP_TAKEOVER, 0,           NULL};
const EffectSpec EFFECT_LONG_SCENE = {
    "long_scene", 210000, 0,    5, false,          2246822519u, 72000, 28000, 8,
    true,         6000,   7000, 7, GROUP_TAKEOVER, 2,           NULL};
static const EffectSpec EFFECT_FULLSCREEN = {
    "fullscreen",     90000, 1, 0, false, 0, 6500, 4000, 9, true, 1300, 1200, 4, GROUP_TAKEOVER, 3,
    fullscreen_accept};
const EffectSpec EFFECT_FLEET_BATTLE = {"fleet_battle", 240000, 1,    6, false,     0, 60000, 0, 0,
                                        false,          1400,   2200, 6, GROUP_SKY, 0, NULL};
const EffectSpec EFFECT_BACKGROUND_BATTLE = {
    "background_battle", 90000, 1,   9, false, 0, 32000, 0, 0, false, 900, 1700, 5,
    GROUP_SKY,           1,     NULL};
const EffectSpec EFFECT_RARE_SKY = {
    "rare_sky", 48000, 0,   3,   true,           2246822519u, 4200, 3200,
    8,          true,  350, 400, RARE_SKY_KINDS, GROUP_SKY,   2,    rare_sky_accept};
const EffectSpec EFFECT_BATTLE_FLYBY = {"battle_flyby", 60000, 1,    6, false,     0, 14000, 0, 0,
                                        false,          900,   1100, 6, GROUP_SKY, 4, NULL};
const EffectSpec EFFECT_DOGFIGHT = {"dogfight", 75000, 1,   4,    false, 0,         18000, 0,
                                    0,          true,  900, 1100, 6,     GROUP_SKY, 3,     NULL};
const EffectSpec EFFECT_HILL_EVENT = {
    "hill_event",     120000,         0, 2,   false, 2654435761u, 36000, 22000, 8, true, 4000, 5000,
    HILL_EVENT_KINDS, GROUP_TAKEOVER, 4, NULL};
const EffectSpec EFFECT_FOREGROUND = {
    "foreground", 180000, 1,   2, false,      2246822519u, 6500, 0, 0,
    false,        650,    900, 4, GROUP_NONE, 0,           NULL};
const EffectSpec EFFECT_SCREEN_FADE = {"screen_fade", 300000, 1,    4, false,      0, 12000, 0, 0,
                                       true,          1800,   1800, 0, GROUP_NONE, 0, NULL};

const EffectSpec *const effect_table[] = {
    &EFFECT_ALIEN_WEATHER,     &EFFECT_WORLD_JUMP,   &EFFECT_LONG_SCENE,  &EFFECT_RARE_SKY,
    &EFFECT_FULLSCREEN,        &EFFECT_FOREGROUND,   &EFFECT_SCREEN_FADE, &EFFECT_BATTLE_FLYBY,
    &EFFECT_BACKGROUND_BATTLE, &EFFECT_FLEET_BATTLE, &EFFECT_DOGFIGHT,    &EFFECT_HILL_EVENT};
const size_t effect_count = sizeof(effect_table) / sizeof(effect_table[0]);

const EffectSpec *forced_effect = NULL;
int forced_kind = -1;

/*
 * Scene time is the clock every effect, season and weather runs on. It is the
 * SDL tick count plus a per-launch offset, so each launch starts at a
 * different point of the season/weather/effect schedule. -seed N makes the
 * offset reproducible; -effect NAME starts it at 0.
 */
Uint32 scene_offset = 0;
static const Uint32 SCENE_OFFSET_RANGE = 3600000;

Uint32 scene_time(void) { return SDL_GetTicks() + scene_offset; }

Uint32 scene_offset_from_seed(Uint32 seed) {
  seed ^= seed >> 16;
  seed *= 2246822519u;
  seed ^= seed >> 13;
  return seed % SCENE_OFFSET_RANGE;
}


Uint32 random_scene_offset(void) {
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  return scene_offset_from_seed((Uint32)now.tv_nsec ^ ((Uint32)now.tv_sec * 2654435761u));
}


const EffectSpec *find_effect(const char *name) {
  for (size_t i = 0; i < effect_count; i++) {
    if (strcmp(effect_table[i]->name, name) == 0) return effect_table[i];
  }
  return NULL;
}


// Returns `kind`, or the kind requested with -effect NAME:KIND for this effect.
int effect_kind(const EffectSpec *spec, int kind) {
  return (forced_effect == spec && forced_kind >= 0) ? forced_kind : kind;
}


// Slot length used by -effect NAME: back to back with a short pause.
static Uint32 effect_cycle(const EffectSpec *spec, bool forced) {
  return forced ? spec->duration + spec->jitter + 3000 : spec->cycle;
}


// Works out whether `slot` of `spec` runs, and when. Forced runs ignore gating.
static bool effect_run(const EffectSpec *spec, Uint32 slot, bool forced, EffectRun *run) {
  Uint32 cycle = effect_cycle(spec, forced);
  Uint32 seed =
      spec->seed_style ? slot * 2654435761u + 0x9e3779b9u : slot * 2246822519u + 3266489917u;
  if (!forced && spec->gate && !spec->gate_after_mix && seed % spec->gate != 0) return false;
  if (spec->mix_mul) {
    seed ^= seed >> 15;
    seed *= spec->mix_mul;
    seed ^= seed >> 13;
  }
  if (!forced && spec->gate && spec->gate_after_mix && seed % spec->gate != 0) return false;
  if (!forced && spec->accept && !spec->accept(slot, seed)) return false;

  Uint32 duration = spec->duration;
  if (spec->jitter) duration += (seed >> spec->jitter_shift) % spec->jitter;
  Uint32 start = 0;
  if (!forced && spec->random_start && cycle > duration) {
    start = (seed >> 16) % (cycle - duration);
  }
  run->seed = seed;
  run->duration = duration;
  run->start = (Uint64)slot * cycle + start;
  return true;
}


// True if a higher priority effect of the same group runs at any point during
// `run`.
static bool effect_blocked(const EffectSpec *spec, const EffectRun *run) {
  if (spec->group == GROUP_NONE) return false;
  Uint64 end = run->start + run->duration;
  for (size_t i = 0; i < effect_count; i++) {
    const EffectSpec *other = effect_table[i];
    if (other == spec || other->group != spec->group || other->priority >= spec->priority) continue;
    // A run never leaves its own slot, so only the slots the window touches
    // matter.
    Uint64 first_slot = run->start / other->cycle;
    Uint64 last_slot = (end - 1) / other->cycle;
    for (Uint64 slot = first_slot; slot <= last_slot; slot++) {
      EffectRun other_run;
      if (effect_run(other, (Uint32)slot, false, &other_run) && other_run.start < end &&
          other_run.start + other_run.duration > run->start && !effect_blocked(other, &other_run)) {
        return true;
      }
    }
  }
  return false;
}


bool effect_window(const EffectSpec *spec, Uint32 ticks, EffectWindow *window) {
  bool forced = forced_effect == spec;
  if (forced_effect && !forced) return false;

  Uint32 cycle = effect_cycle(spec, forced);
  Uint32 slot = ticks / cycle;
  EffectRun run;
  if (!effect_run(spec, slot, forced, &run)) return false;
  if (ticks < run.start || ticks >= run.start + run.duration) return false;
  if (!forced && effect_blocked(spec, &run)) return false;

  Uint32 phase = (Uint32)(ticks - run.start);
  window->slot = slot;
  window->seed = run.seed;
  window->phase = phase;
  window->duration = run.duration;
  window->progress = (double)phase / run.duration;
  window->fade = smooth_step(0.0, spec->fade_in, phase) *
                 (1.0 - smooth_step(run.duration - spec->fade_out, run.duration, phase));
  return true;
}


// Rain and snow must never fall together. When a change swaps one for the
// other, the old one fades out over the first part of the change, the air is
// dry for a moment, and only then does the new one fade in. Changes that keep
// the same kind of precipitation (light rain to heavy rain) just blend.
static double precipitation_level(const double *previous, const double *current, int channel,
                                  int other_channel, double blend) {
  double from = previous[channel];
  double to = current[channel];
  if (previous[other_channel] <= 0.02 && current[other_channel] <= 0.02) {
    return from + (to - from) * blend;
  }
  return from * (1.0 - smooth_step(0.0, 0.42, blend)) + to * smooth_step(0.58, 1.0, blend);
}


// Weather 0 in weather_levels[] is the orange sunset. It is a treat, so only
// one pick in SUNSET_ONE_IN keeps it; the others become one of the later
// weathers. Raise SUNSET_ONE_IN to see it less often.
#define WEATHER_SUNSET 0
#define SUNSET_ONE_IN 5

static int pick_weather(Uint32 seed, int count) {
  int pick = seed % count;
  if (pick == WEATHER_SUNSET && (seed >> 12) % SUNSET_ONE_IN != 0) {
    pick = 2 + (seed >> 5) % (count - 2);
  }
  return pick;
}


SceneWeather get_scene_weather(Uint32 ticks) {
  static const double weather_levels[][11] = {
      {0.10, 0.00, 0.00, 0.95, 0.02, 0.08, 0.00, 0.00, 255, 173, 45},
      {0.25, 0.00, 0.00, 0.72, 0.12, 0.15, 0.00, 0.00, 198, 47, 28},
      {0.78, 0.00, 0.00, 0.10, 0.20, 0.18, 0.00, 0.00, 67, 92, 116},
      {0.74, 0.36, 0.00, 0.12, 0.26, 0.34, 0.00, 0.00, 67, 112, 139},
      {0.90, 0.52, 0.00, 0.02, 0.14, 0.38, 0.00, 0.00, 54, 104, 137},
      {1.00, 0.92, 0.00, 0.00, 0.04, 0.95, 0.90, 0.00, 73, 80, 112},
      {0.72, 0.00, 0.72, 0.12, 0.24, 0.40, 0.00, 0.00, 145, 171, 192},
      {0.97, 0.00, 1.00, 0.00, 0.54, 0.88, 0.04, 0.00, 176, 197, 211},
      {0.36, 0.00, 0.12, 0.00, 0.13, 0.25, 0.00, 0.90, 62, 132, 142},
      {0.88, 0.08, 0.00, 0.15, 1.00, 0.05, 0.00, 0.00, 132, 156, 155}};
  const Uint32 season_duration = 300000;
  const Uint32 season_transition = 20000;
  const Uint32 weather_duration = 65000;
  const Uint32 weather_transition = 12000;
  const int weather_count = sizeof(weather_levels) / sizeof(weather_levels[0]);
  SceneWeather scene = {{0.0, 0.0, 0.0, 0.0}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, {0, 0, 0}};

  Uint32 season_elapsed = ticks % (season_duration * 4);
  int season = season_elapsed / season_duration;
  Uint32 season_phase = season_elapsed % season_duration;
  double season_blend =
      smooth_step(season_duration - season_transition, season_duration, season_phase);
  scene.season[season] = 1.0 - season_blend;
  scene.season[(season + 1) % 4] = season_blend;

  Uint32 weather_slot = ticks / weather_duration;
  Uint32 current_seed = weather_slot * 2654435761u + 0x9e3779b9u;
  Uint32 previous_seed = (weather_slot - 1) * 2246822519u + 3266489917u;
  int current = pick_weather(current_seed, weather_count);
  int previous = pick_weather(previous_seed, weather_count);
  if (previous == current) {
    // Any other weather, but never the sunset (that would bypass its rarity).
    int other = 1 + (current_seed >> 8) % (weather_count - 1);
    if (other == current) other = other % (weather_count - 1) + 1;
    previous = other;
  }
  Uint32 weather_phase = ticks % weather_duration;
  double weather_blend = smooth_step(0.0, weather_transition, weather_phase);
  scene.cloud = weather_levels[previous][0] +
                (weather_levels[current][0] - weather_levels[previous][0]) * weather_blend;
  scene.rain =
      precipitation_level(weather_levels[previous], weather_levels[current], 1, 2, weather_blend);
  scene.snow =
      precipitation_level(weather_levels[previous], weather_levels[current], 2, 1, weather_blend);
  scene.sunshine = weather_levels[previous][3] +
                   (weather_levels[current][3] - weather_levels[previous][3]) * weather_blend;
  scene.fog = weather_levels[previous][4] +
              (weather_levels[current][4] - weather_levels[previous][4]) * weather_blend;
  scene.wind = weather_levels[previous][5] +
               (weather_levels[current][5] - weather_levels[previous][5]) * weather_blend;
  scene.lightning = weather_levels[previous][6] +
                    (weather_levels[current][6] - weather_levels[previous][6]) * weather_blend;
  scene.aurora = weather_levels[previous][7] +
                 (weather_levels[current][7] - weather_levels[previous][7]) * weather_blend;
  for (int channel = 0; channel < 3; channel++) {
    scene.palette[channel] =
        weather_levels[previous][8 + channel] +
        (weather_levels[current][8 + channel] - weather_levels[previous][8 + channel]) *
            weather_blend;
  }
  return scene;
}


// Three drifting aurora ribbons; `strength` is 0..1. Used by the aurora weather
// and by the fullscreen aurora event.
static void draw_aurora(RenderTarget *surface, Uint32 ticks, int scale, double strength) {
  if (strength <= 0.01) return;
  for (int band = 0; band < 3; band++) {
    int previous_x = 0;
    int previous_y = surface->h * (19 + band * 12) / 100;
    for (int segment = 1; segment <= 24; segment++) {
      int x = surface->w * segment / 24;
      double nx = (double)x / surface->w;
      int y = surface->h * (19 + band * 12) / 100 +
              (int)(sin(nx * 6.28318530718 + ticks / (1100.0 + band * 270) + band * 1.9) * scale *
                    (0.03 + band * 0.008));
      Uint8 alpha = (Uint8)(strength * (28 - band * 5));
      thickLineRGBA(surface, previous_x, previous_y, x, y, 2, (Uint8)(70 + band * 24),
                    (Uint8)(164 + band * 18), (Uint8)(137 + band * 30), alpha);
      previous_x = x;
      previous_y = y;
    }
  }
}


void render_weather_effects(RenderTarget *surface, Uint32 ticks, const SceneWeather *weather,
                            int scale) {
  draw_aurora(surface, ticks, scale, weather->aurora);

  if (weather->fog > 0.01) {
    for (int band = 0; band < 3; band++) {
      int y = surface->h * (57 + band * 12) / 100 +
              (int)(sin(ticks / (2900.0 + band * 530)) * scale * 0.012);
      int height = scale / (30 + band * 8);
      if (height < 1) height = 1;
      Uint8 alpha = (Uint8)(weather->fog * (21 - band * 4));
      boxRGBA(surface, 0, y, surface->w - 1, y + height, 150, 177, 181, alpha);
    }
  }

  if (weather->lightning > 0.01) {
    Uint32 phase = ticks % 6400;
    double flash = 0.0;
    if (phase < 220)
      flash = 1.0 - phase / 220.0;
    else if (phase >= 390 && phase < 530)
      flash = 0.55 * (1.0 - (phase - 390) / 140.0);
    if (flash > 0.0) {
      Uint8 alpha = (Uint8)(weather->lightning * 72 * flash);
      boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 188, 205, 225, alpha);
      unsigned int seed = ticks / 7200 * 2654435761u + 0x9e3779b9u;
      int x = surface->w * (18 + seed % 64) / 100;
      int y = surface->h * 27 / 100;
      for (int segment = 0; segment < 7; segment++) {
        int next_x = x + (int)((seed >> (segment % 16)) % (scale / 16 + 1)) - scale / 32;
        int next_y = y + scale / 24;
        thickLineRGBA(surface, x, y, next_x, next_y, 2, 207, 220, 244, (Uint8)(alpha * 0.9));
        if (segment == 3) {
          thickLineRGBA(surface, x, y, x - scale / 30, y + scale / 11, 1, 180, 203, 236, alpha);
        }
        x = next_x;
        y = next_y;
        seed = seed * 1664525u + 1013904223u;
      }
    }
  }
}


void render_world_jump(RenderTarget *surface, Uint32 ticks, int scale) {
  EffectWindow window;
  if (!effect_window(&EFFECT_WORLD_JUMP, ticks, &window)) return;
  Uint32 seed = window.seed;
  Uint32 phase = window.phase;
  double progress = window.progress;
  double fade = window.fade;
  int center_x = surface->w / 2 + (int)(sin(ticks / 115.0) * scale * 0.012);
  int center_y = surface->h * 43 / 100;
  double tunnel_open = sin(progress * 3.14159265359);
  int portal_x_radius = scale * (0.045 + tunnel_open * 0.22);
  int portal_y_radius = scale * (0.035 + tunnel_open * 0.15);
  Uint8 tunnel_alpha = (Uint8)(118 * fade * (0.35 + tunnel_open * 0.65));

  for (int ring = 0; ring < 9; ring++) {
    double ring_scale = 0.34 + ring * 0.105;
    int radius_x = (int)(portal_x_radius * ring_scale);
    int radius_y = (int)(portal_y_radius * ring_scale);
    Uint8 ring_alpha = (Uint8)(tunnel_alpha * (1.0 - ring * 0.065));
    ellipseRGBA(surface, center_x, center_y, radius_x, radius_y, (Uint8)(100 + ring * 14),
                (Uint8)(151 + ring * 9), (Uint8)(218 + ring * 4), ring_alpha);
  }
  for (int streak = 0; streak < 48; streak++) {
    unsigned int streak_seed = seed + streak * 2246822519u;
    double angle = (streak_seed % 6283) / 1000.0 + ticks / 700.0;
    double travel = fmod((streak_seed >> 8) / 4294967295.0 + progress * 1.8, 1.0);
    double distance = portal_x_radius + travel * scale * 0.38;
    int star_x = center_x + (int)(cos(angle) * distance);
    int star_y = center_y + (int)(sin(angle) * distance * 0.68);
    int tail_x = center_x + (int)(cos(angle) * (distance - scale * 0.035));
    int tail_y = center_y + (int)(sin(angle) * (distance - scale * 0.035) * 0.68);
    thickLineRGBA(surface, tail_x, tail_y, star_x, star_y, 1, (Uint8)(155 + streak % 5 * 18),
                  (Uint8)(182 + streak % 4 * 16), 235, (Uint8)(tunnel_alpha * 0.62));
  }

  double destination_mix =
      smooth_step(3900.0, 5150.0, phase) * (1.0 - smooth_step(5850.0, 7200.0, phase));
  static const Uint8 destination_palette[4][3] = {
      {36, 90, 112}, {128, 66, 75}, {74, 103, 68}, {98, 79, 132}};
  int destination = effect_kind(&EFFECT_WORLD_JUMP, seed % 4);
  Uint8 sky_alpha = (Uint8)(84 * destination_mix * fade);
  boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, destination_palette[destination][0],
          destination_palette[destination][1], destination_palette[destination][2], sky_alpha);
  for (int layer = 0; layer < 3; layer++) {
    int base_y = surface->h * (66 + layer * 9) / 100;
    Uint8 terrain_alpha = (Uint8)(148 * destination_mix * fade * (0.85 - layer * 0.16));
    Uint8 terrain_red = (Uint8)(14 + destination * 13 + layer * 5);
    Uint8 terrain_green = (Uint8)(31 + destination * 9 + layer * 4);
    Uint8 terrain_blue = (Uint8)(43 + destination * 14 + layer * 3);
    for (int segment = 0; segment < 18; segment++) {
      int x1 = surface->w * segment / 18;
      int x2 = surface->w * (segment + 1) / 18;
      int y1 = base_y + (int)(sin(segment * 1.4 + destination) * scale * 0.035);
      int y2 = base_y + (int)(sin((segment + 1) * 1.4 + destination) * scale * 0.035);
      filledTrigonRGBA(surface, x1, y1, x2, y2, x2, surface->h, terrain_red, terrain_green,
                       terrain_blue, terrain_alpha);
      filledTrigonRGBA(surface, x1, y1, x2, surface->h, x1, surface->h, terrain_red, terrain_green,
                       terrain_blue, terrain_alpha);
    }
  }
  Uint32 return_phase = phase > 5900 ? phase - 5900 : 0;
  if (return_phase < 900) {
    double return_flash = (1.0 - return_phase / 900.0) * fade;
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 225, 233, 241,
            (Uint8)(150 * return_flash));
  }
}


void render_alien_weather(RenderTarget *surface, Uint32 ticks, int scale) {
  EffectWindow window;
  if (!effect_window(&EFFECT_ALIEN_WEATHER, ticks, &window)) return;
  Uint32 seed = window.seed;
  Uint32 phase = window.phase;
  double fade = window.fade;
  int kind = effect_kind(&EFFECT_ALIEN_WEATHER, seed % 5);
  static const Uint8 sky_colors[5][3] = {
      {66, 142, 106}, {99, 81, 157}, {62, 113, 156}, {158, 83, 91}, {157, 125, 62}};
  Uint8 red = sky_colors[kind][0];
  Uint8 green = sky_colors[kind][1];
  Uint8 blue = sky_colors[kind][2];
  boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, red, green, blue, (Uint8)(18 * fade));

  if (kind == 2) {
    int planet_x = surface->w * (25 + (seed >> 12) % 50) / 100;
    int planet_y = surface->h * 28 / 100;
    int radius = scale / 11;
    filledCircleRGBA(surface, planet_x, planet_y, radius, 32, 38, 51, (Uint8)(150 * fade));
    circleRGBA(surface, planet_x, planet_y, radius + scale / 30, 134, 194, 210,
               (Uint8)(100 * fade));
  }

  int count = surface->w * surface->h / 12000;
  if (count < 24) count = 24;
  if (count > 180) count = 180;
  for (int particle = 0; particle < count; particle++) {
    Uint32 particle_seed = (Uint32)particle * 1664525u + seed;
    int start_x = particle_seed % surface->w;
    int start_y = (particle_seed >> 9) % surface->h;
    int speed = 3 + (particle_seed >> 20) % 7;
    // Every kind falls the way the normal rain does (down, leaning right), so
    // the two never cross at different angles. Each particle moves along its
    // streak.
    double direction_x = 0.25;
    double direction_y = 1.0;
    double streak = scale * (kind == 0 ? 0.023 : 0.017);
    double travel = ticks * 2.0 / (35 + speed * 7);
    double path = sqrt(direction_x * direction_x + direction_y * direction_y);
    double wrapped_x = fmod(start_x + travel * direction_x, (double)surface->w);
    double wrapped_y = fmod(start_y + travel * direction_y, (double)surface->h);
    if (wrapped_x < 0.0) wrapped_x += surface->w;
    if (wrapped_y < 0.0) wrapped_y += surface->h;
    int x = (int)wrapped_x, y = (int)wrapped_y;
    int end_x = x - (int)(direction_x / path * streak);
    int end_y = y - (int)(direction_y / path * streak);
    Uint8 alpha = (Uint8)((18 + (particle_seed >> 24) % 30) * fade);
    thickLineRGBA(surface, x, y, end_x, end_y, 1, red, green, blue, alpha);
    if (particle % 9 == 0)
      filledCircleRGBA(surface, x, y, 1, (Uint8)(red + 25), (Uint8)(green + 25), (Uint8)(blue + 25),
                       alpha);
  }

  if (kind == 3) {
    Uint32 flash_phase = phase % 8400;
    if (flash_phase < 140) {
      Uint8 flash = (Uint8)(45 * fade * (1.0 - flash_phase / 140.0));
      boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 211, 183, 226, flash);
    }
  }
}


void render_fullscreen_event(RenderTarget *surface, Uint32 ticks, int scale,
                             const SceneWeather *weather) {
  EffectWindow window;
  if (!effect_window(&EFFECT_FULLSCREEN, ticks, &window)) return;
  Uint32 seed = window.seed;
  Uint32 phase = window.phase;
  double fade = window.fade;
  // Kinds 4 and 5 are quiet slots that fullscreen_accept() already filters out.
  // When forced, fold them back so the effect always shows something.
  int kind = (seed >> 5) % 6;
  if (forced_effect == &EFFECT_FULLSCREEN) kind %= 4;
  kind = effect_kind(&EFFECT_FULLSCREEN, kind);
  if (kind >= 4) return;
  if (kind == 0) {
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 62, 94, 132, (Uint8)(10 * fade));
    // Top up to full strength without doubling up when the aurora weather is
    // on.
    draw_aurora(surface, ticks, scale, fade * (1.0 - weather->aurora));
  } else if (kind == 1) {
    Uint32 flash_phase = phase % 3400;
    double flash =
        smooth_step(0.0, 90.0, flash_phase) * (1.0 - smooth_step(170.0, 430.0, flash_phase));
    Uint8 alpha = (Uint8)(24 * fade * flash);
    if (alpha > 0) {
      boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 170, 192, 220, alpha);
    }
  } else if (kind == 2) {
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 17, 25, 44, (Uint8)(28 * fade));
    int radius = scale / 8;
    int moon_x = surface->w * (25 + (seed >> 12) % 50) / 100;
    int moon_y = surface->h * (18 + (seed >> 20) % 24) / 100;
    filledCircleRGBA(surface, moon_x, moon_y, radius + scale / 18, 119, 158, 194,
                     (Uint8)(24 * fade));
    filledCircleRGBA(surface, moon_x, moon_y, radius, 171, 183, 190, (Uint8)(190 * fade));
    filledCircleRGBA(surface, moon_x + radius / 3, moon_y, radius - radius / 7, 22, 27, 38,
                     (Uint8)(220 * fade));
  } else {
    int flare_x = surface->w * (28 + (seed >> 12) % 44) / 100;
    int flare_y = surface->h * (18 + (seed >> 20) % 25) / 100;
    int radius = scale / 13;
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 177, 82, 45, (Uint8)(13 * fade));
    filledCircleRGBA(surface, flare_x, flare_y, radius, 245, 137, 75, (Uint8)(45 * fade));
    filledCircleRGBA(surface, flare_x, flare_y, radius / 3, 255, 222, 169, (Uint8)(90 * fade));
    for (int ray = 0; ray < 12; ray++) {
      double angle = ray * 0.52359877559 + ticks / 9000.0;
      int end_x = flare_x + (int)(cos(angle) * scale * 0.42);
      int end_y = flare_y + (int)(sin(angle) * scale * 0.30);
      thickLineRGBA(surface, flare_x, flare_y, end_x, end_y, 1, 251, 174, 105, (Uint8)(18 * fade));
    }
  }
}


void draw_bird(RenderTarget *surface, int x, int y, int size, int wing_lift) {
  const Uint8 shade = 23, alpha = 185;
  int body_length = size;
  int wing_span = size;
  if (wing_lift < 1) wing_lift = 1;

  thickLineRGBA(surface, x - body_length / 2, y, x + body_length / 2, y, 2, shade, 30, 43, alpha);
  filledCircleRGBA(surface, x + body_length / 2, y - 1, 1, shade, 30, 43, alpha);
  thickLineRGBA(surface, x + body_length / 2 + 1, y - 1, x + body_length / 2 + 3, y, 1, shade, 30,
                43, alpha);
  thickLineRGBA(surface, x - body_length / 2, y, x - body_length / 2 - size / 2, y - size / 3, 1,
                shade, 30, 43, alpha);
  thickLineRGBA(surface, x - body_length / 2, y, x - body_length / 2 - size / 2, y + size / 3, 1,
                shade, 30, 43, alpha);

  int shoulder_x = x - size / 5;
  int far_tip_x = x - wing_span;
  int near_tip_x = x - wing_span * 3 / 4;
  int far_tip_y = y - wing_lift;
  int near_tip_y = y + wing_lift * 3 / 4;
  thickLineRGBA(surface, shoulder_x, y, x - size / 2, y - wing_lift / 2, 2, shade, 30, 43, alpha);
  thickLineRGBA(surface, x - size / 2, y - wing_lift / 2, far_tip_x, far_tip_y, 2, shade, 30, 43,
                alpha);
  thickLineRGBA(surface, shoulder_x, y, x - size / 2, y + wing_lift / 3, 2, shade, 30, 43, alpha);
  thickLineRGBA(surface, x - size / 2, y + wing_lift / 3, near_tip_x, near_tip_y, 2, shade, 30, 43,
                alpha);
}


// Sleek, aerodynamic bird silhouette with swept cambered wings, tapered body,
// forked/fanned tail, and optional rim highlight. `dir` is +1 (right) or -1
// (left); `flap` ranges roughly -1.0 (downstroke) to +1.0 (upstroke); `pitch`
// tilts the flight vector in radians.
void draw_sleek_bird(RenderTarget *surface, double cx, double cy, double span, int dir, double flap,
                     double pitch, Uint8 r, Uint8 g, Uint8 b, Uint8 alpha, bool rim_light) {
  if (span < 3.0 || alpha == 0) return;
  double ca = cos(pitch), sa = sin(pitch);
  double fx = dir * ca, fy = dir * sa;  // forward unit vector
  double ux = -sa, uy = ca;             // up-ish normal (screen y is down)
  if (dir < 0) {
    ux = sa;
    uy = -ca;
  }
  double body_len = span * 0.62;
  double head_x = cx + fx * body_len * 0.52;
  double head_y = cy + fy * body_len * 0.52;
  double tail_x = cx - fx * body_len * 0.55;
  double tail_y = cy - fy * body_len * 0.55;
  double beak_x = head_x + fx * span * 0.16;
  double beak_y = head_y + fy * span * 0.16;

  // Tapered fuselage + sleek head/beak
  int body_w = span > 18.0 ? 3 : 2;
  thickLineRGBA(surface, (int)(tail_x), (int)(tail_y), (int)head_x, (int)head_y, body_w, r, g, b,
                alpha);
  lineRGBA(surface, (int)head_x, (int)head_y, (int)beak_x, (int)beak_y, r, g, b, alpha);

  // Forked / swept tail feathers
  double t_spread = span * 0.14;
  double t_back = span * 0.24;
  lineRGBA(surface, (int)tail_x, (int)tail_y, (int)(tail_x - fx * t_back - ux * t_spread),
           (int)(tail_y - fy * t_back - uy * t_spread), r, g, b, (Uint8)(alpha * 0.85));
  lineRGBA(surface, (int)tail_x, (int)tail_y, (int)(tail_x - fx * t_back + ux * t_spread),
           (int)(tail_y - fy * t_back + uy * t_spread), r, g, b, (Uint8)(alpha * 0.85));

  // Swept articulated wings (near wing + far wing for 3/4 depth)
  double sh_x = cx + fx * body_len * 0.10;
  double sh_y = cy + fy * body_len * 0.10;
  int wing_w = span > 20.0 ? 2 : 1;

  // Far wing (higher/further in perspective)
  double f_mid_x = sh_x - fx * span * 0.24 - ux * span * (0.28 + 0.22 * flap);
  double f_mid_y = sh_y - fy * span * 0.24 - uy * span * (0.28 + 0.22 * flap);
  double f_tip_x = sh_x - fx * span * 0.72 - ux * span * (0.14 + 0.34 * flap);
  double f_tip_y = sh_y - fy * span * 0.72 - uy * span * (0.14 + 0.34 * flap);
  thickLineRGBA(surface, (int)sh_x, (int)sh_y, (int)f_mid_x, (int)f_mid_y, wing_w, r, g, b, alpha);
  lineRGBA(surface, (int)f_mid_x, (int)f_mid_y, (int)f_tip_x, (int)f_tip_y, r, g, b, alpha);

  // Near wing (swept lower/closer)
  double n_mid_x = sh_x - fx * span * 0.18 + ux * span * (0.18 - 0.24 * flap);
  double n_mid_y = sh_y - fy * span * 0.18 + uy * span * (0.18 - 0.24 * flap);
  double n_tip_x = sh_x - fx * span * 0.62 + ux * span * (0.10 - 0.36 * flap);
  double n_tip_y = sh_y - fy * span * 0.62 + uy * span * (0.10 - 0.36 * flap);
  thickLineRGBA(surface, (int)sh_x, (int)sh_y, (int)n_mid_x, (int)n_mid_y, wing_w, r, g, b, alpha);
  lineRGBA(surface, (int)n_mid_x, (int)n_mid_y, (int)n_tip_x, (int)n_tip_y, r, g, b, alpha);

  if (rim_light) {
    Uint8 rim_a = (Uint8)(alpha * 0.45);
    lineRGBA(surface, (int)sh_x, (int)sh_y - 1, (int)f_mid_x, (int)f_mid_y - 1, 175, 205, 240,
             rim_a);
    lineRGBA(surface, (int)f_mid_x, (int)f_mid_y - 1, (int)f_tip_x, (int)f_tip_y - 1, 175, 205, 240,
             rim_a);
  }
}
