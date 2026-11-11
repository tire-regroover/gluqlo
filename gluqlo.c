/*
* Gluqlo: Fliqlo for Linux
* https://github.com/alexanderk23/gluqlo
*
* Copyright (c) 2010-2012 Kuźniarski Jacek
* Copyright (c) 2014 Alexander Kovalenko
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <time.h>
#include <math.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_syswm.h"
#include "SDL_gfxPrimitives.h"
#include "SDL_rotozoom.h"

#ifndef FONT
#define FONT "/usr/share/gluqlo/gluqlo.ttf"
#endif

const char* TITLE = "Gluqlo 1.1";
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
static SDL_Surface *date_surface_cache;
static int date_surface_yday = -1;

int width = DEFAULT_WIDTH;
int height = DEFAULT_HEIGHT;
static const Uint32 AMBIENT_FRAME_INTERVAL = 100;
static const int MAX_RAIN_DROPS = 1200;
static bool profile_rendering = false;

TTF_Font *font_time = NULL;
TTF_Font *font_mode = NULL;
TTF_Font *font_date = NULL;

const SDL_Color FONT_COLOR = { 0xda, 0xda, 0xe2, 0 };
const SDL_Color DATE_COLOR = { 0xc6, 0xc2, 0xcf, 0 };
const SDL_Color BACKGROUND_COLOR = { 0x12, 0x14, 0x1d, 0 };
const SDL_Color CARD_BORDER_COLOR = { 0x37, 0x3d, 0x4e, 0 };

SDL_Surface *screen;

SDL_Rect hourBackground;
SDL_Rect minBackground;

SDL_Rect bgrect;
SDL_Surface *bg;
static SDL_Surface *backdrop_base;

// draw rounded box
// see http://lists.libsdl.org/pipermail/sdl-libsdl.org/2006-December/058868.html
void fill_rounded_box_b(SDL_Surface* dst, SDL_Rect *coords, int r, SDL_Color color, Uint8 alpha) {
	Uint32 pixcolor = SDL_MapRGBA(dst->format, color.r, color.g, color.b, alpha);

	int i, j;
	int rpsqrt2 = (int) (r / sqrt(2));
	int yd = dst->pitch / dst->format->BytesPerPixel;
	int w = coords->w / 2 - 1;
	int h = coords->h / 2 - 1;
	int xo = coords->x + w;
	int yo = coords->y + h;

	w -= r;
	h -= r;

	if(w <= 0 || h <= 0) return;

	SDL_LockSurface(dst);
	Uint32 *pixels = (Uint32*)(dst->pixels);

	int sy = (yo - h) * yd;
	int ey = (yo + h) * yd;
	int sx = xo - w;
	int ex = xo + w;

	for(i = sy; i <= ey; i += yd)
		for(j = sx - r; j <= ex + r; j++)
			pixels[i + j] = pixcolor;

	int d = -r;
	int x2m1 = -1;
	int y = r;

	for(int x = 0; x <= rpsqrt2; x++) {
		x2m1 += 2;
		d += x2m1;
		if(d >= 0) {
			y--;
			d -= y * 2;
		}

		for(i = sx - x; i <= ex + x; i++) {
			pixels[sy - y * yd + i] = pixcolor;
		}

		for(i = sx - y; i <= ex + y; i++) {
			pixels[sy - x * yd + i] = pixcolor;
		}

		for(i = sx - y; i <= ex + y; i++) {
			pixels[ey + x * yd + i] = pixcolor;
		}

		for(i = sx - x; i <= ex + x; i++) {
			pixels[ey + y * yd + i] = pixcolor;
		}
	}

	SDL_UnlockSurface(dst);
}

void render_ampm(SDL_Surface *surface, SDL_Rect *rect, int pm) {
	char mode[3];
	SDL_Rect coords;
	snprintf(mode, 3, "%cM", pm ? 'P' : 'A');
	SDL_Surface *ampm = TTF_RenderText_Blended(font_mode, mode, FONT_COLOR);
	int offset = rect->h * 0.127;
	coords.x = rect->x + rect->h * 0.07;
	coords.y = rect->y + (pm ? rect->h - offset - ampm->h : offset);
	SDL_BlitSurface(ampm, 0, surface, &coords);
	SDL_FreeSurface(ampm);
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
	double top = hash_noise(x0, y0, seed) * (1.0 - fx)
		+ hash_noise(x0 + 1, y0, seed) * fx;
	double bottom = hash_noise(x0, y0 + 1, seed) * (1.0 - fx)
		+ hash_noise(x0 + 1, y0 + 1, seed) * fx;
	return top * (1.0 - fy) + bottom * fy;
}

static Uint8 blend_channel(Uint8 from, Uint8 to, double amount) {
	if(amount < 0.0) amount = 0.0;
	if(amount > 1.0) amount = 1.0;
	return (Uint8)(from + (to - from) * amount);
}

static double smooth_step(double edge0, double edge1, double value) {
	double t = (value - edge0) / (edge1 - edge0);
	if(t < 0.0) t = 0.0;
	if(t > 1.0) t = 1.0;
	return t * t * (3.0 - 2.0 * t);
}

// Height of the three ground ridges as a fraction of the screen height at
// horizontal position nx (0..1). Effects use it to sit things on the ground.
enum { RIDGE_FAR, RIDGE_MIDDLE, RIDGE_NEAR };

static double ridge_height(int layer, double nx) {
	switch(layer) {
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
enum { GROUP_NONE = 0, GROUP_TAKEOVER, GROUP_SKY };

#define RARE_SKY_KINDS 43
#define RARE_SKY_CLASSIC_KINDS 23   // kinds below this use the four classic sky objects

typedef struct {
	const char *name;
	Uint32 cycle;          // slot length (ms)
	int seed_style;        // 0: slot * 2246822519u + 3266489917u, 1: slot * 2654435761u + 0x9e3779b9u
	Uint32 gate;           // run only in slots where seed % gate == 0 (0: every slot)
	bool gate_after_mix;   // test the gate on the mixed seed instead of the raw one
	Uint32 mix_mul;        // if non-zero, scramble the seed using this multiplier
	Uint32 duration;       // base run time (ms)
	Uint32 jitter;         // extra run time: (seed >> jitter_shift) % jitter
	int jitter_shift;
	bool random_start;     // start at a seeded offset in the slot instead of at its beginning
	double fade_in;        // ms
	double fade_out;       // ms
	int kind_count;        // number of forceable kinds for -effect NAME:KIND (0: none)
	int group;             // GROUP_*: effects in the same group never overlap
	int priority;          // within a group the lowest number wins (keep unique per group)
	bool (*accept)(Uint32 slot, Uint32 seed); // optional: false means this slot shows nothing
} EffectSpec;

typedef struct {
	Uint32 slot;
	Uint32 seed;
	Uint32 phase;          // ms since the effect started
	Uint32 duration;
	double progress;       // phase / duration
	double fade;           // 0..1 fade in/out factor
} EffectWindow;

typedef struct {
	Uint32 seed;
	Uint32 duration;
	Uint64 start;          // absolute start time (ms)
} EffectRun;

// Rare sky kinds from RARE_SKY_CLASSIC_KINDS up are the rarer ones: only 1 slot in 7 shows them.
static bool rare_sky_accept(Uint32 slot, Uint32 seed) {
	int kind = (seed >> 24) % RARE_SKY_KINDS;
	if(kind < RARE_SKY_CLASSIC_KINDS) return true;
	return (slot * 2654435761u + 0x9e3779b9u) % 7 == 0;
}

// Fullscreen kinds 4 and 5 are deliberate quiet slots (they keep the effect rare).
static bool fullscreen_accept(Uint32 slot, Uint32 seed) {
	(void)slot;
	return (seed >> 5) % 6 < 4;
}

static const EffectSpec EFFECT_ALIEN_WEATHER = {
	"alien_weather", 300000, 0, 4, false, 2246822519u,
	52000, 18000, 8, false, 5000, 7000, 5, GROUP_TAKEOVER, 1, NULL };
static const EffectSpec EFFECT_WORLD_JUMP = {
	"world_jump", 360000, 0, 5, false, 2654435761u,
	7800, 0, 0, false, 350, 450, 4, GROUP_TAKEOVER, 0, NULL };
static const EffectSpec EFFECT_LONG_SCENE = {
	"long_scene", 210000, 0, 5, false, 2246822519u,
	72000, 28000, 8, true, 6000, 7000, 5, GROUP_TAKEOVER, 2, NULL };
static const EffectSpec EFFECT_FULLSCREEN = {
	"fullscreen", 90000, 1, 0, false, 0,
	6500, 4000, 9, true, 1300, 1200, 4, GROUP_TAKEOVER, 3, fullscreen_accept };
static const EffectSpec EFFECT_FLEET_BATTLE = {
	"fleet_battle", 240000, 1, 3, false, 0,
	60000, 0, 0, false, 1400, 2200, 0, GROUP_SKY, 0, NULL };
static const EffectSpec EFFECT_BACKGROUND_BATTLE = {
	"background_battle", 90000, 1, 4, false, 0,
	26000, 0, 0, false, 900, 1700, 0, GROUP_SKY, 1, NULL };
static const EffectSpec EFFECT_RARE_SKY = {
	"rare_sky", 48000, 0, 3, true, 2246822519u,
	4200, 3200, 8, true, 350, 400, RARE_SKY_KINDS, GROUP_SKY, 2, rare_sky_accept };
static const EffectSpec EFFECT_BATTLE_FLYBY = {
	"battle_flyby", 60000, 1, 0, false, 0,
	14000, 0, 0, false, 900, 1100, 0, GROUP_SKY, 3, NULL };
static const EffectSpec EFFECT_FOREGROUND = {
	"foreground", 180000, 1, 2, false, 2246822519u,
	6500, 0, 0, false, 650, 900, 4, GROUP_NONE, 0, NULL };
static const EffectSpec EFFECT_SCREEN_FADE = {
	"screen_fade", 300000, 1, 4, false, 0,
	12000, 0, 0, true, 1800, 1800, 0, GROUP_NONE, 0, NULL };

static const EffectSpec *const effect_table[] = {
	&EFFECT_ALIEN_WEATHER, &EFFECT_WORLD_JUMP, &EFFECT_LONG_SCENE,
	&EFFECT_RARE_SKY, &EFFECT_FULLSCREEN, &EFFECT_FOREGROUND,
	&EFFECT_SCREEN_FADE, &EFFECT_BATTLE_FLYBY, &EFFECT_BACKGROUND_BATTLE,
	&EFFECT_FLEET_BATTLE
};
static const size_t effect_count = sizeof(effect_table) / sizeof(effect_table[0]);

static const EffectSpec *forced_effect = NULL;
static int forced_kind = -1;

/*
 * Scene time is the clock every effect, season and weather runs on. It is the
 * SDL tick count plus a per-launch offset, so each launch starts at a
 * different point of the season/weather/effect schedule. -seed N makes the
 * offset reproducible; -effect NAME starts it at 0.
 */
static Uint32 scene_offset = 0;
static const Uint32 SCENE_OFFSET_RANGE = 3600000;

static Uint32 scene_time(void) {
	return SDL_GetTicks() + scene_offset;
}

static Uint32 scene_offset_from_seed(Uint32 seed) {
	seed ^= seed >> 16;
	seed *= 2246822519u;
	seed ^= seed >> 13;
	return seed % SCENE_OFFSET_RANGE;
}

static Uint32 random_scene_offset(void) {
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return scene_offset_from_seed((Uint32)now.tv_nsec ^ ((Uint32)now.tv_sec * 2654435761u));
}

static const EffectSpec *find_effect(const char *name) {
	for(size_t i = 0; i < effect_count; i++) {
		if(strcmp(effect_table[i]->name, name) == 0) return effect_table[i];
	}
	return NULL;
}

// Returns `kind`, or the kind requested with -effect NAME:KIND for this effect.
static int effect_kind(const EffectSpec *spec, int kind) {
	return (forced_effect == spec && forced_kind >= 0) ? forced_kind : kind;
}

// Slot length used by -effect NAME: back to back with a short pause.
static Uint32 effect_cycle(const EffectSpec *spec, bool forced) {
	return forced ? spec->duration + spec->jitter + 3000 : spec->cycle;
}

// Works out whether `slot` of `spec` runs, and when. Forced runs ignore gating.
static bool effect_run(const EffectSpec *spec, Uint32 slot, bool forced, EffectRun *run) {
	Uint32 cycle = effect_cycle(spec, forced);
	Uint32 seed = spec->seed_style
		? slot * 2654435761u + 0x9e3779b9u
		: slot * 2246822519u + 3266489917u;
	if(!forced && spec->gate && !spec->gate_after_mix && seed % spec->gate != 0) return false;
	if(spec->mix_mul) {
		seed ^= seed >> 15;
		seed *= spec->mix_mul;
		seed ^= seed >> 13;
	}
	if(!forced && spec->gate && spec->gate_after_mix && seed % spec->gate != 0) return false;
	if(!forced && spec->accept && !spec->accept(slot, seed)) return false;

	Uint32 duration = spec->duration;
	if(spec->jitter) duration += (seed >> spec->jitter_shift) % spec->jitter;
	Uint32 start = 0;
	if(!forced && spec->random_start && cycle > duration) {
		start = (seed >> 16) % (cycle - duration);
	}
	run->seed = seed;
	run->duration = duration;
	run->start = (Uint64)slot * cycle + start;
	return true;
}

// True if a higher priority effect of the same group runs at any point during `run`.
static bool effect_blocked(const EffectSpec *spec, const EffectRun *run) {
	if(spec->group == GROUP_NONE) return false;
	Uint64 end = run->start + run->duration;
	for(size_t i = 0; i < effect_count; i++) {
		const EffectSpec *other = effect_table[i];
		if(other == spec || other->group != spec->group || other->priority >= spec->priority) continue;
		// A run never leaves its own slot, so only the slots the window touches matter.
		Uint64 first_slot = run->start / other->cycle;
		Uint64 last_slot = (end - 1) / other->cycle;
		for(Uint64 slot = first_slot; slot <= last_slot; slot++) {
			EffectRun other_run;
			if(effect_run(other, (Uint32)slot, false, &other_run)
				&& other_run.start < end
				&& other_run.start + other_run.duration > run->start
				&& !effect_blocked(other, &other_run)) {
				return true;
			}
		}
	}
	return false;
}

static bool effect_window(const EffectSpec *spec, Uint32 ticks, EffectWindow *window) {
	bool forced = forced_effect == spec;
	if(forced_effect && !forced) return false;

	Uint32 cycle = effect_cycle(spec, forced);
	Uint32 slot = ticks / cycle;
	EffectRun run;
	if(!effect_run(spec, slot, forced, &run)) return false;
	if(ticks < run.start || ticks >= run.start + run.duration) return false;
	if(!forced && effect_blocked(spec, &run)) return false;

	Uint32 phase = (Uint32)(ticks - run.start);
	window->slot = slot;
	window->seed = run.seed;
	window->phase = phase;
	window->duration = run.duration;
	window->progress = (double)phase / run.duration;
	window->fade = smooth_step(0.0, spec->fade_in, phase)
		* (1.0 - smooth_step(run.duration - spec->fade_out, run.duration, phase));
	return true;
}

// True while `spec` is drawing something at `ticks`.
static bool effect_is_active(const EffectSpec *spec, Uint32 ticks) {
	EffectWindow window;
	return effect_window(spec, ticks, &window);
}

typedef struct {
	double season[4];
	double cloud;
	double rain;
	double snow;
	double sunshine;
	double fog;
	double wind;
	double lightning;
	double aurora;
	double palette[3];
} SceneWeather;

// Rain and snow must never fall together. When a change swaps one for the
// other, the old one fades out over the first part of the change, the air is
// dry for a moment, and only then does the new one fade in. Changes that keep
// the same kind of precipitation (light rain to heavy rain) just blend.
static double precipitation_level(const double *previous, const double *current,
	int channel, int other_channel, double blend) {
	double from = previous[channel];
	double to = current[channel];
	if(previous[other_channel] <= 0.02 && current[other_channel] <= 0.02) {
		return from + (to - from) * blend;
	}
	return from * (1.0 - smooth_step(0.0, 0.42, blend))
		+ to * smooth_step(0.58, 1.0, blend);
}

static SceneWeather get_scene_weather(Uint32 ticks) {
	static const double weather_levels[][11] = {
		{ 0.10, 0.00, 0.00, 0.95, 0.02, 0.08, 0.00, 0.00, 255, 173, 45 },
		{ 0.25, 0.00, 0.00, 0.72, 0.12, 0.15, 0.00, 0.00, 198, 47, 28 },
		{ 0.78, 0.00, 0.00, 0.10, 0.20, 0.18, 0.00, 0.00, 67, 92, 116 },
		{ 0.74, 0.36, 0.00, 0.12, 0.26, 0.34, 0.00, 0.00, 67, 112, 139 },
		{ 0.90, 0.52, 0.00, 0.02, 0.14, 0.38, 0.00, 0.00, 54, 104, 137 },
		{ 1.00, 0.92, 0.00, 0.00, 0.04, 0.95, 0.90, 0.00, 73, 80, 112 },
		{ 0.72, 0.00, 0.72, 0.12, 0.24, 0.40, 0.00, 0.00, 145, 171, 192 },
		{ 0.97, 0.00, 1.00, 0.00, 0.54, 0.88, 0.04, 0.00, 176, 197, 211 },
		{ 0.36, 0.00, 0.12, 0.00, 0.13, 0.25, 0.00, 0.90, 62, 132, 142 },
		{ 0.88, 0.08, 0.00, 0.15, 1.00, 0.05, 0.00, 0.00, 132, 156, 155 }
	};
	const Uint32 season_duration = 300000;
	const Uint32 season_transition = 20000;
	const Uint32 weather_duration = 65000;
	const Uint32 weather_transition = 12000;
	const int weather_count = sizeof(weather_levels) / sizeof(weather_levels[0]);
	SceneWeather scene = { { 0.0, 0.0, 0.0, 0.0 },
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, { 0.0, 0.0, 0.0 } };

	Uint32 season_elapsed = ticks % (season_duration * 4);
	int season = season_elapsed / season_duration;
	Uint32 season_phase = season_elapsed % season_duration;
	double season_blend = smooth_step(season_duration - season_transition,
		season_duration, season_phase);
	scene.season[season] = 1.0 - season_blend;
	scene.season[(season + 1) % 4] = season_blend;

	Uint32 weather_slot = ticks / weather_duration;
	Uint32 current_seed = weather_slot * 2654435761u + 0x9e3779b9u;
	Uint32 previous_seed = (weather_slot - 1) * 2246822519u + 3266489917u;
	int current = current_seed % weather_count;
	int previous = previous_seed % weather_count;
	if(previous == current) {
		previous = (previous + 1 + (current_seed >> 8) % (weather_count - 1)) % weather_count;
	}
	Uint32 weather_phase = ticks % weather_duration;
	double weather_blend = smooth_step(0.0, weather_transition, weather_phase);
	scene.cloud = weather_levels[previous][0]
		+ (weather_levels[current][0] - weather_levels[previous][0]) * weather_blend;
	scene.rain = precipitation_level(weather_levels[previous], weather_levels[current],
		1, 2, weather_blend);
	scene.snow = precipitation_level(weather_levels[previous], weather_levels[current],
		2, 1, weather_blend);
	scene.sunshine = weather_levels[previous][3]
		+ (weather_levels[current][3] - weather_levels[previous][3]) * weather_blend;
	scene.fog = weather_levels[previous][4]
		+ (weather_levels[current][4] - weather_levels[previous][4]) * weather_blend;
	scene.wind = weather_levels[previous][5]
		+ (weather_levels[current][5] - weather_levels[previous][5]) * weather_blend;
	scene.lightning = weather_levels[previous][6]
		+ (weather_levels[current][6] - weather_levels[previous][6]) * weather_blend;
	scene.aurora = weather_levels[previous][7]
		+ (weather_levels[current][7] - weather_levels[previous][7]) * weather_blend;
	for(int channel = 0; channel < 3; channel++) {
		scene.palette[channel] = weather_levels[previous][8 + channel]
			+ (weather_levels[current][8 + channel] - weather_levels[previous][8 + channel])
			* weather_blend;
	}
	return scene;
}

// Three drifting aurora ribbons; `strength` is 0..1. Used by the aurora weather
// and by the fullscreen aurora event.
static void draw_aurora(SDL_Surface *surface, Uint32 ticks, int scale, double strength) {
	if(strength <= 0.01) return;
	for(int band = 0; band < 3; band++) {
		int previous_x = 0;
		int previous_y = surface->h * (19 + band * 12) / 100;
		for(int segment = 1; segment <= 24; segment++) {
			int x = surface->w * segment / 24;
			double nx = (double)x / surface->w;
			int y = surface->h * (19 + band * 12) / 100
				+ (int)(sin(nx * 6.28318530718 + ticks / (1100.0 + band * 270)
					+ band * 1.9) * scale * (0.03 + band * 0.008));
			Uint8 alpha = (Uint8)(strength * (28 - band * 5));
			thickLineRGBA(surface, previous_x, previous_y, x, y, 2,
				(Uint8)(70 + band * 24), (Uint8)(164 + band * 18),
				(Uint8)(137 + band * 30), alpha);
			previous_x = x;
			previous_y = y;
		}
	}
}

static void render_weather_effects(SDL_Surface *surface, Uint32 ticks,
	const SceneWeather *weather, int scale) {
	draw_aurora(surface, ticks, scale, weather->aurora);

	if(weather->fog > 0.01) {
		for(int band = 0; band < 3; band++) {
			int y = surface->h * (57 + band * 12) / 100
				+ (int)(sin(ticks / (2900.0 + band * 530)) * scale * 0.012);
			int height = scale / (30 + band * 8);
			if(height < 1) height = 1;
			Uint8 alpha = (Uint8)(weather->fog * (21 - band * 4));
			boxRGBA(surface, 0, y, surface->w - 1, y + height,
				150, 177, 181, alpha);
		}
	}

	if(weather->lightning > 0.01) {
		Uint32 phase = ticks % 6400;
		double flash = 0.0;
		if(phase < 220) flash = 1.0 - phase / 220.0;
		else if(phase >= 390 && phase < 530) flash = 0.55 * (1.0 - (phase - 390) / 140.0);
		if(flash > 0.0) {
			Uint8 alpha = (Uint8)(weather->lightning * 72 * flash);
			boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
				188, 205, 225, alpha);
			unsigned int seed = ticks / 7200 * 2654435761u + 0x9e3779b9u;
			int x = surface->w * (18 + seed % 64) / 100;
			int y = surface->h * 27 / 100;
			for(int segment = 0; segment < 7; segment++) {
				int next_x = x + (int)((seed >> (segment % 16)) % (scale / 16 + 1))
					- scale / 32;
				int next_y = y + scale / 24;
				thickLineRGBA(surface, x, y, next_x, next_y, 2,
					207, 220, 244, (Uint8)(alpha * 0.9));
				if(segment == 3) {
					thickLineRGBA(surface, x, y, x - scale / 30,
						y + scale / 11, 1, 180, 203, 236, alpha);
				}
				x = next_x;
				y = next_y;
				seed = seed * 1664525u + 1013904223u;
			}
		}
	}
}

static void render_world_jump(SDL_Surface *surface, Uint32 ticks, int scale) {
		EffectWindow window;
		if(!effect_window(&EFFECT_WORLD_JUMP, ticks, &window)) return;
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

		for(int ring = 0; ring < 9; ring++) {
			double ring_scale = 0.34 + ring * 0.105;
			int radius_x = (int)(portal_x_radius * ring_scale);
			int radius_y = (int)(portal_y_radius * ring_scale);
			Uint8 ring_alpha = (Uint8)(tunnel_alpha * (1.0 - ring * 0.065));
			ellipseRGBA(surface, center_x, center_y, radius_x, radius_y,
				(Uint8)(100 + ring * 14), (Uint8)(151 + ring * 9),
				(Uint8)(218 + ring * 4), ring_alpha);
		}
		for(int streak = 0; streak < 48; streak++) {
			unsigned int streak_seed = seed + streak * 2246822519u;
			double angle = (streak_seed % 6283) / 1000.0 + ticks / 700.0;
			double travel = fmod((streak_seed >> 8) / 4294967295.0 + progress * 1.8, 1.0);
			double distance = portal_x_radius + travel * scale * 0.38;
			int star_x = center_x + (int)(cos(angle) * distance);
			int star_y = center_y + (int)(sin(angle) * distance * 0.68);
			int tail_x = center_x + (int)(cos(angle) * (distance - scale * 0.035));
			int tail_y = center_y + (int)(sin(angle) * (distance - scale * 0.035) * 0.68);
			thickLineRGBA(surface, tail_x, tail_y, star_x, star_y, 1,
				(Uint8)(155 + streak % 5 * 18), (Uint8)(182 + streak % 4 * 16),
				235, (Uint8)(tunnel_alpha * 0.62));
		}

		double destination_mix = smooth_step(3900.0, 5150.0, phase)
			* (1.0 - smooth_step(5850.0, 7200.0, phase));
		static const Uint8 destination_palette[4][3] = {
			{ 36, 90, 112 }, { 128, 66, 75 }, { 74, 103, 68 }, { 98, 79, 132 }
		};
		int destination = effect_kind(&EFFECT_WORLD_JUMP, seed % 4);
		Uint8 sky_alpha = (Uint8)(84 * destination_mix * fade);
		boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
			destination_palette[destination][0], destination_palette[destination][1],
			destination_palette[destination][2], sky_alpha);
		for(int layer = 0; layer < 3; layer++) {
			int base_y = surface->h * (66 + layer * 9) / 100;
			Uint8 terrain_alpha = (Uint8)(148 * destination_mix * fade * (0.85 - layer * 0.16));
			Uint8 terrain_red = (Uint8)(14 + destination * 13 + layer * 5);
			Uint8 terrain_green = (Uint8)(31 + destination * 9 + layer * 4);
			Uint8 terrain_blue = (Uint8)(43 + destination * 14 + layer * 3);
			for(int segment = 0; segment < 18; segment++) {
				int x1 = surface->w * segment / 18;
				int x2 = surface->w * (segment + 1) / 18;
				int y1 = base_y + (int)(sin(segment * 1.4 + destination) * scale * 0.035);
				int y2 = base_y + (int)(sin((segment + 1) * 1.4 + destination) * scale * 0.035);
				filledTrigonRGBA(surface, x1, y1, x2, y2, x2, surface->h,
					terrain_red, terrain_green, terrain_blue, terrain_alpha);
				filledTrigonRGBA(surface, x1, y1, x2, surface->h, x1, surface->h,
					terrain_red, terrain_green, terrain_blue, terrain_alpha);
			}
		}
		Uint32 return_phase = phase > 5900 ? phase - 5900 : 0;
		if(return_phase < 900) {
			double return_flash = (1.0 - return_phase / 900.0) * fade;
			boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
				225, 233, 241, (Uint8)(150 * return_flash));
		}
	}

static void render_alien_weather(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_ALIEN_WEATHER, ticks, &window)) return;
	Uint32 seed = window.seed;
	Uint32 phase = window.phase;
	double fade = window.fade;
	int kind = effect_kind(&EFFECT_ALIEN_WEATHER, seed % 5);
	static const Uint8 sky_colors[5][3] = {
		{ 66, 142, 106 }, { 99, 81, 157 }, { 62, 113, 156 },
		{ 158, 83, 91 }, { 157, 125, 62 }
	};
	Uint8 red = sky_colors[kind][0];
	Uint8 green = sky_colors[kind][1];
	Uint8 blue = sky_colors[kind][2];
	boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
		red, green, blue, (Uint8)(18 * fade));

	if(kind == 2) {
		int planet_x = surface->w * (25 + (seed >> 12) % 50) / 100;
		int planet_y = surface->h * 28 / 100;
		int radius = scale / 11;
		filledCircleRGBA(surface, planet_x, planet_y, radius,
			32, 38, 51, (Uint8)(150 * fade));
		circleRGBA(surface, planet_x, planet_y, radius + scale / 30,
			134, 194, 210, (Uint8)(100 * fade));
	}

	int count = surface->w * surface->h / 12000;
	if(count < 24) count = 24;
	if(count > 180) count = 180;
	for(int particle = 0; particle < count; particle++) {
		Uint32 particle_seed = (Uint32)particle * 1664525u + seed;
		int start_x = particle_seed % surface->w;
		int start_y = (particle_seed >> 9) % surface->h;
		int speed = 3 + (particle_seed >> 20) % 7;
		int travel = (int)(ticks / (35 + speed * 7));
		int x, y, end_x, end_y;
		if(kind == 0) {
			x = (start_x + travel * speed) % surface->w;
			y = (start_y + travel / 2) % surface->h;
			end_x = x - scale / 90;
			end_y = y + scale / 50;
		} else if(kind == 1) {
			x = (start_x - travel * speed + surface->w * 2) % surface->w;
			y = (start_y - travel) % surface->h;
			if(y < 0) y += surface->h;
			end_x = x + scale / 100;
			end_y = y + scale / 70;
		} else {
			x = (start_x + travel * speed / 2) % surface->w;
			y = (start_y + travel) % surface->h;
			end_x = x - scale / 130;
			end_y = y + scale / 60;
		}
		Uint8 alpha = (Uint8)((18 + (particle_seed >> 24) % 30) * fade);
		thickLineRGBA(surface, x, y, end_x, end_y, 1, red, green, blue, alpha);
		if(particle % 9 == 0) filledCircleRGBA(surface, x, y, 1,
			(Uint8)(red + 25), (Uint8)(green + 25), (Uint8)(blue + 25), alpha);
	}

	if(kind == 3) {
		Uint32 flash_phase = phase % 8400;
		if(flash_phase < 140) {
			Uint8 flash = (Uint8)(45 * fade * (1.0 - flash_phase / 140.0));
			boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
				211, 183, 226, flash);
		}
	}
}

static void draw_bird(SDL_Surface *surface, int x, int y, int size, int wing_lift) {
	const Uint8 shade = 23, alpha = 185;
	int body_length = size;
	int wing_span = size;
	if(wing_lift < 1) wing_lift = 1;

	thickLineRGBA(surface, x - body_length / 2, y, x + body_length / 2, y,
		2, shade, 30, 43, alpha);
	filledCircleRGBA(surface, x + body_length / 2, y - 1, 1, shade, 30, 43, alpha);
	thickLineRGBA(surface, x + body_length / 2 + 1, y - 1,
		x + body_length / 2 + 3, y, 1, shade, 30, 43, alpha);
	thickLineRGBA(surface, x - body_length / 2, y,
		x - body_length / 2 - size / 2, y - size / 3, 1, shade, 30, 43, alpha);
	thickLineRGBA(surface, x - body_length / 2, y,
		x - body_length / 2 - size / 2, y + size / 3, 1, shade, 30, 43, alpha);

	int shoulder_x = x - size / 5;
	int far_tip_x = x - wing_span;
	int near_tip_x = x - wing_span * 3 / 4;
	int far_tip_y = y - wing_lift;
	int near_tip_y = y + wing_lift * 3 / 4;
	thickLineRGBA(surface, shoulder_x, y, x - size / 2, y - wing_lift / 2,
		2, shade, 30, 43, alpha);
	thickLineRGBA(surface, x - size / 2, y - wing_lift / 2,
		far_tip_x, far_tip_y, 2, shade, 30, 43, alpha);
	thickLineRGBA(surface, shoulder_x, y, x - size / 2, y + wing_lift / 3,
		2, shade, 30, 43, alpha);
	thickLineRGBA(surface, x - size / 2, y + wing_lift / 3,
		near_tip_x, near_tip_y, 2, shade, 30, 43, alpha);
}

/*
 * Ship drawing
 *
 * Ships are seen from the side. Each part is a polygon described in tenths of
 * the ship's unit size, with +x pointing forward and +y down, so one table
 * serves both directions. The parts are drawn back to front: far wing, hull,
 * shading, then lights, so translucent parts do not stack on each other.
 */
typedef signed char ShipPoint[2];

static int ship_coord(int origin, int unit, int sign, int tenths) {
	return origin + sign * (int)floor(tenths * unit / 10.0 + 0.5);
}

static Uint8 ship_shade(int value, int percent) {
	int shaded = value * percent / 100;
	return (Uint8)(shaded > 255 ? 255 : shaded);
}

static Uint8 ship_light(int value, int percent) {
	return (Uint8)(value + (232 - value) * percent / 100);
}

static void ship_shape(SDL_Surface *surface, int x, int y, int unit, int direction,
	const ShipPoint *points, int count, Uint8 red, Uint8 green, Uint8 blue,
	Uint8 alpha, Uint8 edge_alpha) {
	Sint16 vx[24], vy[24];
	if(count > 24) count = 24;
	for(int i = 0; i < count; i++) {
		vx[i] = (Sint16)ship_coord(x, unit, direction, points[i][0]);
		vy[i] = (Sint16)ship_coord(y, unit, 1, points[i][1]);
	}
	filledPolygonRGBA(surface, vx, vy, count, red, green, blue, alpha);
	if(edge_alpha > 0) {
		aapolygonRGBA(surface, vx, vy, count, ship_light(red, 45),
			ship_light(green, 45), ship_light(blue, 45), edge_alpha);
	}
}

static void ship_line(SDL_Surface *surface, int x, int y, int unit, int direction,
	int x1, int y1, int x2, int y2, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha) {
	aalineRGBA(surface, ship_coord(x, unit, direction, x1), ship_coord(y, unit, 1, y1),
		ship_coord(x, unit, direction, x2), ship_coord(y, unit, 1, y2),
		red, green, blue, alpha);
}

// A soft engine glow: a faint exhaust cone, a halo and a pale core.
static void ship_engine(SDL_Surface *surface, int x, int y, int unit, int direction,
	int ex, int ey, int radius_tenths, Uint8 alpha) {
	int cx = ship_coord(x, unit, direction, ex);
	int cy = ship_coord(y, unit, 1, ey);
	int radius = radius_tenths * unit / 10;
	if(radius < 1) radius = 1;
	filledTrigonRGBA(surface, cx, cy - radius, cx, cy + radius,
		cx - direction * radius * 3, cy, 96, 170, 226, (Uint8)(alpha * 0.22));
	filledCircleRGBA(surface, cx, cy, radius + 1 + radius / 2, 96, 170, 226,
		(Uint8)(alpha * 0.14));
	filledCircleRGBA(surface, cx, cy, radius, 205, 238, 255, (Uint8)(alpha * 0.85));
}

static void ship_window(SDL_Surface *surface, int x, int y, int unit, int direction,
	int wx, int wy, Uint8 alpha) {
	int px = ship_coord(x, unit, direction, wx);
	int py = ship_coord(y, unit, 1, wy);
	pixelRGBA(surface, px, py, 238, 214, 150, (Uint8)(alpha * 0.9));
	if(unit >= 14) pixelRGBA(surface, px + direction, py, 238, 214, 150, (Uint8)(alpha * 0.6));
}

static void ship_box(SDL_Surface *surface, int x, int y, int unit, int direction,
	int x1, int y1, int x2, int y2, Uint8 red, Uint8 green, Uint8 blue,
	Uint8 alpha, Uint8 edge_alpha) {
	ShipPoint box[4] = {
		{ (signed char)x2, (signed char)y1 }, { (signed char)x2, (signed char)y2 },
		{ (signed char)x1, (signed char)y2 }, { (signed char)x1, (signed char)y1 } };
	ship_shape(surface, x, y, unit, direction, box, 4, red, green, blue, alpha, edge_alpha);
}

// Fighter: a twin-pod gunship seen from above. A slim fuselage with a canopy
// is joined by swept struts to two engine pods, so it reads as a craft with
// a wide stance instead of a single arrow.
static void draw_battle_ship(SDL_Surface *surface, int x, int y, int size,
	int direction, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha) {
	static const ShipPoint far_pod[] = {
		{14, -12}, {11, -13}, {-12, -13}, {-14, -11}, {-14, -8}, {11, -8}, {14, -10} };
	static const ShipPoint near_pod[] = {
		{14, 12}, {11, 13}, {-12, 13}, {-14, 11}, {-14, 8}, {11, 8}, {14, 10} };
	static const ShipPoint far_strut[] = { {5, -8}, {-8, -8}, {-4, -3}, {2, -3} };
	static const ShipPoint near_strut[] = { {5, 8}, {-8, 8}, {-4, 3}, {2, 3} };
	static const ShipPoint fuselage[] = {
		{23, 0}, {18, -2}, {8, -3}, {-10, -3}, {-12, -2}, {-12, 2}, {-10, 3},
		{8, 3}, {18, 2} };

	ship_shape(surface, x, y, size, direction, far_pod, 7,
		ship_shade(red, 70), ship_shade(green, 70), ship_shade(blue, 70), alpha, alpha / 2);
	ship_shape(surface, x, y, size, direction, far_strut, 4,
		ship_shade(red, 82), ship_shade(green, 82), ship_shade(blue, 82), alpha, alpha / 3);
	ship_shape(surface, x, y, size, direction, near_strut, 4,
		ship_shade(red, 82), ship_shade(green, 82), ship_shade(blue, 82), alpha, alpha / 3);
	ship_shape(surface, x, y, size, direction, near_pod, 7,
		ship_light(red, 10), ship_light(green, 10), ship_light(blue, 10), alpha, alpha / 2);
	ship_shape(surface, x, y, size, direction, fuselage, 9,
		ship_light(red, 4), ship_light(green, 4), ship_light(blue, 4), alpha, alpha / 2);
	// Bright team-coloured stripe along each pod.
	ship_line(surface, x, y, size, direction, 10, -10, -10, -10,
		ship_light(red, 75), ship_light(green, 75), ship_light(blue, 75),
		(Uint8)(alpha * 0.7));
	ship_line(surface, x, y, size, direction, 10, 10, -10, 10,
		ship_light(red, 75), ship_light(green, 75), ship_light(blue, 75),
		(Uint8)(alpha * 0.7));
	filledEllipseRGBA(surface, ship_coord(x, size, direction, 10), y,
		4 * size / 10 + 2, 2 * size / 10 + 1, 156, 214, 240, (Uint8)(alpha * 0.95));
	ship_engine(surface, x, y, size, direction, -14, -10, 3, alpha);
	ship_engine(surface, x, y, size, direction, -14, 10, 3, alpha);
	ship_engine(surface, x, y, size, direction, -12, 0, 2, alpha);
}

// Frigate: a twin-hull warship seen from above. Two long armoured hulls are
// tied together by cross decks and a central command block with a dome, so
// it reads as a ladder-like frame rather than a single hull.
static void draw_frigate(SDL_Surface *surface, int x, int y, int size,
	int direction, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha) {
	static const ShipPoint far_hull[] = {
		{16, -13}, {13, -14}, {-28, -14}, {-30, -12}, {-30, -7}, {-28, -5},
		{8, -5}, {12, -8} };
	static const ShipPoint near_hull[] = {
		{16, 13}, {13, 14}, {-28, 14}, {-30, 12}, {-30, 7}, {-28, 5},
		{8, 5}, {12, 8} };
	static const ShipPoint core[] = {
		{35, 0}, {29, -3}, {22, -4}, {10, -5}, {-10, -5}, {-10, 5}, {10, 5},
		{22, 4}, {29, 3} };
	static const ShipPoint prow_plate[] = {
		{35, 0}, {29, -3}, {22, -4}, {22, 4}, {29, 3} };
	Uint8 lr = ship_light(red, 75), lg = ship_light(green, 75), lb = ship_light(blue, 75);

	ship_shape(surface, x, y, size, direction, far_hull, 8,
		ship_shade(red, 75), ship_shade(green, 75), ship_shade(blue, 75), alpha, alpha / 2);
	ship_shape(surface, x, y, size, direction, near_hull, 8,
		ship_light(red, 8), ship_light(green, 8), ship_light(blue, 8), alpha, alpha / 2);
	// Cross decks between the hulls.
	ship_box(surface, x, y, size, direction, -25, -5, -20, 5,
		ship_shade(red, 85), ship_shade(green, 85), ship_shade(blue, 85), alpha, alpha / 3);
	ship_shape(surface, x, y, size, direction, core, 9,
		red, green, blue, alpha, alpha / 2);
	ship_shape(surface, x, y, size, direction, prow_plate, 5,
		ship_light(red, 16), ship_light(green, 16), ship_light(blue, 16), alpha, alpha / 2);
	ship_line(surface, x, y, size, direction, 12, -10, -25, -10, lr, lg, lb,
		(Uint8)(alpha * 0.65));
	ship_line(surface, x, y, size, direction, 12, 10, -25, 10, lr, lg, lb,
		(Uint8)(alpha * 0.65));
	ship_line(surface, x, y, size, direction, 34, 0, 24, 0,
		200, 230, 245, (Uint8)(alpha * 0.9));
	filledEllipseRGBA(surface, ship_coord(x, size, direction, 3), y,
		6 * size / 10 + 1, 3 * size / 10 + 1, 156, 214, 240, (Uint8)(alpha * 0.9));

	// Gun turrets on the hulls, and lit portholes along the core.
	int turret_x[] = { 6, -16 };
	for(int i = 0; i < 2; i++) {
		for(int side = -1; side <= 1; side += 2) {
			int cx = ship_coord(x, size, direction, turret_x[i]);
			int cy = ship_coord(y, size, 1, side * 9);
			filledEllipseRGBA(surface, cx, cy, size / 5 + 2, size / 10 + 1,
				ship_light(red, 18), ship_light(green, 18), ship_light(blue, 18), alpha);
			ship_line(surface, x, y, size, direction, turret_x[i] + 1, side * 9,
				turret_x[i] + 7, side * 9, lr, lg, lb, alpha);
		}
	}
	for(int window = 0; window < 4; window++) {
		ship_window(surface, x, y, size, direction, -8 + window * 4, -4, alpha);
		ship_window(surface, x, y, size, direction, -8 + window * 4, 4, alpha);
	}
	ship_engine(surface, x, y, size, direction, -30, -10, 2, alpha);
	ship_engine(surface, x, y, size, direction, -30, 10, 2, alpha);
	ship_engine(surface, x, y, size, direction, -10, 0, 2, alpha);
}

// Cruiser: a modular spine ship. A thin keel carries, front to back, a sensor
// nose, an armoured gun deck, a tall crew drum, a fuel tank and a flared drive.
// The varied blocks give it a broken, built-up silhouette.
static void draw_cruiser(SDL_Surface *surface, int x, int y, int size,
	int direction, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha) {
	static const ShipPoint nose[] = {
		{23, 1}, {20, -3}, {15, -3}, {15, 3}, {21, 3} };
	static const ShipPoint deck[] = {
		{15, -1}, {13, -4}, {3, -4}, {3, 3}, {15, 3} };
	static const ShipPoint drum[] = {
		{1, -6}, {-1, -9}, {-6, -9}, {-8, -6}, {-8, 6}, {-6, 9}, {-1, 9}, {1, 6} };
	static const ShipPoint drive[] = {
		{-15, -3}, {-15, 3}, {-22, 8}, {-22, -8} };
	Uint8 dr = ship_shade(red, 60), dg = ship_shade(green, 60), db = ship_shade(blue, 60);
	Uint8 lr = ship_light(red, 70), lg = ship_light(green, 70), lb = ship_light(blue, 70);

	// Keel: only the stretches between blocks show, so the alpha does not stack.
	ship_box(surface, x, y, size, direction, -9, -1, -8, 1,
		dr, dg, db, alpha, 0);
	ship_box(surface, x, y, size, direction, 1, -1, 3, 1,
		dr, dg, db, alpha, 0);
	ship_shape(surface, x, y, size, direction, drive, 4,
		ship_shade(red, 70), ship_shade(green, 70), ship_shade(blue, 70), alpha, alpha / 2);
	ship_box(surface, x, y, size, direction, -15, -5, -9, 5,
		ship_light(red, 4), ship_light(green, 4), ship_light(blue, 4), alpha, alpha / 2);
	ship_line(surface, x, y, size, direction, -13, -5, -13, 5, dr, dg, db,
		(Uint8)(alpha * 0.8));
	ship_line(surface, x, y, size, direction, -11, -5, -11, 5, dr, dg, db,
		(Uint8)(alpha * 0.8));
	ship_shape(surface, x, y, size, direction, drum, 8,
		ship_light(red, 10), ship_light(green, 10), ship_light(blue, 10), alpha, alpha / 2);
	ship_line(surface, x, y, size, direction, 1, 0, -8, 0, dr, dg, db,
		(Uint8)(alpha * 0.8));
	ship_shape(surface, x, y, size, direction, deck, 5,
		red, green, blue, alpha, alpha / 2);
	ship_shape(surface, x, y, size, direction, nose, 5,
		ship_light(red, 14), ship_light(green, 14), ship_light(blue, 14), alpha, alpha / 2);
	ship_line(surface, x, y, size, direction, 20, -1, 16, -1,
		200, 230, 245, (Uint8)(alpha * 0.95));
	ship_line(surface, x, y, size, direction, 13, -4, 3, -4, lr, lg, lb,
		(Uint8)(alpha * 0.6));
	ship_line(surface, x, y, size, direction, -1, -9, -6, -9, lr, lg, lb,
		(Uint8)(alpha * 0.6));

	// A raised bridge at the back of the deck and a gun turret forward of it.
	static const ShipPoint bridge[] = { {9, -4}, {8, -8}, {4, -8}, {4, -4} };
	ship_shape(surface, x, y, size, direction, bridge, 4,
		ship_light(red, 16), ship_light(green, 16), ship_light(blue, 16), alpha, alpha / 2);
	ship_line(surface, x, y, size, direction, 8, -6, 5, -6,
		200, 230, 245, (Uint8)(alpha * 0.95));
	filledEllipseRGBA(surface, ship_coord(x, size, direction, 12), ship_coord(y, size, 1, -4),
		size / 5 + 2, size / 10 + 1,
		ship_light(red, 18), ship_light(green, 18), ship_light(blue, 18), alpha);
	ship_line(surface, x, y, size, direction, 13, -5, 19, -5, lr, lg, lb, alpha);
	// Lit portholes on the crew drum and a hangar slit on the deck.
	for(int row = 0; row < 4; row++) {
		for(int column = 0; column < 3; column++) {
			ship_window(surface, x, y, size, direction, -1 - column * 2, -6 + row * 4, alpha);
		}
	}
	ship_line(surface, x, y, size, direction, 13, 1, 5, 1,
		238, 214, 150, (Uint8)(alpha * 0.8));
	ship_engine(surface, x, y, size, direction, -22, -5, 1, alpha);
	ship_engine(surface, x, y, size, direction, -22, 0, 1, alpha);
	ship_engine(surface, x, y, size, direction, -22, 5, 1, alpha);
}

static void render_fullscreen_event(SDL_Surface *surface, Uint32 ticks, int scale,
	const SceneWeather *weather) {
	EffectWindow window;
	if(!effect_window(&EFFECT_FULLSCREEN, ticks, &window)) return;
	Uint32 seed = window.seed;
	Uint32 phase = window.phase;
	double fade = window.fade;
	// Kinds 4 and 5 are quiet slots that fullscreen_accept() already filters out.
	// When forced, fold them back so the effect always shows something.
	int kind = (seed >> 5) % 6;
	if(forced_effect == &EFFECT_FULLSCREEN) kind %= 4;
	kind = effect_kind(&EFFECT_FULLSCREEN, kind);
	if(kind >= 4) return;
	if(kind == 0) {
		boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
			62, 94, 132, (Uint8)(10 * fade));
		// Top up to full strength without doubling up when the aurora weather is on.
		draw_aurora(surface, ticks, scale, fade * (1.0 - weather->aurora));
	} else if(kind == 1) {
		Uint32 flash_phase = phase % 3400;
		double flash = smooth_step(0.0, 90.0, flash_phase)
			* (1.0 - smooth_step(170.0, 430.0, flash_phase));
		Uint8 alpha = (Uint8)(24 * fade * flash);
		if(alpha > 0) {
			boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
				170, 192, 220, alpha);
		}
	} else if(kind == 2) {
		boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
			17, 25, 44, (Uint8)(28 * fade));
		int radius = scale / 8;
		int moon_x = surface->w * (25 + (seed >> 12) % 50) / 100;
		int moon_y = surface->h * (18 + (seed >> 20) % 24) / 100;
		filledCircleRGBA(surface, moon_x, moon_y, radius + scale / 18,
			119, 158, 194, (Uint8)(24 * fade));
		filledCircleRGBA(surface, moon_x, moon_y, radius,
			171, 183, 190, (Uint8)(190 * fade));
		filledCircleRGBA(surface, moon_x + radius / 3, moon_y, radius - radius / 7,
			22, 27, 38, (Uint8)(220 * fade));
	} else {
		int flare_x = surface->w * (28 + (seed >> 12) % 44) / 100;
		int flare_y = surface->h * (18 + (seed >> 20) % 25) / 100;
		int radius = scale / 13;
		boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
			177, 82, 45, (Uint8)(13 * fade));
		filledCircleRGBA(surface, flare_x, flare_y, radius,
			245, 137, 75, (Uint8)(45 * fade));
		filledCircleRGBA(surface, flare_x, flare_y, radius / 3,
			255, 222, 169, (Uint8)(90 * fade));
		for(int ray = 0; ray < 12; ray++) {
			double angle = ray * 0.52359877559 + ticks / 9000.0;
			int end_x = flare_x + (int)(cos(angle) * scale * 0.42);
			int end_y = flare_y + (int)(sin(angle) * scale * 0.30);
			thickLineRGBA(surface, flare_x, flare_y, end_x, end_y, 1,
				251, 174, 105, (Uint8)(18 * fade));
		}
	}
}

static void render_battle_flyby(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_BATTLE_FLYBY, ticks, &window)) return;
	Uint32 phase = window.phase;

	int size = scale / 170;
	if(size < 4) size = 4;
	double fade = window.fade;
	double progress = window.progress;
	int left_x = (int)(-size + progress * (surface->w + 2.0 * size));
	int right_x = surface->w - left_x;
	int center_y = surface->h * 17 / 100;
	int arc = (int)(sin(progress * 6.28318530718) * scale * 0.018);
	int maneuver = (int)(sin(progress * 12.56637061436) * scale * 0.014);
	int left_y = center_y + arc + maneuver;
	int right_y = center_y - arc - maneuver;
	Uint8 alpha = (Uint8)(100 * fade);

	draw_battle_ship(surface, left_x, left_y, size, 1, 105, 163, 183, alpha);
	draw_battle_ship(surface, right_x, right_y, size, -1, 177, 130, 102, alpha);

	Uint32 volley_phase = phase % 2600;
	if(volley_phase < 300 && progress > 0.08 && progress < 0.94) {
		int bolt_length = scale / 18;
		if(bolt_length < 12) bolt_length = 12;
		Uint8 bolt_alpha = (Uint8)(alpha * 0.8);
		thickLineRGBA(surface, left_x + size, left_y,
			left_x + size + bolt_length, left_y - size / 3,
			1, 143, 204, 224, bolt_alpha);
		thickLineRGBA(surface, right_x - size, right_y,
			right_x - size - bolt_length, right_y + size / 3,
			1, 220, 164, 126, bolt_alpha);
	}
	if(volley_phase > 1850 && volley_phase < 2250) {
		double hit_progress = (double)(volley_phase - 1850) / 400.0;
		int impact_x = surface->w / 2
			+ (int)(sin(phase / 1700.0) * scale * 0.045);
		int impact_y = center_y + (int)(sin(phase / 950.0) * scale * 0.025);
		Uint8 impact_alpha = (Uint8)(alpha * (1.0 - hit_progress));
		int radius = size / 2 + (int)(size * hit_progress);
		filledCircleRGBA(surface, impact_x, impact_y, radius,
			219, 158, 111, (Uint8)(impact_alpha * 0.65));
		filledCircleRGBA(surface, impact_x, impact_y, size / 4,
			238, 216, 177, impact_alpha);
	}
}

static void render_background_battle(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_BACKGROUND_BATTLE, ticks, &window)) return;
	Uint32 phase = window.phase;

	int size = scale / 170;
	if(size < 4) size = 4;
	Uint32 seed = window.seed;
	int winner = seed & 1;
	double fade = window.fade;
	double progress = window.progress;
	double approach = smooth_step(0.04, 0.34, progress);
	double withdrawal = smooth_step(0.80, 0.98, progress);
	int center_y = surface->h * 18 / 100;
	int left_x = (int)(-size * 4 + approach * (surface->w * 0.38 + size * 4)
		- (winner ? withdrawal * surface->w * 0.60 : 0));
	int right_x = (int)(surface->w + size * 4
		- approach * (surface->w * 0.38 + size * 4)
		+ (winner == 0 ? withdrawal * surface->w * 0.60 : 0));
	int left_count = 2 + (seed >> 5) % 3;
	int right_count = 2 + (seed >> 9) % 3;
	int arc = (int)(sin(ticks / 1750.0) * scale * 0.010);
	Uint8 alpha = (Uint8)(100 * fade);

	for(int side = 0; side < 2; side++) {
		int direction = side == 0 ? 1 : -1;
		int count = side == 0 ? left_count : right_count;
		int anchor_x = side == 0 ? left_x : right_x;
		int side_y = center_y + (side == 0 ? arc : -arc);
		Uint8 red = side == 0 ? 105 : 177;
		Uint8 green = side == 0 ? 163 : 130;
		Uint8 blue = side == 0 ? 183 : 102;
		for(int ship = 0; ship < count; ship++) {
			int lane = ship == 0 ? 0 : (ship % 2 ? (ship + 1) / 2 : -(ship / 2));
			int ship_x = anchor_x + direction * ship * size * 2
				+ (int)(sin(ticks / (620.0 + ship * 150) + side * 2.0) * size * 0.7);
			int ship_y = side_y + lane * scale / 34
				+ (int)(sin(ticks / (410.0 + ship * 93) + ship) * size * 0.7);
			unsigned int type = seed >> (ship + side * 5);
			if(ship == 0 && (type & 1)) {
				draw_cruiser(surface, ship_x, ship_y, size * 2, direction,
					red, green, blue, alpha);
			} else if(type % 3 == 0) {
				draw_frigate(surface, ship_x, ship_y, size, direction,
					red, green, blue, alpha);
			} else {
				draw_battle_ship(surface, ship_x, ship_y,
					size + (ship % 2), direction, red, green, blue, alpha);
			}
		}
	}

	Uint32 volley_phase = (phase + (seed >> 16)) % 2100;
	if(volley_phase < 240 && progress > 0.15 && progress < 0.86) {
		double shot_progress = (double)volley_phase / 240.0;
		for(int bolt = 0; bolt < 3; bolt++) {
			int lane_y = center_y + (bolt - 1) * scale / 36;
			int shot_x = left_x + size * 3
				+ (int)((right_x - left_x - size * 6) * shot_progress);
			int shot_y = lane_y + (int)(sin(shot_progress * 3.14159265359 + bolt) * scale / 42);
			thickLineRGBA(surface, shot_x - size, shot_y, shot_x, shot_y,
				2, 139, 208, 235, (Uint8)(alpha * 0.95));
			shot_x = right_x - size * 3
				- (int)((right_x - left_x - size * 6) * shot_progress);
			shot_y = lane_y - (int)(sin(shot_progress * 3.14159265359 + bolt) * scale / 42);
			thickLineRGBA(surface, shot_x + size, shot_y, shot_x, shot_y,
				2, 245, 164, 112, (Uint8)(alpha * 0.95));
		}
	}
	if(progress > 0.24 && progress < 0.84 && (phase % 2500) < 240) {
		double blast = 1.0 - (phase % 2500) / 240.0;
		int impact_x = (winner == 0 ? right_x : left_x)
				+ (int)(sin(ticks / 510.0) * scale * 0.015);
			int impact_y = center_y + (int)(cos(ticks / 390.0) * scale * 0.012);
		int radius = size / 2 + (int)(size * (1.0 - blast));
		Uint8 impact_alpha = (Uint8)(125 * fade * blast);
		filledCircleRGBA(surface, impact_x, impact_y, radius,
			230, 128, 79, impact_alpha);
		circleRGBA(surface, impact_x, impact_y, radius + size / 2,
			255, 202, 139, (Uint8)(impact_alpha * 0.8));
	}
}

static void render_fleet_battle(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_FLEET_BATTLE, ticks, &window)) return;
	Uint32 phase = window.phase;
	Uint32 seed = window.seed;
	int winner = (seed ^ (seed >> 13) ^ (seed >> 23)) & 1;
	double progress = window.progress;
	double fade = window.fade;
	double approach = smooth_step(0.04, 0.31, progress);
	double retreat = smooth_step(0.78, 0.97, progress);
	double defeat = smooth_step(0.48, 0.84, progress);
	int size = scale / 72;
	if(size < 10) size = 10;
	int fighter_size = size / 2;
	if(fighter_size < 7) fighter_size = 7;
	double left_hold = surface->w * (0.37 + 0.008 * sin(ticks / 3600.0));
	double right_hold = surface->w * (0.63 + 0.008 * sin(ticks / 3900.0 + 1.7));
	int left_x = (int)(-size * 6 + approach * (left_hold + size * 6)
		- (winner == 0 ? 0 : retreat * surface->w * 0.62));
	int right_x = (int)(surface->w + size * 6
		- approach * (surface->w - right_hold + size * 6)
		+ (winner == 1 ? 0 : retreat * surface->w * 0.62));
	int center_y = surface->h * 19 / 100;
	int arc = (int)(sin(ticks / 1900.0 + progress * 8.0) * scale * 0.010);
	int left_count = 3 + (seed >> 4) % 4;
	int right_count = 3 + (seed >> 12) % 4;
	Uint8 left_alpha = (Uint8)(96 * fade * (winner == 0 ? 1.0 : 1.0 - defeat * 0.75));
	Uint8 right_alpha = (Uint8)(96 * fade * (winner == 1 ? 1.0 : 1.0 - defeat * 0.75));

	for(int side = 0; side < 2; side++) {
		int direction = side == 0 ? 1 : -1;
		int count = side == 0 ? left_count : right_count;
		int anchor_x = side == 0 ? left_x : right_x;
		int side_arc = side == 0 ? arc : -arc;
		int red = side == 0 ? 100 : 170;
		int green = side == 0 ? 147 : 126;
		int blue = side == 0 ? 167 : 101;
		Uint8 side_alpha = side == 0 ? left_alpha : right_alpha;

		for(int ship = 0; ship < count; ship++) {
			int lane = ship == 0 ? 0 : (ship % 2 ? (ship + 1) / 2 : -(ship / 2));
			int offset = direction * ((ship + 1) / 2) * size * 3;
			int weave = (int)(sin(ticks / (510.0 + ship * 127)
				+ ship * 1.8 + side * 2.4) * scale * 0.009);
			int ship_x = anchor_x + offset + weave;
			int ship_y = center_y + side_arc + lane * scale / 30
				+ (int)(sin(ticks / (370.0 + ship * 83) + ship * 1.3) * size * 0.75);
			unsigned int type = seed >> ((ship * 3 + side * 7) % 24);
			if(ship == 0 && ((seed >> (side + 2)) & 1)) {
				draw_cruiser(surface, ship_x, ship_y, size * 2, direction,
					red, green, blue, side_alpha);
			} else if(type % 3 == 0) {
				draw_frigate(surface, ship_x, ship_y, size, direction,
					red, green, blue, side_alpha);
			} else {
				draw_battle_ship(surface, ship_x, ship_y,
					fighter_size + (ship % 3 == 0 ? 2 : 0), direction,
					red, green, blue, side_alpha);
			}
		}
	}

	double reinforcement = smooth_step(0.36, 0.45, progress)
		* (1.0 - smooth_step(0.70, 0.79, progress));
	if(reinforcement > 0.0) {
		int reserves = 1 + (seed >> 20) % 3;
		for(int side = 0; side < 2; side++) {
			int direction = side == 0 ? 1 : -1;
			int anchor_x = side == 0 ? left_x : right_x;
			int red = side == 0 ? 100 : 170;
			int green = side == 0 ? 147 : 126;
			int blue = side == 0 ? 167 : 101;
			for(int ship = 0; ship < reserves; ship++) {
				int x = anchor_x - direction * (int)(surface->w * 0.16 * (1.0 - reinforcement))
					- direction * ship * fighter_size * 3;
				int y = center_y + (side == 0 ? arc : -arc)
					+ (ship - reserves / 2) * scale / 25;
				draw_battle_ship(surface, x, y, fighter_size, direction,
					red, green, blue, (Uint8)(85 * fade * reinforcement));
			}
		}
	}

	Uint32 volley_phase = (phase + (seed >> 9)) % 1800;
	if(volley_phase < 280 && progress > 0.14 && progress < 0.88) {
		double shot_progress = (double)volley_phase / 280.0;
		for(int bolt = 0; bolt < 4; bolt++) {
			int lane_y = center_y + (bolt - 1) * scale / 34;
			int shot_x = left_x + size * 3
				+ (int)((right_x - left_x - size * 6) * shot_progress);
			int shot_y = lane_y + (int)(sin(shot_progress * 3.14159265359 + bolt) * scale / 38);
			thickLineRGBA(surface, shot_x - size * 2, shot_y, shot_x, shot_y,
				2, 135, 200, 230, (Uint8)(92 * fade));
			shot_x = right_x - size * 3
				- (int)((right_x - left_x - size * 6) * shot_progress);
			shot_y = lane_y - (int)(sin(shot_progress * 3.14159265359 + bolt) * scale / 38);
			thickLineRGBA(surface, shot_x + size * 2, shot_y, shot_x, shot_y,
				2, 239, 154, 108, (Uint8)(92 * fade));
		}
	}

	Uint32 impact_phase = (phase + (seed >> 16)) % 2300;
	if(progress > 0.22 && progress < 0.86 && impact_phase < 300) {
		unsigned int blast_seed = seed ^ 0xa511e9b3u ^ (phase / 450 * 0x9e3779b9u);
		int loser_x = winner == 0 ? right_x : left_x;
		int blast_x = loser_x + (int)(blast_seed % (unsigned int)(size * 10 + 1)) - size * 5;
		int blast_y = center_y + (int)((blast_seed >> 8) % (unsigned int)(scale / 10 + 1))
			- scale / 20;
		int blast_radius = size / 3 + (int)(size * impact_phase / 900.0);
		Uint8 blast_alpha = (Uint8)(130 * fade * (1.0 - impact_phase / 300.0));
		filledCircleRGBA(surface, blast_x, blast_y, blast_radius,
			225, 119, 72, blast_alpha);
		circleRGBA(surface, blast_x, blast_y, blast_radius + size / 2,
			255, 201, 137, (Uint8)(blast_alpha * 0.8));
	}
}

static void render_long_scene(SDL_Surface *surface, Uint32 ticks, int scale) {
		EffectWindow window;
		if(!effect_window(&EFFECT_LONG_SCENE, ticks, &window)) return;
		Uint32 seed = window.seed;
		double progress = window.progress;
		double fade = window.fade;
		int kind = effect_kind(&EFFECT_LONG_SCENE, seed % 5);

		if(kind == 0) {
			for(int tree = 0; tree < 7; tree++) {
				int tree_x = surface->w * (8 + tree * 13) / 100;
				double nx = (double)tree_x / surface->w;
				double ridge = ridge_height(RIDGE_NEAR, nx);
				int base_y = (int)(surface->h * ridge);
				double growth = smooth_step(0.03 + tree * 0.025, 0.72 + tree * 0.025, progress);
				int tree_height = (int)(scale * (0.035 + (tree % 3) * 0.012) * growth);
				if(tree_height < 2) continue;
				Uint8 alpha = (Uint8)(175 * fade);
				thickLineRGBA(surface, tree_x, base_y, tree_x, base_y - tree_height,
					2 + tree % 2, 61, 72, 61, alpha);
				for(int branch = 1; branch <= 3; branch++) {
					int branch_y = base_y - tree_height * branch / 4;
					int branch_length = tree_height * (4 - branch) / 5;
					thickLineRGBA(surface, tree_x, branch_y,
						tree_x - branch_length, branch_y - branch_length / 2,
						2, 67, 82, 66, alpha);
					thickLineRGBA(surface, tree_x, branch_y,
						tree_x + branch_length, branch_y - branch_length / 2,
						2, 67, 82, 66, alpha);
					if(growth > 0.72) {
						filledCircleRGBA(surface, tree_x - branch_length,
							branch_y - branch_length / 2, branch_length / 3 + 1,
							74, 112, 78, alpha);
						filledCircleRGBA(surface, tree_x + branch_length,
							branch_y - branch_length / 2, branch_length / 3 + 1,
							86, 124, 83, alpha);
					}
				}
			}
		} else if(kind == 1) {
			int flood_y = surface->h * (82 - (int)(progress * 9)) / 100;
			boxRGBA(surface, 0, flood_y, surface->w - 1, surface->h - 1,
				28, 79, 116, (Uint8)(58 * fade));
			for(int band = 0; band < 5; band++) {
				int previous_x = 0;
				int previous_y = flood_y + band * scale / 38;
				for(int segment = 1; segment <= 20; segment++) {
					int next_x = surface->w * segment / 20;
					int next_y = flood_y + band * scale / 38
						+ (int)(sin(segment * 0.8 + ticks / (500.0 + band * 80))
							* scale * 0.012);
					thickLineRGBA(surface, previous_x, previous_y, next_x, next_y,
						1 + band % 2, 115, 183, 208, (Uint8)(70 * fade));
					previous_x = next_x;
					previous_y = next_y;
				}
			}
		} else if(kind == 2) {
			boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
				135, 57, 27, (Uint8)(17 * fade));
			for(int fire = 0; fire < 12; fire++) {
				int fire_x = surface->w * (8 + fire * 7) / 100;
				double nx = (double)fire_x / surface->w;
				double ridge = ridge_height(RIDGE_MIDDLE, nx);
				int fire_y = (int)(surface->h * ridge);
				int flicker = (int)(fabs(sin(ticks / (170.0 + fire * 11) + fire)) * scale * 0.025);
				thickLineRGBA(surface, fire_x, fire_y, fire_x + 2, fire_y - flicker,
					2, 238, 112, 55, (Uint8)(120 * fade));
				filledCircleRGBA(surface, fire_x + 4, fire_y - flicker, 2,
					255, 192, 106, (Uint8)(145 * fade));
			}
		} else if(kind == 3) {
			boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
				36, 57, 78, (Uint8)(24 * fade));
			for(int band = 0; band < 6; band++) {
				int previous_x = 0;
				int previous_y = surface->h * (17 + band * 10) / 100;
				for(int segment = 1; segment <= 18; segment++) {
					int next_x = surface->w * segment / 18;
					int next_y = surface->h * (17 + band * 10) / 100
						+ (int)(sin(segment * 0.6 + ticks / 1400.0 + band) * scale * 0.05);
					thickLineRGBA(surface, previous_x, previous_y, next_x, next_y, 2,
						78, 162, 163, (Uint8)(28 * fade));
					previous_x = next_x;
					previous_y = next_y;
				}
			}
		} else {
			boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
				17, 20, 32, (Uint8)(34 * fade));
			int figure_x = surface->w * (25 + seed % 50) / 100;
			int figure_y = surface->h * 83 / 100;
			int figure_height = scale / 10;
			filledTrigonRGBA(surface, figure_x, figure_y - figure_height,
				figure_x - figure_height / 3, figure_y,
				figure_x + figure_height / 3, figure_y, 12, 16, 25,
				(Uint8)(150 * fade));
			filledCircleRGBA(surface, figure_x, figure_y - figure_height,
				figure_height / 6, 18, 22, 30, (Uint8)(160 * fade));
		}
	}

/*
 * Rare sky events
 *
 * Every rare sky event is a small function taking a SkyEvent: an object that
 * crosses the sky (x, y) in `direction`, `size` pixels big, with `alpha`
 * already faded for fade-in/out. progress/phase say how far along it is.
 * To add one: write sky_event_NN(), append it to sky_event_kinds[] below and
 * bump RARE_SKY_KINDS.
 */
typedef struct {
	SDL_Surface *surface;
	Uint32 ticks;
	int scale;             // shorter screen side
	Uint32 seed;
	Uint32 phase;          // ms since the event started
	double progress;       // 0..1 through the event
	double fade;           // 0..1 fade in/out
	int direction;         // 1: moves left to right, -1: right to left
	int x, y;              // current position of the object
	int size;
	Uint8 alpha;           // 150 * fade
} SkyEvent;

#define SKY_EVENT_UNPACK(e) \
	SDL_Surface *surface = (e)->surface; \
	Uint32 ticks = (e)->ticks; \
	int scale = (e)->scale; \
	Uint32 seed = (e)->seed; \
	Uint32 phase = (e)->phase; \
	double progress = (e)->progress; \
	double fade = (e)->fade; \
	int direction = (e)->direction; \
	int x = (e)->x, y = (e)->y; \
	int center_x = x, center_y = y; \
	int size = (e)->size; \
	Uint8 alpha = (e)->alpha; \
	(void)surface; (void)ticks; (void)scale; (void)seed; (void)phase; \
	(void)progress; (void)fade; (void)direction; (void)x; (void)y; \
	(void)center_x; (void)center_y; (void)size; (void)alpha

static void sky_event_23(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int ground_x = center_x < 0 ? 0 : center_x;
			if(ground_x >= surface->w) ground_x = surface->w - 1;
			double nx = (double)ground_x / surface->w;
			double ridge = ridge_height(RIDGE_NEAR, nx);
			int ground_y = (int)(surface->h * ridge);
			Uint8 pulse = (Uint8)(alpha * (0.45 + 0.55 * (0.5 + 0.5 * sin(progress * 18.0))));
			boxRGBA(surface, center_x - size, ground_y - size / 2,
				center_x + size, ground_y + size / 2, 35, 38, 42, alpha);
			filledCircleRGBA(surface, center_x - size / 2, ground_y - size / 2,
				2, 83, 123, 77, alpha);
			filledCircleRGBA(surface, center_x - size, ground_y - size,
				size / 2, 83, 56, 55, pulse);
			filledCircleRGBA(surface, center_x + size, ground_y - size,
				size / 2, 54, 75, 120, pulse);
}

static void sky_event_24(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int fold_size = size * 3;
			int unfold = (int)(fold_size * (0.35 + 0.65 * sin(progress * 3.14159265359)));
			filledTrigonRGBA(surface, center_x, center_y - fold_size,
				center_x - unfold, center_y + fold_size / 2,
				center_x, center_y, 167, 193, 205, alpha);
			filledTrigonRGBA(surface, center_x, center_y - fold_size,
				center_x, center_y, center_x + unfold, center_y + fold_size / 2,
				211, 177, 137, alpha);
			filledTrigonRGBA(surface, center_x, center_y - fold_size,
				center_x - fold_size / 3, center_y + fold_size,
				center_x + fold_size / 3, center_y + fold_size, 120, 151, 169, alpha);
			thickLineRGBA(surface, center_x, center_y - fold_size,
				center_x, center_y + fold_size, 1, 244, 227, 198, alpha);
			for(int crease = 1; crease <= 4; crease++) {
				int crease_x = center_x + (crease % 2 ? unfold : -unfold) * crease / 4;
				thickLineRGBA(surface, center_x, center_y - fold_size + crease * fold_size / 2,
					crease_x, center_y + fold_size / 2, 1, 235, 220, 193, alpha);
			}
}

static void sky_event_25(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int wheel_radius = scale / 180;
			if(wheel_radius < 3) wheel_radius = 3;
			int wheelbase = scale / 18;
			if(wheelbase < 34) wheelbase = 34;
			int rear_x = direction > 0
				? -wheelbase + (int)(progress * (surface->w + 2.0 * wheelbase))
				: surface->w + wheelbase - (int)(progress * (surface->w + 2.0 * wheelbase));
			int front_x = rear_x + direction * wheelbase;
			int ridge_x = rear_x + direction * wheelbase / 2;
			if(ridge_x < 0) ridge_x = 0;
			if(ridge_x >= surface->w) ridge_x = surface->w - 1;
			double nx = (double)ridge_x / surface->w;
			double ridge = ridge_height(RIDGE_NEAR, nx);
			int wheel_y = (int)(surface->h * ridge) - wheel_radius;
			int seat_x = rear_x + direction * wheelbase * 38 / 100;
			int crank_x = rear_x + direction * wheelbase * 64 / 100;
			int seat_y = wheel_y - wheel_radius * 3;
			circleRGBA(surface, rear_x, wheel_y, wheel_radius, 26, 30, 36, alpha);
			circleRGBA(surface, front_x, wheel_y, wheel_radius, 26, 30, 36, alpha);
			for(int spoke = 0; spoke < 6; spoke++) {
				double spoke_angle = progress * 6.28318530718 + spoke * 1.0471975512;
				int spoke_dx = (int)(cos(spoke_angle) * wheel_radius);
				int spoke_dy = (int)(sin(spoke_angle) * wheel_radius);
				thickLineRGBA(surface, rear_x, wheel_y, rear_x + spoke_dx,
					wheel_y + spoke_dy, 1, 119, 130, 137, alpha);
				thickLineRGBA(surface, front_x, wheel_y, front_x + spoke_dx,
					wheel_y + spoke_dy, 1, 119, 130, 137, alpha);
			}
			thickLineRGBA(surface, rear_x, wheel_y, seat_x, seat_y, 2, 205, 120, 74, alpha);
			thickLineRGBA(surface, seat_x, seat_y, crank_x, wheel_y, 2, 205, 120, 74, alpha);
			thickLineRGBA(surface, crank_x, wheel_y, rear_x, wheel_y, 2, 205, 120, 74, alpha);
			thickLineRGBA(surface, seat_x, seat_y, front_x, wheel_y, 2, 205, 120, 74, alpha);
			thickLineRGBA(surface, seat_x, seat_y - wheel_radius * 2,
				crank_x, wheel_y - wheel_radius * 4, 3, 54, 65, 77, alpha);
			double pedal_angle = progress * 12.56637061436;
			int pedal_x = crank_x + (int)(cos(pedal_angle) * wheel_radius * 2);
			int pedal_y = wheel_y + (int)(sin(pedal_angle) * wheel_radius * 2);
			int handlebar_x = front_x - direction * wheelbase / 8;
			int handlebar_y = wheel_y - wheel_radius * 3;
			thickLineRGBA(surface, seat_x, seat_y - wheel_radius * 2,
				handlebar_x, handlebar_y, 2, 54, 65, 77, alpha);
			thickLineRGBA(surface, seat_x, seat_y - wheel_radius * 2,
				pedal_x, pedal_y, 2, 54, 65, 77, alpha);
			thickLineRGBA(surface, pedal_x, pedal_y, crank_x, wheel_y,
				1, 205, 120, 74, alpha);
			filledCircleRGBA(surface, crank_x, wheel_y - wheel_radius * 6,
				wheel_radius / 2 + 1, 224, 194, 151, alpha);
}

static void sky_event_26(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			for(int creature = 0; creature < 5; creature++) {
				int creature_x = center_x + (creature - 2) * size * 2;
				int creature_y = center_y + (int)(sin(progress * 6.28 + creature) * size * 2);
				thickLineRGBA(surface, creature_x - size / 2, creature_y,
					creature_x, creature_y - size / 2, 1, 25, 30, 36, alpha);
				thickLineRGBA(surface, creature_x, creature_y - size / 2,
					creature_x + size / 2, creature_y, 1, 25, 30, 36, alpha);
			}
			int fish_y = surface->h * 61 / 100
				- (int)(sin(progress * 3.14159265359) * scale * 0.14);
			filledTrigonRGBA(surface, center_x + size * 2, fish_y,
				center_x - size, fish_y - size, center_x - size, fish_y + size,
				105, 187, 203, alpha);
			filledTrigonRGBA(surface, center_x - size, fish_y - size,
				center_x - size * 2, fish_y, center_x - size, fish_y + size,
				84, 157, 176, alpha);
}

static void sky_event_27(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			for(int cube = 0; cube < 4; cube++) {
				int cube_x = center_x + (cube - 2) * size * 2;
				int cube_y = center_y + (int)(sin(progress * 6.28 + cube) * size * 2);
				int half = size / 2 + 2;
				double rotation = progress * 6.28318530718 + cube * 0.9;
				int offset_x = (int)(cos(rotation) * half);
				int offset_y = (int)(sin(rotation) * half);
				int top_x = cube_x + offset_x;
				int top_y = cube_y - offset_y;
				int right_x = cube_x + offset_y;
				int right_y = cube_y + offset_x;
				int bottom_x = cube_x - offset_x;
				int bottom_y = cube_y + offset_y;
				int left_x = cube_x - offset_y;
				int left_y = cube_y - offset_x;
				filledTrigonRGBA(surface, top_x, top_y,
					right_x, right_y, bottom_x, bottom_y,
					155, 214, 237, (Uint8)(alpha * 0.38));
				filledTrigonRGBA(surface, top_x, top_y,
					left_x, left_y, bottom_x, bottom_y,
					200, 230, 241, (Uint8)(alpha * 0.28));
				thickLineRGBA(surface, top_x, top_y, bottom_x, bottom_y,
					1, 224, 245, 251, alpha);
				filledCircleRGBA(surface, top_x, top_y, 1, 255, 255, 255, alpha);
			}
}

static void sky_event_28(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			for(int card = 0; card < 3; card++) {
				int card_x = center_x + (card - 1)
					* (int)(size * (2.0 + 0.3 * sin(progress * 6.28318530718 + card)));
				int card_y = center_y - abs(card - 1) * size / 2
					+ (int)(cos(progress * 6.28318530718 + card) * size / 3);
				int card_half_width = (int)(size * (0.18
					+ 0.32 * fabs(cos(progress * 6.28318530718 + card))));
				if(card_half_width < 2) card_half_width = 2;
				boxRGBA(surface, card_x - card_half_width, card_y - size,
					card_x + card_half_width, card_y + size, 225, 220, 203, alpha);
				thickLineRGBA(surface, card_x, card_y - size / 2,
					card_x, card_y + size / 2, 1, 163, 57, 64, alpha);
			}
			int coin_radius = size / 2 + 2;
			int coin_x = center_x + (int)(sin(progress * 12.56) * size * 4);
			int coin_y = center_y - size * 2 - (int)(fabs(sin(progress * 12.56)) * size * 3);
			filledCircleRGBA(surface, coin_x, coin_y, coin_radius, 205, 161, 83, alpha);
			circleRGBA(surface, coin_x, coin_y, coin_radius - 1, 255, 226, 159, alpha);
}

static void sky_event_29(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
				82, 17, 29, (Uint8)(15 * fade));
			unsigned int bolt_seed = seed;
			int bolt_x = center_x;
			int bolt_y = surface->h * 12 / 100;
			for(int bolt = 0; bolt < 8; bolt++) {
				bolt_seed = bolt_seed * 1664525u + 1013904223u;
				int next_x = bolt_x + (int)(bolt_seed % (scale / 12 + 1)) - scale / 24;
				int next_y = bolt_y + scale / 15;
				thickLineRGBA(surface, bolt_x, bolt_y, next_x, next_y,
					2, 232, 67, 83, alpha);
				bolt_x = next_x;
				bolt_y = next_y;
			}
}

static void sky_event_30(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			for(int cup = 0; cup < 3; cup++) {
				int cup_x = center_x + (cup - 1) * size * 3;
				int cup_y = center_y + abs(cup - 1) * size / 3;
				boxRGBA(surface, cup_x - size, cup_y - size,
					cup_x + size, cup_y + size, 211, 204, 190, alpha);
				filledCircleRGBA(surface, cup_x, cup_y - size, size / 2,
					(Uint8)(105 + cup * 33), (Uint8)(59 + cup * 21),
					(Uint8)(36 + cup * 19), alpha);
				circleRGBA(surface, cup_x + size, cup_y, size / 2,
					211, 204, 190, alpha);
				for(int steam = 0; steam < 3; steam++) {
					int steam_x = cup_x - size / 2 + steam * size / 2;
					int steam_y = cup_y - size * 2
						- (int)(fabs(sin(progress * 8.0 + steam)) * size);
					thickLineRGBA(surface, steam_x, steam_y + size,
						steam_x + size / 4, steam_y, 1, 206, 206, 198,
						(Uint8)(alpha * 0.65));
				}
			}
}

static void sky_event_31(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			for(int sprig = 0; sprig < 5; sprig++) {
				int stem_x = center_x + (sprig - 2) * size;
				int stem_y = center_y + size * 2;
				int tip_y = stem_y - size * (3 + sprig % 3);
				int sway = (int)(sin(ticks / 330.0 + sprig * 1.4) * size / 5);
				thickLineRGBA(surface, stem_x, stem_y, stem_x + sway, tip_y,
					2, 92, 130, 73, alpha);
				for(int leaf = 1; leaf <= 4; leaf++) {
					int leaf_y = stem_y - leaf * size * 3 / 4;
					int leaf_direction = leaf % 2 ? -1 : 1;
					int leaf_x = stem_x + sway * leaf / 4;
					int leaf_tip_x = leaf_x + leaf_direction * size;
					filledTrigonRGBA(surface, leaf_x, leaf_y,
						leaf_tip_x, leaf_y - size / 3,
						leaf_tip_x, leaf_y + size / 3,
						(Uint8)(74 + sprig * 6), (Uint8)(126 + leaf * 9), 71, alpha);
				}
			}
}

static void sky_event_32(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			for(int light = 0; light < 18; light++) {
				int light_x = (seed + light * surface->w / 18) % surface->w;
				int light_y = surface->h * (14 + (light * 17 + seed) % 62) / 100;
				Uint8 pulse = (Uint8)(alpha * (0.35 + 0.65
					* (0.5 + 0.5 * sin(progress * 18.0 + light))));
				filledCircleRGBA(surface, light_x, light_y, 2 + light % 3,
					(Uint8)(90 + light * 7), (Uint8)(129 + light * 5),
					(Uint8)(176 + light * 3), pulse);
			}
}

static void sky_event_33(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int burner_y = surface->h * 78 / 100;
			filledTrigonRGBA(surface, center_x, burner_y - size,
				center_x - size, burner_y + size / 2,
				center_x + size, burner_y + size / 2, 104, 87, 67, alpha);
			for(int curl = 0; curl < 4; curl++) {
				int smoke_x = center_x + (int)(sin(progress * 7.0 + curl) * size * 2);
				int smoke_y = burner_y - size * (2 + curl)
					- (int)(progress * size * (curl + 1));
				filledCircleRGBA(surface, smoke_x, smoke_y, size / 3 + 1,
					(Uint8)(151 + curl * 12), (Uint8)(159 + curl * 10),
					(Uint8)(166 + curl * 8), (Uint8)(alpha * (0.45 - curl * 0.07)));
			}
}

static void sky_event_34(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			for(int wisp = 0; wisp < 7; wisp++) {
				int smoke_x = center_x + (int)(sin(progress * 5.0 + wisp) * size * (2 + wisp));
				int smoke_y = center_y - wisp * size / 2;
				filledCircleRGBA(surface, smoke_x, smoke_y, size / 2 + 1,
					(Uint8)(143 + wisp * 9), (Uint8)(150 + wisp * 8),
					(Uint8)(162 + wisp * 7), (Uint8)(alpha * 0.42));
			}
}

static void sky_event_35(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int temple_y = surface->h * 71 / 100;
			for(int step = 0; step < 5; step++) {
				int half_width = size * (5 - step);
				int top_y = temple_y - step * size / 2;
				boxRGBA(surface, center_x - half_width, top_y - size / 2,
					center_x + half_width, top_y + size / 2,
					(Uint8)(116 + step * 12), (Uint8)(102 + step * 8), 83, alpha);
			}
			for(int ray = 0; ray < 12; ray++) {
				double angle = ray * 0.52359877559 + progress;
				int end_x = center_x + (int)(cos(angle) * size * 5);
				int end_y = surface->h * 32 / 100 + (int)(sin(angle) * size * 3);
				thickLineRGBA(surface, center_x, surface->h * 32 / 100,
					end_x, end_y, 1, 218, 183, 120, (Uint8)(alpha * 0.45));
			}
}

static void sky_event_36(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
				24, 12, 39, (Uint8)(20 * fade));
			int eye_x = center_x;
			int eye_y = surface->h * 36 / 100;
			int eye_radius = size * 2;
			filledCircleRGBA(surface, eye_x, eye_y, eye_radius, 5, 4, 14, alpha);
			circleRGBA(surface, eye_x, eye_y, eye_radius + size,
				117, 89, 161, (Uint8)(alpha * 0.8));
			for(int tendril = 0; tendril < 8; tendril++) {
				double angle = tendril * 0.78539816339 + progress * direction;
				int previous_x = eye_x + (int)(cos(angle) * eye_radius);
				int previous_y = eye_y + (int)(sin(angle) * eye_radius);
				for(int segment = 1; segment <= 5; segment++) {
					int next_x = eye_x + (int)(cos(angle) * (eye_radius + segment * size));
					int next_y = eye_y + (int)(sin(angle) * (eye_radius + segment * size)
						+ sin(progress * 10.0 + segment + tendril) * size / 2);
					thickLineRGBA(surface, previous_x, previous_y, next_x, next_y,
						2, 83, 68, 127, (Uint8)(alpha * 0.72));
					previous_x = next_x;
					previous_y = next_y;
				}
			}
}

static void sky_event_37(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int mirror_x = surface->w * (30 + (seed >> 8) % 40) / 100;
			int mirror_y = surface->h * 43 / 100;
			int mirror_width = size * 6;
			int mirror_height = size * 10;
			boxRGBA(surface, mirror_x - mirror_width / 2, mirror_y - mirror_height / 2,
				mirror_x + mirror_width / 2, mirror_y + mirror_height / 2,
				37, 55, 76, (Uint8)(alpha * 0.45));
			thickLineRGBA(surface, mirror_x - mirror_width / 2, mirror_y - mirror_height / 2,
				mirror_x + mirror_width / 2, mirror_y - mirror_height / 2,
				2, 205, 221, 231, alpha);
			thickLineRGBA(surface, mirror_x - mirror_width / 2, mirror_y + mirror_height / 2,
				mirror_x + mirror_width / 2, mirror_y + mirror_height / 2,
				2, 205, 221, 231, alpha);
			for(int reflection = 0; reflection < 8; reflection++) {
				int line_y = mirror_y - mirror_height / 2 + reflection * mirror_height / 7;
				int ripple = (int)(sin(progress * 12.0 + reflection) * size / 2);
				thickLineRGBA(surface, mirror_x - mirror_width / 2 + size / 3 + ripple,
					line_y, mirror_x + mirror_width / 2 - size / 3 - ripple,
					line_y + size / 4, 1, 150, 196, 218,
					(Uint8)(alpha * (0.45 + reflection * 0.04)));
			}
			int reflected_sun_y = mirror_y + mirror_height / 3
				+ (int)(sin(progress * 8.0) * size / 2);
			filledCircleRGBA(surface, mirror_x, reflected_sun_y, size / 2,
				226, 218, 188, (Uint8)(alpha * 0.72));
}

static void sky_event_38(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int water_y = surface->h * 77 / 100;
			for(int wave = 0; wave < 9; wave++) {
				int previous_x = 0;
				int previous_y = water_y + wave * size / 2;
				for(int segment = 1; segment <= 16; segment++) {
					int next_x = surface->w * segment / 16;
					int next_y = water_y + wave * size / 2
						+ (int)(sin(segment * 0.75 + progress * 10.0 + wave) * size / 3);
					thickLineRGBA(surface, previous_x, previous_y, next_x, next_y,
						1, 101, 156, 190, (Uint8)(alpha * 0.55));
					previous_x = next_x;
					previous_y = next_y;
				}
			}
			for(int glint = 0; glint < 14; glint++) {
				int glint_x = (seed + glint * surface->w / 14) % surface->w;
				int glint_y = water_y + (int)(sin(progress * 8.0 + glint) * size * 2);
				thickLineRGBA(surface, glint_x - size / 3, glint_y,
					glint_x + size / 3, glint_y, 1,
					192, 222, 231, (Uint8)(alpha * 0.65));
			}
}

static void sky_event_39(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int orbit_x = center_x;
			int orbit_y = center_y;
			int orbit_radius = size * 5;
			ellipseRGBA(surface, orbit_x, orbit_y, orbit_radius,
				orbit_radius / 2, 137, 172, 201, (Uint8)(alpha * 0.6));
			for(int satellite = 0; satellite < 5; satellite++) {
				double angle = progress * 12.56637061436 + satellite * 1.25663706144;
				int satellite_x = orbit_x + (int)(cos(angle) * orbit_radius);
				int satellite_y = orbit_y + (int)(sin(angle) * orbit_radius / 2);
				filledCircleRGBA(surface, satellite_x, satellite_y,
					2 + satellite % 3, 180, 204, 221, alpha);
			}
			filledCircleRGBA(surface, orbit_x, orbit_y, size,
				(Uint8)(180 + (int)(35 * sin(progress * 12.56))), 141, 215, alpha);
}

static void sky_event_40(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int plane_y = center_y + (int)(sin(progress * 6.28318530718) * size * 2);
			int nose_x = center_x + direction * size * 3;
			int tail_x = center_x - direction * size * 3;
			thickLineRGBA(surface, tail_x, plane_y, nose_x, plane_y,
				3, 140, 153, 164, alpha);
			thickLineRGBA(surface, center_x - direction * size,
				plane_y - size / 5, center_x + direction * size,
				plane_y + size / 5, 2, 170, 182, 191, alpha);
			filledTrigonRGBA(surface, tail_x + direction * size,
				plane_y, tail_x - direction * size, plane_y - size,
				tail_x - direction * size, plane_y + size,
				126, 141, 154, alpha);
			if((phase / 240) % 2 == 0) {
				filledCircleRGBA(surface, center_x - size * 2, plane_y,
					2, 255, 54, 45, alpha);
				filledCircleRGBA(surface, center_x + size * 2, plane_y,
					2, 255, 54, 45, alpha);
			}
}

static void sky_event_41(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int planet_x = surface->w / 2;
			int planet_y = surface->h * 66 / 100;
			int satellite_x = planet_x + (int)(cos(progress * 6.28318530718) * scale * 0.22);
			int satellite_y = planet_y + (int)(sin(progress * 6.28318530718) * scale * 0.11);
			filledCircleRGBA(surface, planet_x, planet_y, scale / 12,
				32, 46, 65, (Uint8)(alpha * 0.6));
			int panel_width = size * 2;
			boxRGBA(surface, satellite_x - panel_width * 2, satellite_y - size / 3,
				satellite_x - size, satellite_y + size / 3,
				79, 133, 163, alpha);
			boxRGBA(surface, satellite_x + size, satellite_y - size / 3,
				satellite_x + panel_width * 2, satellite_y + size / 3,
				79, 133, 163, alpha);
			boxRGBA(surface, satellite_x - size, satellite_y - size / 2,
				satellite_x + size, satellite_y + size / 2,
				151, 164, 170, alpha);
			if((phase / 300) % 2 == 0) {
				filledCircleRGBA(surface, satellite_x, satellite_y,
					2, 255, 48, 41, alpha);
			}
}

static void sky_event_42(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
			int blimp_y = center_y + (int)(sin(progress * 6.28318530718) * size);
			int blimp_length = size * 5;
			for(int section = -2; section <= 2; section++) {
				int section_radius = size * (3 - abs(section)) / 2;
				if(section_radius < 2) section_radius = 2;
				filledCircleRGBA(surface, center_x + section * size,
					blimp_y, section_radius, 165, 169, 158, alpha);
			}
			thickLineRGBA(surface, center_x - blimp_length / 3,
				blimp_y + size / 2, center_x + blimp_length / 3,
				blimp_y + size / 2, 2, 100, 103, 99, alpha);
			filledTrigonRGBA(surface, center_x + direction * blimp_length / 2,
				blimp_y, center_x + direction * blimp_length / 2 - direction * size,
				blimp_y - size, center_x + direction * blimp_length / 2 - direction * size,
				blimp_y + size, 127, 137, 133, alpha);
			if((phase / 360) % 2 == 0) {
				filledCircleRGBA(surface, center_x, blimp_y,
					2, 255, 48, 41, alpha);
			}
}

#define SKY_CLASSIC_VARIANTS 3

// The three classic sky objects: pulsing star, comet and shooting star.
static void sky_event_classic(const SkyEvent *event, int variant) {
	SKY_EVENT_UNPACK(event);
	switch(variant) {
		case 0: {
			int pulse_radius = size + (int)(progress * size * 2);
			filledCircleRGBA(surface, x, y, size / 2 + 1,
				245, 233, 207, alpha);
			circleRGBA(surface, x, y, pulse_radius,
				162, 193, 213, (Uint8)(alpha * 0.72));
			for(int ray = 0; ray < 8; ray++) {
				double angle = ray * 0.78539816339 + progress;
				int ray_x = x + (int)(cos(angle) * pulse_radius * 1.7);
				int ray_y = y + (int)(sin(angle) * pulse_radius * 1.7);
				thickLineRGBA(surface, x, y, ray_x, ray_y, 1,
					180, 203, 220, (Uint8)(alpha * 0.35));
			}
			break;
		}
		case 1: {
			int tail_x = x - direction * size * 4;
			int tail_y = y - size;
			thickLineRGBA(surface, x, y, tail_x, tail_y,
				2, 194, 205, 218, alpha);
			filledCircleRGBA(surface, x, y, size / 2,
				227, 224, 211, alpha);
			break;
		}
		default: {
			int trail_x = x - direction * size * 4;
			int trail_y = y + size;
			thickLineRGBA(surface, trail_x, trail_y, x, y,
				2, 202, 177, 130, alpha);
			filledCircleRGBA(surface, x, y, size / 2,
				235, 221, 185, alpha);
			for(int fragment = 1; fragment <= 2; fragment++) {
				int fragment_x = x - direction * size * (fragment + 1);
				int fragment_y = y + size * fragment;
				pixelRGBA(surface, fragment_x, fragment_y,
					214, 193, 159, (Uint8)(alpha * 0.75));
			}
			break;
		}
	}
}

static void (*const sky_event_kinds[])(const SkyEvent *) = {
	sky_event_23, sky_event_24, sky_event_25, sky_event_26, sky_event_27,
	sky_event_28, sky_event_29, sky_event_30, sky_event_31, sky_event_32,
	sky_event_33, sky_event_34, sky_event_35, sky_event_36, sky_event_37,
	sky_event_38, sky_event_39, sky_event_40, sky_event_41, sky_event_42
};
static_assert(RARE_SKY_CLASSIC_KINDS + sizeof(sky_event_kinds) / sizeof(sky_event_kinds[0])
	== RARE_SKY_KINDS, "RARE_SKY_KINDS does not match sky_event_kinds[]");

static void render_rare_sky_event(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_RARE_SKY, ticks, &window)) return;

	SkyEvent event;
	event.surface = surface;
	event.ticks = ticks;
	event.scale = scale;
	event.seed = window.seed;
	event.phase = window.phase;
	event.progress = window.progress;
	event.fade = window.fade;
	event.direction = (window.seed & 1) ? 1 : -1;
	event.y = surface->h * (8 + (window.seed >> 8) % 39) / 100;
	int margin = scale / 12;
	event.x = event.direction > 0
		? -margin + (int)(window.progress * (surface->w + 2.0 * margin))
		: surface->w + margin - (int)(window.progress * (surface->w + 2.0 * margin));
	event.size = scale / (26 + (window.seed >> 16) % 24);
	if(event.size < 4) event.size = 4;
	event.alpha = (Uint8)(150 * window.fade);

	// Kinds >= RARE_SKY_CLASSIC_KINDS are thinned out by rare_sky_accept().
	int kind = effect_kind(&EFFECT_RARE_SKY, (window.seed >> 24) % RARE_SKY_KINDS);
	if(kind < RARE_SKY_CLASSIC_KINDS) {
		sky_event_classic(&event, kind % SKY_CLASSIC_VARIANTS);
	} else {
		sky_event_kinds[kind - RARE_SKY_CLASSIC_KINDS](&event);
	}
}

static void render_foreground_event(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_FOREGROUND, ticks, &window)) return;
	Uint32 seed = window.seed;
	int kind = effect_kind(&EFFECT_FOREGROUND, seed % 4);
	double progress = window.progress;
	double fade = window.fade;
	int count = 24 + kind * 7;
	for(int particle = 0; particle < count; particle++) {
		Uint32 particle_seed = (Uint32)particle * 1664525u + seed;
		int start_x = particle_seed % surface->w;
		int start_y = (particle_seed >> 9) % surface->h;
		int speed = 3 + (particle_seed >> 20) % 8;
		int drift = (int)(ticks / (30 + speed * 7));
		int particle_x = (start_x + drift + surface->w) % surface->w;
		int particle_y = start_y;
		Uint8 red = 191, green = 204, blue = 215;
		Uint8 alpha = (Uint8)((18 + (particle_seed >> 24) % 32) * fade);
		if(kind == 0) {
			particle_y = (start_y + drift * 2) % surface->h;
			particle_x += (int)(sin(ticks / 500.0 + particle) * scale * 0.02);
			red = 207;
			green = (Uint8)(91 + particle % 5 * 18);
			blue = 54;
			thickLineRGBA(surface, particle_x, particle_y,
				particle_x - 2, particle_y + 4, 1, red, green, blue, alpha);
		} else if(kind == 1) {
			particle_y = (start_y - drift + surface->h) % surface->h;
			red = 245;
			green = (Uint8)(112 + particle % 4 * 18);
			blue = 67;
			filledCircleRGBA(surface, particle_x, particle_y, 1 + particle % 2,
				red, green, blue, alpha);
		} else if(kind == 2) {
			particle_y = (start_y + drift * 3) % surface->h;
			red = 167;
			green = 213;
			blue = 231;
			filledCircleRGBA(surface, particle_x, particle_y, 1 + particle % 2,
				red, green, blue, alpha);
		} else {
			particle_y = surface->h * (28 + particle_seed % 58) / 100
				- (int)(progress * scale * (0.025 + particle % 3 * 0.01));
			int radius = scale / 230;
			if(radius < 2) radius = 2;
			filledCircleRGBA(surface, particle_x, particle_y, radius,
				153, 167, 180, (Uint8)(alpha * 0.3));
		}
	}
}

static void render_rare_screen_fade(SDL_Surface *surface, Uint32 ticks) {
	EffectWindow window;
	if(!effect_window(&EFFECT_SCREEN_FADE, ticks, &window)) return;
	Uint8 color = (window.seed & 0x80) ? 255 : 0;
	boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
		color, color, color, (Uint8)(255 * window.fade));
}

void render_backdrop(SDL_Surface *surface, Uint32 ticks) {
	int scale = surface->h < surface->w ? surface->h : surface->w;
	if(scale < 1) scale = 1;
	if(surface->format->BytesPerPixel != 4) {
		fprintf(stderr, "Unsupported screen pixel format: expected 32-bit pixels\n");
		return;
	}
	if(backdrop_base && (backdrop_base->w != surface->w || backdrop_base->h != surface->h)) {
		SDL_FreeSurface(backdrop_base);
		backdrop_base = NULL;
	}

	bool cached = backdrop_base && SDL_BlitSurface(backdrop_base, NULL, surface, NULL) == 0;
	if(backdrop_base && !cached) {
		fprintf(stderr, "Unable to restore backdrop cache: %s\n", SDL_GetError());
	}
	if(!cached) {
		if(SDL_MUSTLOCK(surface) && SDL_LockSurface(surface) < 0) {
			fprintf(stderr, "Unable to lock screen surface: %s\n", SDL_GetError());
			return;
		}

		double drift = ticks * 0.000018;
		for(int y = 0; y < surface->h; y += 4) {
			double ny = (double)(y + 2) / surface->h;
			double sky_blend = ny / 0.68;
			for(int x = 0; x < surface->w; x += 4) {
				double nx = (double)(x + 2) / surface->w;
				double fine = smooth_noise(nx * 25.0 + drift * 0.35, ny * 21.0, 41);
				Uint8 red = blend_channel(5, 35, sky_blend);
				Uint8 green = blend_channel(8, 37, sky_blend);
				Uint8 blue = blend_channel(21, 67, sky_blend);

				if(ny < 0.62) {
					double broad = smooth_noise(nx * 3.2 + drift * 0.25, ny * 3.8, 101);
					double middle = smooth_noise(nx * 8.5 - drift * 0.42, ny * 10.0 + drift * 0.12, 211);
					double detail = smooth_noise(nx * 23.0 + drift * 0.75, ny * 28.0 - drift * 0.2, 307);
					double turbulence = broad * 0.48 + middle * 0.34 + detail * 0.18;
					double band = 0.27 + (smooth_noise(nx * 2.3 + drift * 0.08, 1.4, 419) - 0.5) * 0.24;
					double band_width = 0.24 + broad * 0.06;
					double band_edge = 1.0 - fabs(ny - band) / band_width;
					double nebula = band_edge > 0.0
						? (turbulence - 0.36) * 3.4 * band_edge : 0.0;
					if(nebula > 0.0) {
						double hue = smooth_noise(nx * 4.0 - drift * 0.08, ny * 5.5, 523);
						Uint8 nebula_red = blend_channel(62, 151, hue);
						Uint8 nebula_green = blend_channel(49, 83, hue);
						Uint8 nebula_blue = blend_channel(145, 226, 1.0 - hue * 0.45);
						double light = nebula * (0.24 + 0.30 * turbulence);
						red = blend_channel(red, nebula_red, light);
						green = blend_channel(green, nebula_green, light);
						blue = blend_channel(blue, nebula_blue, light);
					}

					double cloud_mass = smooth_noise(nx * 4.2 + drift * 0.42, ny * 6.0 + drift * 0.08, 631) * 0.62
						+ smooth_noise(nx * 13.0 + drift * 0.7, ny * 19.0 - drift * 0.16, 719) * 0.38;
					double cloud_band = 0.48 + 0.035 * (smooth_noise(nx * 3.0 + drift * 0.14, 2.0, 811) - 0.5);
					double cloud_envelope = 1.0 - fabs(ny - cloud_band) / 0.14;
					double cloud_density = cloud_envelope > 0.0
						? (cloud_mass - 0.50) * 3.2 * cloud_envelope : 0.0;
					if(cloud_density > 0.0) {
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

				Uint32 pixel = SDL_MapRGB(surface->format, red, green, blue);
				int block_width = surface->w - x < 4 ? surface->w - x : 4;
				int block_height = surface->h - y < 4 ? surface->h - y : 4;
				for(int row_index = 0; row_index < block_height; row_index++) {
					Uint32 *row = (Uint32 *)((Uint8 *)surface->pixels + (y + row_index) * surface->pitch);
					for(int column = 0; column < block_width; column++) {
						row[x + column] = pixel;
					}
				}
			}
		}
		if(SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);

		backdrop_base = SDL_ConvertSurface(surface, surface->format, SDL_SWSURFACE);
		if(!backdrop_base) {
			fprintf(stderr, "Unable to cache backdrop: %s\n", SDL_GetError());
		}
	}

	SceneWeather weather = get_scene_weather(ticks);
	double warm_blend = smooth_step(0.72, 0.95, weather.sunshine);
	static const Uint8 season_tint[4][3] = {
		{ 40, 68, 43 }, { 72, 77, 43 }, { 92, 48, 23 }, { 40, 56, 83 }
	};
	double tint_red = 0.0, tint_green = 0.0, tint_blue = 0.0;
	for(int i = 0; i < 4; i++) {
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
	Uint8 tint_alpha = (Uint8)(19 + weather.cloud * 12 + weather.fog * 18
		+ weather.lightning * 12 + weather.aurora * 12 + weather.sunshine * 33);
	boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
		(Uint8)tint_red, (Uint8)tint_green, (Uint8)tint_blue, tint_alpha);
	render_fullscreen_event(surface, ticks, scale, &weather);
	render_weather_effects(surface, ticks, &weather, scale);
	render_alien_weather(surface, ticks, scale);
	render_world_jump(surface, ticks, scale);
	if(weather.sunshine > 0.01) {
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
	for(int i = 0; i < star_count; i++) {
		star_seed = star_seed * 1664525u + 1013904223u;
		int sx = star_seed % surface->w;
		star_seed = star_seed * 1664525u + 1013904223u;
		int sy = star_seed % (surface->h * 54 / 100);
		double twinkle = 0.5 + 0.5 * sin(ticks / (700.0 + i % 13 * 91) + i * 1.73);
		Uint8 alpha = (Uint8)(35 + twinkle * 95);
		if(i % 31 == 0) filledCircleRGBA(surface, sx, sy, 3, 126, 158, 226, 13);
		pixelRGBA(surface, sx, sy, 205, 214, 235, alpha);
		if(i % 31 == 0) filledCircleRGBA(surface, sx, sy, 1, 237, 232, 222, alpha);
	}

	static const int bird_group_x[] = { 8, 36, 65, 91 };
	static const int bird_group_y[] = { 29, 43, 34, 48 };
	for(size_t group = 0; group < sizeof(bird_group_x) / sizeof(bird_group_x[0]); group++) {
		int bird_span = scale / 85;
		if(bird_span < 4) bird_span = 4;
		int formation_width = 27 * bird_span;
		int exit_margin = bird_span * 2;
		int route_length = surface->w + formation_width + exit_margin * 2;
		int travel = (int)((Uint64)ticks * route_length / 90000);
		int route_start = route_length * bird_group_x[group] / 100;
		int group_x = (route_start + travel) % route_length - exit_margin;
		int group_y = surface->h * bird_group_y[group] / 100
			+ (int)(sin(ticks / 1700.0 + group * 2.1) * scale * 0.008);
		static const int formation_x[] = { 0, -3, -6, -9, -12, -15, -18, -21, -24, -27 };
		static const int formation_y[] = { 0, 3, -3, 5, -6, 1, 7, -4, 4, -1 };
		for(int bird = 0; bird < 10; bird++) {
			int bird_x = group_x + formation_x[bird] * bird_span;
			int bird_y = group_y + formation_y[bird] * bird_span;
			int flap = (int)((0.5 + 0.5 * sin(ticks / 170.0 + group * 1.8 + bird))
				* (bird_span + 1));
			draw_bird(surface, bird_x, bird_y, bird_span, flap);
		}
	}

	// Battles, flyby and rare sky events never overlap (see GROUP_SKY).
	render_fleet_battle(surface, ticks, scale);
	render_background_battle(surface, ticks, scale);
	render_battle_flyby(surface, ticks, scale);
	render_rare_sky_event(surface, ticks, scale);
	render_long_scene(surface, ticks, scale);

	int autumn = weather.season[2] * 16;
	for(int i = 0; i < autumn; i++) {
		unsigned int seed = (unsigned int)i * 2246822519u + 3266489917u;
		int x = (seed + ticks / (32 + i % 5 * 9)) % surface->w;
		int y = (seed / 11 + ticks * (2 + i % 3) / 55) % surface->h;
		int sway = (int)(sin(ticks / 370.0 + i * 1.7) * scale * 0.012);
		Uint8 alpha = (Uint8)(55 + 90 * weather.season[2]);
		thickLineRGBA(surface, x, y, x + sway + 2, y + 3,
			2, (Uint8)(154 + i % 3 * 20), (Uint8)(76 + i % 4 * 13), 43, alpha);
	}

	unsigned int rain_seed = 0x7f4a7c15u;
	double storm_strength = smooth_step(0.25, 0.75, weather.lightning);
	int rain_count = (int)(surface->w * surface->h / 1650.0 * weather.rain
		* (1.0 + storm_strength * 2.0));
	if(rain_count > MAX_RAIN_DROPS) rain_count = MAX_RAIN_DROPS;
	for(int i = 0; i < rain_count; i++) {
		rain_seed = rain_seed * 1664525u + 1013904223u;
		int start_x = rain_seed % surface->w;
		rain_seed = rain_seed * 1664525u + 1013904223u;
		int start_y = rain_seed % surface->h;
		int length = 5 + (rain_seed >> 24) % 10;
		int speed = 5 + i % 8;
		double travel = ticks * speed / 150.0;
		int y = (start_y + (int)travel) % (surface->h + length);
		int x = (start_x + (int)(travel * (0.16 + weather.wind * 0.22))) % surface->w;
		int x_end = x - length / 3;
		int y_start = y - length;
		Uint8 alpha = (Uint8)(38 + (rain_seed >> 16) % 48 + weather.rain * 35);
		thickLineRGBA(surface, x, y, x_end, y_start, 1,
			154, 174, 207, alpha);
	}

	int snow_count = (int)(surface->w * surface->h / 3400.0 * weather.snow);
	for(int i = 0; i < snow_count; i++) {
		unsigned int seed = (unsigned int)i * 1664525u + 1013904223u;
		int x = (seed + ticks / (12 + i % 7 * 4)) % surface->w;
		int y = (seed / 13 + ticks / (18 + i % 5 * 7)) % surface->h;
		filledCircleRGBA(surface, x, y, i % 9 == 0 ? 2 : 1,
			202, 216, 231, (Uint8)(85 + weather.snow * 80));
	}
	Uint8 warm_alpha = (Uint8)(warm_blend * (75 + weather.sunshine * 15));
	boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
		warm_red, warm_green, warm_blue, warm_alpha);
}



void blit_digits(SDL_Surface *surface, SDL_Rect *rect, int spc, char digits[], SDL_Color color) {
	SDL_Surface *glyph;
	SDL_Rect coords;

	if(digits[1]) {
		int gap = spc + spc / 2;
		SDL_Surface *first_glyph = TTF_RenderGlyph_Blended(font_time, digits[0], color);
		SDL_Surface *second_glyph = TTF_RenderGlyph_Blended(font_time, digits[1], color);
		if(!first_glyph || !second_glyph) {
			fprintf(stderr, "Unable to render clock digits: %s\n", TTF_GetError());
			if(first_glyph) SDL_FreeSurface(first_glyph);
			if(second_glyph) SDL_FreeSurface(second_glyph);
			return;
		}
		int total_width = first_glyph->w + gap + second_glyph->w;
		coords.x = rect->x + (rect->w - total_width) / 2;
		coords.y = rect->y + (rect->h - first_glyph->h) / 2;
		SDL_BlitSurface(first_glyph, 0, surface, &coords);
		coords.x += first_glyph->w + gap;
		SDL_FreeSurface(first_glyph);
		coords.y = rect->y + (rect->h - second_glyph->h) / 2;
		SDL_BlitSurface(second_glyph, 0, surface, &coords);
		SDL_FreeSurface(second_glyph);
	} else {
		// single digit
		glyph = TTF_RenderGlyph_Blended(font_time, digits[0], color);
		coords.x = rect->x + (rect->w - glyph->w) / 2;
		coords.y = rect->y + (rect->h - glyph->h) / 2;
		SDL_BlitSurface(glyph, 0, surface, &coords);
		SDL_FreeSurface(glyph);
	}
}


void render_digits(SDL_Surface *surface, SDL_Rect *background, char digits[], char prevdigits[], int maxsteps, int step) {
	SDL_Rect rect, dstrect;
	SDL_Color color;
	double scale;
	double intensity;

	// int spc = surface->h * .0125;
	bool is_h = surface->h < surface->w;
	int spc = is_h ? surface->h * .0125 : surface->w * .0125;

	if(step == maxsteps - 1) {
		rect = *background;
		SDL_SetClipRect(surface, &rect);
		SDL_BlitSurface(bg, NULL, surface, &rect);
		blit_digits(surface, background, spc, digits, FONT_COLOR);
		SDL_SetClipRect(surface, NULL);
		if(!animate) return;

		rect.h = (is_h ? surface->h : surface->w) * 0.005;
		rect.w = background->w;
		rect.x = background->x;
		rect.y = background->y + (background->h - rect.h) / 2;
		SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 7, 9, 15));
		rect.y += rect.h;
		rect.h = 1;
		SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 0x35, 0x3a, 0x49));
		return;
	}

	// Rebuild the card each frame because the back buffer is not preserved by SDL_Flip.
	rect = *background;
	SDL_SetClipRect(surface, &rect);
	SDL_BlitSurface(bg, 0, surface, &rect);
	rect.h = background->h / 2;
	SDL_SetClipRect(surface, &rect);
	blit_digits(surface, background, spc, digits, FONT_COLOR);
	SDL_SetClipRect(surface, NULL);

	int halfsteps = maxsteps / 2;
	int upperhalf = (step+1) <= halfsteps;
	if(upperhalf) {
		rect.x = background->x;
		rect.y = background->y + background->h / 2;
		rect.w = background->w;
		rect.h = background->h - background->h / 2;
		SDL_SetClipRect(surface, &rect);
		blit_digits(surface, background, spc, prevdigits, FONT_COLOR);
		SDL_SetClipRect(surface, NULL);
	}

	if(upperhalf) {
		scale = 1.0 - (1.0 * step) / (halfsteps - 1);
		intensity = 1.0 - (1.0 * step) / (halfsteps - 1);
	} else {
		scale = ((1.0 * step) - halfsteps + 1) / halfsteps;
		intensity = ((1.0 * step) - halfsteps + 1) / halfsteps;
	}
	color.r = FONT_COLOR.r * intensity;
	color.g = FONT_COLOR.g * intensity;
	color.b = FONT_COLOR.b * intensity;

	// create surface to scale from filled background surface
	SDL_Surface *bgcopy = SDL_ConvertSurface(bg, bg->format, bg->flags);
	rect.x = 0;
	rect.y = 0;
	rect.w = bgcopy->w;
	rect.h = bgcopy->h;
	blit_digits(bgcopy, &rect, spc, upperhalf ? prevdigits : digits, color);

	// scale and blend it to dest
	SDL_Surface *scaled = zoomSurface(bgcopy, 1.0, scale, 1);
	rect.x = 0;
	rect.y = upperhalf ? 0 : scaled->h / 2;
	rect.w = scaled->w;
	rect.h = scaled->h / 2;
	dstrect.x = background->x;
	dstrect.y = background->y + ( upperhalf ? ((background->h - scaled->h) / 2) : background->h / 2);
	dstrect.w = rect.w;
	dstrect.h = rect.h;
	SDL_SetClipRect(surface, &dstrect);
	SDL_BlitSurface(scaled, &rect, surface, &dstrect);
	SDL_SetClipRect(surface, NULL);
	SDL_FreeSurface(scaled);
	SDL_FreeSurface(bgcopy);

	if(!animate) return;
	// draw divider
	rect.h = (is_h ? surface->h : surface->w) * 0.005;
	rect.w = background->w;
	rect.x = background->x;
	rect.y = background->y + (background->h - rect.h) / 2;
	SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 7, 9, 15));
	rect.y += rect.h;
	rect.h = 1;
	SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 0x35, 0x3a, 0x49));
}

// Backdrop picture reused by the frames of one flip while an overlay is showing.
static SDL_Surface *flip_backdrop;
static bool flip_backdrop_valid = false;

void render_clock(int maxsteps, int step, bool ambient_frame) {
	static Uint32 last_profile_report;
	char buffer[3], buffer2[3];
	struct tm *_time;
	time_t rawtime;
	static const int offset_x[] = { -1, 0, 1, 1, 1, 0, -1, -1 };
	static const int offset_y[] = { 0, -1, -1, 0, 1, 1, 0, -1 };
	int phase, new_burnin_x, new_burnin_y;
	bool moved = false;
	Uint32 frame_start = profile_rendering ? SDL_GetTicks() : 0;
	Uint32 backdrop_elapsed = 0;

	time(&rawtime);
	_time = localtime(&rawtime);
	phase = (rawtime / 300) % 8;
	new_burnin_x = offset_x[phase] * (screen->w / 80);
	new_burnin_y = offset_y[phase] * (screen->h / 80);
	if(new_burnin_x != burnin_x || new_burnin_y != burnin_y) {
		hourBackground.x += new_burnin_x - burnin_x;
		hourBackground.y += new_burnin_y - burnin_y;
		minBackground.x += new_burnin_x - burnin_x;
		minBackground.y += new_burnin_y - burnin_y;
		burnin_x = new_burnin_x;
		burnin_y = new_burnin_y;
		moved = true;
	}
	Uint32 now = scene_time();
	// The screen buffer is not preserved between frames, so anything drawn over
	// the cards (foreground particles, screen fades) must start from a freshly
	// drawn backdrop, or its alpha would pile up on the previous frame.
	bool overlay_active = effect_is_active(&EFFECT_FOREGROUND, now)
		|| effect_is_active(&EFFECT_SCREEN_FADE, now);
	bool backdrop_redrawn = moved || ambient_frame || overlay_active;
	if(moved || ambient_frame) {
		Uint32 backdrop_start = profile_rendering ? SDL_GetTicks() : 0;
		render_backdrop(screen, now);
		if(profile_rendering) backdrop_elapsed = SDL_GetTicks() - backdrop_start;
		flip_backdrop_valid = false;
	} else if(overlay_active) {
		// Flip frame with an overlay: render the backdrop once, then reuse that
		// picture for the rest of the flip so frames stay cheap.
		if(!(flip_backdrop_valid && flip_backdrop
			&& SDL_BlitSurface(flip_backdrop, NULL, screen, NULL) == 0)) {
			Uint32 backdrop_start = profile_rendering ? SDL_GetTicks() : 0;
			render_backdrop(screen, now);
			if(profile_rendering) backdrop_elapsed = SDL_GetTicks() - backdrop_start;
			if(flip_backdrop && (flip_backdrop->w != screen->w || flip_backdrop->h != screen->h)) {
				SDL_FreeSurface(flip_backdrop);
				flip_backdrop = NULL;
			}
			if(!flip_backdrop) {
				flip_backdrop = SDL_ConvertSurface(screen, screen->format, SDL_SWSURFACE);
				flip_backdrop_valid = flip_backdrop != NULL;
			} else {
				flip_backdrop_valid = SDL_BlitSurface(screen, NULL, flip_backdrop, NULL) == 0;
			}
		}
	}
	bool time_changed = _time->tm_hour != past_h || _time->tm_min != past_m;
	bool redraw = ambient_frame || (backdrop_redrawn && !time_changed);
	int render_steps = redraw ? 20 : maxsteps;
	int render_step = redraw ? 19 : step;
	bool card_drawn[2] = { false, false };

	// draw hours
	if(_time->tm_hour != past_h || backdrop_redrawn) {
		int h = twentyfourh ? _time->tm_hour : (_time->tm_hour + 11) % 12 + 1;
		// past_h is stored in 24-hour form; convert it the same way as h
		// so the flip animation shows the previous 12-hour value.
		int prev_h = twentyfourh ? past_h : (past_h + 11) % 12 + 1;
		// A card the backdrop just painted over is redrawn as it is, not mid-flip.
		bool hour_static = backdrop_redrawn && (redraw || _time->tm_hour == past_h);
		int hour_steps = hour_static ? 20 : render_steps;
		int hour_step = hour_static ? 19 : render_step;
		if(leadingzero) {
			snprintf(buffer, 3, "%02d", h);
			snprintf(buffer2, 3, "%02d", hour_static ? h : prev_h);
		} else {
			snprintf(buffer, 3, "%d", h);
			snprintf(buffer2, 3, "%d", hour_static ? h : prev_h);
		}
		render_digits(screen, &hourBackground, buffer, buffer2, hour_steps, hour_step);
		// draw am/pm
		if(!twentyfourh) render_ampm(screen, &hourBackground, _time->tm_hour >= 12);
		card_drawn[0] = true;
	}

	// draw minutes
	if(_time->tm_min != past_m || backdrop_redrawn) {
		bool min_static = backdrop_redrawn && (redraw || _time->tm_min == past_m);
		snprintf(buffer, 3, "%02d", _time->tm_min);
		snprintf(buffer2, 3, "%02d", min_static ? _time->tm_min : past_m);
		render_digits(screen, &minBackground, buffer, buffer2,
			min_static ? 20 : render_steps, min_static ? 19 : render_step);
		card_drawn[1] = true;
	}

	if(_time->tm_yday != date_surface_yday) {
		char date[128];
		strftime(date, sizeof(date), "%A, %B", _time);
		snprintf(date + strlen(date), sizeof(date) - strlen(date), " %d, %d", _time->tm_mday, _time->tm_year + 1900);
		SDL_Surface *new_date_surface = TTF_RenderText_Blended(font_date, date, DATE_COLOR);
		if(!new_date_surface) {
			fprintf(stderr, "Unable to render date: %s\n", TTF_GetError());
		} else {
			if(date_surface_cache) SDL_FreeSurface(date_surface_cache);
			date_surface_cache = new_date_surface;
			date_surface_yday = _time->tm_yday;
		}
	}
	if((backdrop_redrawn || _time->tm_yday != past_yday)
		&& date_surface_cache && date_surface_yday == _time->tm_yday) {
		SDL_Rect date_coords;
		date_coords.x = (screen->w - date_surface_cache->w) / 2 + burnin_x;
		date_coords.y = date_y + burnin_y;
		SDL_BlitSurface(date_surface_cache, NULL, screen, &date_coords);
		past_yday = _time->tm_yday;
	}

	SceneWeather weather = get_scene_weather(now);
	SDL_Rect *clock_cards[] = { &hourBackground, &minBackground };
	for(size_t card_index = 0; card_index < sizeof(clock_cards) / sizeof(clock_cards[0]); card_index++) {
		SDL_Rect *card = clock_cards[card_index];
		// Only weather over cards drawn this frame, or it would build up on stale ones.
		if(!card_drawn[card_index]) continue;
		SDL_SetClipRect(screen, card);
		int drops = (int)(card->w / 42.0 * weather.rain);
		for(int i = 0; i < drops; i++) {
			Uint32 seed = (Uint32)(i + card_index * 97) * 2246822519u + 3266489917u;
			int drop_x = card->x + seed % card->w;
			int length = 4 + (seed >> 24) % 7;
			int speed = 7 + (seed >> 12) % 7;
			int drop_y = card->y
				+ (int)((seed + now * speed / 90) % card->h);
			Uint8 alpha = (Uint8)(24 + (seed >> 18) % 30);
			thickLineRGBA(screen, drop_x, drop_y, drop_x - 1, drop_y - length,
				1, 151, 174, 211, alpha);
		}
		int flakes = (int)(card->w / 50.0 * weather.snow);
		for(int i = 0; i < flakes; i++) {
			Uint32 seed = (Uint32)(i + card_index * 61) * 1664525u + 1013904223u;
			int flake_x = card->x + seed % card->w;
			int flake_y = card->y
				+ (int)((seed + now * (2 + (seed >> 16) % 4) / 80) % card->h);
			pixelRGBA(screen, flake_x, flake_y, 208, 221, 236, 110);
		}
	}
	SDL_SetClipRect(screen, NULL);
	int scene_scale = screen->h < screen->w ? screen->h : screen->w;
	render_foreground_event(screen, now, scene_scale);
	render_rare_screen_fade(screen, now);

	// flip backbuffer
	SDL_Flip(screen);
	if(profile_rendering) {
		Uint32 frame_elapsed = SDL_GetTicks() - frame_start;
		if(frame_elapsed >= 50 && frame_start - last_profile_report >= 1000) {
			fprintf(stderr, "Gluqlo slow frame: %u ms (backdrop: %u ms)\n",
				frame_elapsed, backdrop_elapsed);
			last_profile_report = frame_start;
		}
	}

	if(render_step == render_steps - 1) {
		past_h = _time->tm_hour;
		past_m = _time->tm_min;
	}
}

void render_animation() {
	if(!animate) {
		render_clock(20, 19, false);
		return;
	}

	const int DURATION = 260;
	const int FRAME_COUNT = 20;
	int start_tick = SDL_GetTicks();
	for(int frame = 0; frame < FRAME_COUNT; frame++) {
		render_clock(FRAME_COUNT, frame, false);
		int elapsed = SDL_GetTicks() - start_tick;
		int next_frame = DURATION * (frame + 1) / FRAME_COUNT;
		if(elapsed < next_frame) SDL_Delay(next_frame - elapsed);
	}
}

Uint32 update_time(Uint32 interval, void *param) {
	SDL_Event e;
	SDL_Event pending_event;
	int pending = SDL_PeepEvents(&pending_event, 1, SDL_PEEKEVENT,
		SDL_EVENTMASK(SDL_USEREVENT));
	if(pending < 0) {
		fprintf(stderr, "Unable to check pending clock events: %s\n", SDL_GetError());
		return interval;
	}
	if(pending == 0) {
		e.type = SDL_USEREVENT;
		e.user.code = 0;
		e.user.data1 = NULL;
		e.user.data2 = NULL;
		if(SDL_PushEvent(&e) < 0) {
			fprintf(stderr, "Unable to queue clock event: %s\n", SDL_GetError());
		}
	}

	return interval;
}

// Returns the value that follows option argv[*i] and advances *i, or NULL (with a message) if it is missing.
static const char *option_value(int argc, char **argv, int *i) {
	if(*i + 1 >= argc) {
		fprintf(stderr, "Option %s needs a value\n", argv[*i]);
		return NULL;
	}
	return argv[++*i];
}

static bool parse_int_in_range(const char *text, long min, long max, long *value) {
	char *end = NULL;
	errno = 0;
	long parsed = strtol(text, &end, 0);
	if(errno != 0 || end == text || *end != '\0' || parsed < min || parsed > max) return false;
	*value = parsed;
	return true;
}

int main(int argc, char** argv ) {
	char *wid_env;
	static char sdlwid[100];
	double display_scale_factor = 1;
	bool seed_given = false;
	Uint32 scene_seed = 0;
	profile_rendering = getenv("GLUQLO_PROFILE") != NULL;

	Uint32 wid = 0;
	Display *display;
	XWindowAttributes windowAttributes;

	for(int i = 1; i < argc; i++) {
		if(strcmp("--help",argv[i]) == 0 || strcmp("-help", argv[i]) == 0) {
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
			printf("  -effect NAME[:KIND]\tShow only one scheduled effect, repeating (NAME 'list' lists them)\n");
			printf("  -seed N\tStart the scenery (season, weather, effects) at a fixed point instead of a random one\n");
			return 0;
		} else if(strcmp("-root", argv[i]) == 0 || strcmp("-f", argv[i]) == 0 || strcmp("--fullscreen", argv[i]) == 0) {
			fullscreen = true;
		} else if(strcmp("-noflip", argv[i]) == 0) {
			animate = false;
		} else if(strcmp("-anykeyclose", argv[i]) == 0) {
			anykeyclose = true;
		} else if(strcmp("-ampm", argv[i]) == 0) {
			twentyfourh = false;
		} else if(strcmp("-leadingzero", argv[i]) == 0) {
			leadingzero = true;
		} else if(strcmp("-r", argv[i]) == 0 || strcmp("--resolution", argv[i]) == 0) {
			const char *value = option_value(argc, argv, &i);
			if(!value) return 1;
			int parsed_width, parsed_height;
			char trailing;
			if(sscanf(value, "%dx%d%c", &parsed_width, &parsed_height, &trailing) != 2
				|| parsed_width < 1 || parsed_width > 16384
				|| parsed_height < 1 || parsed_height > 16384) {
				fprintf(stderr, "Invalid resolution -- %s (expected WxH, e.g. 1280x720)\n", value);
				return 1;
			}
			width = parsed_width;
			height = parsed_height;
		} else if(strcmp("-w", argv[i]) == 0 || strcmp("-h", argv[i]) == 0) {
			bool is_width = argv[i][1] == 'w';
			const char *value = option_value(argc, argv, &i);
			if(!value) return 1;
			long parsed;
			if(!parse_int_in_range(value, 1, 16384, &parsed)) {
				fprintf(stderr, "Invalid %s -- %s\n", is_width ? "width" : "height", value);
				return 1;
			}
			if(is_width) width = (int)parsed;
			else height = (int)parsed;
		} else if(strcmp("-s", argv[i]) == 0) {
			const char *value = option_value(argc, argv, &i);
			if(!value) return 1;
			char *end = NULL;
			double parsed = strtod(value, &end);
			if(end == value || *end != '\0' || !(parsed > 0.0 && parsed <= 16.0)) {
				fprintf(stderr, "Invalid scale factor -- %s (expected a number above 0)\n", value);
				return 1;
			}
			display_scale_factor = parsed;
		} else if(strcmp("-window-id", argv[i]) == 0) {
			const char *value = option_value(argc, argv, &i);
			if(!value) return 1;
			long parsed;
			if(!parse_int_in_range(value, 1, 0xffffffffL, &parsed)) {
				fprintf(stderr, "Invalid window id -- %s\n", value);
				return 1;
			}
			wid = (Uint32)parsed;
		} else if(strcmp("-seed", argv[i]) == 0) {
			const char *value = option_value(argc, argv, &i);
			if(!value) return 1;
			long parsed;
			if(!parse_int_in_range(value, 0, 0xffffffffL, &parsed)) {
				fprintf(stderr, "Invalid seed -- %s (expected a number)\n", value);
				return 1;
			}
			scene_seed = (Uint32)parsed;
			seed_given = true;
		} else if(strcmp("-effect", argv[i]) == 0) {
			const char *effect_value = option_value(argc, argv, &i);
			if(!effect_value) {
				fprintf(stderr, "Try -effect list for the available effects.\n");
				return 1;
			}
			char effect_arg[64];
			snprintf(effect_arg, sizeof(effect_arg), "%s", effect_value);
			if(strcmp(effect_arg, "list") == 0) {
				for(size_t effect = 0; effect < effect_count; effect++) {
					if(effect_table[effect]->kind_count > 0) {
						printf("%s (kinds 0-%d)\n", effect_table[effect]->name,
							effect_table[effect]->kind_count - 1);
					} else {
						printf("%s\n", effect_table[effect]->name);
					}
				}
				return 0;
			}
			char *kind_separator = strchr(effect_arg, ':');
			if(kind_separator) *kind_separator = '\0';
			forced_effect = find_effect(effect_arg);
			if(!forced_effect) {
				printf("Unknown effect -- %s\n", effect_arg);
				printf("Try -effect list for the available effects.\n");
				return 1;
			}
			if(kind_separator) {
				char *kind_end = NULL;
				long requested = strtol(kind_separator + 1, &kind_end, 10);
				if(kind_end == kind_separator + 1 || *kind_end != '\0'
					|| requested < 0 || requested >= forced_effect->kind_count) {
					if(forced_effect->kind_count > 0) {
						printf("Effect %s takes a kind from 0 to %d\n",
							forced_effect->name, forced_effect->kind_count - 1);
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

	// Each launch starts at a different point of the season/weather/effect schedule.
	// -effect starts at 0 so the chosen effect begins straight away.
	if(forced_effect) scene_offset = 0;
	else if(seed_given) scene_offset = scene_offset_from_seed(scene_seed);
	else scene_offset = random_scene_offset();

	/* If no window argument, check environment */
	if(wid == 0) {
		if ((wid_env = getenv("XSCREENSAVER_WINDOW")) != NULL ) {
			wid = strtol(wid_env, (char **) NULL, 0); /* Base 0 autodetects hex/dec */
		}
	}

	/* Get win attrs if we've been given a window, otherwise we'll use our own */
	if(wid != 0) {
		if ((display = XOpenDisplay(NULL)) != NULL) { /* Use the default display */
			Status attributes_status = XGetWindowAttributes(display, (Window) wid, &windowAttributes);
			XCloseDisplay(display);
			if(attributes_status) {
				snprintf(sdlwid, 100, "SDL_WINDOWID=0x%X", wid);
				putenv(sdlwid); /* Tell SDL to use this window */
				width = windowAttributes.width;
				height = windowAttributes.height;
			} else {
				fprintf(stderr, "Unable to read attributes of window 0x%X\n", wid);
				wid = 0;
			}
		}
	}

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);

	if(fullscreen && (!wid)) {
		screen = SDL_SetVideoMode(0, 0, 32, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN);
	} else {
		screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE|SDL_DOUBLEBUF);
	}

	if (!screen) {
		fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
		return 1;
	}

	if(fullscreen || wid) {
		SDL_ShowCursor(SDL_DISABLE);
	}

	SDL_WM_SetCaption(TITLE, TITLE);

	width = screen->w * display_scale_factor;
	height = screen->h * display_scale_factor;

	bool is_horizontal = width > height;

	TTF_Init();
	atexit(TTF_Quit);
	font_time = TTF_OpenFont(FONT, (is_horizontal ? height : width) / 1.68 );
	font_mode = TTF_OpenFont(FONT, (is_horizontal ? height : width) / 16.5);
	if (!font_time || !font_mode) {
		fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
		return 1;
	}
	static const char *date_font_paths[] = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
		"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
		"/usr/share/fonts/truetype/freefont/FreeSans.ttf"
	};
	int date_font_size = (is_horizontal ? height : width) / 24;
	for(size_t i = 0; i < sizeof(date_font_paths) / sizeof(date_font_paths[0]) && !font_date; i++) {
		font_date = TTF_OpenFont(date_font_paths[i], date_font_size);
	}
	if (!font_date) {
		fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
		return 1;
	}

	// Draw the layered forest before placing the clock over it.
	render_backdrop(screen, scene_time());

	// calculate box coordinates
	int rectsize;
	int spacing;
	int radius;
	int date_gap = TTF_FontHeight(font_date) / 2;

	if (is_horizontal) {
		rectsize = height * 0.6;
		spacing = width * .031;
		radius = rectsize * .0952;
	}
	else {
		spacing = height * .031;
		int max_rectsize = (height - TTF_FontHeight(font_date) - date_gap * 2 - spacing) / 2;
		rectsize = width * 0.6 < max_rectsize ? width * 0.6 : max_rectsize;
		radius = rectsize * .0952;
	}

	int jitter_width  = 1;
	int jitter_height = 1;
	if (display_scale_factor != 1) {
		jitter_width  = (screen->w - width) * 0.5;
		jitter_height = (screen->h - height) * 0.5;
	}

	hourBackground.w = rectsize;
	hourBackground.h = rectsize;
	minBackground.w = rectsize;
	minBackground.h = rectsize;

	if (is_horizontal) {
		hourBackground.x = 0.5 * (width - (2 * rectsize) - spacing)
										+ jitter_width;
		hourBackground.y = (height - rectsize - date_gap - TTF_FontHeight(font_date)) / 2 + jitter_height;

		minBackground.x = hourBackground.x + rectsize + spacing;
		minBackground.y = hourBackground.y;
		date_y = hourBackground.y + rectsize + date_gap;
	}
	else {
		hourBackground.y = (height - (2 * rectsize) - spacing - date_gap - TTF_FontHeight(font_date)) / 2
										+ jitter_height;
		hourBackground.x = (width - rectsize) / 2 + jitter_width;

		minBackground.y = hourBackground.y + rectsize + spacing;
		minBackground.x = hourBackground.x;
		date_y = minBackground.y + rectsize + date_gap;
	}

	// create background surface
	bgrect.x = 0;
	bgrect.y = 0;
	bgrect.w = rectsize;
	bgrect.h = rectsize;
	Uint32 alpha_mask = ~(screen->format->Rmask | screen->format->Gmask | screen->format->Bmask);
	bg = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA, rectsize, rectsize, 32,
		screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, alpha_mask);
	fill_rounded_box_b(bg, &bgrect, radius, CARD_BORDER_COLOR, 232);
	int border_width = rectsize / 120;
	if(border_width < 1) border_width = 1;
	SDL_Rect cardrect;
	cardrect.x = border_width;
	cardrect.y = border_width;
	cardrect.w = rectsize - 2 * border_width;
	cardrect.h = rectsize - 2 * border_width;
	int inner_radius = radius > border_width ? radius - border_width : 1;
	fill_rounded_box_b(bg, &cardrect, inner_radius, BACKGROUND_COLOR, 190);

	// draw current time
	render_clock(20, 19, false);

	// main loop
	bool done = false;
	SDL_Event event;
	SDL_TimerID timer = SDL_AddTimer(AMBIENT_FRAME_INTERVAL, update_time, NULL);
	if(!timer) {
		fprintf(stderr, "Unable to start clock timer: %s\n", SDL_GetError());
		SDL_FreeSurface(bg);
		if(backdrop_base) SDL_FreeSurface(backdrop_base);
		if(flip_backdrop) SDL_FreeSurface(flip_backdrop);
		if(date_surface_cache) SDL_FreeSurface(date_surface_cache);
		TTF_CloseFont(font_time);
		TTF_CloseFont(font_mode);
		TTF_CloseFont(font_date);
		return 1;
	}

	int mouse_x = -1;
	int mouse_y = -1;

	while(!done && SDL_WaitEvent(&event)) {
		switch(event.type) {
			case SDL_USEREVENT:
			{
				time_t rawtime;
				struct tm current_time;
				time(&rawtime);
				localtime_r(&rawtime, &current_time);
				bool time_changed = current_time.tm_hour != past_h
					|| current_time.tm_min != past_m;
				if(time_changed) {
					render_animation();
				} else {
					render_clock(20, 19, true);
				}
				break;
			}
			case SDL_KEYDOWN:
				if(anykeyclose){
					done = true;
					break;
				}
				switch(event.key.keysym.sym) {
					case SDLK_ESCAPE:
					case SDLK_q:
						done = true;
						break;
					default:
						break;
				}
				break;

			case SDL_MOUSEMOTION:
				if ( (mouse_x == -1) || (mouse_y == -1) ) //lifehack
					{
						mouse_x = event.motion.x;
						mouse_y = event.motion.y;

					}

				if(((mouse_x != event.motion.x) || (mouse_y != event.motion.y)) && anykeyclose)
					done = true;
				break;

			case SDL_QUIT:
				done = true;
				break;
		}
	}

	SDL_RemoveTimer(timer);

	SDL_FreeSurface(bg);
	if(backdrop_base) SDL_FreeSurface(backdrop_base);
	if(flip_backdrop) SDL_FreeSurface(flip_backdrop);
	if(date_surface_cache) SDL_FreeSurface(date_surface_cache);

	TTF_CloseFont(font_time);
	TTF_CloseFont(font_mode);
	TTF_CloseFont(font_date);

	return 0;
}
