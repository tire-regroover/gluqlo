#ifndef GLUQLO_SCENE_CORE_H
#define GLUQLO_SCENE_CORE_H

#include "gluqlo.h"

enum { GROUP_NONE = 0, GROUP_TAKEOVER, GROUP_SKY };

typedef struct {
  const char *name;
  Uint32 cycle;
  int seed_style;
  Uint32 gate;
  bool gate_after_mix;
  Uint32 mix_mul;
  Uint32 duration;
  Uint32 jitter;
  int jitter_shift;
  bool random_start;
  double fade_in;
  double fade_out;
  int kind_count;
  int group;
  int priority;
  bool (*accept)(Uint32 slot, Uint32 seed);
} EffectSpec;

typedef struct {
  Uint32 slot;
  Uint32 seed;
  Uint32 phase;
  Uint32 duration;
  double progress;
  double fade;
} EffectWindow;

typedef struct {
  double season[4];
  double cloud;
  double rain;
  double snow;
  double fog;
  double wind;
  double lightning;
  double aurora;
  double sunshine;
  Uint8 palette[3];
} SceneWeather;

extern const EffectSpec *const effect_table[];
extern const size_t effect_count;
extern const EffectSpec *forced_effect;
extern int forced_kind;
extern Uint32 scene_offset;

extern const EffectSpec EFFECT_FLEET_BATTLE;
extern const EffectSpec EFFECT_BACKGROUND_BATTLE;
extern const EffectSpec EFFECT_RARE_SKY;
extern const EffectSpec EFFECT_BATTLE_FLYBY;
extern const EffectSpec EFFECT_DOGFIGHT;
extern const EffectSpec EFFECT_HILL_EVENT;
extern const EffectSpec EFFECT_FOREGROUND;
extern const EffectSpec EFFECT_SCREEN_FADE;
extern const EffectSpec EFFECT_LONG_SCENE;

Uint32 scene_time(void);
Uint32 scene_offset_from_seed(Uint32 seed);
Uint32 random_scene_offset(void);
const EffectSpec *find_effect(const char *name);
int effect_kind(const EffectSpec *spec, int kind);
bool effect_window(const EffectSpec *spec, Uint32 ticks, EffectWindow *window);

SceneWeather get_scene_weather(Uint32 ticks);
void render_weather_effects(RenderTarget *surface, Uint32 ticks, const SceneWeather *weather,
                            int scale);
void render_world_jump(RenderTarget *surface, Uint32 ticks, int scale);
void render_alien_weather(RenderTarget *surface, Uint32 ticks, int scale);
void render_fullscreen_event(RenderTarget *surface, Uint32 ticks, int scale,
                             const SceneWeather *weather);

void draw_bird(RenderTarget *surface, int x, int y, int size, int wing_lift);
void draw_sleek_bird(RenderTarget *surface, double cx, double cy, double span, int dir, double flap,
                     double pitch, Uint8 r, Uint8 g, Uint8 b, Uint8 alpha, bool rim_light);

#endif
