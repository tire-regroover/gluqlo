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
#define HILL_EVENT_KINDS 9          // things that happen on the hills (see render_hill_event)

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
	"fleet_battle", 240000, 1, 6, false, 0,
	60000, 0, 0, false, 1400, 2200, 0, GROUP_SKY, 0, NULL };
static const EffectSpec EFFECT_BACKGROUND_BATTLE = {
	"background_battle", 90000, 1, 9, false, 0,
	32000, 0, 0, false, 900, 1700, 0, GROUP_SKY, 1, NULL };
static const EffectSpec EFFECT_RARE_SKY = {
	"rare_sky", 48000, 0, 3, true, 2246822519u,
	4200, 3200, 8, true, 350, 400, RARE_SKY_KINDS, GROUP_SKY, 2, rare_sky_accept };
static const EffectSpec EFFECT_BATTLE_FLYBY = {
	"battle_flyby", 60000, 1, 6, false, 0,
	14000, 0, 0, false, 900, 1100, 0, GROUP_SKY, 4, NULL };
static const EffectSpec EFFECT_DOGFIGHT = {
	"dogfight", 75000, 1, 4, false, 0,
	18000, 0, 0, true, 900, 1100, 0, GROUP_SKY, 3, NULL };
static const EffectSpec EFFECT_HILL_EVENT = {
	"hill_event", 120000, 0, 2, false, 2654435761u,
	36000, 22000, 8, true, 4000, 5000, HILL_EVENT_KINDS, GROUP_TAKEOVER, 4, NULL };
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
	&EFFECT_FLEET_BATTLE, &EFFECT_DOGFIGHT, &EFFECT_HILL_EVENT
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

// Weather 0 in weather_levels[] is the orange sunset. It is a treat, so only one
// pick in SUNSET_ONE_IN keeps it; the others become one of the later weathers.
// Raise SUNSET_ONE_IN to see it less often.
#define WEATHER_SUNSET 0
#define SUNSET_ONE_IN 5

static int pick_weather(Uint32 seed, int count) {
	int pick = seed % count;
	if(pick == WEATHER_SUNSET && (seed >> 12) % SUNSET_ONE_IN != 0) {
		pick = 2 + (seed >> 5) % (count - 2);
	}
	return pick;
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
	int current = pick_weather(current_seed, weather_count);
	int previous = pick_weather(previous_seed, weather_count);
	if(previous == current) {
		// Any other weather, but never the sunset (that would bypass its rarity).
		int other = 1 + (current_seed >> 8) % (weather_count - 1);
		if(other == current) other = other % (weather_count - 1) + 1;
		previous = other;
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
		// Each particle moves along its own streak: kind 0 falls slanting to the
		// right, kind 1 rises to the left, the others fall almost straight down.
		double direction_x = kind == 0 ? 0.55 : (kind == 1 ? -0.7 : 0.4);
		double direction_y = kind == 1 ? -1.0 : 1.0;
		double streak = scale * (kind == 0 ? 0.023 : 0.017);
		double travel = ticks * 2.0 / (35 + speed * 7);
		double path = sqrt(direction_x * direction_x + direction_y * direction_y);
		double wrapped_x = fmod(start_x + travel * direction_x, (double)surface->w);
		double wrapped_y = fmod(start_y + travel * direction_y, (double)surface->h);
		if(wrapped_x < 0.0) wrapped_x += surface->w;
		if(wrapped_y < 0.0) wrapped_y += surface->h;
		int x = (int)wrapped_x, y = (int)wrapped_y;
		int end_x = x - (int)(direction_x / path * streak);
		int end_y = y - (int)(direction_y / path * streak);
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
 * Ships are seen from the side, nose towards +x, +y down. A design is the upper
 * half of a hull outline in tenths of a unit, mirrored to make the whole hull.
 * The hull is painted in four bands of light from above with a dark outline and
 * glowing engines. Each ship is drawn fully opaque and then blended onto the
 * scene at the requested opacity, so its overlapping parts do not stack up.
 *
 * Ships come in three roles. Small ones are the fighters used everywhere,
 * mid-size and big ones only appear in the fleet battle.
 */
typedef signed char ShipPoint[2];

typedef struct {
	const ShipPoint *points;
	int count;
} ShipOutline;

#define SHIP_OUTLINE(points) { points, (int)(sizeof(points) / sizeof(points[0])) }
#define SHIP_NO_OUTLINE { NULL, 0 }
#define SHIP_ENGINE_SIZE 2     // engine glow radius, tenths of a unit
#define SHIP_OPACITY 235       // opacity of a fully faded in ship

typedef struct {
	ShipOutline hull;          // upper half, nose first, tail last (both at y = 0)
	ShipOutline plates[2];     // lighter plates drawn over the hull, if any
	const ShipPoint *spikes;   // (x, height) of each spike on the back, if any
	int spike_count;
	ShipPoint engines[3];
	int engine_count;
	ShipPoint accent[2];       // glowing slit between these two points
	bool accent_on;
	bool eye;                  // a glowing eye in the middle instead of a slit
	int nose, tail;            // extent of the hull along x
	int weapon;                // main weapon, fired from the nose (WEAPON_*)
	ShipPoint turrets[4];      // gun turret mounts on the hull, if any
	int turret_count;
	int turret_weapon;         // what the turrets fire
} ShipDesign;

typedef struct {
	Uint8 tone[4][3];          // shadow, mid, light, highlight
	Uint8 glow[3];             // engine glow and bounced light
	Uint8 accent[3];           // glowing slits and eyes
} ShipTeam;

static const ShipTeam ship_teams[2] = {
	{ { {8, 12, 19}, {16, 25, 38}, {29, 43, 60}, {55, 74, 95} },
		{90, 190, 255}, {150, 235, 255} },
	{ { {19, 11, 8}, {36, 21, 15}, {55, 34, 24}, {88, 64, 48} },
		{255, 150, 70}, {255, 208, 120} }
};

/*
 * Weapons
 *
 * The weapon families are modelled on X4: Foundations. Each one has its own
 * look and its own timing: pulse lasers and mass drivers throw many fast bolts,
 * beam emitters hold a continuous beam, plasma is a slow heavy orb, flak bursts
 * into shrapnel short of the target, missiles and swarms curve in leaving smoke,
 * torpedoes are huge and slow, and the railgun charges before it fires.
 * Small ships carry one gun; mid-size and big ships also have turrets.
 */
enum { WEAPON_PULSE, WEAPON_BEAM, WEAPON_PLASMA, WEAPON_MASS_DRIVER, WEAPON_FLAK,
	WEAPON_MISSILE, WEAPON_SWARM, WEAPON_TORPEDO, WEAPON_RAIL, WEAPON_COUNT };

typedef struct {
	double impact_ms;          // time from firing until a shot reaches the target
	double life_ms;            // time until nothing of a shot is left on screen
	int shots;                 // shots in one volley
	double gap_ms;             // time between the shots of a volley
	double blast;              // size of the impact, in shot-size units
} WeaponSpec;

static const WeaponSpec weapon_specs[WEAPON_COUNT] = {
	{ 260.0, 820.0, 3, 90.0, 0.16 },     // pulse laser
	{ 420.0, 1100.0, 1, 0.0, 0.20 },     // beam emitter (the beam itself lasts 900 ms)
	{ 720.0, 1320.0, 1, 0.0, 0.50 },     // plasma cannon
	{ 200.0, 650.0, 6, 42.0, 0.10 },     // mass driver
	{ 480.0, 1030.0, 1, 0.0, 0.40 },     // flak
	{ 950.0, 1500.0, 1, 0.0, 0.45 },     // missile
	{ 880.0, 1430.0, 5, 75.0, 0.25 },    // swarm missiles
	{ 1500.0, 2100.0, 1, 0.0, 0.90 },    // torpedo
	{ 300.0, 850.0, 1, 0.0, 0.30 }       // railgun (charges for 300 ms, then fires)
};

static const ShipPoint hull_dagger[] = {{45,0},{30,-3},{12,-5},{-4,-14},{-20,-22},{-14,-8},{-26,-5},{-28,0}};
static const ShipPoint spine_dagger[] = {{40,0},{18,-2},{-20,-2},{-28,0}};
static const ShipPoint hull_manta[] = {{32,0},{20,-6},{4,-16},{-12,-30},{-20,-27},{-14,-15},{-26,-8},{-32,-4},{-32,0}};
static const ShipPoint spine_manta[] = {{34,0},{10,-4},{-30,-3},{-32,0}};
static const ShipPoint hull_hammerhead[] = {{38,0},{37,-18},{29,-19},{26,-6},{-8,-7},{-22,-12},{-30,-9},{-30,0}};
static const ShipPoint spine_hammerhead[] = {{26,0},{24,-3},{-8,-4},{-30,-3},{-30,0}};
static const ShipPoint hull_mandible[] = {{8,0},{22,-4},{38,-12},{22,-18},{2,-15},{-14,-12},{-26,-8},{-32,-3},{-32,0}};
static const ShipPoint spine_mandible[] = {{2,0},{0,-5},{-30,-4},{-32,0}};
static const ShipPoint hull_trident[] = {{46,0},{24,-3},{16,-5},{30,-16},{12,-12},{-4,-9},{-24,-12},{-30,-6},{-30,0}};
static const ShipPoint spine_trident[] = {{44,0},{10,-2},{-28,-2},{-30,0}};
static const ShipPoint hull_scythe[] = {{30,0},{22,-6},{8,-16},{-8,-26},{-28,-36},{-20,-20},{-14,-10},{-28,-6},{-30,0}};
static const ShipPoint spine_scythe[] = {{32,0},{6,-4},{-28,-3},{-30,0}};
static const ShipPoint hull_bat[] = {{26,0},{14,-8},{-4,-20},{-10,-31},{-16,-22},{-18,-31},{-24,-19},{-28,-25},{-30,-8},{-32,0}};
static const ShipPoint spine_bat[] = {{28,0},{8,-4},{-30,-3},{-32,0}};
static const ShipPoint hull_leviathan[] = {{52,0},{38,-4},{12,-7},{-30,-8},{-46,-5},{-50,0}};
static const ShipPoint spine_leviathan[] = {{48,0},{12,-3},{-46,-2},{-50,0}};
static const ShipPoint spikes_leviathan[] = {{20,14},{4,18},{-12,18},{-28,14}};
static const ShipPoint hull_wedge[] = {{42,0},{-4,-16},{-30,-22},{-36,-14},{-36,0}};
static const ShipPoint plate_wedge[] = {{30,0},{-6,-10},{-30,-14},{-34,-8},{-34,0}};
static const ShipPoint top_wedge[] = {{16,0},{-8,-5},{-30,-7},{-34,-4},{-34,0}};
static const ShipPoint hull_swept[] = {{36,0},{12,-4},{28,-22},{6,-20},{-6,-8},{-20,-10},{-26,-4},{-26,0}};
static const ShipPoint spine_swept[] = {{34,0},{10,-2},{-24,-2},{-26,0}};

// Indexes into ship_designs[], grouped by role.
enum { SHIP_SMALL, SHIP_MID, SHIP_BIG };
static const int ship_small_designs[] = { 0, 1, 5, 6, 9 };   // dagger, manta, scythe, bat, swept
static const int ship_mid_designs[] = { 2, 3, 4 };           // hammerhead, mandible, trident
static const int ship_big_designs[] = { 7, 8 };              // leviathan, wedge

static const ShipDesign ship_designs[] = {
	// 0 dagger
	{ SHIP_OUTLINE(hull_dagger), { SHIP_OUTLINE(spine_dagger), SHIP_NO_OUTLINE }, NULL, 0,
		{ {-28,-3}, {-28,3}, {0,0} }, 2, { {34,0}, {22,0} }, true, false, 45, -28,
		WEAPON_RAIL, { {0,0} }, 0, WEAPON_PULSE },
	// 1 manta
	{ SHIP_OUTLINE(hull_manta), { SHIP_OUTLINE(spine_manta), SHIP_NO_OUTLINE }, NULL, 0,
		{ {-32,-3}, {-32,3}, {0,0} }, 2, { {26,0}, {12,0} }, true, false, 32, -32,
		WEAPON_SWARM, { {0,0} }, 0, WEAPON_PULSE },
	// 2 hammerhead
	{ SHIP_OUTLINE(hull_hammerhead), { SHIP_OUTLINE(spine_hammerhead), SHIP_NO_OUTLINE }, NULL, 0,
		{ {-30,-4}, {-30,4}, {0,0} }, 2, { {36,-15}, {36,15} }, true, false, 38, -30,
		WEAPON_FLAK, { {30,-12}, {30,12}, {-12,0} }, 3, WEAPON_MASS_DRIVER },
	// 3 mandible
	{ SHIP_OUTLINE(hull_mandible), { SHIP_OUTLINE(spine_mandible), SHIP_NO_OUTLINE }, NULL, 0,
		{ {-32,-3}, {-32,3}, {0,0} }, 2, { {0,0}, {0,0} }, false, true, 38, -32,
		WEAPON_PLASMA, { {-6,-8}, {-6,8}, {-22,0} }, 3, WEAPON_PULSE },
	// 4 trident
	{ SHIP_OUTLINE(hull_trident), { SHIP_OUTLINE(spine_trident), SHIP_NO_OUTLINE }, NULL, 0,
		{ {-30,-3}, {-30,3}, {0,0} }, 2, { {38,0}, {26,0} }, true, false, 46, -30,
		WEAPON_MISSILE, { {16,-9}, {16,9}, {-12,0} }, 3, WEAPON_FLAK },
	// 5 scythe
	{ SHIP_OUTLINE(hull_scythe), { SHIP_OUTLINE(spine_scythe), SHIP_NO_OUTLINE }, NULL, 0,
		{ {-30,-3}, {-30,3}, {0,0} }, 2, { {24,0}, {10,0} }, true, false, 30, -30,
		WEAPON_BEAM, { {0,0} }, 0, WEAPON_PULSE },
	// 6 bat
	{ SHIP_OUTLINE(hull_bat), { SHIP_OUTLINE(spine_bat), SHIP_NO_OUTLINE }, NULL, 0,
		{ {-32,-3}, {-32,3}, {0,0} }, 2, { {20,0}, {8,0} }, true, false, 26, -32,
		WEAPON_PULSE, { {0,0} }, 0, WEAPON_PULSE },
	// 7 leviathan
	{ SHIP_OUTLINE(hull_leviathan), { SHIP_OUTLINE(spine_leviathan), SHIP_NO_OUTLINE },
		spikes_leviathan, 4,
		{ {-50,-2}, {-50,2}, {0,0} }, 2, { {44,0}, {30,0} }, true, false, 52, -50,
		WEAPON_TORPEDO, { {30,0}, {8,0}, {-14,0}, {-34,0} }, 4, WEAPON_MASS_DRIVER },
	// 8 wedge
	{ SHIP_OUTLINE(hull_wedge), { SHIP_OUTLINE(plate_wedge), SHIP_OUTLINE(top_wedge) }, NULL, 0,
		{ {-36,-10}, {-36,0}, {-36,10} }, 3, { {26,0}, {8,0} }, true, false, 42, -36,
		WEAPON_PULSE, { {10,0}, {-10,-6}, {-10,6}, {-26,0} }, 4, WEAPON_BEAM },
	// 9 forward-swept
	{ SHIP_OUTLINE(hull_swept), { SHIP_OUTLINE(spine_swept), SHIP_NO_OUTLINE }, NULL, 0,
		{ {-26,-3}, {-26,3}, {0,0} }, 2, { {28,0}, {16,0} }, true, false, 36, -26,
		WEAPON_MASS_DRIVER, { {0,0} }, 0, WEAPON_PULSE }
};

// Maps design coordinates (tenths of a unit, origin at the design's own
// origin) to screen pixels. The ship is centred on x along its length.
typedef struct {
	int x, y, direction;
	double scale;              // pixels per tenth
	double middle;             // design x that lands on `x`
	double cos_pitch, sin_pitch; // tilt of the nose above the horizontal
} ShipTransform;

static void ship_to_pixels(const ShipTransform *xf, int tx, int ty, int *px, int *py) {
	double along = (tx - xf->middle) * xf->scale;
	double down = ty * xf->scale;
	*px = xf->x + (int)floor(xf->direction * (along * xf->cos_pitch + down * xf->sin_pitch) + 0.5);
	*py = xf->y + (int)floor(-along * xf->sin_pitch + down * xf->cos_pitch + 0.5);
}

// Fills px/py with the mirrored outline of a half outline; returns the point count.
static int ship_build_polygon(const ShipTransform *xf, const ShipOutline *outline,
	int *px, int *py) {
	int count = 0;
	for(int i = 0; i < outline->count; i++) {
		ship_to_pixels(xf, outline->points[i][0], outline->points[i][1], &px[count], &py[count]);
		count++;
	}
	for(int i = outline->count - 2; i >= 1; i--) {
		ship_to_pixels(xf, outline->points[i][0], -outline->points[i][1], &px[count], &py[count]);
		count++;
	}
	return count;
}

static Uint8 ship_mix(int from, int to, double amount) {
	return (Uint8)(from + (to - from) * amount + 0.5);
}

// Colour of a lighting band (3 highlight .. 0 shadow), pushed lighter (bias > 0)
// or darker (bias < 0).
static void ship_tone(const ShipTeam *team, int band, double bias, Uint8 *rgb) {
	const Uint8 *target = team->tone[bias >= 0 ? 3 : 0];
	double amount = bias >= 0 ? bias : -bias;
	for(int i = 0; i < 3; i++) rgb[i] = ship_mix(team->tone[band][i], target[i], amount);
}

// The vertical stretches of a polygon on pixel column `col`, top to bottom.
static int ship_column_spans(const int *px, const int *py, int count, int col, int spans[][2]) {
	double x = col + 0.5;
	double crossings[24];
	int found = 0;
	for(int i = 0; i < count && found < 24; i++) {
		int j = (i + 1) % count;
		if((px[i] <= x && x < px[j]) || (px[j] <= x && x < px[i])) {
			crossings[found++] = py[i] + (x - px[i]) * (double)(py[j] - py[i]) / (px[j] - px[i]);
		}
	}
	for(int i = 1; i < found; i++) {
		double value = crossings[i];
		int j = i - 1;
		while(j >= 0 && crossings[j] > value) {
			crossings[j + 1] = crossings[j];
			j--;
		}
		crossings[j + 1] = value;
	}
	int span_count = 0;
	for(int i = 0; i + 1 < found; i += 2) {
		int top = (int)ceil(crossings[i]);
		int bottom = (int)floor(crossings[i + 1]);
		if(bottom >= top && span_count < 12) {
			spans[span_count][0] = top;
			spans[span_count][1] = bottom;
			span_count++;
		}
	}
	return span_count;
}

// Paints one polygon in four bands of light with a dark outline.
static void ship_paint(SDL_Surface *surface, const ShipTeam *team, const int *px,
	const int *py, int count, double bias, int outline_width) {
	int min_x = px[0], max_x = px[0];
	for(int i = 1; i < count; i++) {
		if(px[i] < min_x) min_x = px[i];
		if(px[i] > max_x) max_x = px[i];
	}
	static const int bands[3] = { 3, 2, 1 };
	for(int col = min_x; col <= max_x; col++) {
		int spans[12][2];
		int span_count = ship_column_spans(px, py, count, col, spans);
		for(int span = 0; span < span_count; span++) {
			int top = spans[span][0], bottom = spans[span][1];
			int height = bottom - top + 1;
			int band_height[3];
			band_height[0] = (int)(height * 0.14 + 0.5);
			if(band_height[0] < 1) band_height[0] = 1;
			band_height[1] = (int)(height * 0.28 + 0.5);
			band_height[2] = (int)(height * 0.30 + 0.5);
			int y = top;
			Uint8 rgb[3];
			for(int band = 0; band < 3 && y <= bottom; band++) {
				if(band_height[band] <= 0) continue;
				int band_bottom = y + band_height[band] - 1;
				if(band_bottom > bottom) band_bottom = bottom;
				ship_tone(team, bands[band], bias, rgb);
				vlineRGBA(surface, col, y, band_bottom, rgb[0], rgb[1], rgb[2], 255);
				y = band_bottom + 1;
			}
			if(y <= bottom) {
				ship_tone(team, 0, bias, rgb);
				bool bounce = height >= 7;
				vlineRGBA(surface, col, y, bounce ? bottom - 1 : bottom, rgb[0], rgb[1], rgb[2], 255);
				if(bounce) {
					// A little bounced light along the lower edge.
					vlineRGBA(surface, col, bottom, bottom,
						ship_mix(rgb[0], team->glow[0], 0.35),
						ship_mix(rgb[1], team->glow[1], 0.35),
						ship_mix(rgb[2], team->glow[2], 0.35), 255);
				}
			}
		}
	}
	for(int i = 0; i < count; i++) {
		int j = (i + 1) % count;
		thickLineRGBA(surface, px[i], py[i], px[j], py[j], outline_width, 8, 10, 18, 255);
	}
}

// Scratch copy of the screen area under the ship being drawn.
static SDL_Surface *ship_backup;

// Draws one ship centred on (x, y), `length` pixels long, nose towards `direction`
// (1: right, -1: left) and tilted up by `pitch` radians, blended onto the
// surface at `opacity` (0..255).
static void draw_ship_design(SDL_Surface *surface, int x, int y, int design_index,
	int length, int direction, int team_index, Uint8 opacity, double pitch = 0.0) {
	if(opacity == 0) return;
	const ShipDesign *design = &ship_designs[design_index];
	const ShipTeam *team = &ship_teams[team_index];
	ShipTransform xf;
	xf.x = x;
	xf.y = y;
	xf.direction = direction;
	xf.scale = length / (double)(design->nose - design->tail);
	xf.middle = (design->nose + design->tail) / 2.0;
	xf.cos_pitch = cos(pitch);
	xf.sin_pitch = sin(pitch);

	// Everything the ship draws stays inside this area, which is saved first.
	// (A tilted ship still fits: nothing is further than 0.8 lengths from its centre.)
	int reach = length + length / 4 + 12;
	int left = x - reach < 0 ? 0 : x - reach;
	int top = y - reach < 0 ? 0 : y - reach;
	int right = x + reach + 1 > surface->w ? surface->w : x + reach + 1;
	int bottom = y + reach + 1 > surface->h ? surface->h : y + reach + 1;
	if(right <= left || bottom <= top) return;
	SDL_Rect area;
	area.x = (Sint16)left;
	area.y = (Sint16)top;
	area.w = (Uint16)(right - left);
	area.h = (Uint16)(bottom - top);
	if(opacity < 255) {
		if(!ship_backup || ship_backup->w < area.w || ship_backup->h < area.h) {
			int backup_w = ship_backup && ship_backup->w > area.w ? ship_backup->w : area.w;
			int backup_h = ship_backup && ship_backup->h > area.h ? ship_backup->h : area.h;
			if(ship_backup) SDL_FreeSurface(ship_backup);
			ship_backup = SDL_CreateRGBSurface(SDL_SWSURFACE, backup_w, backup_h, 32,
				surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, 0);
			if(!ship_backup) return;
		}
		SDL_Rect source = area;
		SDL_Rect target;
		target.x = 0;
		target.y = 0;
		target.w = 0;
		target.h = 0;
		SDL_BlitSurface(surface, &source, ship_backup, &target);
	}
	SDL_Rect previous_clip;
	SDL_GetClipRect(surface, &previous_clip);
	SDL_SetClipRect(surface, &area);

	int outline_width = xf.scale * 10.0 >= 16.0 ? 2 : 1;
	int engine_radius = (int)(SHIP_ENGINE_SIZE * xf.scale + 0.5);
	if(engine_radius < 1) engine_radius = 1;

	// Engine plumes go behind the hull.
	int nozzle_x[3], nozzle_y[3];
	for(int engine = 0; engine < design->engine_count; engine++) {
		ship_to_pixels(&xf, design->engines[engine][0], design->engines[engine][1],
			&nozzle_x[engine], &nozzle_y[engine]);
		for(int layer = 0; layer < 4; layer++) {
			int plume = (int)(engine_radius * (8.0 - layer * 1.8));
			int plume_width = (int)(engine_radius * (1.3 - layer * 0.25));
			if(plume_width < 1) plume_width = 1;
			// The plume points straight back from the nose; its width is across the ship.
			double tail_x = -direction * xf.cos_pitch, tail_y = xf.sin_pitch;
			double side_x = direction * xf.sin_pitch, side_y = xf.cos_pitch;
			filledTrigonRGBA(surface,
				nozzle_x[engine] + (int)(side_x * plume_width), nozzle_y[engine] + (int)(side_y * plume_width),
				nozzle_x[engine] - (int)(side_x * plume_width), nozzle_y[engine] - (int)(side_y * plume_width),
				nozzle_x[engine] + (int)(tail_x * plume), nozzle_y[engine] + (int)(tail_y * plume),
				team->glow[0], team->glow[1], team->glow[2], 36 + layer * 30);
		}
		filledCircleRGBA(surface, nozzle_x[engine], nozzle_y[engine], engine_radius * 2,
			team->glow[0], team->glow[1], team->glow[2], 46);
	}

	int px[24], py[24];
	for(int spike = 0; spike < design->spike_count; spike++) {
		for(int side = -1; side <= 1; side += 2) {
			int spike_x = design->spikes[spike][0];
			int spike_height = design->spikes[spike][1];
			ship_to_pixels(&xf, spike_x, side * 7, &px[0], &py[0]);
			ship_to_pixels(&xf, spike_x - 5, side * spike_height, &px[1], &py[1]);
			ship_to_pixels(&xf, spike_x - 10, side * 7, &px[2], &py[2]);
			ship_paint(surface, team, px, py, 3, -0.30, outline_width);
		}
	}
	int count = ship_build_polygon(&xf, &design->hull, px, py);
	ship_paint(surface, team, px, py, count, 0.0, outline_width);
	for(int plate = 0; plate < 2; plate++) {
		if(design->plates[plate].count == 0) continue;
		count = ship_build_polygon(&xf, &design->plates[plate], px, py);
		ship_paint(surface, team, px, py, count, 0.16 * (plate + 1), 1);
	}

	if(design->accent_on) {
		int x1, y1, x2, y2;
		ship_to_pixels(&xf, design->accent[0][0], design->accent[0][1], &x1, &y1);
		ship_to_pixels(&xf, design->accent[1][0], design->accent[1][1], &x2, &y2);
		thickLineRGBA(surface, x1, y1, x2, y2, 3,
			team->accent[0], team->accent[1], team->accent[2], 70);
		thickLineRGBA(surface, x1, y1, x2, y2, 1,
			team->accent[0], team->accent[1], team->accent[2], 255);
	}
	// Gun turrets: a round mount with barrels that look different for each weapon.
	for(int turret = 0; turret < design->turret_count; turret++) {
		int cx, cy;
		ship_to_pixels(&xf, design->turrets[turret][0], design->turrets[turret][1], &cx, &cy);
		int base = (int)(xf.scale * 3.4 + 0.5);
		if(base < 2) base = 2;
		double forward_x = direction * xf.cos_pitch, forward_y = -xf.sin_pitch;
		double across_x = -forward_y, across_y = forward_x;
		int weapon = design->turret_weapon;
		double barrel = base * (weapon == WEAPON_FLAK ? 1.7 : 2.5);
		int barrels = weapon == WEAPON_MASS_DRIVER ? 2 : 1;
		int barrel_width = (weapon == WEAPON_FLAK || base >= 4) ? 3 : 2;
		filledCircleRGBA(surface, cx, cy, base + 1, 8, 10, 18, 255);
		for(int b = 0; b < barrels; b++) {
			double shift = barrels == 2 ? (b ? 0.5 : -0.5) * base : 0.0;
			int sx = cx + (int)(across_x * shift), sy = cy + (int)(across_y * shift);
			int ex = sx + (int)(forward_x * barrel), ey = sy + (int)(forward_y * barrel);
			thickLineRGBA(surface, sx, sy, ex, ey, barrel_width + 2, 8, 10, 18, 255);
			thickLineRGBA(surface, sx, sy, ex, ey, barrel_width,
				team->tone[2][0], team->tone[2][1], team->tone[2][2], 255);
			if(weapon == WEAPON_BEAM || weapon == WEAPON_PULSE) {
				filledCircleRGBA(surface, ex, ey, 1,
					team->accent[0], team->accent[1], team->accent[2], 255);
			}
		}
		filledCircleRGBA(surface, cx, cy, base, team->tone[2][0], team->tone[2][1],
			team->tone[2][2], 255);
		filledCircleRGBA(surface, cx - base / 3, cy - base / 3, (base + 2) / 3,
			team->tone[3][0], team->tone[3][1], team->tone[3][2], 255);
	}
	if(design->eye) {
		int eye_x, eye_y;
		ship_to_pixels(&xf, 0, 0, &eye_x, &eye_y);
		int eye_unit = (int)(xf.scale * 10.0 + 0.5);
		filledCircleRGBA(surface, eye_x, eye_y, eye_unit * 6 / 10 + 2,
			team->accent[0], team->accent[1], team->accent[2], 60);
		filledEllipseRGBA(surface, eye_x, eye_y, 3 * eye_unit / 10 + 2, eye_unit / 10 + 1,
			team->accent[0], team->accent[1], team->accent[2], 255);
		filledCircleRGBA(surface, eye_x, eye_y, eye_unit / 10 < 1 ? 1 : eye_unit / 10,
			255, 255, 255, 255);
	}
	// Hot engine cores sit on top of the hull.
	for(int engine = 0; engine < design->engine_count; engine++) {
		filledCircleRGBA(surface, nozzle_x[engine], nozzle_y[engine], engine_radius + 1,
			ship_mix(team->glow[0], 255, 0.5), ship_mix(team->glow[1], 255, 0.5),
			ship_mix(team->glow[2], 255, 0.5), 255);
		filledCircleRGBA(surface, nozzle_x[engine], nozzle_y[engine],
			engine_radius / 2 < 1 ? 1 : engine_radius / 2, 255, 255, 255, 255);
	}

	if(opacity < 255) {
		// Blend the saved background back over the ship: ship * opacity + scene * (1 - opacity).
		SDL_SetAlpha(ship_backup, SDL_SRCALPHA, (Uint8)(255 - opacity));
		SDL_Rect source;
		source.x = 0;
		source.y = 0;
		source.w = area.w;
		source.h = area.h;
		SDL_Rect target = area;
		SDL_BlitSurface(ship_backup, &source, surface, &target);
		SDL_SetAlpha(ship_backup, 0, 255);
	}
	SDL_SetClipRect(surface, &previous_clip);
}

// The design that `variety` picks among the ships of a role.
static int ship_design_index(int role, unsigned int variety) {
	const int *designs = ship_small_designs;
	int count = (int)(sizeof(ship_small_designs) / sizeof(ship_small_designs[0]));
	if(role == SHIP_MID) {
		designs = ship_mid_designs;
		count = (int)(sizeof(ship_mid_designs) / sizeof(ship_mid_designs[0]));
	} else if(role == SHIP_BIG) {
		designs = ship_big_designs;
		count = (int)(sizeof(ship_big_designs) / sizeof(ship_big_designs[0]));
	}
	return designs[variety % (unsigned int)count];
}

// Draws a ship of the given role. `variety` picks one of the designs of that role.
static void draw_ship_role(SDL_Surface *surface, int x, int y, int role,
	unsigned int variety, int length, int direction, int team_index, Uint8 opacity,
	double pitch = 0.0) {
	draw_ship_design(surface, x, y, ship_design_index(role, variety), length,
		direction, team_index, opacity, pitch);
}

// Where a point of a design (the nose, a turret) ends up on screen.
static void ship_mount_point(int design_index, int x, int y, int length, int direction,
	double pitch, int tx, int ty, double *out_x, double *out_y) {
	const ShipDesign *design = &ship_designs[design_index];
	double scale = length / (double)(design->nose - design->tail);
	double middle = (design->nose + design->tail) / 2.0;
	double along = (tx - middle) * scale;
	double down = ty * scale;
	*out_x = x + direction * (along * cos(pitch) + down * sin(pitch));
	*out_y = y - along * sin(pitch) + down * cos(pitch);
}

/*
 * Weapon and explosion effects
 *
 * Both are stateless: given how many milliseconds ago a shot was fired they
 * draw exactly what is on screen now.
 */
static const Uint8 weapon_team_colors[2][3] = { { 110, 205, 235 }, { 255, 165, 105 } };

static unsigned int fx_hash(unsigned int a, unsigned int b) {
	unsigned int value = a * 2654435761u + b * 2246822519u + 0x9e3779b9u;
	value ^= value >> 15;
	value *= 2246822519u;
	value ^= value >> 13;
	return value;
}

static Uint8 weapon_alpha(double alpha) {
	if(alpha < 0.0) alpha = 0.0;
	if(alpha > 255.0) alpha = 255.0;
	return (Uint8)alpha;
}

// Radius a shot of a ship `length` pixels long is drawn at.
static double weapon_unit(int length) {
	double unit = 10.0 + length * 0.25;
	return unit > 34.0 ? 34.0 : unit;
}

// A soft round glow: a faint halo, a brighter body and a hot core.
static void weapon_glow(SDL_Surface *surface, double x, double y, double radius,
	const Uint8 *color, double alpha) {
	int r = (int)(radius + 0.5);
	if(r < 1) r = 1;
	filledCircleRGBA(surface, (int)x, (int)y, r * 2, color[0], color[1], color[2],
		weapon_alpha(alpha * 0.22));
	filledCircleRGBA(surface, (int)x, (int)y, r + r / 2, color[0], color[1], color[2],
		weapon_alpha(alpha * 0.55));
	filledCircleRGBA(surface, (int)x, (int)y, (r + 1) / 2, 255, 255, 255, weapon_alpha(alpha));
}

// An explosion `age` ms after it began: a white flash, a fireball of overlapping
// blobs that cools from yellow through orange to grey smoke, a shockwave ring,
// glowing debris, and for a destroyed ship tumbling hull fragments and two
// smaller blasts after the first.
static void draw_explosion(SDL_Surface *surface, double x, double y, double age,
	double radius, bool big, unsigned int seed, int team, double fade) {
	if(radius < 2.0) radius = 2.0;
	double life = big ? 1700.0 : 380.0 + radius * 10.0;
	if(!big && life > 900.0) life = 900.0;
	if(age < 0.0 || age >= life || fade <= 0.0) return;
	double p = age / life;
	double inv = 1.0 - p;

	static const Uint8 stops[4][3] = {
		{ 255, 238, 190 }, { 246, 142, 62 }, { 118, 50, 34 }, { 60, 58, 66 } };
	double position = p * 3.0;
	int stop = (int)position;
	if(stop > 2) stop = 2;
	double between = position - stop;
	Uint8 red = ship_mix(stops[stop][0], stops[stop + 1][0], between);
	Uint8 green = ship_mix(stops[stop][1], stops[stop + 1][1], between);
	Uint8 blue = ship_mix(stops[stop][2], stops[stop + 1][2], between);
	double body = 215.0 * fade * pow(inv, 1.3);
	int blobs = big ? 9 : 5;
	for(int blob = 0; blob < blobs; blob++) {
		unsigned int h = fx_hash(seed, blob);
		double angle = (h % 6283) / 1000.0;
		double spread = ((h >> 12) % 100) / 100.0;
		double distance = radius * (0.15 + 0.7 * spread) * (0.3 + 0.7 * sqrt(p));
		double size = radius * (0.35 + 0.35 * ((h >> 20) % 100) / 100.0)
			* (0.3 + 0.7 * smooth_step(0.0, 0.3, p));
		filledCircleRGBA(surface, (int)(x + cos(angle) * distance),
			(int)(y + sin(angle) * distance), (int)size + 1,
			red, green, blue, weapon_alpha(body * 0.8));
	}
	if(p < 0.3) {
		double hot = 1.0 - p / 0.3;
		filledCircleRGBA(surface, (int)x, (int)y, (int)(radius * (0.2 + 0.6 * hot)) + 1,
			255, 250, 235, weapon_alpha(240.0 * fade * hot));
	}
	double ring = radius * (0.5 + 2.2 * sqrt(p));
	circleRGBA(surface, (int)x, (int)y, (int)ring, 255, 214, 160,
		weapon_alpha(170.0 * fade * inv * inv));
	if(big) {
		circleRGBA(surface, (int)x, (int)y, (int)(ring * 0.7), 255, 180, 110,
			weapon_alpha(120.0 * fade * inv * inv));
	}
	int pieces = big ? 16 : 8;
	for(int piece = 0; piece < pieces; piece++) {
		unsigned int h = fx_hash(seed + 77u, piece);
		double angle = (h % 6283) / 1000.0;
		double speed = radius * (1.2 + 2.0 * ((h >> 12) % 100) / 100.0);
		double reach = speed * (1.0 - pow(inv, 2.2));
		double trail = radius * 0.55 * inv;
		thickLineRGBA(surface, (int)(x + cos(angle) * reach), (int)(y + sin(angle) * reach),
			(int)(x + cos(angle) * (reach - trail)), (int)(y + sin(angle) * (reach - trail)),
			1, 255, 215 - piece * 3, 120, weapon_alpha(230.0 * fade * inv));
	}
	if(big) {
		const ShipTeam *hull = &ship_teams[team];
		for(int fragment = 0; fragment < 6; fragment++) {
			unsigned int h = fx_hash(seed + 191u, fragment);
			double angle = (h % 6283) / 1000.0;
			double speed = radius * (0.5 + 1.1 * ((h >> 12) % 100) / 100.0);
			double reach = speed * (1.0 - pow(inv, 1.6));
			double cx = x + cos(angle) * reach;
			double cy = y + sin(angle) * reach * 0.8;
			double spin = ((h >> 8) % 628) / 100.0 + p * (3.0 + fragment);
			double size = radius * 0.2;
			int px[3], py[3];
			for(int corner = 0; corner < 3; corner++) {
				double corner_angle = spin + corner * 2.0944;
				double corner_size = size * (corner == 0 ? 1.5 : 0.8);
				px[corner] = (int)(cx + cos(corner_angle) * corner_size);
				py[corner] = (int)(cy + sin(corner_angle) * corner_size);
			}
			Uint8 piece_alpha = weapon_alpha(255.0 * fade * inv);
			filledTrigonRGBA(surface, px[0], py[0], px[1], py[1], px[2], py[2],
				hull->tone[2][0], hull->tone[2][1], hull->tone[2][2], piece_alpha);
			lineRGBA(surface, px[0], py[0], px[1], py[1], hull->glow[0], hull->glow[1],
				hull->glow[2], weapon_alpha(piece_alpha * 0.8));
		}
		if(age > 160.0) {
			draw_explosion(surface, x + radius * 0.6, y - radius * 0.25, age - 160.0,
				radius * 0.55, false, seed + 1u, team, fade);
		}
		if(age > 340.0) {
			draw_explosion(surface, x - radius * 0.55, y + radius * 0.3, age - 340.0,
				radius * 0.5, false, seed + 2u, team, fade);
		}
	}
}

// Point on the route of a guided shot: the straight line from the muzzle to the
// target, bent sideways by up to `bend` pixels half way.
static void weapon_route(double fx, double fy, double dx, double dy, double nx, double ny,
	double fraction, double bend, double *x, double *y) {
	double arc = fraction < 1.0 ? bend * sin(3.14159265359 * fraction) : 0.0;
	*x = fx + dx * fraction + nx * arc;
	*y = fy + dy * fraction + ny * arc;
}

/*
 * One shot of `weapon`, fired from (fx, fy) at (tx, ty), `age` ms ago.
 * outcome: 0 the shot misses (guided weapons are shot down half way),
 *          1 it hits, 2 it hits and destroys the target (the caller draws that).
 * `unit` is the size of a shot in pixels (see weapon_unit), `shot` the index
 * of the shot within its volley.
 */
static void draw_weapon_shot(SDL_Surface *surface, int weapon, int shot, double fx, double fy,
	double tx, double ty, double age, double unit, int team, double fade, int outcome,
	unsigned int seed) {
	const WeaponSpec *spec = &weapon_specs[weapon];
	if(age < 0.0 || age >= spec->life_ms || fade <= 0.0) return;
	const Uint8 *color = weapon_team_colors[team];
	bool guided = weapon == WEAPON_MISSILE || weapon == WEAPON_SWARM || weapon == WEAPON_TORPEDO;
	if(guided && outcome == 0) {
		// Shot down by point defence before it arrives.
		tx = fx + (tx - fx) * 0.7;
		ty = fy + (ty - fy) * 0.7;
	}
	double dx = tx - fx, dy = ty - fy;
	double span = sqrt(dx * dx + dy * dy);
	if(span < 1.0) span = 1.0;
	double ux = dx / span, uy = dy / span;
	double nx = -uy, ny = ux;
	double p = age / spec->impact_ms;
	double after = age - spec->impact_ms;
	bool lands = outcome != 0 || guided;
	bool arrived = lands && p >= 1.0;
	double miss_fade = 1.0;
	if(!lands && p > 1.0) miss_fade = 1.0 - (p - 1.0) / (spec->life_ms / spec->impact_ms - 1.0);
	double blast = unit * spec->blast * 1.6;
	if(age < 90.0 && weapon != WEAPON_RAIL && weapon != WEAPON_BEAM) {
		weapon_glow(surface, fx, fy, unit * 0.2 * (1.0 - age / 90.0), color, 230.0 * fade);
	}

	switch(weapon) {
		case WEAPON_PULSE: {
			if(!arrived) {
				double hx = fx + dx * p, hy = fy + dy * p;
				double tail = unit * 0.55;
				thickLineRGBA(surface, (int)(hx - ux * tail), (int)(hy - uy * tail),
					(int)hx, (int)hy, 2, color[0], color[1], color[2],
					weapon_alpha(170.0 * fade * miss_fade));
				thickLineRGBA(surface, (int)(hx - ux * tail * 0.55), (int)(hy - uy * tail * 0.55),
					(int)hx, (int)hy, 1, 255, 255, 255, weapon_alpha(240.0 * fade * miss_fade));
			}
			break;
		}
		case WEAPON_MASS_DRIVER: {
			double offset = ((shot % 3) - 1) * unit * 0.1;
			if(!arrived) {
				double hx = fx + dx * p + nx * offset, hy = fy + dy * p + ny * offset;
				double tail = unit * 0.55;
				thickLineRGBA(surface, (int)(hx - ux * tail), (int)(hy - uy * tail),
					(int)hx, (int)hy, 1, 255, 232, 160, weapon_alpha(235.0 * fade * miss_fade));
				pixelRGBA(surface, (int)hx, (int)hy, 255, 255, 255, weapon_alpha(255.0 * fade));
			}
			break;
		}
		case WEAPON_PLASMA: {
			static const Uint8 plasma[3] = { 140, 255, 170 };
			if(!arrived) {
				double radius = unit * 0.2 * (1.0 + 0.12 * sin(age / 45.0));
				for(int k = 5; k >= 1; k--) {
					double pk = p - k * 0.05;
					if(pk < 0.0) continue;
					weapon_glow(surface, fx + dx * pk, fy + dy * pk, radius * (1.0 - k * 0.13),
						plasma, 150.0 * fade * miss_fade * (1.0 - k / 6.0));
				}
				weapon_glow(surface, fx + dx * p, fy + dy * p, radius, plasma,
					235.0 * fade * miss_fade);
			} else if(outcome != 0 && after < 500.0) {
				circleRGBA(surface, (int)tx, (int)ty, (int)(unit * 0.3 + after * unit * 0.0018),
					plasma[0], plasma[1], plasma[2], weapon_alpha(200.0 * fade * (1.0 - after / 500.0)));
			}
			break;
		}
		case WEAPON_FLAK: {
			double bx = fx + dx * 0.92, by = fy + dy * 0.92;
			if(p < 1.0) {
				double hx = fx + dx * 0.92 * p, hy = fy + dy * 0.92 * p;
				double tail = unit * 0.4;
				thickLineRGBA(surface, (int)(hx - ux * tail), (int)(hy - uy * tail),
					(int)hx, (int)hy, 2, 255, 185, 90, weapon_alpha(210.0 * fade));
				pixelRGBA(surface, (int)hx, (int)hy, 255, 255, 255, weapon_alpha(255.0 * fade));
			} else if(after < 560.0) {
				double burst = after / 560.0;
				for(int piece = 0; piece < 14; piece++) {
					unsigned int h = fx_hash(seed + 5u, piece);
					double angle = piece * 0.4488 + (h % 100) * 0.003;
					double near_edge = unit * (0.15 + 0.95 * sqrt(burst));
					double far_edge = near_edge + unit * 0.4 * (1.0 - burst);
					thickLineRGBA(surface, (int)(bx + cos(angle) * near_edge),
						(int)(by + sin(angle) * near_edge), (int)(bx + cos(angle) * far_edge),
						(int)(by + sin(angle) * far_edge), 1, 255, 215, 120,
						weapon_alpha(235.0 * fade * (1.0 - burst)));
				}
				filledCircleRGBA(surface, (int)bx, (int)by, (int)(unit * 0.3 * (0.5 + burst)) + 1,
					110, 96, 84, weapon_alpha(110.0 * fade * (1.0 - burst)));
				if(after < 140.0) {
					filledCircleRGBA(surface, (int)bx, (int)by, (int)(unit * 0.45),
						255, 245, 210, weapon_alpha(230.0 * fade * (1.0 - after / 140.0)));
				}
			}
			if(outcome == 1) draw_explosion(surface, bx, by, after, blast, false, seed, team, fade);
			return;
		}
		case WEAPON_MISSILE:
		case WEAPON_SWARM:
		case WEAPON_TORPEDO: {
			bool swarm = weapon == WEAPON_SWARM, torpedo = weapon == WEAPON_TORPEDO;
			double bend = torpedo ? 0.0
				: swarm ? unit * (0.9 + 0.35 * (shot % 3)) * (shot % 2 ? 1.0 : -1.0)
				: unit * 0.7 * ((seed & 1) ? 1.0 : -1.0);
			double step = torpedo ? 0.035 : 0.05;
			double smoke_size = torpedo ? 3.0 : (swarm ? 1.0 : 1.5);
			double smoke_fade = p < 1.0 ? 1.0
				: 1.0 - after / (spec->life_ms - spec->impact_ms);
			if(smoke_fade < 0.0) smoke_fade = 0.0;
			double reach = p < 1.0 ? p : 1.0;
			for(int k = 1; k <= 10; k++) {
				double pk = reach - k * step;
				if(pk < 0.0) break;
				double sx, sy;
				weapon_route(fx, fy, dx, dy, nx, ny, pk, bend, &sx, &sy);
				filledCircleRGBA(surface, (int)sx, (int)sy, (int)(smoke_size + k / 3.0),
					176, 176, 188, weapon_alpha(90.0 * fade * smoke_fade * (1.0 - k / 11.0)));
			}
			if(p < 1.0) {
				double hx, hy, ax, ay;
				weapon_route(fx, fy, dx, dy, nx, ny, p, bend, &hx, &hy);
				weapon_route(fx, fy, dx, dy, nx, ny, p + 0.01, bend, &ax, &ay);
				double bx = ax - hx, by = ay - hy;
				double bl = sqrt(bx * bx + by * by);
				if(bl < 0.001) { bx = ux; by = uy; } else { bx /= bl; by /= bl; }
				double body = unit * (torpedo ? 0.7 : (swarm ? 0.22 : 0.42));
				int width = torpedo ? 3 : (swarm ? 1 : 2);
				double tail_x = hx - bx * body, tail_y = hy - by * body;
				double flame = body * (0.9 + 0.3 * sin(age / 25.0 + shot));
				static const Uint8 flame_color[3] = { 255, 170, 80 };
				static const Uint8 torpedo_flame[3] = { 120, 190, 255 };
				weapon_glow(surface, tail_x, tail_y, torpedo ? unit * 0.28 : unit * 0.12,
					torpedo ? torpedo_flame : flame_color, 200.0 * fade);
				thickLineRGBA(surface, (int)tail_x, (int)tail_y,
					(int)(tail_x - bx * flame), (int)(tail_y - by * flame), width,
					255, 200, 120, weapon_alpha(210.0 * fade));
				thickLineRGBA(surface, (int)tail_x, (int)tail_y, (int)hx, (int)hy, width + (torpedo ? 1 : 0),
					torpedo ? 150 : 205, torpedo ? 160 : 208, torpedo ? 190 : 218, weapon_alpha(255.0 * fade));
				filledCircleRGBA(surface, (int)hx, (int)hy, torpedo ? 2 : 1,
					255, torpedo ? 90 : 255, torpedo ? 80 : 255, weapon_alpha(255.0 * fade));
			}
			if(arrived && outcome != 2) {
				draw_explosion(surface, tx, ty, after, blast, torpedo && outcome == 1,
					seed, team, fade);
			}
			return;
		}
		case WEAPON_RAIL: {
			if(age < 300.0) {
				double charge = age / 300.0;
				weapon_glow(surface, fx, fy, unit * 0.5 * charge, color, 230.0 * fade * charge);
				for(int tick = 0; tick < 4; tick++) {
					double angle = tick * 1.5708 + 0.4;
					double outer = unit * (1.1 - 0.8 * charge);
					lineRGBA(surface, (int)(fx + cos(angle) * outer), (int)(fy + sin(angle) * outer),
						(int)(fx + cos(angle) * outer * 0.6), (int)(fy + sin(angle) * outer * 0.6),
						255, 255, 255, weapon_alpha(210.0 * fade * charge));
				}
			} else {
				double since = age - 300.0;
				double f = (1.0 - since / 550.0) * fade;
				double ex = tx, ey = ty;
				if(outcome == 0) { ex = tx + ux * span * 0.6; ey = ty + uy * span * 0.6; }
				thickLineRGBA(surface, (int)fx, (int)fy, (int)ex, (int)ey, 5,
					color[0], color[1], color[2], weapon_alpha(70.0 * f));
				thickLineRGBA(surface, (int)fx, (int)fy, (int)ex, (int)ey, 3,
					color[0], color[1], color[2], weapon_alpha(150.0 * f));
				thickLineRGBA(surface, (int)fx, (int)fy, (int)ex, (int)ey, 1,
					255, 255, 255, weapon_alpha(255.0 * f));
				circleRGBA(surface, (int)fx, (int)fy, (int)(unit * 0.15 + since * unit * 0.001),
					color[0], color[1], color[2], weapon_alpha(170.0 * f));
				if(outcome == 1) draw_explosion(surface, tx, ty, since, blast, false, seed, team, fade);
			}
			return;
		}
		case WEAPON_BEAM: {
			if(age < 900.0) {
				double sweep = outcome == 0
					? unit * 1.1 * (1.0 - 2.0 * age / 900.0)
					: unit * 0.06 * sin(age / 23.0);
				double ex = tx + nx * sweep, ey = ty + ny * sweep;
				double reach = smooth_step(0.0, 70.0, age);
				double hx = fx + (ex - fx) * reach, hy = fy + (ey - fy) * reach;
				double f = smooth_step(0.0, 60.0, age) * (1.0 - smooth_step(650.0, 900.0, age)) * fade;
				int thick = 1 + (int)(unit / 16.0);
				thickLineRGBA(surface, (int)fx, (int)fy, (int)hx, (int)hy, thick * 2 + 3,
					color[0], color[1], color[2], weapon_alpha(50.0 * f));
				thickLineRGBA(surface, (int)fx, (int)fy, (int)hx, (int)hy, thick + 1,
					color[0], color[1], color[2], weapon_alpha(130.0 * f));
				thickLineRGBA(surface, (int)fx, (int)fy, (int)hx, (int)hy, thick,
					255, 255, 255, weapon_alpha(245.0 * f));
				weapon_glow(surface, fx, fy, unit * 0.22, color, 220.0 * f);
				weapon_glow(surface, hx, hy, unit * 0.28, color, 220.0 * f);
				if(outcome != 0 && reach >= 1.0) {
					for(int spark = 0; spark < 4; spark++) {
						unsigned int h = fx_hash(seed + (unsigned int)(age / 45.0), spark);
						double angle = (h % 6283) / 1000.0;
						double length = unit * (0.3 + ((h >> 12) % 60) / 100.0);
						lineRGBA(surface, (int)hx, (int)hy, (int)(hx + cos(angle) * length),
							(int)(hy + sin(angle) * length), 255, 230, 170, weapon_alpha(220.0 * f));
					}
				}
			}
			if(outcome == 1 && after > 0.0) {
				draw_explosion(surface, tx, ty, after, blast, false, seed, team, fade);
			}
			return;
		}
	}
	// Pulse, mass driver and plasma end in a small blast where they hit.
	if(arrived && outcome == 1) {
		draw_explosion(surface, tx, ty, after, blast < 2.0 ? 2.0 : blast, false, seed, team, fade);
	}
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

/*
 * Dogfights
 *
 * Like every effect a dogfight is stateless: where each ship is, which way it
 * points and which shots are in the air are all worked out from the time since
 * the effect began. A dogfight is made of pairs of ships, one of each team.
 * Each pair flies several passes across the sky. In a pass the leader flies a
 * route of wide loops, and the chaser follows the same route a little behind,
 * closing and dropping back, and fires its weapon at it. Most volleys miss; one
 * hits. The leader that was hit trails smoke for the rest of the pass, and on
 * the last pass it is destroyed. The next pass goes the other way with the
 * roles swapped.
 */
#define DOG_BURSTS 5            // volleys the chaser fires in a pass
#define DOG_HIT_BURST 3         // the volley that hits (the middle shot of it)

typedef struct {
	SDL_Surface *surface;
	int scale;
	Uint32 seed;
	int pair_count;
	int pass_count;
	double pass_ms;             // length of one pass
	double stagger_ms;          // delay between the start of one pair and the next
	int length;                 // ship length in pixels
} Dogfight;

typedef struct {
	bool visible;
	double x, y;
	int direction;              // 1: nose points right, -1: left
	double pitch;               // nose above the horizontal, radians
	double u;                   // 0: entering the screen, 1: leaving it
	int pass;
	int team;
} DogPose;

static unsigned int dog_hash(unsigned int a, unsigned int b) {
	return fx_hash(a, b);
}

// A point of the looping route of pair `pair` on pass `pass`; u runs 0..1 across the screen.
static void dog_route_point(const Dogfight *fight, int pair, int pass, double u,
	double *x, double *y) {
	int width = fight->surface->w;
	int loops = (int)(width / (fight->scale * 0.4) + 0.5);
	if(loops < 2) loops = 2;
	if(loops > 6) loops = 6;
	double margin = fight->scale / 10.0;
	double travel = width + 2.0 * margin;
	double drift = travel / (6.28318530718 * loops);
	double reach = drift * 1.45;          // wider than the drift, so the route loops
	double height = fight->scale * 0.032;
	double start = (dog_hash(fight->seed, pair * 8 + pass) % 6283) / 1000.0;
	double angle = 6.28318530718 * loops * u + start;
	double along = -margin + travel * u - reach * (sin(angle) - sin(start));
	*x = (pass % 2) ? width - along : along;
	*y = fight->surface->h * 0.078 - height * cos(angle);
}

// Where a ship of `pair` is at time t (ms since the effect began). Role 0 leads, role 1 chases.
static void dog_pose(const Dogfight *fight, int pair, int role, double t, DogPose *pose) {
	pose->visible = false;
	double local = t - pair * fight->stagger_ms;
	if(local < 0.0 || local >= fight->pass_count * fight->pass_ms) return;
	int pass = (int)(local / fight->pass_ms);
	double u = (local - pass * fight->pass_ms) / fight->pass_ms * 1.15;
	if(role == 1) u -= 0.1 * (1.0 + 0.4 * sin(u * 9.0 + pair + pass));
	double before_x, before_y, after_x, after_y;
	dog_route_point(fight, pair, pass, u - 0.003, &before_x, &before_y);
	dog_route_point(fight, pair, pass, u + 0.003, &after_x, &after_y);
	dog_route_point(fight, pair, pass, u, &pose->x, &pose->y);
	double heading = atan2(-(after_y - before_y), after_x - before_x);
	if(after_x >= before_x) {
		pose->direction = 1;
		pose->pitch = heading;
	} else {
		pose->direction = -1;
		pose->pitch = atan2(sin(3.14159265359 - heading), cos(3.14159265359 - heading));
	}
	pose->u = u;
	pose->pass = pass;
	pose->team = (role + pass) % 2;
	pose->visible = pose->x > -fight->length && pose->x < fight->surface->w + fight->length;
}

// When, within a pass, a volley starts; and when its shot number `bolt` is fired.
static double dog_shot_time(const Dogfight *fight, int burst, int bolt, int weapon) {
	return (0.18 + 0.15 * burst) * fight->pass_ms + bolt * weapon_specs[weapon].gap_ms;
}

// When, within a pass, the leader is hit by the chaser's weapon.
static double dog_hit_time(const Dogfight *fight, int weapon) {
	return dog_shot_time(fight, DOG_HIT_BURST, weapon_specs[weapon].shots / 2, weapon)
		+ weapon_specs[weapon].impact_ms;
}

static void render_dogfight(SDL_Surface *surface, const EffectWindow *window, int scale,
	int pair_count, int pass_count, double stagger) {
	Dogfight fight;
	fight.surface = surface;
	fight.scale = scale;
	fight.seed = window->seed;
	fight.pair_count = pair_count;
	fight.pass_count = pass_count;
	fight.pass_ms = (window->duration - 800.0) / (pass_count + (pair_count - 1) * stagger);
	fight.stagger_ms = fight.pass_ms * stagger;
	fight.length = scale / 34;
	if(fight.length < 14) fight.length = 14;
	double t = window->phase;
	double fade = window->fade;
	Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
	double unit = weapon_unit(fight.length);

	for(int pair = 0; pair < pair_count; pair++) {
		double local = t - pair * fight.stagger_ms;
		if(local < 0.0 || local >= pass_count * fight.pass_ms) continue;
		int pass = (int)(local / fight.pass_ms);
		double pass_age = local - pass * fight.pass_ms;
		bool last_pass = pass == pass_count - 1;
		double pass_start = pair * fight.stagger_ms + pass * fight.pass_ms;
		// The chaser is the second ship; it and the leader swap teams every pass.
		int chaser_team = (1 + pass) % 2, leader_team = pass % 2;
		int chaser_design = ship_design_index(SHIP_SMALL,
			dog_hash(fight.seed, pair * 2 + chaser_team) >> 4);
		int weapon = ship_designs[chaser_design].weapon;
		const WeaponSpec *spec = &weapon_specs[weapon];
		double hit_time = dog_hit_time(&fight, weapon);
		bool leader_hit = pass_age >= hit_time;
		bool leader_destroyed = leader_hit && last_pass;

		// Engine trails, and smoke behind the leader once it has been hit.
		for(int role = 0; role < 2; role++) {
			DogPose pose;
			dog_pose(&fight, pair, role, t, &pose);
			if(!pose.visible || (role == 0 && leader_destroyed)) continue;
			double last_x = pose.x, last_y = pose.y;
			for(int step = 1; step <= 10; step++) {
				DogPose earlier;
				dog_pose(&fight, pair, role, t - step * 50.0, &earlier);
				if(!earlier.visible || earlier.pass != pose.pass) break;
				thickLineRGBA(surface, (int)last_x, (int)last_y, (int)earlier.x, (int)earlier.y,
					1, ship_teams[pose.team].glow[0], ship_teams[pose.team].glow[1],
					ship_teams[pose.team].glow[2], (Uint8)(95 * fade * (1.0 - step / 11.0)));
				last_x = earlier.x;
				last_y = earlier.y;
			}
			if(role == 0 && leader_hit) {
				for(int puff = 1; puff <= 9; puff++) {
					double puff_time = t - puff * 110.0;
					if(puff_time - pass_start < hit_time) break;
					DogPose smoke;
					dog_pose(&fight, pair, 0, puff_time, &smoke);
					if(!smoke.visible || smoke.pass != pose.pass) break;
					filledCircleRGBA(surface, (int)smoke.x, (int)smoke.y, 1 + puff / 2,
						150, 150, 160, (Uint8)(70 * fade * (1.0 - puff / 10.0)));
				}
			}
		}

		// The ships.
		for(int role = 0; role < 2; role++) {
			DogPose pose;
			dog_pose(&fight, pair, role, t, &pose);
			if(!pose.visible || (role == 0 && leader_destroyed)) continue;
			unsigned int variety = dog_hash(fight.seed, pair * 2 + pose.team) >> 4;
			draw_ship_role(surface, (int)floor(pose.x + 0.5), (int)floor(pose.y + 0.5),
				SHIP_SMALL, variety, fight.length, pose.direction, pose.team, opacity,
				pose.pitch);
			if(role == 0 && leader_hit && !leader_destroyed) {
				// A burning patch where it was hit.
				filledCircleRGBA(surface, (int)pose.x, (int)pose.y, fight.length / 5 + 1,
					255, 140, 60, (Uint8)(110 * fade * (0.5 + 0.5 * sin(t / 70.0))));
			}
		}

		// Shots from the chaser at the leader, using the chaser's own weapon.
		for(int burst = 0; burst < DOG_BURSTS; burst++) {
			if(last_pass && burst > DOG_HIT_BURST) break;
			for(int bolt = 0; bolt < spec->shots; bolt++) {
				double fired = pass_start + dog_shot_time(&fight, burst, bolt, weapon);
				double age = t - fired;
				if(age < 0.0 || age >= spec->life_ms) continue;
				bool hit = burst == DOG_HIT_BURST && bolt == spec->shots / 2;
				// A beam stays on its target, so it follows both ships live.
				bool live = weapon == WEAPON_BEAM;
				DogPose shooter, target;
				dog_pose(&fight, pair, 1, live ? t : fired, &shooter);
				dog_pose(&fight, pair, 0, live ? t : fired + spec->impact_ms, &target);
				if(!shooter.visible || !target.visible) continue;
				double from_x, from_y;
				ship_mount_point(chaser_design, (int)shooter.x, (int)shooter.y, fight.length,
					shooter.direction, shooter.pitch, ship_designs[chaser_design].nose, 0,
					&from_x, &from_y);
				double to_x = target.x, to_y = target.y;
				if(!hit) {
					// Misses pass to one side or the other of the target.
					double side = ((burst * 3 + bolt) % 2 ? 1.0 : -1.0)
						* fight.length * (0.7 + 0.2 * ((burst + bolt) % 3));
					double dx = to_x - from_x, dy = to_y - from_y;
					double span = sqrt(dx * dx + dy * dy) + 1.0;
					to_x += -dy / span * side;
					to_y += dx / span * side;
				}
				draw_weapon_shot(surface, weapon, bolt, from_x, from_y, to_x, to_y, age, unit,
					chaser_team, fade, hit ? (last_pass ? 2 : 1) : 0,
					dog_hash(fight.seed, pair * 64 + pass * 16 + burst * 4 + bolt));
			}
		}

		// The destroyed leader blows up where it was when the shot landed.
		if(leader_destroyed) {
			DogPose last;
			dog_pose(&fight, pair, 0, pass_start + hit_time, &last);
			draw_explosion(surface, last.x, last.y, pass_age - hit_time, fight.length * 1.0,
				true, dog_hash(fight.seed, pair * 5 + 1), leader_team, fade);
		}
	}
}

// The old flyby: one ship from each side crosses the sky, firing pulse bolts at the
// other and passing it in the middle. Nothing is hit.
static void render_battle_flyby(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_BATTLE_FLYBY, ticks, &window)) return;
	Uint32 phase = window.phase;

	int size = scale / 170;
	if(size < 4) size = 4;
	int length = scale / 38;
	if(length < 14) length = 14;
	double fade = window.fade;
	double progress = window.progress;
	int left_x = (int)(-length + progress * (surface->w + 2.0 * length));
	int right_x = surface->w - left_x;
	int center_y = surface->h * 11 / 100;
	int arc = (int)(sin(progress * 6.28318530718) * scale * 0.018);
	int maneuver = (int)(sin(progress * 12.56637061436) * scale * 0.014);
	int left_y = center_y + arc + maneuver;
	int right_y = center_y - arc - maneuver;
	Uint8 alpha = (Uint8)(150 * fade);
	Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);

	draw_ship_role(surface, left_x, left_y, SHIP_SMALL, window.seed >> 4,
		length, 1, 0, opacity);
	draw_ship_role(surface, right_x, right_y, SHIP_SMALL, window.seed >> 11,
		length, -1, 1, opacity);

	Uint32 volley_phase = phase % 2600;
	if(volley_phase < 300 && progress > 0.08 && progress < 0.94) {
		int bolt_length = scale / 18;
		if(bolt_length < 12) bolt_length = 12;
		Uint8 bolt_alpha = (Uint8)(alpha * 0.8);
		thickLineRGBA(surface, left_x + length / 2, left_y,
			left_x + length / 2 + bolt_length, left_y - size / 3,
			1, 143, 204, 224, bolt_alpha);
		thickLineRGBA(surface, right_x - length / 2, right_y,
			right_x - length / 2 - bolt_length, right_y + size / 3,
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

// One pair of small ships chasing each other in loops, then trading places.
static void render_dogfight_effect(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_DOGFIGHT, ticks, &window)) return;
	render_dogfight(surface, &window, scale, 1, 2, 0.0);
}

// A furball: two or three pairs fighting at once.
static void render_background_battle(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_BACKGROUND_BATTLE, ticks, &window)) return;
	render_dogfight(surface, &window, scale, 2 + (window.seed >> 5) % 2, 3, 0.35);
}

/*
 * Fleet battle
 *
 * Two fleets close, hold and trade fire. Like every effect it is stateless: a
 * ship's place is a function of time, each gun fires on a schedule of its own,
 * and a shot is drawn from how long ago it was fired. The losing fleet loses its
 * ships one at a time in the second half of the battle (the last one survives and
 * retreats); the winners' guns favour a ship that is about to die, so every
 * destruction is preceded by hits.
 */
typedef struct {
	SDL_Surface *surface;
	int scale;
	Uint32 seed;
	double start;               // scene time the battle began
	double duration;
	int winner;                 // the side that does not lose ships
	int size, fighter_size;
	int count[2];
	int center_y;
} Fleet;

// Time between volleys of each weapon (ms); turrets fire half as often again.
static const double weapon_period[WEAPON_COUNT] = {
	1300.0, 3400.0, 2800.0, 1700.0, 2100.0, 3800.0, 3400.0, 6500.0, 4200.0 };

static double fleet_progress(const Fleet *fleet, double t) {
	return (t - fleet->start) / fleet->duration;
}

static void fleet_pose(const Fleet *fleet, int side, int ship, double t, double *x, double *y) {
	double progress = fleet_progress(fleet, t);
	double approach = smooth_step(0.04, 0.31, progress);
	double retreat = smooth_step(0.78, 0.97, progress);
	int width = fleet->surface->w;
	double anchor;
	if(side == 0) {
		double hold = width * (0.37 + 0.008 * sin(t / 3600.0));
		anchor = -fleet->size * 6 + approach * (hold + fleet->size * 6)
			- (fleet->winner == 0 ? 0.0 : retreat * width * 0.62);
	} else {
		double hold = width * (0.63 + 0.008 * sin(t / 3900.0 + 1.7));
		anchor = width + fleet->size * 6 - approach * (width - hold + fleet->size * 6)
			+ (fleet->winner == 1 ? 0.0 : retreat * width * 0.62);
	}
	int direction = side == 0 ? 1 : -1;
	int lane = ship == 0 ? 0 : (ship % 2 ? (ship + 1) / 2 : -(ship / 2));
	double offset = direction * ((ship + 1) / 2) * fleet->size * 3;
	double weave = sin(t / (510.0 + ship * 127) + ship * 1.8 + side * 2.4) * fleet->scale * 0.009;
	double arc = sin(t / 1900.0 + progress * 8.0) * fleet->scale * 0.010;
	*x = anchor + offset + weave;
	*y = fleet->center_y + (side == 0 ? arc : -arc) + lane * fleet->scale / 36.0
		+ sin(t / (370.0 + ship * 83) + ship * 1.3) * fleet->size * 0.75;
}

static int fleet_role(const Fleet *fleet, int side, int ship) {
	unsigned int type = fleet->seed >> ((ship * 3 + side * 7) % 24);
	if(ship == 0 && ((fleet->seed >> (side + 2)) & 1)) return SHIP_BIG;
	if(type % 3 == 0) return SHIP_MID;
	return SHIP_SMALL;
}

static int fleet_length(const Fleet *fleet, int ship, int role) {
	if(role == SHIP_BIG) return fleet->size * 10;
	if(role == SHIP_MID) return fleet->size * 7;
	return (fleet->fighter_size + (ship % 3 == 0 ? 2 : 0)) * 5;
}

static int fleet_design(const Fleet *fleet, int side, int ship) {
	unsigned int variety = (fleet->seed >> 9) + ship * 5 + side * 3;
	return ship_design_index(fleet_role(fleet, side, ship), variety);
}

// Scene time at which a ship of the losing fleet is destroyed; -1 for any other ship.
static double fleet_death(const Fleet *fleet, int side, int ship) {
	if(side == fleet->winner || ship == fleet->count[side] - 1) return -1.0;
	double when = 0.46 + 0.34 * (fx_hash(fleet->seed, side * 16 + ship) % 100) / 100.0;
	return fleet->start + when * fleet->duration;
}

// Draws the shots in the air now from every gun of one ship.
static void fleet_fire(const Fleet *fleet, int side, int ship, double now, double fade) {
	int design_index = fleet_design(fleet, side, ship);
	const ShipDesign *design = &ship_designs[design_index];
	int length = fleet_length(fleet, ship, fleet_role(fleet, side, ship));
	double unit = weapon_unit(length);
	double death = fleet_death(fleet, side, ship);
	int direction = side == 0 ? 1 : -1;
	int enemy = 1 - side;
	for(int gun = 0; gun < 1 + design->turret_count; gun++) {
		int weapon = gun == 0 ? design->weapon : design->turret_weapon;
		const WeaponSpec *spec = &weapon_specs[weapon];
		unsigned int gun_hash = fx_hash(fleet->seed, 1000 + side * 200 + ship * 20 + gun);
		double period = weapon_period[weapon] * (gun == 0 ? 1.0 : 1.5)
			* (0.85 + 0.3 * (gun_hash % 100) / 100.0);
		double offset = (gun_hash >> 8) % (unsigned int)period;
		double volley = (spec->shots - 1) * spec->gap_ms;
		double since = now - fleet->start - offset;
		if(since < 0.0) continue;
		int newest = (int)(since / period);
		double earliest = since - spec->life_ms - volley;
		int oldest = earliest < 0.0 ? 0 : (int)(earliest / period);
		int mount_x = gun == 0 ? design->nose : design->turrets[gun - 1][0];
		int mount_y = gun == 0 ? 0 : design->turrets[gun - 1][1];
		for(int k = oldest; k <= newest; k++) {
			double volley_start = fleet->start + offset + k * period;
			double volley_progress = fleet_progress(fleet, volley_start);
			if(volley_progress < 0.14 || volley_progress > 0.9) continue;
			if(death >= 0.0 && volley_start >= death) continue;
			unsigned int volley_hash = fx_hash(gun_hash, k);
			double lands = volley_start + spec->impact_ms;
			// Prefer a ship that is about to be destroyed.
			int target = -1;
			bool finishing = false;
			for(int j = 0; j < fleet->count[enemy]; j++) {
				double d = fleet_death(fleet, enemy, j);
				if(d > lands && d < lands + 1500.0 && volley_hash % 100 < 70) {
					target = j;
					finishing = true;
					break;
				}
			}
			if(target < 0) {
				int first = (volley_hash >> 8) % fleet->count[enemy];
				for(int tries = 0; tries < fleet->count[enemy]; tries++) {
					int j = (first + tries) % fleet->count[enemy];
					double d = fleet_death(fleet, enemy, j);
					if(d < 0.0 || d > lands) {
						target = j;
						break;
					}
				}
			}
			if(target < 0) continue;
			bool hit = finishing || (volley_hash >> 16) % 100 < 40;
			int target_length = fleet_length(fleet, target, fleet_role(fleet, enemy, target));
			for(int bolt = 0; bolt < spec->shots; bolt++) {
				double fired = volley_start + bolt * spec->gap_ms;
				double age = now - fired;
				if(age < 0.0 || age >= spec->life_ms) continue;
				// A beam stays on its target, so it follows both ships live.
				bool live = weapon == WEAPON_BEAM;
				double sx, sy, tx, ty, mx, my;
				fleet_pose(fleet, side, ship, live ? now : fired, &sx, &sy);
				fleet_pose(fleet, enemy, target, live ? now : fired + spec->impact_ms, &tx, &ty);
				ship_mount_point(design_index, (int)sx, (int)sy, length, direction, 0.0,
					mount_x, mount_y, &mx, &my);
				double dx = tx - mx, dy = ty - my;
				double span = sqrt(dx * dx + dy * dy) + 1.0;
				if(hit) {
					// Hits spread over the hull.
					ty += ((bolt + k) % 3 - 1) * target_length * 0.12;
				} else {
					double side_offset = ((volley_hash >> 4) % 2 ? 1.0 : -1.0)
						* target_length * (0.5 + 0.25 * (bolt % 3));
					tx += -dy / span * side_offset;
					ty += dx / span * side_offset;
				}
				draw_weapon_shot(fleet->surface, weapon, bolt, mx, my, tx, ty, age, unit,
					side, fade, hit ? 1 : 0, fx_hash(volley_hash, bolt));
			}
		}
	}
}

static void render_fleet_battle(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_FLEET_BATTLE, ticks, &window)) return;
	Fleet fleet;
	fleet.surface = surface;
	fleet.scale = scale;
	fleet.seed = window.seed;
	fleet.start = (double)ticks - window.phase;
	fleet.duration = window.duration;
	fleet.winner = (window.seed ^ (window.seed >> 13) ^ (window.seed >> 23)) & 1;
	fleet.size = scale / 72;
	if(fleet.size < 10) fleet.size = 10;
	fleet.fighter_size = fleet.size / 2;
	if(fleet.fighter_size < 7) fleet.fighter_size = 7;
	fleet.count[0] = 3 + (window.seed >> 4) % 4;
	fleet.count[1] = 3 + (window.seed >> 12) % 4;
	fleet.center_y = surface->h * 12 / 100;
	Uint32 seed = window.seed;
	double progress = window.progress;
	double fade = window.fade;
	double now = ticks;
	Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);

	// The ships. One that is about to be destroyed burns and trails smoke.
	for(int side = 0; side < 2; side++) {
		int direction = side == 0 ? 1 : -1;
		for(int ship = 0; ship < fleet.count[side]; ship++) {
			double death = fleet_death(&fleet, side, ship);
			if(death >= 0.0 && now >= death) continue;
			double x, y;
			fleet_pose(&fleet, side, ship, now, &x, &y);
			int role = fleet_role(&fleet, side, ship);
			int length = fleet_length(&fleet, ship, role);
			if(death >= 0.0 && now > death - 3500.0) {
				for(int puff = 1; puff <= 9; puff++) {
					double px, py;
					fleet_pose(&fleet, side, ship, now - puff * 110.0, &px, &py);
					filledCircleRGBA(surface, (int)(px - direction * length * 0.3), (int)py,
						2 + puff / 2 + length / 30, 150, 150, 160,
						weapon_alpha(85.0 * fade * (1.0 - puff / 10.0)));
				}
			}
			draw_ship_design(surface, (int)floor(x + 0.5), (int)floor(y + 0.5),
				fleet_design(&fleet, side, ship), length, direction, side, opacity);
			if(death >= 0.0 && now > death - 3500.0) {
				weapon_glow(surface, x - direction * length * 0.15, y - length * 0.04,
					length * 0.09, weapon_team_colors[1], fade * (110.0 + 90.0 * sin(now / 55.0 + ship)));
			}
		}
	}

	double reinforcement = smooth_step(0.36, 0.45, progress)
		* (1.0 - smooth_step(0.70, 0.79, progress));
	if(reinforcement > 0.0) {
		int reserves = 1 + (seed >> 20) % 3;
		for(int side = 0; side < 2; side++) {
			int direction = side == 0 ? 1 : -1;
			double anchor_x, anchor_y;
			fleet_pose(&fleet, side, 0, now, &anchor_x, &anchor_y);
			for(int ship = 0; ship < reserves; ship++) {
				int x = (int)anchor_x - direction * (int)(surface->w * 0.16 * (1.0 - reinforcement))
					- direction * ship * fleet.fighter_size * 3;
				int y = (int)anchor_y + (ship - reserves / 2) * scale / 25;
				draw_ship_role(surface, x, y, SHIP_SMALL, (seed >> 13) + ship * 5 + side * 3,
					fleet.fighter_size * 5, direction, side,
					(Uint8)(SHIP_OPACITY * 0.85 * fade * reinforcement));
			}
		}
	}

	// Weapons fire, and the doomed ships blow up.
	for(int side = 0; side < 2; side++) {
		for(int ship = 0; ship < fleet.count[side]; ship++) {
			fleet_fire(&fleet, side, ship, now, fade);
		}
	}
	for(int side = 0; side < 2; side++) {
		for(int ship = 0; ship < fleet.count[side]; ship++) {
			double death = fleet_death(&fleet, side, ship);
			if(death < 0.0 || now < death || now - death >= 1700.0) continue;
			double x, y;
			fleet_pose(&fleet, side, ship, death, &x, &y);
			int length = fleet_length(&fleet, ship, fleet_role(&fleet, side, ship));
			draw_explosion(surface, x, y, now - death, length * 0.55 + 6.0, true,
				fx_hash(seed, 300 + side * 16 + ship), side, fade);
		}
	}
}

// Screen row of ridge `layer` at pixel column x.
static double ground_y(SDL_Surface *surface, int layer, double x) {
	double nx = x / surface->w;
	if(nx < 0.0) nx = 0.0;
	if(nx > 0.999) nx = 0.999;
	return surface->h * ridge_height(layer, nx);
}

/*
 * The garden: plants grow on the near ridge, one after another. There are five
 * kinds (round trees, pines, cypresses, flowering bushes and giant glowing
 * mushrooms) in one of three colour themes (lush, autumn, blossom), with grass
 * sprouting between them. Saplings shoot up, canopies fill out, trees sway in
 * the wind, and late in the event the bushes flower and the trees bear fruit.
 */
static void draw_hill_garden(SDL_Surface *surface, Uint32 ticks, int scale, Uint32 seed,
	double progress, double fade) {
	static const Uint8 themes[3][3][3] = {
		{ { 44, 84, 60 }, { 96, 154, 88 }, { 245, 238, 200 } },     // lush
		{ { 118, 58, 30 }, { 214, 130, 52 }, { 232, 196, 76 } },    // autumn
		{ { 58, 96, 72 }, { 116, 170, 116 }, { 248, 152, 196 } }    // blossom
	};
	const Uint8 *dark = themes[seed % 3][0];
	const Uint8 *light = themes[seed % 3][1];
	const Uint8 *flower = themes[seed % 3][2];
	int width = surface->w;
	Uint8 alpha = (Uint8)(215 * fade);
	scale = scale * 17 / 10;     // plants are drawn larger than the other hill things

	// Grass first, so the plants stand in it.
	for(int blade = 0; blade < 96; blade++) {
		unsigned int h = fx_hash(seed, 500 + blade);
		double x = width * (blade + 0.5) / 96.0 + (int)(h % 9) - 4;
		double sprout = smooth_step(0.02, 0.3, progress - (h % 11) * 0.012);
		double length = scale * (0.010 + (h >> 8) % 8 * 0.002) * sprout;
		if(length < 1.0) continue;
		double sway = sin(ticks / 650.0 + blade * 0.8) * length * 0.35;
		double base = ground_y(surface, RIDGE_NEAR, x) + 2.0;
		thickLineRGBA(surface, (int)x, (int)base, (int)(x + sway), (int)(base - length), 1,
			(Uint8)(dark[0] + 20), (Uint8)(dark[1] + 24), (Uint8)(dark[2] + 14),
			(Uint8)(alpha * 0.8));
	}

	const int plants = 11;
	for(int plant = 0; plant < plants; plant++) {
		unsigned int h = fx_hash(seed, plant);
		double r = (h % 100) / 100.0;
		double x = width * (0.05 + 0.9 * plant / (plants - 1.0)) + (int)((h >> 8) % 36) - 18;
		double start = 0.03 + plant * 0.028;
		double growth = smooth_step(start, start + 0.45, progress);
		double canopy = smooth_step(start + 0.28, start + 0.65, progress);
		if(growth <= 0.0) continue;
		double base = ground_y(surface, RIDGE_NEAR, x) + 2.0;
		double sway = sin(ticks / 900.0 + plant * 1.3) * scale * 0.004 * canopy;
		// A fixed mix of the five kinds, rotated by the seed, so every garden has some of each.
		static const int species_plan[11] = { 0, 1, 3, 0, 2, 1, 4, 0, 3, 1, 2 };
		int species = species_plan[(plant + seed) % 11];
		int bx = (int)x, by = (int)base;

		if(species == 0) {
			// Round tree: a trunk and a crown of overlapping blobs, lit from above.
			double th = scale * (0.030 + 0.014 * r) * growth;
			int tx = (int)(x + sway * 0.3), ty = (int)(base - th);
			thickLineRGBA(surface, bx, by, tx, ty, 2 + (int)(growth * 2), 54, 40, 34, alpha);
			double crown = scale * (0.022 + 0.008 * r) * canopy;
			for(int blob = 0; blob < 5; blob++) {
				double angle = blob * 1.2566 + r * 3.0;
				int cx = (int)(x + sway + cos(angle) * crown * 0.75);
				int cy = (int)(base - th - crown * 0.4 + sin(angle) * crown * 0.5);
				filledCircleRGBA(surface, cx, cy, (int)(crown * 0.62) + 1,
					dark[0], dark[1], dark[2], alpha);
			}
			for(int blob = 0; blob < 3; blob++) {
				double angle = blob * 2.1 + r * 3.0 + 0.6;
				int cx = (int)(x + sway + cos(angle) * crown * 0.5 - crown * 0.15);
				int cy = (int)(base - th - crown * 0.75 + sin(angle) * crown * 0.3);
				filledCircleRGBA(surface, cx, cy, (int)(crown * 0.4) + 1,
					light[0], light[1], light[2], (Uint8)(alpha * 0.85));
			}
			double fruit = smooth_step(0.72, 0.9, progress);
			if(fruit > 0.0 && canopy > 0.9) {
				for(int f = 0; f < 5; f++) {
					double angle = f * 1.7 + r * 5.0;
					int fx = (int)(x + sway + cos(angle) * crown * 0.8);
					int fy = (int)(base - th - crown * 0.4 + sin(angle) * crown * 0.5);
					filledCircleRGBA(surface, fx, fy, 2, flower[0], flower[1], flower[2],
						(Uint8)(alpha * fruit));
				}
			}
		} else if(species == 1) {
			// Pine: stacked tiers that widen toward the ground.
			double th = scale * (0.045 + 0.016 * r) * growth;
			thickLineRGBA(surface, bx, by, bx, (int)(base - th * 0.25), 2, 54, 40, 34, alpha);
			for(int tier = 0; tier < 4; tier++) {
				double tier_base = base - th * (0.12 + tier * 0.2);
				double tier_width = scale * 0.017 * (1.0 - tier * 0.2) * (0.3 + 0.7 * canopy);
				double tier_top = tier_base - th * 0.3;
				int shift = (int)(sway * (tier + 1) * 0.35);
				filledTrigonRGBA(surface, (int)(x - tier_width) + shift / 2, (int)tier_base,
					(int)(x + tier_width) + shift / 2, (int)tier_base, bx + shift, (int)tier_top,
					dark[0], dark[1], dark[2], alpha);
				filledTrigonRGBA(surface, (int)(x - tier_width * 0.5) + shift / 2, (int)tier_base - 1,
					bx + shift / 2, (int)tier_base - 1, bx + shift, (int)tier_top,
					light[0], light[1], light[2], (Uint8)(alpha * 0.6));
			}
		} else if(species == 2) {
			// Cypress: a tall slim flame of leaves.
			double ch = scale * (0.05 + 0.018 * r) * growth;
			double cw = scale * 0.0085 * (0.4 + 0.6 * canopy);
			filledEllipseRGBA(surface, (int)(x + sway * 0.6), (int)(base - ch * 0.5), (int)cw + 1,
				(int)(ch * 0.5) + 1, dark[0], dark[1], dark[2], alpha);
			filledEllipseRGBA(surface, (int)(x + sway * 0.6 - cw * 0.3), (int)(base - ch * 0.55),
				(int)(cw * 0.45) + 1, (int)(ch * 0.4) + 1, light[0], light[1], light[2],
				(Uint8)(alpha * 0.7));
		} else if(species == 3) {
			// Bush that comes into flower late in the event.
			double bush = scale * 0.018 * growth;
			for(int blob = 0; blob < 3; blob++) {
				filledCircleRGBA(surface, (int)(x + (blob - 1) * bush * 0.8 + sway * 0.4),
					(int)(base - bush * 0.6), (int)(bush * (blob == 1 ? 0.8 : 0.6)) + 1,
					dark[0], dark[1], dark[2], alpha);
			}
			double bloom = smooth_step(0.5 + start * 0.4, 0.78 + start * 0.4, progress);
			for(int f = 0; f < 9; f++) {
				unsigned int fh = fx_hash(h, f);
				int fx = (int)(x + ((int)(fh % 100) - 50) * bush * 0.022 + sway * 0.4);
				int fy = (int)(base - bush * (0.4 + (fh >> 8) % 60 / 100.0));
				if((fh >> 16) % 100 > bloom * 100.0) continue;
				filledCircleRGBA(surface, fx, fy, 2, flower[0], flower[1], flower[2], alpha);
				pixelRGBA(surface, fx, fy, 255, 255, 235, alpha);
			}
		} else {
			// Giant mushroom with a softly glowing cap.
			double mh = scale * (0.026 + 0.010 * r) * growth;
			double cap = scale * 0.016 * (0.3 + 0.7 * canopy);
			boxRGBA(surface, bx - 2, (int)(base - mh), bx + 2, by, 150, 142, 130, alpha);
			double pulse = 0.65 + 0.35 * sin(ticks / 420.0 + plant);
			filledCircleRGBA(surface, bx, (int)(base - mh), (int)(cap * 1.8) + 1,
				light[0], light[1], light[2], (Uint8)(26 * fade * pulse * canopy));
			filledEllipseRGBA(surface, bx, (int)(base - mh), (int)cap + 1, (int)(cap * 0.7) + 1,
				dark[0] + 40, dark[1] + 10, dark[2] + 20, alpha);
			boxRGBA(surface, bx - (int)cap - 1, (int)(base - mh), bx + (int)cap + 1,
				(int)(base - mh) + 2, 30, 24, 30, alpha);
			for(int spot = 0; spot < 3; spot++) {
				filledCircleRGBA(surface, bx + (int)((spot - 1) * cap * 0.55),
					(int)(base - mh - cap * 0.3 - (spot == 1 ? cap * 0.15 : 0)), 1 + (int)(cap * 0.12),
					190, 255, 230, (Uint8)(235 * fade * pulse * canopy));
			}
		}
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
			draw_hill_garden(surface, ticks, scale, seed, progress, fade);
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
 * Hill events
 *
 * Things that happen on the hills. Each is a small function taking a HillEvent.
 * They stand on the near ridge (the one below the clock cards, so they are
 * seen), except the volcano, which sits on the middle ridge at one edge of the
 * screen. `t` is seconds since the event began, `progress` runs 0..1 over it,
 * and `fade` fades the whole thing in and out. To add one: write hill_xxx(),
 * append it to hill_event_kinds[] and bump HILL_EVENT_KINDS.
 */
typedef struct {
	SDL_Surface *surface;
	Uint32 ticks;
	int scale;
	Uint32 seed;
	double t;
	double progress;
	double fade;
	int direction;         // 1: things go left to right, -1: right to left
} HillEvent;

// A repeatable random number in 0..1 for the event.
static double hill_random(const HillEvent *event, int a, int b) {
	return (fx_hash(event->seed + a * 7919u, b) % 1000) / 1000.0;
}

// A small walking figure with a lantern on a pole.
static void hill_walker(SDL_Surface *surface, double x, double ground, double height,
	int direction, double step, bool lantern, double fade, double t, int index) {
	Uint8 alpha = weapon_alpha(235.0 * fade);
	double hip = ground - height * 0.45;
	double swing = sin(step) * height * 0.22;
	int width = height > 22 ? 3 : 2;
	thickLineRGBA(surface, (int)x, (int)hip, (int)(x + swing), (int)ground, width, 12, 15, 22, alpha);
	thickLineRGBA(surface, (int)x, (int)hip, (int)(x - swing), (int)ground, width, 12, 15, 22, alpha);
	thickLineRGBA(surface, (int)x, (int)hip, (int)(x + direction * height * 0.05),
		(int)(ground - height * 0.8), width + 1, 14, 18, 26, alpha);
	filledCircleRGBA(surface, (int)(x + direction * height * 0.07), (int)(ground - height * 0.92),
		(int)(height * 0.1) + 1, 14, 18, 26, alpha);
	if(lantern) {
		double hand_x = x + direction * height * 0.2, hand_y = ground - height * 0.58;
		double lamp_x = x + direction * height * 0.42, lamp_y = ground - height * 0.5;
		thickLineRGBA(surface, (int)hand_x, (int)hand_y, (int)lamp_x, (int)(lamp_y - height * 0.1), 1,
			12, 15, 22, alpha);
		static const Uint8 warm[3] = { 255, 190, 100 };
		weapon_glow(surface, lamp_x, lamp_y, height * 0.11 * (0.9 + 0.1 * sin(t * 9.0 + index)),
			warm, 235.0 * fade);
		ellipseRGBA(surface, (int)lamp_x, (int)ground, (int)(height * 0.8), (int)(height * 0.09),
			255, 190, 100, weapon_alpha(40.0 * fade));
	}
}

// A village of small houses: windows light up one by one, chimneys smoke, and a
// church tower stands in the middle.
static void hill_village(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int houses = 8;
	for(int house = 0; house < houses; house++) {
		double r = hill_random(event, house, 1), r2 = hill_random(event, house, 2);
		double x = surface->w * (0.06 + 0.88 * (house + 0.5 + (r - 0.5) * 0.5) / houses);
		double base = ground_y(surface, RIDGE_NEAR, x) + event->scale * 0.004;
		double width = event->scale * (0.04 + 0.016 * r2);
		double height = width * (0.75 + 0.5 * r);
		bool tower = house == 3;
		if(tower) { width *= 0.7; height *= 2.4; }
		double arrive = smooth_step(0.0, 0.1, event->progress - house * 0.012);
		if(arrive <= 0.0) continue;
		Uint8 alpha = weapon_alpha(235.0 * event->fade * arrive);
		int x0 = (int)(x - width / 2), x1 = (int)(x + width / 2);
		int y1 = (int)base, y0 = (int)(base - height);
		boxRGBA(surface, x0, y0, x1, y1, 16, 20, 30, alpha);
		int roof = (int)(width * (tower ? 1.2 : 0.55));
		filledTrigonRGBA(surface, x0 - 2, y0, x1 + 2, y0, (int)x, y0 - roof, 10, 12, 20, alpha);
		int windows = tower ? 2 : 2 + (int)(r * 2.0);
		for(int k = 0; k < windows; k++) {
			double lit = smooth_step(0.15 + house * 0.045 + k * 0.03, 0.2 + house * 0.045 + k * 0.03,
				event->progress) * (1.0 - smooth_step(0.9, 0.97, event->progress));
			if(lit <= 0.0) continue;
			double flicker = 0.88 + 0.12 * sin(event->t * 7.0 + house * 3.1 + k);
			int wx = x0 + (int)(width * (k + 0.5) / windows) - 1;
			int wy = y0 + (int)(height * (0.3 + 0.25 * (k % 2)));
			filledCircleRGBA(surface, wx + 1, wy + 2, 6, 255, 190, 100,
				weapon_alpha(32.0 * lit * event->fade));
			boxRGBA(surface, wx, wy, wx + 2, wy + 3, 255, 214, 130,
				weapon_alpha(240.0 * lit * flicker * event->fade));
		}
		if(!tower && house % 2 == 0) {
			int chimney_x = x1 - 4, chimney_y = y0 - roof / 2 - 3;
			boxRGBA(surface, chimney_x, chimney_y, chimney_x + 2, chimney_y + 5, 12, 15, 24, alpha);
			for(int puff = 0; puff < 5; puff++) {
				double age = fmod(event->t / 2.2 + puff * 0.2 + house * 0.13, 1.0);
				filledCircleRGBA(surface, (int)(chimney_x + sin(age * 5.0 + house) * 3.0 + age * 8.0),
					(int)(chimney_y - age * event->scale * 0.07), (int)(1.5 + age * 3.5),
					150, 152, 162, weapon_alpha(85.0 * (1.0 - age) * event->fade));
			}
		}
	}
}

// A line of travellers with lanterns walks along the ridge.
static void hill_caravan(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int walkers = 7;
	double spacing = event->scale * 0.05;
	double margin = event->scale * 0.1;
	double travel = surface->w + 2.0 * margin + walkers * spacing;
	double lead = event->progress * travel - margin;
	for(int walker = 0; walker < walkers; walker++) {
		double progress_x = lead - walker * spacing;
		double x = event->direction > 0 ? progress_x : surface->w - progress_x;
		if(x < -20 || x > surface->w + 20) continue;
		double ground = ground_y(surface, RIDGE_NEAR, x) + 1.0;
		double height = event->scale * (walker == 3 ? 0.046 : 0.04);
		if(walker == walkers - 1) height *= 0.7;
		hill_walker(surface, x, ground, height, event->direction,
			event->t * 6.0 + walker * 1.9, walker % 2 == 0, event->fade, event->t, walker);
	}
}

// Wind turbines rise on the ridge and begin to turn, with red lights blinking on top.
static void hill_turbines(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int turbines = 5;
	Uint8 alpha = weapon_alpha(235.0 * event->fade);
	for(int i = 0; i < turbines; i++) {
		double x = event->surface->w * (0.07 + 0.86 * i / (turbines - 1.0));
		double rise = smooth_step(0.02 + i * 0.03, 0.14 + i * 0.03, event->progress);
		if(rise <= 0.0) continue;
		double base = ground_y(surface, RIDGE_NEAR, x) + 2.0;
		double tower = event->scale * (0.1 + 0.016 * (i % 2)) * rise;
		int top_x = (int)x, top_y = (int)(base - tower);
		thickLineRGBA(surface, (int)x, (int)base, top_x, top_y, 3, 20, 26, 38, alpha);
		thickLineRGBA(surface, (int)x + 1, (int)base, top_x + 1, top_y, 1, 52, 62, 82, (Uint8)(alpha * 0.8));
		double blade = tower * 0.5;
		double spin = event->t * (1.1 + 0.18 * i) * (0.4 + 0.6 * rise) + i;
		for(int b = 0; b < 3; b++) {
			double angle = spin + b * 2.0944;
			int tip_x = top_x + (int)(cos(angle) * blade), tip_y = top_y + (int)(sin(angle) * blade);
			thickLineRGBA(surface, top_x, top_y, tip_x, tip_y, 3, 28, 34, 48, alpha);
			thickLineRGBA(surface, top_x, top_y, tip_x, tip_y, 1, 90, 104, 128, (Uint8)(alpha * 0.8));
		}
		filledCircleRGBA(surface, top_x, top_y, 3, 40, 46, 62, alpha);
		if(fmod(event->t + i * 0.37, 1.6) < 0.35 && rise > 0.9) {
			static const Uint8 red[3] = { 255, 60, 50 };
			weapon_glow(surface, top_x, top_y - 4, 3.0, red, 235.0 * event->fade);
		}
	}
}

// Paper lanterns float up from the hills, swaying as they climb and fading near the top.
static void hill_lanterns(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int lanterns = 28;
	for(int i = 0; i < lanterns; i++) {
		double rx = hill_random(event, i, 1), ph = hill_random(event, i, 2), rs = hill_random(event, i, 3);
		double cycle = 13.0 + rs * 8.0;
		double u = fmod(event->t / cycle + ph, 1.0);
		double x0 = surface->w * (0.03 + 0.94 * rx);
		double x = x0 + sin(event->t * 0.8 + i) * event->scale * 0.012 + u * event->scale * 0.06 * event->direction;
		double y = ground_y(surface, RIDGE_NEAR, x0) - u * surface->h * 0.62;
		double envelope = smooth_step(0.0, 0.1, u) * (1.0 - smooth_step(0.7, 1.0, u));
		double size = event->scale * (0.013 + 0.006 * rs);
		double flicker = 0.85 + 0.15 * sin(event->t * 8.0 + i * 2.0);
		Uint8 a = weapon_alpha(255.0 * envelope * event->fade);
		filledCircleRGBA(surface, (int)x, (int)y, (int)(size * 3.4), 255, 140, 60,
			weapon_alpha(22.0 * envelope * event->fade * flicker));
		filledCircleRGBA(surface, (int)x, (int)y, (int)(size * 2.2), 255, 160, 70,
			weapon_alpha(40.0 * envelope * event->fade * flicker));
		boxRGBA(surface, (int)(x - size * 0.5), (int)(y - size * 0.65), (int)(x + size * 0.5),
			(int)(y + size * 0.65), 255, (Uint8)(150 + 40 * flicker), 70, a);
		boxRGBA(surface, (int)(x - size * 0.3), (int)(y - size * 0.4), (int)(x + size * 0.3),
			(int)(y + size * 0.4), 255, 224, 150, weapon_alpha(220.0 * envelope * event->fade * flicker));
		lineRGBA(surface, (int)(x - size * 0.5), (int)(y - size * 0.65), (int)(x + size * 0.5),
			(int)(y - size * 0.65), 90, 40, 20, a);
		lineRGBA(surface, (int)(x - size * 0.5), (int)(y + size * 0.65), (int)(x + size * 0.5),
			(int)(y + size * 0.65), 90, 40, 20, a);
	}
}

// A tent and a campfire at the edge of the screen; two figures sit by the fire and
// sparks and smoke rise from it.
static void hill_campfire(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double x = event->direction > 0 ? surface->w * 0.075 : surface->w * 0.925;
	double ground = ground_y(surface, RIDGE_NEAR, x) + 2.0;
	double s = event->scale * 1.4;
	double build = smooth_step(0.0, 0.1, event->progress)
		* (1.0 - smooth_step(0.9, 1.0, event->progress));
	Uint8 alpha = weapon_alpha(235.0 * event->fade);
	// The tent stands on the far side of the fire.
	double tent_x = x - event->direction * s * 0.06;
	double tent_width = s * 0.035;
	filledTrigonRGBA(surface, (int)(tent_x - tent_width), (int)ground, (int)(tent_x + tent_width),
		(int)ground, (int)tent_x, (int)(ground - s * 0.045), 22, 28, 40, alpha);
	filledTrigonRGBA(surface, (int)tent_x, (int)ground, (int)(tent_x + event->direction * s * 0.012),
		(int)ground, (int)tent_x, (int)(ground - s * 0.03), 8, 10, 16, alpha);
	// The glow of the fire on the ground and the tent.
	filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.09), (int)(s * 0.014),
		255, 150, 60, weapon_alpha(34.0 * event->fade * build));
	double flare = 0.85 + 0.15 * sin(event->t * 11.0) + 0.1 * sin(event->t * 17.0);
	filledCircleRGBA(surface, (int)x, (int)(ground - s * 0.015), (int)(s * 0.05 * flare), 255, 140, 50,
		weapon_alpha(26.0 * event->fade * build));
	thickLineRGBA(surface, (int)(x - s * 0.012), (int)ground, (int)(x + s * 0.012), (int)(ground - s * 0.004),
		3, 44, 30, 24, alpha);
	thickLineRGBA(surface, (int)(x + s * 0.012), (int)ground, (int)(x - s * 0.012), (int)(ground - s * 0.004),
		3, 54, 36, 26, alpha);
	for(int tongue = 0; tongue < 5; tongue++) {
		double sway = sin(event->t * (9.0 + tongue * 2.0) + tongue) * s * 0.003;
		double h = s * (0.018 + 0.006 * (tongue % 3)) * build * (0.75 + 0.25 * sin(event->t * 13.0 + tongue * 2.3));
		double w = s * 0.007;
		double fx = x + (tongue - 2) * s * 0.004;
		filledTrigonRGBA(surface, (int)(fx - w), (int)(ground - 2), (int)(fx + w), (int)(ground - 2),
			(int)(fx + sway), (int)(ground - h), 250, 120, 40, weapon_alpha(215.0 * event->fade));
		filledTrigonRGBA(surface, (int)(fx - w * 0.55), (int)(ground - 2), (int)(fx + w * 0.55), (int)(ground - 2),
			(int)(fx + sway), (int)(ground - h * 0.75), 255, 205, 110, weapon_alpha(225.0 * event->fade));
	}
	for(int spark = 0; spark < 12; spark++) {
		double u = fmod(event->t / 1.7 + spark * 0.083, 1.0);
		double sx = x + sin(spark * 7.0 + event->t * 2.5) * s * 0.014 * u + event->direction * u * s * 0.01;
		double sy = ground - s * 0.02 - u * s * 0.13;
		filledCircleRGBA(surface, (int)sx, (int)sy, spark % 3 == 0 ? 2 : 1, 255, (Uint8)(210 - 90 * u), 100,
			weapon_alpha(240.0 * (1.0 - u) * event->fade * build));
	}
	for(int puff = 0; puff < 6; puff++) {
		double u = fmod(event->t / 4.5 + puff / 6.0, 1.0);
		filledCircleRGBA(surface, (int)(x + sin(u * 4.0 + puff) * s * 0.01 + u * s * 0.03 * event->direction),
			(int)(ground - s * 0.03 - u * s * 0.17), (int)(s * (0.006 + 0.014 * u)), 120, 120, 134,
			weapon_alpha(70.0 * (1.0 - u) * event->fade * build));
	}
	// Two figures sit by the fire, facing it.
	for(int person = 0; person < 2; person++) {
		double px = x + (person == 0 ? -s * 0.028 : s * 0.034);
		int facing = px < x ? 1 : -1;
		filledEllipseRGBA(surface, (int)px, (int)(ground - s * 0.008), (int)(s * 0.01), (int)(s * 0.009),
			14, 18, 26, alpha);
		filledCircleRGBA(surface, (int)(px + facing * s * 0.002), (int)(ground - s * 0.024),
			(int)(s * 0.005) + 1, 14, 18, 26, alpha);
		filledCircleRGBA(surface, (int)(px + facing * s * 0.005), (int)(ground - s * 0.022), 1,
			255, 170, 90, weapon_alpha(200.0 * event->fade * build));
	}
}

// Glowing plants along the ridge: waves of light pass through them, with a few
// that are much bigger than the rest.
static void hill_bloom(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int plants = 34;
	static const Uint8 colors[3][3] = { { 90, 235, 215 }, { 175, 125, 255 }, { 255, 125, 205 } };
	for(int i = 0; i < plants; i++) {
		double r = hill_random(event, i, 1);
		double x = event->surface->w * (0.02 + 0.96 * (i + 0.5 + (r - 0.5) * 0.6) / plants);
		double grow = smooth_step(0.0 + i * 0.004, 0.18 + i * 0.004, event->progress)
			* (1.0 - smooth_step(0.93, 1.0, event->progress));
		if(grow <= 0.0) continue;
		double base = ground_y(surface, RIDGE_NEAR, x) + 2.0;
		double tall = event->scale * (0.018 + 0.03 * r * r) * grow;
		double wave = 0.5 + 0.5 * sin(x * 0.018 * event->direction - event->t * 3.3);
		double lean = sin(event->t * 0.9 + i) * tall * 0.12;
		const Uint8 *c = colors[i % 3];
		int top_x = (int)(x + lean), top_y = (int)(base - tall);
		filledEllipseRGBA(surface, (int)x, (int)base, (int)(tall * 0.7) + 2, 2, c[0], c[1], c[2],
			weapon_alpha(40.0 * wave * event->fade * grow));
		thickLineRGBA(surface, (int)x, (int)base, top_x, top_y, 1, 30, 70, 64, weapon_alpha(220.0 * event->fade));
		thickLineRGBA(surface, (int)x, (int)(base - tall * 0.4), (int)(x - tall * 0.25), (int)(base - tall * 0.55), 1,
			30, 70, 64, weapon_alpha(200.0 * event->fade));
		double bulb = (2.0 + tall * 0.12) * (0.8 + 0.4 * wave);
		weapon_glow(surface, top_x, top_y, bulb, c, (110.0 + 140.0 * wave) * event->fade * grow);
		if(r > 0.7) {
			// The big ones unfold a ring of petals.
			for(int petal = 0; petal < 6; petal++) {
				double angle = petal * 1.0472 + event->t * 0.2;
				lineRGBA(surface, top_x, top_y, (int)(top_x + cos(angle) * bulb * 3.2),
					(int)(top_y + sin(angle) * bulb * 2.0), c[0], c[1], c[2],
					weapon_alpha((90.0 + 120.0 * wave) * event->fade * grow));
			}
		}
	}
}

// A herd of four-legged animals walks along the ridge, a calf at the back.
static void hill_herd(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int animals = 6;
	double spacing = event->scale * 0.115;
	double margin = event->scale * 0.1;
	double travel = surface->w + 2.0 * margin + animals * spacing;
	double lead = event->progress * travel - margin;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	for(int animal = 0; animal < animals; animal++) {
		double progress_x = lead - animal * spacing - hill_random(event, animal, 1) * spacing * 0.4;
		double x = event->direction > 0 ? progress_x : surface->w - progress_x;
		if(x < -40 || x > surface->w + 40) continue;
		double size = event->scale * (animal == animals - 1 ? 0.03 : 0.052);
		double ground = ground_y(surface, RIDGE_NEAR, x) + 1.0;
		int d = event->direction;
		double gait = event->t * 3.4 + animal * 0.9;
		double bob = fabs(sin(gait)) * size * 0.04;
		double body_y = ground - size * 0.7 - bob;
		// Far legs are a shade lighter than the body, so the legs read as separate.
		for(int leg = 0; leg < 4; leg++) {
			double hip_x = x + (leg < 2 ? 0.38 : -0.38) * size;
			double swing = sin(gait + leg * 1.6) * size * 0.2;
			bool far_leg = leg % 2 == 1;
			thickLineRGBA(surface, (int)hip_x, (int)body_y, (int)(hip_x + swing * d), (int)ground,
				far_leg ? 2 : 3, far_leg ? 22 : 12, far_leg ? 26 : 15, far_leg ? 36 : 22, alpha);
		}
		filledEllipseRGBA(surface, (int)x, (int)body_y, (int)(size * 0.55), (int)(size * 0.26), 12, 15, 22, alpha);
		double neck_x = x + d * size * 0.45, neck_y = body_y - size * 0.1;
		double head_x = x + d * size * 0.78, head_y = body_y - size * 0.5 + sin(gait * 0.5) * size * 0.04;
		thickLineRGBA(surface, (int)neck_x, (int)neck_y, (int)head_x, (int)head_y, (int)(size * 0.2) + 1,
			12, 15, 22, alpha);
		filledEllipseRGBA(surface, (int)(head_x + d * size * 0.08), (int)head_y, (int)(size * 0.13) + 1,
			(int)(size * 0.08) + 1, 12, 15, 22, alpha);
		thickLineRGBA(surface, (int)(x - d * size * 0.5), (int)(body_y - size * 0.08),
			(int)(x - d * size * 0.62), (int)(body_y + size * 0.05), 2, 12, 15, 22, alpha);
		if(animal % 2 == 0 && animal != animals - 1) {
			// Antlers: a main beam with two tines.
			int ax = (int)head_x, ay = (int)(head_y - size * 0.05);
			int tx = (int)(head_x - d * size * 0.1), ty = (int)(head_y - size * 0.5);
			thickLineRGBA(surface, ax, ay, tx, ty, 1, 30, 32, 40, alpha);
			thickLineRGBA(surface, (int)((ax + tx) / 2), (int)((ay + ty) / 2), (int)(tx + d * size * 0.2),
				(int)(ty + size * 0.1), 1, 30, 32, 40, alpha);
			thickLineRGBA(surface, tx, ty, (int)(tx + d * size * 0.12), (int)(ty - size * 0.14), 1, 30, 32, 40, alpha);
		}
	}
}

// A volcano on the middle ridge at one edge of the screen. It grows, its crater begins
// to glow, and it erupts: lava is thrown in arcs, lava runs down the cone and an ash
// cloud rises and drifts.
static void hill_volcano(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double x = event->direction > 0 ? surface->w * 0.075 : surface->w * 0.925;
	double base = ground_y(surface, RIDGE_NEAR, x) + s * 0.012;
	double rise = smooth_step(0.0, 0.12, event->progress);
	double erupt = smooth_step(0.14, 0.3, event->progress)
		* (1.0 - smooth_step(0.86, 0.98, event->progress))
		* (0.7 + 0.3 * sin(event->t * 0.8));
	double cone_width = s * 0.24, cone_height = s * 0.1 * rise;
	double crater_y = base - cone_height;
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	int dir = event->direction;
	int left = (int)(x - cone_width / 2), right = (int)(x + cone_width / 2);
	int crater_left = (int)(x - cone_width * 0.07), crater_right = (int)(x + cone_width * 0.07);
	// The ash cloud goes behind the cone.
	for(int puff = 0; puff < 14; puff++) {
		double u = fmod(event->t / 7.0 + puff / 14.0, 1.0);
		double px = x + sin(u * 3.0 + puff) * s * 0.02 + dir * u * s * 0.14;
		double py = crater_y - u * surface->h * 0.45;
		double radius = s * (0.018 + 0.055 * u);
		double heat = 1.0 - smooth_step(0.0, 0.35, u);
		filledCircleRGBA(surface, (int)px, (int)py, (int)radius, (Uint8)(44 + 150 * heat), (Uint8)(38 + 50 * heat),
			(Uint8)(48 + 10 * heat), weapon_alpha(110.0 * (1.0 - u) * erupt * event->fade));
	}
	filledTrigonRGBA(surface, left, (int)base + 4, right, (int)base + 4, crater_left, (int)crater_y, 26, 20, 28, alpha);
	filledTrigonRGBA(surface, crater_left, (int)crater_y, crater_right, (int)crater_y, right, (int)base + 4, 26, 20, 28, alpha);
	filledTrigonRGBA(surface, crater_left, (int)crater_y, crater_right, (int)crater_y, (int)x, (int)(crater_y + s * 0.012), 255, 120, 50,
		weapon_alpha(255.0 * erupt * event->fade));
	filledCircleRGBA(surface, (int)x, (int)crater_y, (int)(s * 0.06 * (0.5 + erupt)), 255, 110, 50,
		weapon_alpha(34.0 * erupt * event->fade));
	filledCircleRGBA(surface, (int)x, (int)crater_y, (int)(s * 0.032 * (0.5 + erupt)), 255, 140, 60,
		weapon_alpha(80.0 * erupt * event->fade));
	filledCircleRGBA(surface, (int)x, (int)crater_y, (int)(s * 0.013 * (0.5 + erupt)) + 1, 255, 224, 160,
		weapon_alpha(210.0 * erupt * event->fade));
	// Lava runs down the cone in glowing streams.
	for(int stream = 0; stream < 3; stream++) {
		double side = (stream - 1) * 0.75;
		double length = (0.4 + 0.2 * stream) * erupt;
		double sx = x + side * cone_width * 0.04, sy = crater_y;
		for(int step = 1; step <= 8; step++) {
			double f = step / 8.0 * length;
			double nx = x + side * cone_width * (0.04 + 0.4 * f) + sin(step * 1.3 + stream) * s * 0.004;
			double ny = crater_y + (base - crater_y) * f;
			thickLineRGBA(surface, (int)sx, (int)sy, (int)nx, (int)ny, 2, 255, (Uint8)(130 + 40 * stream), 50,
				weapon_alpha(210.0 * (1.0 - f) * event->fade));
			sx = nx;
			sy = ny;
		}
	}
	// Lava thrown from the crater in arcs.
	for(int blob = 0; blob < 24; blob++) {
		double r = hill_random(event, blob, 1), r2 = hill_random(event, blob, 2);
		double u = fmod(event->t * 0.55 + blob * 0.0417 + r2, 1.0);
		double vx = (r - 0.5) * cone_width * 1.3 + dir * cone_width * 0.1;
		double peak = s * (0.08 + 0.16 * r2);
		double bx = x + vx * u, by = crater_y - peak * 4.0 * u * (1.0 - u);
		int radius = 2 + blob % 3;
		filledCircleRGBA(surface, (int)bx, (int)by, radius + 2, 255, 130, 50,
			weapon_alpha(50.0 * (1.0 - 0.5 * u) * erupt * event->fade));
		filledCircleRGBA(surface, (int)bx, (int)by, radius, 255, (Uint8)(215 - 110 * u), 80,
			weapon_alpha(255.0 * (1.0 - 0.5 * u) * erupt * event->fade));
	}
	if(erupt > 0.05) {
		boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 255, 90, 40, weapon_alpha(9.0 * erupt * event->fade));
	}
}

// A chain of signal towers on the ridge lights up one after another, each sending
// a searchlight beam into the sky that sweeps slowly from side to side.
static void hill_beacons(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int towers = 6;
	double s = event->scale;
	for(int i = 0; i < towers; i++) {
		int order = event->direction > 0 ? i : towers - 1 - i;
		double x = surface->w * (0.07 + 0.86 * i / (towers - 1.0));
		double base = ground_y(surface, RIDGE_NEAR, x) + 2.0;
		double height = s * 0.058;
		int top_x = (int)x, top_y = (int)(base - height);
		double lit = smooth_step(0.08 + order * 0.07, 0.12 + order * 0.07, event->progress)
			* (1.0 - smooth_step(0.88, 0.97, event->progress));
		Uint8 alpha = weapon_alpha(235.0 * event->fade);
		if(lit > 0.0) {
			double aim = -1.5708 + sin(event->t * 0.5 + i * 0.9) * 0.55;
			double half = 0.045;
			double reach = surface->h * 0.95;
			double ax = top_x + cos(aim - half) * reach, ay = top_y + sin(aim - half) * reach;
			double bx = top_x + cos(aim + half) * reach, by = top_y + sin(aim + half) * reach;
			filledTrigonRGBA(surface, top_x, top_y, (int)ax, (int)ay, (int)bx, (int)by,
				235, 228, 190, weapon_alpha(24.0 * lit * event->fade));
			filledTrigonRGBA(surface, top_x, top_y, (int)(top_x + cos(aim - half * 0.4) * reach),
				(int)(top_y + sin(aim - half * 0.4) * reach), (int)(top_x + cos(aim + half * 0.4) * reach),
				(int)(top_y + sin(aim + half * 0.4) * reach), 255, 250, 220,
				weapon_alpha(26.0 * lit * event->fade));
		}
		filledTrigonRGBA(surface, (int)(x - s * 0.008), (int)base, (int)(x + s * 0.008), (int)base,
			top_x, top_y, 20, 24, 36, alpha);
		boxRGBA(surface, top_x - 4, top_y - 3, top_x + 4, top_y, 16, 20, 30, alpha);
		static const Uint8 flame[3] = { 255, 170, 80 };
		weapon_glow(surface, top_x, top_y - 5, s * 0.009 * (0.9 + 0.1 * sin(event->t * 12.0 + i)),
			flame, 235.0 * lit * event->fade);
	}
}

static void (*const hill_event_kinds[HILL_EVENT_KINDS])(const HillEvent *) = {
	hill_village, hill_caravan, hill_turbines, hill_lanterns, hill_campfire,
	hill_bloom, hill_herd, hill_volcano, hill_beacons
};

static void render_hill_event(SDL_Surface *surface, Uint32 ticks, int scale) {
	EffectWindow window;
	if(!effect_window(&EFFECT_HILL_EVENT, ticks, &window)) return;
	HillEvent event;
	event.surface = surface;
	event.ticks = ticks;
	event.scale = scale;
	event.seed = window.seed;
	event.t = window.phase / 1000.0;
	event.progress = window.progress;
	event.fade = window.fade;
	event.direction = (window.seed >> 6) & 1 ? 1 : -1;
	int kind = effect_kind(&EFFECT_HILL_EVENT, (window.seed >> 10) % HILL_EVENT_KINDS);
	hill_event_kinds[kind](&event);
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
	render_dogfight_effect(surface, ticks, scale);
	render_rare_sky_event(surface, ticks, scale);
	render_long_scene(surface, ticks, scale);
	render_hill_event(surface, ticks, scale);

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
		// The streak leans the same way the drop travels.
		int x_end = x - (int)(length * (0.16 + weather.wind * 0.22) + 0.5);
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
	if(ship_backup) SDL_FreeSurface(ship_backup);
	if(date_surface_cache) SDL_FreeSurface(date_surface_cache);

	TTF_CloseFont(font_time);
	TTF_CloseFont(font_mode);
	TTF_CloseFont(font_date);

	return 0;
}
