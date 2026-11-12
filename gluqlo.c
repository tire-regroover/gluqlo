/*
* Gluqlo L-Space: a flip clock that lives inside a night landscape
* https://github.com/tire-regroover/gluqlo (branch l-space)
*
* L-Space is a fork of Gluqlo (Fliqlo for Linux) by Alexander Kovalenko,
* https://github.com/alexanderk23/gluqlo, which is itself based on noflipqlo by
* Jacek Kuźniarski. The flip cards, the flip animation and the licence come from
* those projects. What is new in L-Space is everything behind and around the cards:
* a procedural backdrop with seasons and weather, and hundreds of scheduled
* scenery events (things that cross the sky, happen on the hills, or take over the
* whole screen) including ship battles with nine weapon types. See README.md and
* NOTES.md for the details and for how to add an event.
*
* Copyright (c) 2010-2012 Kuźniarski Jacek
* Copyright (c) 2014 Alexander Kovalenko
* Copyright (c) 2026 tire-regroover (L-Space scenery, weather and events)
*
* L-Space was written with the help of AI coding assistants: GitHub Copilot
* (Anthropic Claude), DeepSeek and Qwen. Their output was directed and reviewed
* by tire-regroover.
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

const char* TITLE = "Gluqlo L-Space";
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

#define RARE_SKY_KINDS 139
#define RARE_SKY_CLASSIC_KINDS 23   // kinds below this use the four classic sky objects
#define HILL_EVENT_KINDS 95         // things that happen on the hills (see render_hill_event)

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
		// Every kind falls the way the normal rain does (down, leaning right), so
		// the two never cross at different angles. Each particle moves along its streak.
		double direction_x = 0.25;
		double direction_y = 1.0;
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

// Fireflies drift over the ridge and blink on and off at their own pace.
static void hill_fireflies(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int flies = 70;
	static const Uint8 glow[3] = { 200, 255, 120 };
	double s = event->scale;
	double build = smooth_step(0.0, 0.12, event->progress)
		* (1.0 - smooth_step(0.88, 1.0, event->progress));
	for(int i = 0; i < flies; i++) {
		double r1 = hill_random(event, i, 1), r2 = hill_random(event, i, 2), r3 = hill_random(event, i, 3);
		double home_x = surface->w * (0.02 + 0.96 * r1);
		double home_y = ground_y(surface, RIDGE_NEAR, home_x) - 6.0 - r2 * surface->h * 0.2;
		double x = home_x + sin(event->t * (0.4 + r3 * 0.5) + i * 1.7) * s * 0.03;
		double y = home_y + cos(event->t * (0.5 + r2 * 0.4) + i) * s * 0.02;
		double blink = 0.5 + 0.5 * sin(event->t * (1.2 + r3 * 1.5) + i * 2.3);
		blink = blink * blink * blink;
		if(blink < 0.03) continue;
		weapon_glow(surface, x, y, 1.0 + s * 0.0025, glow, 235.0 * blink * build * event->fade);
	}
}

// Kites on strings: small figures stand on the ridge and let kites climb, sway and
// trail ribbons.
static void hill_kites(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int kites = 4;
	double s = event->scale;
	int d = event->direction;
	static const Uint8 colors[4][3] = {
		{ 230, 80, 70 }, { 245, 190, 70 }, { 80, 170, 230 }, { 150, 220, 110 } };
	static const double shape_x[4] = { 0.0, 0.8, 0.0, -0.8 };
	static const double shape_y[4] = { -1.2, 0.0, 1.6, 0.0 };
	for(int i = 0; i < kites; i++) {
		double anchor_x = surface->w * (0.12 + 0.76 * (i + 0.5) / kites);
		double ground = ground_y(surface, RIDGE_NEAR, anchor_x) + 1.0;
		double height = s * 0.034;
		hill_walker(surface, anchor_x, ground, height, d, 0.0, false, event->fade, event->t, i);
		double rise = smooth_step(0.04 + i * 0.04, 0.2 + i * 0.04, event->progress);
		double hand_x = anchor_x + d * height * 0.2, hand_y = ground - height * 0.58;
		double target_x = anchor_x + d * s * (0.05 + 0.03 * (i % 2)) + sin(event->t * 0.7 + i * 2.0) * s * 0.03;
		double target_y = ground - surface->h * (0.26 + 0.08 * (i % 3)) + cos(event->t * 0.9 + i) * s * 0.015;
		double kx = hand_x + (target_x - hand_x) * rise, ky = hand_y + (target_y - hand_y) * rise;
		Uint8 line_alpha = weapon_alpha(170.0 * event->fade);
		double last_x = hand_x, last_y = hand_y;
		for(int k = 1; k <= 10; k++) {
			double u = k / 10.0;
			double lx = hand_x + (kx - hand_x) * u;
			double ly = hand_y + (ky - hand_y) * u + sin(3.14159265 * u) * s * 0.012 * rise;
			lineRGBA(surface, (int)last_x, (int)last_y, (int)lx, (int)ly, 210, 210, 200, line_alpha);
			last_x = lx;
			last_y = ly;
		}
		double ksz = s * 0.02;
		double tilt = sin(event->t * 1.3 + i) * 0.3;
		double ca = cos(tilt), sa = sin(tilt);
		int px[4], py[4];
		for(int p = 0; p < 4; p++) {
			px[p] = (int)(kx + (shape_x[p] * ca - shape_y[p] * sa) * ksz);
			py[p] = (int)(ky + (shape_x[p] * sa + shape_y[p] * ca) * ksz);
		}
		const Uint8 *c = colors[i % 4];
		Uint8 alpha = weapon_alpha(235.0 * event->fade * rise);
		filledTrigonRGBA(surface, px[0], py[0], px[3], py[3], px[2], py[2],
			(Uint8)(c[0] * 0.7), (Uint8)(c[1] * 0.7), (Uint8)(c[2] * 0.7), alpha);
		filledTrigonRGBA(surface, px[0], py[0], px[1], py[1], px[2], py[2], c[0], c[1], c[2], alpha);
		lineRGBA(surface, px[0], py[0], px[2], py[2], 60, 50, 50, alpha);
		lineRGBA(surface, px[1], py[1], px[3], py[3], 60, 50, 50, alpha);
		double tail_x = px[2], tail_y = py[2];
		for(int k = 1; k <= 7; k++) {
			double bx = px[2] + sin(event->t * 3.0 + k * 0.9 + i) * ksz * 0.35 * (k / 7.0 + 0.3);
			double by = py[2] + k * ksz * 0.55;
			lineRGBA(surface, (int)tail_x, (int)tail_y, (int)bx, (int)by, 210, 210, 200, line_alpha);
			const Uint8 *bow = colors[(i + k) % 4];
			filledCircleRGBA(surface, (int)bx, (int)by, k % 2 ? 2 : 1, bow[0], bow[1], bow[2], alpha);
			tail_x = bx;
			tail_y = by;
		}
	}
}

// Fireworks: shells climb from the ridge, burst into rings of sparks that fall and fade.
static void hill_fireworks(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int shells = 6;
	static const Uint8 palette[5][3] = {
		{ 255, 90, 80 }, { 255, 205, 90 }, { 110, 200, 255 }, { 170, 255, 140 }, { 230, 130, 255 } };
	double s = event->scale;
	for(int b = 0; b < shells; b++) {
		double phase = event->t / 4.6 + b * 0.37;
		int cycle = (int)floor(phase);
		double u = phase - cycle;
		unsigned int h = fx_hash(event->seed + b * 131u, cycle);
		double bx = surface->w * (0.1 + 0.8 * (h & 255) / 255.0);
		double top = surface->h * (0.14 + 0.28 * ((h >> 8) & 255) / 255.0);
		double ground = ground_y(surface, RIDGE_NEAR, bx);
		const Uint8 *c = palette[(h >> 16) % 5];
		const double launch = 0.22;
		if(u < launch) {
			double k = u / launch;
			for(int j = 0; j < 6; j++) {
				double kk = k - j * 0.05;
				if(kk < 0.0) break;
				double yy = ground + (top - ground) * (1.0 - (1.0 - kk) * (1.0 - kk));
				filledCircleRGBA(surface, (int)(bx + sin(kk * 30.0) * 1.5), (int)yy, j == 0 ? 2 : 1,
					255, 220, 150, weapon_alpha(230.0 * (1.0 - j / 6.0) * event->fade));
			}
			continue;
		}
		double age = (u - launch) / (1.0 - launch);
		double radius = s * (0.1 + 0.07 * ((h >> 24) & 255) / 255.0);
		if(age < 0.1) {
			static const Uint8 white[3] = { 255, 250, 235 };
			weapon_glow(surface, bx, top, s * 0.03 * (1.0 - age * 8.0), white, 235.0 * event->fade);
		}
		double ease = 1.0 - pow(1.0 - age, 3.0);
		double ease_back = 1.0 - pow(1.0 - age * 0.93, 3.0);
		double fall = s * 0.12 * age * age;
		double fade = pow(1.0 - age, 1.5) * event->fade;
		const int sparks = 40;
		for(int j = 0; j < sparks; j++) {
			if(age > 0.5 && ((int)(event->t * 20.0) + j) % 3 == 0) continue;
			double a = j * 6.2831853 / sparks + (h & 7);
			double speed = j % 2 ? 0.7 : 1.0;
			int sx = (int)(bx + cos(a) * radius * speed * ease);
			int sy = (int)(top + sin(a) * radius * speed * ease + fall);
			filledCircleRGBA(surface, sx, sy, age < 0.5 ? 2 : 1, c[0], c[1], c[2], weapon_alpha(255.0 * fade));
			filledCircleRGBA(surface, (int)(bx + cos(a) * radius * speed * ease_back),
				(int)(top + sin(a) * radius * speed * ease_back + fall * 0.8), 1, c[0], c[1], c[2],
				weapon_alpha(110.0 * fade));
		}
	}
}

// A train with lit windows follows the ridge, its engine puffing smoke and shining a
// headlight ahead.
static void hill_train(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int cars = 6;
	double s = event->scale;
	int d = event->direction;
	double car_length = s * 0.07, gap = s * 0.008, margin = s * 0.1;
	double travel = surface->w + 2.0 * margin + (cars + 1) * (car_length + gap);
	double lead = event->progress * travel - margin;
	double wheel = s * 0.005;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	for(int car = 0; car <= cars; car++) {
		bool engine = car == 0;
		double front_px = lead - car * (car_length + gap);
		double rear_px = front_px - car_length * (engine ? 0.85 : 1.0);
		double fx = d > 0 ? front_px : surface->w - front_px;
		double rx = d > 0 ? rear_px : surface->w - rear_px;
		if((fx < -30 && rx < -30) || (fx > surface->w + 30 && rx > surface->w + 30)) continue;
		double fy = ground_y(surface, RIDGE_NEAR, fx) + 1.0;
		double ry = ground_y(surface, RIDGE_NEAR, rx) + 1.0;
		double height = s * (engine ? 0.034 : 0.028);
		int ax = (int)rx, ay = (int)(ry - wheel);
		int bx = (int)fx, by = (int)(fy - wheel);
		int cx = bx, cy = (int)(fy - wheel - height);
		int dx = ax, dy = (int)(ry - wheel - height);
		Uint8 r = engine ? 20 : 26, g = engine ? 24 : 32, b = engine ? 34 : 46;
		filledTrigonRGBA(surface, ax, ay, bx, by, cx, cy, r, g, b, alpha);
		filledTrigonRGBA(surface, ax, ay, cx, cy, dx, dy, r, g, b, alpha);
		int windows = engine ? 1 : 4;
		for(int k = 0; k < windows; k++) {
			double u = engine ? 0.3 : (k + 0.5) / 4.0;
			int wx = (int)(rx + (fx - rx) * u);
			int wy = (int)(ry + (fy - ry) * u - wheel - height * 0.6);
			double flicker = 0.9 + 0.1 * sin(event->t * 5.0 + car * 2.0 + k);
			boxRGBA(surface, wx - 2, wy - 2, wx + 2, wy + 1, 255, 214, 130,
				weapon_alpha(240.0 * flicker * event->fade));
		}
		for(int k = 0; k < 3; k++) {
			double u = 0.15 + 0.35 * k;
			filledCircleRGBA(surface, (int)(rx + (fx - rx) * u), (int)(ry + (fy - ry) * u - wheel * 0.6),
				(int)(wheel * 0.8) + 1, 8, 10, 16, alpha);
		}
		if(engine) {
			double chx = rx + (fx - rx) * 0.75, chy = ry + (fy - ry) * 0.75 - wheel - height;
			boxRGBA(surface, (int)chx - 2, (int)(chy - height * 0.4), (int)chx + 2, (int)chy, 14, 18, 26, alpha);
			for(int puff = 0; puff < 8; puff++) {
				double age = fmod(event->t / 1.6 + puff / 8.0, 1.0);
				filledCircleRGBA(surface, (int)(chx - d * age * s * 0.12 + sin(age * 6.0 + puff) * 2.0),
					(int)(chy - height * 0.4 - age * s * 0.07), (int)(2.0 + age * 5.0), 150, 152, 162,
					weapon_alpha(105.0 * (1.0 - age) * event->fade));
			}
			static const Uint8 lamp[3] = { 255, 235, 170 };
			double lx = fx + d * 3.0, ly = fy - wheel - height * 0.5;
			filledTrigonRGBA(surface, (int)lx, (int)ly, (int)(lx + d * s * 0.16), (int)(ly - s * 0.02),
				(int)(lx + d * s * 0.16), (int)(ly + s * 0.025), 255, 235, 170, weapon_alpha(26.0 * event->fade));
			weapon_glow(surface, lx, ly, s * 0.006, lamp, 235.0 * event->fade);
		}
	}
}

// A rocket on a gantry vents steam, ignites and climbs out of the top of the screen
// on a column of smoke.
static void hill_rocket(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double x = surface->w * (d > 0 ? 0.2 : 0.8);
	double ground = ground_y(surface, RIDGE_NEAR, x) + 2.0;
	double rocket_h = s * 0.075, rocket_w = s * 0.011;
	const double liftoff = 0.34;
	double flight = (event->progress - liftoff) / 0.4;
	if(flight < 0.0) flight = 0.0;
	if(flight > 1.0) flight = 1.0;
	double climb = surface->h + rocket_h * 2.0;
	double alt = flight * flight * climb;
	double drift = flight * flight * s * 0.05 * d;
	double ignite = smooth_step(liftoff - 0.08, liftoff, event->progress)
		* (1.0 - smooth_step(liftoff + 0.2, liftoff + 0.34, event->progress));
	double dissipate = 1.0 - smooth_step(0.74, 0.95, event->progress);
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	// The gantry and pad, with a blinking light on top.
	double tower_x = x - d * rocket_w * 3.0;
	boxRGBA(surface, (int)(tower_x - 2), (int)(ground - rocket_h * 0.95), (int)(tower_x + 2), (int)ground,
		18, 22, 32, alpha);
	for(int arm = 0; arm < 3; arm++) {
		double ay = ground - rocket_h * (0.3 + 0.25 * arm);
		thickLineRGBA(surface, (int)tower_x, (int)ay, (int)(x - d * rocket_w * 0.6), (int)ay, 1, 18, 22, 32,
			event->progress < liftoff ? alpha : (Uint8)0);
	}
	boxRGBA(surface, (int)(x - rocket_w * 2.5), (int)(ground - 2), (int)(x + rocket_w * 2.5), (int)(ground + 1),
		30, 34, 46, alpha);
	if(fmod(event->t, 1.2) < 0.4) {
		static const Uint8 red[3] = { 255, 60, 50 };
		weapon_glow(surface, tower_x, ground - rocket_h * 0.95 - 3.0, 2.5, red, 235.0 * event->fade);
	}
	// Steam and fire flare at the pad.
	filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.07), (int)(s * 0.012),
		255, 150, 60, weapon_alpha(50.0 * ignite * event->fade));
	for(int puff = 0; puff < 12; puff++) {
		double u = fmod(event->t / 2.5 + puff / 12.0, 1.0);
		double side = (puff % 2 ? 1.0 : -1.0) * u * s * 0.1;
		filledCircleRGBA(surface, (int)(x + side), (int)(ground - u * s * 0.015), (int)(s * (0.008 + 0.02 * u)),
			225, 230, 240, weapon_alpha(90.0 * (1.0 - u) * ignite * event->fade));
	}
	// The smoke column hangs where the rocket has been.
	const int trail = 24;
	for(int k = 0; k < trail && flight > 0.0; k++) {
		double past = flight - k * 0.018;
		if(past <= 0.0) break;
		double px = x + past * past * s * 0.05 * d + sin(k * 1.7) * 3.0;
		double py = ground - past * past * climb;
		filledCircleRGBA(surface, (int)px, (int)py, (int)(s * (0.008 + 0.018 * k / trail)), 190, 190, 200,
			weapon_alpha(120.0 * (1.0 - (double)k / trail) * dissipate * event->fade));
	}
	if(flight >= 1.0) return;
	// The rocket.
	double rx = x + drift, base = ground - alt - 2.0;
	int top_y = (int)(base - rocket_h * 0.78);
	boxRGBA(surface, (int)(rx - rocket_w / 2), top_y, (int)(rx + rocket_w / 2), (int)base, 205, 210, 220, alpha);
	boxRGBA(surface, (int)(rx - rocket_w / 2), (int)(base - rocket_h * 0.5), (int)(rx + rocket_w / 2),
		(int)(base - rocket_h * 0.42), 200, 60, 55, alpha);
	filledTrigonRGBA(surface, (int)(rx - rocket_w / 2), top_y, (int)(rx + rocket_w / 2), top_y,
		(int)rx, (int)(base - rocket_h), 200, 60, 55, alpha);
	filledTrigonRGBA(surface, (int)(rx - rocket_w / 2), (int)base, (int)(rx - rocket_w * 1.1), (int)(base + 2),
		(int)(rx - rocket_w / 2), (int)(base - rocket_h * 0.28), 150, 45, 42, alpha);
	filledTrigonRGBA(surface, (int)(rx + rocket_w / 2), (int)base, (int)(rx + rocket_w * 1.1), (int)(base + 2),
		(int)(rx + rocket_w / 2), (int)(base - rocket_h * 0.28), 150, 45, 42, alpha);
	filledCircleRGBA(surface, (int)rx, (int)(base - rocket_h * 0.62), (int)(rocket_w * 0.22) + 1, 90, 160, 210, alpha);
	double flame = smooth_step(liftoff - 0.05, liftoff + 0.02, event->progress);
	if(flame > 0.0) {
		double flicker = 0.75 + 0.25 * sin(event->t * 40.0);
		double length = rocket_h * (0.25 + 0.5 * flight) * flame * flicker;
		filledTrigonRGBA(surface, (int)(rx - rocket_w * 0.45), (int)base, (int)(rx + rocket_w * 0.45), (int)base,
			(int)rx, (int)(base + length), 255, 140, 40, weapon_alpha(225.0 * event->fade));
		filledTrigonRGBA(surface, (int)(rx - rocket_w * 0.25), (int)base, (int)(rx + rocket_w * 0.25), (int)base,
			(int)rx, (int)(base + length * 0.65), 255, 225, 140, weapon_alpha(235.0 * event->fade));
		static const Uint8 hot[3] = { 255, 170, 80 };
		weapon_glow(surface, rx, base + 2.0, rocket_w * 1.4 * flame, hot, 200.0 * event->fade);
	}
}

// A flying saucer glides in, hovers and beams up a cow, then leaves.
static void hill_abduction(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double target_x = surface->w * (0.3 + 0.4 * hill_random(event, 1, 1));
	double ground = ground_y(surface, RIDGE_NEAR, target_x) + 2.0;
	double arrive = smooth_step(0.0, 0.16, event->progress);
	double leave = smooth_step(0.84, 1.0, event->progress);
	double saucer_x = target_x - d * surface->w * 0.6 * (1.0 - arrive) + d * surface->w * 0.7 * leave * leave;
	double saucer_y = surface->h * 0.3 + sin(event->t * 1.1) * s * 0.006 - leave * surface->h * 0.2;
	double a = s * 0.05, b = s * 0.012;
	double beam = smooth_step(0.2, 0.26, event->progress) * (1.0 - smooth_step(0.74, 0.8, event->progress));
	double bottom = saucer_y + b;
	double pulse = 0.8 + 0.2 * sin(event->t * 9.0);
	if(beam > 0.0) {
		double near_w = s * 0.012, far_w = s * 0.055;
		filledTrigonRGBA(surface, (int)(saucer_x - near_w), (int)bottom, (int)(saucer_x + near_w), (int)bottom,
			(int)(saucer_x - far_w), (int)ground, 170, 255, 190, weapon_alpha(34.0 * beam * pulse * event->fade));
		filledTrigonRGBA(surface, (int)(saucer_x + near_w), (int)bottom, (int)(saucer_x + far_w), (int)ground,
			(int)(saucer_x - far_w), (int)ground, 170, 255, 190, weapon_alpha(34.0 * beam * pulse * event->fade));
		for(int k = 0; k < 5; k++) {
			double u = fmod(event->t * 0.6 + k * 0.2, 1.0);
			double ring_w = far_w + (near_w - far_w) * u;
			ellipseRGBA(surface, (int)saucer_x, (int)(ground + (bottom - ground) * u), (int)ring_w,
				(int)(ring_w * 0.15) + 1, 200, 255, 210, weapon_alpha(110.0 * beam * (1.0 - u) * event->fade));
		}
	}
	// The cow stands on the ground, then rises in the beam and shrinks.
	double lift = smooth_step(0.32, 0.72, event->progress);
	if(lift < 0.97) {
		double size = s * 0.04 * (1.0 - 0.5 * lift);
		double cow_x = target_x + sin(event->t * 2.0) * s * 0.004 * lift;
		double cow_y = ground - size * 0.7 - lift * (ground - bottom - size);
		double shade = lift * lift;
		Uint8 cr = (Uint8)(14 + 150 * shade), cg = (Uint8)(18 + 190 * shade), cb = (Uint8)(26 + 150 * shade);
		Uint8 alpha = weapon_alpha(238.0 * event->fade);
		for(int leg = 0; leg < 4; leg++) {
			double hip_x = cow_x + (leg < 2 ? 0.3 : -0.3) * size;
			double sway = sin(event->t * 3.0 + leg) * size * 0.12 * lift;
			thickLineRGBA(surface, (int)hip_x, (int)cow_y, (int)(hip_x + sway), (int)(cow_y + size * 0.6), 2,
				cr, cg, cb, alpha);
		}
		filledEllipseRGBA(surface, (int)cow_x, (int)cow_y, (int)(size * 0.5), (int)(size * 0.25), cr, cg, cb, alpha);
		filledEllipseRGBA(surface, (int)(cow_x + d * size * 0.58), (int)(cow_y - size * 0.1), (int)(size * 0.15),
			(int)(size * 0.11), cr, cg, cb, alpha);
	}
	// The saucer.
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	filledEllipseRGBA(surface, (int)saucer_x, (int)(saucer_y - b * 0.7), (int)(a * 0.4), (int)(b * 1.5),
		150, 215, 230, weapon_alpha(210.0 * event->fade));
	filledEllipseRGBA(surface, (int)saucer_x, (int)saucer_y, (int)a, (int)b, 120, 132, 150, alpha);
	filledEllipseRGBA(surface, (int)saucer_x, (int)(saucer_y + b * 0.4), (int)(a * 0.7), (int)(b * 0.5),
		84, 94, 112, alpha);
	static const Uint8 light[3] = { 255, 240, 160 };
	for(int j = 0; j < 7; j++) {
		double angle = 0.4 + j * (3.14159265 - 0.8) / 6.0;
		if(((int)(event->t * 6.0) + j) % 3 == 0) {
			weapon_glow(surface, saucer_x + cos(angle) * a * 0.85 * (d > 0 ? 1 : -1),
				saucer_y + sin(angle) * b * 0.55, 2.0, light, 235.0 * event->fade);
		}
	}
}

// Geysers erupt one after another: a water column, arcing drops and steam.
static void hill_geysers(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int geysers = 4;
	double s = event->scale;
	for(int i = 0; i < geysers; i++) {
		double x = surface->w * (0.14 + 0.72 * (i + 0.5 * hill_random(event, i, 1)) / geysers);
		double ground = ground_y(surface, RIDGE_NEAR, x) + 2.0;
		double period = 6.0 + 2.0 * hill_random(event, i, 2);
		double local = fmod(event->t / period + hill_random(event, i, 3), 1.0);
		double erupt = smooth_step(0.0, 0.04, local) * (1.0 - smooth_step(0.22, 0.3, local));
		double height = s * (0.1 + 0.07 * hill_random(event, i, 4)) * erupt;
		filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.03), (int)(s * 0.006), 70, 60, 64,
			weapon_alpha(235.0 * event->fade));
		filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.018), (int)(s * 0.003), 110, 190, 210,
			weapon_alpha(220.0 * event->fade));
		if(erupt > 0.02) {
			double wobble = sin(event->t * 14.0 + i) * s * 0.002;
			thickLineRGBA(surface, (int)x, (int)ground, (int)(x + wobble), (int)(ground - height * 0.85), 3,
				200, 235, 250, weapon_alpha(170.0 * erupt * event->fade));
			for(int j = 0; j < 22; j++) {
				double r = hill_random(event, i * 40 + j, 5), r2 = hill_random(event, i * 40 + j, 6);
				double u = fmod(event->t * 1.3 + j / 22.0, 1.0);
				double dx = (r - 0.5) * s * 0.07 * u;
				double dy = height * 4.0 * u * (1.0 - u) * (0.6 + 0.4 * r2);
				filledCircleRGBA(surface, (int)(x + dx), (int)(ground - dy), 1 + j % 2, 200, 235, 250,
					weapon_alpha(230.0 * erupt * event->fade));
			}
		}
		for(int puff = 0; puff < 8; puff++) {
			double u = fmod(event->t / 3.5 + puff / 8.0 + i * 0.2, 1.0);
			double px = x + event->direction * u * s * 0.05 + sin(u * 5.0 + puff) * s * 0.01;
			double py = ground - height * 0.5 - u * s * 0.12;
			filledCircleRGBA(surface, (int)px, (int)py, (int)(s * (0.008 + 0.02 * u)), 225, 230, 240,
				weapon_alpha(80.0 * (1.0 - u) * (0.3 + 0.7 * erupt) * event->fade));
		}
	}
}

// A circle of standing stones: runes wake up on them one by one, then a pillar of
// light rises from the middle with motes spiralling up it.
static void hill_stones(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int stones = 7;
	double s = event->scale;
	double span = s * 0.5;
	double center_x = surface->w * 0.5;
	double center_ground = ground_y(surface, RIDGE_NEAR, center_x) + 2.0;
	static const Uint8 rune[3] = { 110, 230, 255 };
	double pillar = smooth_step(0.5, 0.62, event->progress) * (1.0 - smooth_step(0.84, 0.94, event->progress));
	if(pillar > 0.0) {
		double width = s * 0.03 * (0.8 + 0.2 * sin(event->t * 3.0));
		boxRGBA(surface, (int)(center_x - width), 0, (int)(center_x + width), (int)center_ground,
			rune[0], rune[1], rune[2], weapon_alpha(24.0 * pillar * event->fade));
		boxRGBA(surface, (int)(center_x - width * 0.4), 0, (int)(center_x + width * 0.4), (int)center_ground,
			220, 250, 255, weapon_alpha(46.0 * pillar * event->fade));
		for(int j = 0; j < 18; j++) {
			double u = fmod(event->t * 0.25 + j / 18.0, 1.0);
			double angle = u * 12.0 + j;
			filledCircleRGBA(surface, (int)(center_x + cos(angle) * width * 0.9),
				(int)(center_ground - u * surface->h * 0.8), 2, rune[0], rune[1], rune[2],
				weapon_alpha(220.0 * pillar * (1.0 - u * 0.5) * event->fade));
		}
	}
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	for(int i = 0; i < stones; i++) {
		double x = center_x - span / 2.0 + span * i / (stones - 1.0);
		double ground = ground_y(surface, RIDGE_NEAR, x) + 2.0;
		double height = s * (0.045 + 0.015 * hill_random(event, i, 1));
		double width = height * 0.35;
		int order = event->direction > 0 ? i : stones - 1 - i;
		filledTrigonRGBA(surface, (int)(x - width / 2), (int)ground, (int)(x + width / 2), (int)ground,
			(int)(x + width * 0.4), (int)(ground - height), 30, 34, 46, alpha);
		filledTrigonRGBA(surface, (int)(x - width / 2), (int)ground, (int)(x + width * 0.4), (int)(ground - height),
			(int)(x - width * 0.4), (int)(ground - height), 30, 34, 46, alpha);
		double wake = smooth_step(0.1 + order * 0.06, 0.16 + order * 0.06, event->progress)
			* (1.0 - smooth_step(0.9, 0.98, event->progress));
		if(wake <= 0.0) continue;
		double glow = wake * (0.8 + 0.2 * sin(event->t * 4.0 + i));
		double mid = ground - height * 0.55;
		lineRGBA(surface, (int)(x - width * 0.2), (int)(mid - height * 0.15), (int)(x + width * 0.2),
			(int)(mid - height * 0.15), rune[0], rune[1], rune[2], weapon_alpha(235.0 * glow * event->fade));
		lineRGBA(surface, (int)x, (int)(mid - height * 0.15), (int)x, (int)(mid + height * 0.15),
			rune[0], rune[1], rune[2], weapon_alpha(235.0 * glow * event->fade));
		circleRGBA(surface, (int)x, (int)(mid + height * 0.05), (int)(width * 0.18) + 1,
			rune[0], rune[1], rune[2], weapon_alpha(235.0 * glow * event->fade));
		weapon_glow(surface, x, mid, height * 0.12, rune, 150.0 * glow * event->fade);
		if(pillar > 0.0) {
			lineRGBA(surface, (int)x, (int)(ground - height), (int)center_x, (int)(center_ground - 2.0),
				rune[0], rune[1], rune[2], weapon_alpha(90.0 * pillar * event->fade));
		}
	}
}

#define SCENE_PI 3.14159265359
#define SCENE_TAU 6.28318530718

// Screen row of the near ridge under pixel column x, for things standing on it.
static double hill_ground(const HillEvent *event, double x) {
	return ground_y(event->surface, RIDGE_NEAR, x) + 2.0;
}

// A column near one side of the screen (so it is not hidden by the cards): `fraction` of the
// width from the side things come from.
static double hill_side(const HillEvent *event, double fraction) {
	return event->surface->w * (event->direction > 0 ? fraction : 1.0 - fraction);
}

// A lit Ferris wheel rises on the ridge and turns, its gondolas and rim lights blinking.
static void hill_ferris(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = hill_side(event, 0.1);
	double ground = hill_ground(event, cx);
	double radius = s * 0.12 * smooth_step(0.0, 0.12, event->progress);
	if(radius < 3.0) return;
	double hub_y = ground - radius - s * 0.012;
	double lit = smooth_step(0.1, 0.2, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
	Uint8 alpha = weapon_alpha(235.0 * event->fade);
	static const Uint8 colors[4][3] = {
		{ 255, 120, 110 }, { 120, 220, 255 }, { 255, 220, 110 }, { 160, 255, 150 } };
	thickLineRGBA(surface, (int)cx, (int)hub_y, (int)(cx - radius * 0.6), (int)ground, 3, 18, 22, 32, alpha);
	thickLineRGBA(surface, (int)cx, (int)hub_y, (int)(cx + radius * 0.6), (int)ground, 3, 18, 22, 32, alpha);
	const int spokes = 12;
	double spin = event->t * 0.22 * event->direction;
	int gondola = (int)(s * 0.007) + 2;
	for(int k = 0; k < spokes; k++) {
		double angle = spin + k * SCENE_TAU / spokes;
		double tip_x = cx + cos(angle) * radius, tip_y = hub_y + sin(angle) * radius;
		double next_x = cx + cos(angle + SCENE_TAU / spokes) * radius;
		double next_y = hub_y + sin(angle + SCENE_TAU / spokes) * radius;
		thickLineRGBA(surface, (int)cx, (int)hub_y, (int)tip_x, (int)tip_y, 1, 40, 46, 64, alpha);
		thickLineRGBA(surface, (int)tip_x, (int)tip_y, (int)next_x, (int)next_y, 2, 34, 40, 58, alpha);
		boxRGBA(surface, (int)tip_x - gondola, (int)tip_y, (int)tip_x + gondola, (int)tip_y + gondola * 2,
			26, 30, 44, alpha);
		boxRGBA(surface, (int)tip_x - 1, (int)tip_y + 1, (int)tip_x + 1, (int)tip_y + gondola + 1,
			255, 214, 130, weapon_alpha(230.0 * lit * event->fade));
		double blink = 0.55 + 0.45 * sin(event->t * 5.0 + k * 1.7);
		weapon_glow(surface, tip_x, tip_y - 1.0, 2.2, colors[k % 4], 230.0 * lit * blink * event->fade);
	}
	filledCircleRGBA(surface, (int)cx, (int)hub_y, 4, 44, 50, 70, alpha);
	boxRGBA(surface, (int)(cx - radius * 0.7), (int)(ground - 2), (int)(cx + radius * 0.7), (int)(ground + 2),
		22, 26, 38, alpha);
}

// A small observatory: the dome slit opens and a laser guide star beam climbs into the sky.
static void hill_observatory(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = hill_side(event, 0.13);
	double ground = hill_ground(event, cx);
	double r = s * 0.05, wall = s * 0.035;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	double open = smooth_step(0.06, 0.14, event->progress) * (1.0 - smooth_step(0.9, 0.96, event->progress));
	double dome_y = ground - wall;
	boxRGBA(surface, (int)(cx - r), (int)dome_y, (int)(cx + r), (int)ground, 22, 26, 38, alpha);
	for(int k = 0; k < 3; k++) {
		int wx = (int)(cx - r * 0.6 + k * r * 0.6);
		boxRGBA(surface, wx - 1, (int)(dome_y + wall * 0.3), wx + 1, (int)(dome_y + wall * 0.3) + 5,
			255, 214, 130, weapon_alpha(220.0 * open * event->fade));
	}
	filledPieRGBA(surface, (int)cx, (int)dome_y, (int)r, 180, 360, 34, 40, 58, alpha);
	double top = dome_y - r;
	double slit = r * 0.24 * open;
	if(slit > 0.5) {
		boxRGBA(surface, (int)(cx - slit), (int)top, (int)(cx + slit), (int)(top + r * 0.55), 8, 10, 16, alpha);
	}
	if(open <= 0.3) return;
	static const Uint8 green[3] = { 120, 255, 170 };
	double tilt = event->direction * (0.12 + 0.18 * sin(event->t * 0.25));
	double length = s * 1.3;
	double ex = cx + sin(tilt) * length, ey = top - cos(tilt) * length;
	double flicker = (0.85 + 0.15 * sin(event->t * 13.0)) * open * event->fade;
	thickLineRGBA(surface, (int)cx, (int)top, (int)ex, (int)ey, 7, green[0], green[1], green[2],
		weapon_alpha(16.0 * flicker));
	thickLineRGBA(surface, (int)cx, (int)top, (int)ex, (int)ey, 3, green[0], green[1], green[2],
		weapon_alpha(50.0 * flicker));
	thickLineRGBA(surface, (int)cx, (int)top, (int)ex, (int)ey, 1, 220, 255, 235, weapon_alpha(200.0 * flicker));
	weapon_glow(surface, cx, top, 4.0, green, 200.0 * open * event->fade);
	double star_x = cx + sin(tilt) * s * 0.7, star_y = top - cos(tilt) * s * 0.7;
	double twinkle = 3.0 + 1.5 * sin(event->t * 3.0);
	weapon_glow(surface, star_x, star_y, twinkle, green, 220.0 * open * event->fade);
	lineRGBA(surface, (int)(star_x - twinkle * 3.0), (int)star_y, (int)(star_x + twinkle * 3.0), (int)star_y,
		220, 255, 235, weapon_alpha(150.0 * open * event->fade));
	lineRGBA(surface, (int)star_x, (int)(star_y - twinkle * 3.0), (int)star_x, (int)(star_y + twinkle * 3.0),
		220, 255, 235, weapon_alpha(150.0 * open * event->fade));
}

// Old windmills grow on the ridge and their lattice sails start to turn.
static void hill_windmills(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int mills = 3;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	double lit = smooth_step(0.15, 0.25, event->progress) * (1.0 - smooth_step(0.9, 0.97, event->progress));
	for(int i = 0; i < mills; i++) {
		double x = surface->w * (0.08 + 0.84 * i / (mills - 1.0));
		double rise = smooth_step(0.02 + i * 0.05, 0.12 + i * 0.05, event->progress);
		if(rise <= 0.0) continue;
		double base = hill_ground(event, x);
		double height = s * (0.09 + 0.015 * (i % 2)) * rise;
		double wide = height * 0.2, narrow = height * 0.11;
		double top = base - height;
		filledTrigonRGBA(surface, (int)(x - wide), (int)base, (int)(x + wide), (int)base,
			(int)(x - narrow), (int)top, 24, 28, 40, alpha);
		filledTrigonRGBA(surface, (int)(x + wide), (int)base, (int)(x + narrow), (int)top,
			(int)(x - narrow), (int)top, 24, 28, 40, alpha);
		filledTrigonRGBA(surface, (int)(x - narrow - 2), (int)top, (int)(x + narrow + 2), (int)top,
			(int)x, (int)(top - height * 0.18), 14, 16, 26, alpha);
		filledCircleRGBA(surface, (int)x, (int)(base - height * 0.22), 6, 255, 190, 100,
			weapon_alpha(30.0 * lit * event->fade));
		boxRGBA(surface, (int)x - 2, (int)(base - height * 0.28), (int)x + 2, (int)(base - height * 0.16),
			255, 214, 130, weapon_alpha(235.0 * lit * event->fade));
		double hub_x = x, hub_y = top - height * 0.02;
		double spin = event->t * (0.45 + 0.1 * i) * rise + i * 1.1;
		double arm = height * 0.75;
		for(int k = 0; k < 4; k++) {
			double a = spin + k * SCENE_PI / 2.0;
			double ux = cos(a), uy = sin(a);
			double tip_x = hub_x + ux * arm, tip_y = hub_y + uy * arm;
			double px = -uy, py = ux, wd = arm * 0.2;
			double ax = hub_x + ux * arm * 0.3, ay = hub_y + uy * arm * 0.3;
			filledTrigonRGBA(surface, (int)ax, (int)ay, (int)tip_x, (int)tip_y,
				(int)(tip_x + px * wd), (int)(tip_y + py * wd), 62, 68, 90, weapon_alpha(190.0 * event->fade));
			filledTrigonRGBA(surface, (int)ax, (int)ay, (int)(ax + px * wd), (int)(ay + py * wd),
				(int)(tip_x + px * wd), (int)(tip_y + py * wd), 62, 68, 90, weapon_alpha(190.0 * event->fade));
			thickLineRGBA(surface, (int)hub_x, (int)hub_y, (int)tip_x, (int)tip_y, 2, 20, 24, 36, alpha);
		}
		filledCircleRGBA(surface, (int)hub_x, (int)hub_y, 3, 44, 50, 70, alpha);
	}
}

typedef struct {
	const HillEvent *event;
	double grow;       // 0..1 how much of the tree exists
	double lit;        // 0..1 how bright its lights are
	int max_depth;
	int salt;
} HillTree;

static void hill_tree_branch(const HillTree *tree, double x, double y, double angle, double length,
	int depth, int id) {
	const HillEvent *event = tree->event;
	double reveal = tree->grow * (tree->max_depth + 1) - depth;
	if(reveal <= 0.0) return;
	if(reveal > 1.0) reveal = 1.0;
	double a = angle + sin(event->t * 0.7 + depth * 0.5 + id * 0.37) * 0.018 * (depth + 1);
	double x1 = x + sin(a) * length * reveal, y1 = y - cos(a) * length * reveal;
	int width = tree->max_depth - depth + 1;
	if(width > 7) width = 7;
	thickLineRGBA(event->surface, (int)x, (int)y, (int)x1, (int)y1, width, 28, 22, 30,
		weapon_alpha(238.0 * event->fade));
	if(depth >= tree->max_depth - 1 && tree->lit > 0.0) {
		static const Uint8 tint[3][3] = { { 255, 200, 110 }, { 255, 150, 170 }, { 150, 220, 255 } };
		double twinkle = 0.6 + 0.4 * sin(event->t * 2.5 + id * 1.9);
		weapon_glow(event->surface, x1, y1, depth == tree->max_depth ? 3.0 : 2.0, tint[id % 3],
			230.0 * tree->lit * twinkle * reveal * event->fade);
	}
	if(reveal < 1.0 || depth >= tree->max_depth) return;
	double r1 = hill_random(event, id + tree->salt, 1), r2 = hill_random(event, id + tree->salt, 2);
	hill_tree_branch(tree, x1, y1, a - 0.35 - 0.3 * r1, length * (0.72 + 0.08 * r2), depth + 1, id * 2);
	hill_tree_branch(tree, x1, y1, a + 0.35 + 0.3 * r2, length * (0.72 + 0.08 * r1), depth + 1, id * 2 + 1);
}

// Two bare trees at the sides of the screen grow branch by branch and then light up with coloured lamps.
static void hill_lighttree(const HillEvent *event) {
	for(int i = 0; i < 2; i++) {
		double x = event->surface->w * (i ? 0.92 : 0.08);
		double ground = hill_ground(event, x);
		HillTree tree;
		tree.event = event;
		tree.grow = smooth_step(0.02 + i * 0.06, 0.4 + i * 0.06, event->progress);
		tree.lit = smooth_step(0.35, 0.55, event->progress) * (1.0 - smooth_step(0.9, 0.97, event->progress));
		tree.max_depth = 6;
		tree.salt = i * 1000;
		if(tree.lit > 0.0) {
			ellipseRGBA(event->surface, (int)x, (int)ground, (int)(event->scale * 0.05),
				(int)(event->scale * 0.008), 255, 200, 110, weapon_alpha(70.0 * tree.lit * event->fade));
		}
		hill_tree_branch(&tree, x, ground, 0.0, event->scale * 0.075, 0, 1);
	}
}

// One meteorite hitting the near ridge: streak, flash, shock ring, debris, dust and a crater that cools.
static void hill_impact(const HillEvent *event, int id, double start, double x, int from) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double duration = event->progress > 0.001 ? event->t / event->progress : 47.0;
	double age = (event->progress - start) * duration;
	if(age < 0.0) return;
	double ground = hill_ground(event, x);
	const double flight = 1.5;
	static const Uint8 hot[3] = { 255, 170, 90 };
	if(age < flight) {
		double u = age / flight;
		double sx = x - from * s * 0.5, sy = ground - s * 0.8;
		double hx = sx + (x - sx) * u, hy = sy + (ground - sy) * u;
		double tx = hx - (x - sx) * 0.22, ty = hy - (ground - sy) * 0.22;
		thickLineRGBA(surface, (int)tx, (int)ty, (int)hx, (int)hy, 5, 255, 150, 70, weapon_alpha(40.0 * event->fade));
		thickLineRGBA(surface, (int)tx, (int)ty, (int)hx, (int)hy, 3, 255, 180, 100, weapon_alpha(120.0 * event->fade));
		thickLineRGBA(surface, (int)tx, (int)ty, (int)hx, (int)hy, 1, 255, 250, 235, weapon_alpha(230.0 * event->fade));
		weapon_glow(surface, hx, hy, 4.0 + u * 3.0, hot, 235.0 * event->fade);
		return;
	}
	double q = age - flight;
	double glow = exp(-q / 6.0);
	filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.03), (int)(s * 0.007), 10, 8, 10,
		weapon_alpha(230.0 * event->fade));
	filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.022), (int)(s * 0.005), 255, 110, 50,
		weapon_alpha(200.0 * glow * event->fade));
	if(q < 1.4) {
		double flash = 1.0 - q / 1.4;
		weapon_glow(surface, x, ground - s * 0.01, s * 0.05 * flash + 2.0, hot, 240.0 * flash * event->fade);
	}
	if(q < 3.5) {
		double w = q / 3.5;
		for(int ring = 0; ring < 2; ring++) {
			double rw = w - ring * 0.1;
			if(rw <= 0.0) continue;
			ellipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.02 + rw * s * 0.22),
				(int)(s * 0.004 + rw * s * 0.03), 255, 220, 170, weapon_alpha(160.0 * (1.0 - rw) * event->fade));
		}
	}
	for(int puff = 0; puff < 7; puff++) {
		double u = q / 8.0 - puff * 0.05;
		if(u <= 0.0 || u >= 1.0) continue;
		double px = x + sin(puff * 2.3) * s * 0.03 * u + from * u * s * 0.03;
		double py = ground - u * s * 0.14 - (puff % 3) * s * 0.01;
		filledCircleRGBA(surface, (int)px, (int)py, (int)(s * (0.012 + 0.03 * u)), 120, 105, 95,
			weapon_alpha(90.0 * (1.0 - u) * event->fade));
	}
	const double gravity = s * 1.2;
	for(int j = 0; j < 10; j++) {
		double r1 = hill_random(event, id * 20 + j, 1), r2 = hill_random(event, id * 20 + j, 2);
		double vx = (r1 - 0.5) * s * 0.5, vy = s * (0.25 + 0.35 * r2);
		double land = 2.0 * vy / gravity;
		double tt = q < land ? q : land;
		double px = x + vx * tt, py = ground - (vy * tt - 0.5 * gravity * tt * tt);
		if(q < land) {
			filledCircleRGBA(surface, (int)px, (int)py, 1 + j % 2, 255, 150 + j * 5, 80,
				weapon_alpha(235.0 * event->fade));
		} else {
			filledCircleRGBA(surface, (int)px, (int)py, 1 + j % 2, 50, 42, 44, weapon_alpha(230.0 * event->fade));
			filledCircleRGBA(surface, (int)px, (int)py, 2 + j % 2, 255, 120, 50,
				weapon_alpha(70.0 * glow * event->fade));
		}
	}
}

// Three meteorites crash into the ridge one after another.
static void hill_meteorite(const HillEvent *event) {
	hill_impact(event, 1, 0.1, event->surface->w * 0.12, 1);
	hill_impact(event, 2, 0.4, event->surface->w * 0.88, -1);
	hill_impact(event, 3, 0.68, event->surface->w * 0.22, 1);
}

// Dust devils: whirling funnels of sand wander along the ridge.
static void hill_dustdevils(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int devils = 3, slices = 16;
	for(int i = 0; i < devils; i++) {
		double u = fmod(event->progress * 1.3 + i / (double)devils, 1.0);
		double edge = smooth_step(0.0, 0.08, u) * (1.0 - smooth_step(0.92, 1.0, u));
		double x = surface->w * (0.05 + 0.9 * (event->direction > 0 ? u : 1.0 - u));
		double base = hill_ground(event, x);
		double height = s * (0.1 + 0.04 * (i % 2));
		for(int k = 0; k < slices; k++) {
			double v = (k + 0.5) / slices;
			double width = s * (0.005 + 0.04 * v);
			double sway = sin(event->t * 2.5 - v * 5.0 + i) * s * 0.012 * v;
			double shade = 0.6 + 0.4 * cos(k * 1.7 - event->t * 10.0);
			filledEllipseRGBA(surface, (int)(x + sway), (int)(base - v * height), (int)width,
				(int)(width * 0.25) + 1, 190, 168, 135, weapon_alpha(150.0 * (1.0 - 0.4 * v) * shade * edge * event->fade));
		}
		for(int j = 0; j < 10; j++) {
			double v = fmod(event->t * 0.15 + j / 10.0 + i * 0.3, 1.0);
			double angle = event->t * (6.0 + i) + j * 2.1;
			double width = s * (0.005 + 0.04 * v);
			double sway = sin(event->t * 2.5 - v * 5.0 + i) * s * 0.012 * v;
			double bx = x + sway + cos(angle) * width * 1.1;
			double by = base - v * height + sin(angle) * width * 0.25;
			lineRGBA(surface, (int)bx, (int)by, (int)(bx - sin(angle) * 4.0), (int)(by + cos(angle)), 60, 50, 44,
				weapon_alpha(200.0 * edge * event->fade));
		}
		ellipseRGBA(surface, (int)x, (int)base, (int)(s * 0.03), (int)(s * 0.004), 170, 150, 120,
			weapon_alpha(60.0 * edge * event->fade));
	}
}

// A laser show: emitters along the ridge sweep fans of coloured beams across the sky.
static void hill_lasers(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	static const Uint8 colors[4][3] = {
		{ 255, 70, 200 }, { 70, 230, 255 }, { 120, 255, 120 }, { 255, 200, 70 } };
	const int emitters = 5;
	double show = smooth_step(0.06, 0.12, event->progress) * (1.0 - smooth_step(0.9, 0.96, event->progress));
	for(int i = 0; i < emitters; i++) {
		double x = surface->w * (0.08 + 0.84 * i / (emitters - 1.0));
		double ground = hill_ground(event, x);
		boxRGBA(surface, (int)x - 3, (int)ground - 4, (int)x + 3, (int)ground + 1, 20, 24, 36,
			weapon_alpha(238.0 * event->fade));
		if(show <= 0.0) continue;
		const Uint8 *c = colors[i % 4];
		for(int b = 0; b < 3; b++) {
			double angle = (b - 1) * 0.5 + sin(event->t * (0.5 + 0.17 * i) + i * 1.3 + b) * 0.35;
			double length = s * 1.1;
			double ex = x + sin(angle) * length, ey = ground - cos(angle) * length;
			double flicker = (0.85 + 0.15 * sin(event->t * 17.0 + i * 3.0 + b)) * show * event->fade;
			thickLineRGBA(surface, (int)x, (int)ground, (int)ex, (int)ey, 7, c[0], c[1], c[2],
				weapon_alpha(14.0 * flicker));
			thickLineRGBA(surface, (int)x, (int)ground, (int)ex, (int)ey, 3, c[0], c[1], c[2],
				weapon_alpha(45.0 * flicker));
			thickLineRGBA(surface, (int)x, (int)ground, (int)ex, (int)ey, 1, 255, 255, 255,
				weapon_alpha(150.0 * flicker));
		}
		weapon_glow(surface, x, ground - 4.0, 4.0, c, 200.0 * show * event->fade);
	}
}

// A jagged arc of lightning between two points, drawn as a glow and a white core.
static void hill_arc(SDL_Surface *surface, double x0, double y0, double x1, double y1, int segments,
	double jitter, unsigned int seed, const Uint8 *color, double alpha) {
	double px = x0, py = y0;
	for(int k = 1; k <= segments; k++) {
		double u = (double)k / segments;
		double nx = x0 + (x1 - x0) * u, ny = y0 + (y1 - y0) * u;
		if(k < segments) {
			nx += ((fx_hash(seed, k) % 2001) / 1000.0 - 1.0) * jitter;
			ny += ((fx_hash(seed, k + 64) % 2001) / 1000.0 - 1.0) * jitter;
		}
		thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 3, color[0], color[1], color[2],
			weapon_alpha(alpha * 0.25));
		thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 1, 235, 240, 255, weapon_alpha(alpha));
		px = nx;
		py = ny;
	}
}

// Tesla towers charge up and throw lightning between their spheres and down to the ground.
static void hill_tesla(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int towers = 4;
	static const Uint8 blue[3] = { 140, 180, 255 };
	double tx[4], ty[4], rise[4];
	double charge = smooth_step(0.1, 0.2, event->progress) * (1.0 - smooth_step(0.9, 0.96, event->progress));
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	for(int i = 0; i < towers; i++) {
		double x = surface->w * (0.07 + 0.86 * i / (towers - 1.0));
		rise[i] = smooth_step(0.0 + i * 0.02, 0.1 + i * 0.02, event->progress);
		double base = hill_ground(event, x);
		double height = s * (0.12 + 0.02 * (i % 2)) * rise[i];
		double top = base - height;
		double sphere = s * 0.014;
		double wide = s * 0.012;
		tx[i] = x;
		ty[i] = top - sphere;
		if(rise[i] <= 0.0) continue;
		filledTrigonRGBA(surface, (int)(x - wide), (int)base, (int)(x + wide), (int)base, (int)x, (int)top,
			20, 24, 36, alpha);
		for(int bar = 1; bar < 4; bar++) {
			double f = bar / 4.0;
			lineRGBA(surface, (int)(x - wide * (1.0 - f)), (int)(base - height * f), (int)(x + wide * (1.0 - f)),
				(int)(base - height * f), 60, 68, 92, alpha);
		}
		filledCircleRGBA(surface, (int)x, (int)ty[i], (int)sphere, 70, 78, 100, alpha);
		weapon_glow(surface, x, ty[i], sphere * 0.9, blue,
			200.0 * charge * (0.7 + 0.3 * sin(event->t * 9.0 + i)) * event->fade);
	}
	if(charge <= 0.1) return;
	unsigned int burst = (unsigned int)(event->t * 1.5);
	unsigned int tick = (unsigned int)(event->t * 14.0);
	for(int i = 0; i < towers; i++) {
		if(rise[i] < 0.95) continue;
		if(i < towers - 1 && rise[i + 1] >= 0.95 && fx_hash(event->seed, burst * 8 + i) % 3 != 0) {
			hill_arc(surface, tx[i], ty[i], tx[i + 1], ty[i + 1], 9, s * 0.012,
				fx_hash(event->seed, tick * 8 + i), blue, 230.0 * charge * event->fade);
		}
		if(fx_hash(event->seed, burst * 8 + 4 + i) % 4 == 0) {
			double ex = tx[i] + (hill_random(event, i, 9) - 0.5) * s * 0.2;
			hill_arc(surface, tx[i], ty[i], ex, hill_ground(event, ex), 8, s * 0.01,
				fx_hash(event->seed, tick * 8 + 4 + i), blue, 200.0 * charge * event->fade);
		}
	}
}

// Hot air balloons are inflated on the ridge, their burners roar and they lift off one by one.
static void hill_balloons(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	static const Uint8 palette[4][2][3] = {
		{ { 150, 70, 70 }, { 225, 195, 150 } }, { { 60, 110, 150 }, { 215, 215, 195 } },
		{ { 90, 130, 80 }, { 225, 185, 110 } }, { { 130, 90, 150 }, { 215, 205, 175 } } };
	const int count = 4;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	for(int i = 0; i < count; i++) {
		double x0 = hill_side(event, 0.07 + 0.08 * i);
		double ground = hill_ground(event, x0);
		double inflate = smooth_step(0.02 + 0.03 * i, 0.14 + 0.03 * i, event->progress);
		if(inflate <= 0.0) continue;
		double launch = 0.3 + 0.1 * i;
		double rise = smooth_step(launch, launch + 0.4, event->progress);
		double bx = x0 + event->direction * rise * rise * surface->w * 0.2
			+ sin(event->t * 0.6 + i * 1.7) * s * 0.01 * rise;
		double by = ground - rise * s * 0.95;
		double r = s * (0.042 + 0.006 * (i % 2)) * inflate;
		double rope = s * 0.03 * inflate;
		double ex = bx + sin(event->t * 0.5 + i) * r * 0.05, ey = by - rope - r * 1.15;
		lineRGBA(surface, (int)bx - 3, (int)by, (int)(ex - r * 0.45), (int)(ey + r), 30, 26, 24, alpha);
		lineRGBA(surface, (int)bx + 3, (int)by, (int)(ex + r * 0.45), (int)(ey + r), 30, 26, 24, alpha);
		filledEllipseRGBA(surface, (int)ex, (int)ey, (int)(r * 0.88) + 1, (int)(r * 1.18) + 1, 20, 22, 30, alpha);
		const Uint8 *a = palette[i % 4][0], *b = palette[i % 4][1];
		filledEllipseRGBA(surface, (int)ex, (int)ey, (int)(r * 0.85), (int)(r * 1.15), a[0], a[1], a[2], alpha);
		filledEllipseRGBA(surface, (int)ex, (int)ey, (int)(r * 0.55), (int)(r * 1.15), b[0], b[1], b[2], alpha);
		filledEllipseRGBA(surface, (int)ex, (int)ey, (int)(r * 0.25), (int)(r * 1.15), a[0], a[1], a[2], alpha);
		boxRGBA(surface, (int)bx - 4, (int)by, (int)bx + 4, (int)by + 5, 70, 50, 40, alpha);
		double flare = fmod(event->t + i * 0.8, 2.6);
		if(flare < 0.6 && inflate > 0.5) {
			static const Uint8 warm[3] = { 255, 190, 110 };
			double f = 1.0 - flare / 0.6;
			filledTrigonRGBA(surface, (int)bx - 2, (int)(by - rope * 0.9), (int)bx + 2, (int)(by - rope * 0.9),
				(int)bx, (int)(by - rope * 0.9 - r * 0.5 * f), 255, 160, 70, weapon_alpha(220.0 * f * event->fade));
			filledEllipseRGBA(surface, (int)ex, (int)ey, (int)(r * 0.6), (int)(r * 0.9), 255, 190, 110,
				weapon_alpha(45.0 * f * event->fade));
			weapon_glow(surface, bx, by - rope * 0.95, r * 0.12, warm, 230.0 * f * event->fade);
		}
	}
}

// A giant walking figure strides slowly along the middle ridge, its eye and chest glowing.
static void hill_giant(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double H = s * 0.26;
	double margin = H * 0.5;
	double span = surface->w + 2.0 * margin;
	double x = d > 0 ? -margin + event->progress * span : surface->w + margin - event->progress * span;
	double ground = ground_y(surface, RIDGE_MIDDLE, x) + 2.0;
	double gait = event->t * SCENE_TAU * 0.2;
	double thigh = H * 0.24, shin = H * 0.24, swing = H * 0.17;
	double hip_y = ground - (thigh + shin) * 0.93 - fabs(cos(gait)) * H * 0.015;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	filledEllipseRGBA(surface, (int)x, (int)ground, (int)(H * 0.3), (int)(H * 0.03), 8, 10, 16,
		weapon_alpha(110.0 * event->fade));
	for(int leg = 0; leg < 2; leg++) {
		double ph = gait + leg * SCENE_PI;
		double foot_x = x + sin(ph) * swing, foot_y = ground - fmax(0.0, cos(ph)) * H * 0.05;
		double knee_x = (x + foot_x) / 2.0 + d * H * 0.05, knee_y = (hip_y + foot_y) / 2.0 - H * 0.01;
		Uint8 shade = leg ? 44 : 32;
		thickLineRGBA(surface, (int)x, (int)hip_y, (int)knee_x, (int)knee_y, 9, shade, shade + 4, shade + 14, alpha);
		thickLineRGBA(surface, (int)knee_x, (int)knee_y, (int)foot_x, (int)foot_y, 7, shade, shade + 4, shade + 14, alpha);
		thickLineRGBA(surface, (int)foot_x, (int)foot_y, (int)(foot_x + d * H * 0.07), (int)foot_y, 5,
			shade, shade + 4, shade + 14, alpha);
	}
	double shoulder_x = x + d * H * 0.03, shoulder_y = hip_y - H * 0.33;
	thickLineRGBA(surface, (int)x, (int)hip_y, (int)shoulder_x, (int)shoulder_y, 16, 40, 46, 62, alpha);
	thickLineRGBA(surface, (int)(x + d * 4), (int)hip_y, (int)(shoulder_x + d * 4), (int)shoulder_y, 2,
		60, 70, 92, weapon_alpha(200.0 * event->fade));
	for(int arm = 0; arm < 2; arm++) {
		double ph = gait + arm * SCENE_PI + SCENE_PI;
		double hand_x = shoulder_x + sin(ph) * H * 0.13, hand_y = shoulder_y + H * 0.3 - fmax(0.0, cos(ph)) * H * 0.03;
		thickLineRGBA(surface, (int)shoulder_x, (int)shoulder_y, (int)hand_x, (int)hand_y, 6,
			arm ? 48 : 34, arm ? 54 : 40, arm ? 72 : 56, alpha);
	}
	double head_x = shoulder_x + d * H * 0.03, head_y = shoulder_y - H * 0.09;
	filledCircleRGBA(surface, (int)head_x, (int)head_y, (int)(H * 0.065), 42, 48, 66, alpha);
	static const Uint8 eye[3] = { 255, 130, 70 }, core[3] = { 110, 220, 255 };
	weapon_glow(surface, head_x + d * H * 0.03, head_y, 2.5, eye, 230.0 * event->fade);
	weapon_glow(surface, x + d * H * 0.02, (hip_y + shoulder_y) / 2.0, H * 0.025, core,
		200.0 * event->fade * (0.7 + 0.3 * sin(event->t * 2.0)));
}

// A clock tower rises and its lit face spins fast; every few seconds the bell sends out rings.
static void hill_clocktower(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = hill_side(event, 0.1);
	double ground = hill_ground(event, cx);
	double w = s * 0.035, h = s * 0.22 * smooth_step(0.0, 0.1, event->progress);
	if(h < 4.0) return;
	double top = ground - h;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	double lit = smooth_step(0.1, 0.18, event->progress) * (1.0 - smooth_step(0.92, 0.97, event->progress));
	boxRGBA(surface, (int)(cx - w), (int)top, (int)(cx + w), (int)ground, 26, 30, 44, alpha);
	boxRGBA(surface, (int)(cx - w), (int)top, (int)(cx - w * 0.6), (int)ground, 40, 46, 64, alpha);
	filledTrigonRGBA(surface, (int)(cx - w - 3), (int)top, (int)(cx + w + 3), (int)top, (int)cx,
		(int)(top - w * 1.7), 16, 18, 28, alpha);
	double face_y = top + w * 1.15, face_r = w * 0.78;
	filledCircleRGBA(surface, (int)cx, (int)face_y, (int)face_r + 2, 40, 46, 64, alpha);
	filledCircleRGBA(surface, (int)cx, (int)face_y, (int)face_r, (Uint8)(30 + 225 * lit), (Uint8)(34 + 198 * lit),
		(Uint8)(46 + 124 * lit), alpha);
	for(int tick = 0; tick < 12; tick++) {
		double a = tick * SCENE_TAU / 12.0;
		lineRGBA(surface, (int)(cx + cos(a) * face_r * 0.8), (int)(face_y + sin(a) * face_r * 0.8),
			(int)(cx + cos(a) * face_r * 0.95), (int)(face_y + sin(a) * face_r * 0.95), 40, 32, 28, alpha);
	}
	double minute = event->t * 0.6 - SCENE_PI / 2.0, hour = event->t * 0.05 - SCENE_PI / 2.0;
	thickLineRGBA(surface, (int)cx, (int)face_y, (int)(cx + cos(minute) * face_r * 0.82),
		(int)(face_y + sin(minute) * face_r * 0.82), 1, 30, 24, 22, alpha);
	thickLineRGBA(surface, (int)cx, (int)face_y, (int)(cx + cos(hour) * face_r * 0.5),
		(int)(face_y + sin(hour) * face_r * 0.5), 2, 30, 24, 22, alpha);
	for(int k = 0; k < 3; k++) {
		double wy = face_y + face_r + w * (0.7 + 0.7 * k);
		if(wy + 6 > ground) break;
		boxRGBA(surface, (int)cx - 2, (int)wy, (int)cx + 2, (int)wy + 7, 255, 214, 130,
			weapon_alpha(220.0 * lit * event->fade));
	}
	double local = fmod(event->t, 8.0);
	for(int ring = 0; ring < 3; ring++) {
		double age = local - ring * 0.8;
		if(age <= 0.0 || age >= 2.2) continue;
		double u = age / 2.2;
		int radius = (int)(face_r + u * s * 0.12);
		Uint8 a = weapon_alpha(150.0 * (1.0 - u) * lit * event->fade);
		circleRGBA(surface, (int)cx, (int)face_y, radius, 255, 220, 150, a);
		circleRGBA(surface, (int)cx, (int)face_y, radius + 1, 255, 220, 150, a);
	}
}

// Point `u` (0..1) of a drone formation shape, in -1..1 units.
static void drone_shape(int shape, double u, double *px, double *py) {
	switch(shape) {
		case 0:
			*px = cos(u * SCENE_TAU);
			*py = sin(u * SCENE_TAU);
			break;
		case 1: {
			double f = u * 10.0;
			int k = (int)f % 10;
			double frac = f - floor(f);
			double a0 = -SCENE_PI / 2.0 + k * SCENE_PI / 5.0, a1 = a0 + SCENE_PI / 5.0;
			double r0 = k % 2 == 0 ? 1.0 : 0.42, r1 = k % 2 == 0 ? 0.42 : 1.0;
			*px = cos(a0) * r0 * (1.0 - frac) + cos(a1) * r1 * frac;
			*py = sin(a0) * r0 * (1.0 - frac) + sin(a1) * r1 * frac;
			break;
		}
		case 2: {
			double a = u * SCENE_TAU;
			*px = 16.0 * pow(sin(a), 3.0) / 17.0;
			*py = -(13.0 * cos(a) - 5.0 * cos(2.0 * a) - 2.0 * cos(3.0 * a) - cos(4.0 * a)) / 17.0;
			break;
		}
		default:
			*px = -1.0 + 2.0 * u;
			*py = 0.35 * sin(u * SCENE_TAU * 2.0);
			break;
	}
}

// A swarm of drone lights lifts off the ridge and forms a ring, a star, a heart and a wave.
static void hill_drones(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	const int drones = 48;
	static const Uint8 palette[4][3] = {
		{ 255, 200, 110 }, { 120, 220, 255 }, { 255, 120, 160 }, { 150, 255, 160 } };
	double cx = surface->w * (0.5 + 0.3 * sin(event->progress * SCENE_TAU * 0.8));
	double cy = surface->h * 0.1, radius = event->scale * 0.075;
	double st = event->t / 6.5;
	int a = (int)st % 4, b = (a + 1) % 4;
	double blend = smooth_step(0.75, 1.0, st - floor(st));
	const Uint8 *color = palette[blend > 0.5 ? b : a];
	for(int i = 0; i < drones; i++) {
		double u = (double)i / drones;
		double ax, ay, bx, by;
		drone_shape(a, u, &ax, &ay);
		drone_shape(b, u, &bx, &by);
		double target_x = cx + (ax + (bx - ax) * blend) * radius;
		double target_y = cy + (ay + (by - ay) * blend) * radius;
		double r = hill_random(event, i, 1);
		double gx = surface->w * (0.08 + 0.84 * (i + 0.5) / drones);
		double gy = ground_y(surface, RIDGE_NEAR, gx);
		double up = smooth_step(0.02 + 0.08 * r, 0.16 + 0.08 * r, event->progress)
			* (1.0 - smooth_step(0.84 + 0.08 * r, 0.94 + 0.05 * r, event->progress));
		if(up <= 0.0) continue;
		weapon_glow(surface, gx + (target_x - gx) * up, gy + (target_y - gy) * up, 2.2, color,
			235.0 * event->fade * (0.75 + 0.25 * sin(event->t * 3.0 + i)));
	}
}

// A cave mouth in a mound: a stream of bats pours out and fans over the sky.
static void hill_bats(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double cx = hill_side(event, 0.12);
	double ground = hill_ground(event, cx);
	double mound_w = s * 0.07, mound_h = s * 0.045;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	filledEllipseRGBA(surface, (int)cx, (int)(ground + 2), (int)mound_w, (int)mound_h, 18, 22, 32, alpha);
	filledEllipseRGBA(surface, (int)(cx + d * mound_w * 0.2), (int)(ground - mound_h * 0.1), (int)(mound_w * 0.28),
		(int)(mound_h * 0.5), 4, 5, 8, alpha);
	const int bats = 44;
	double active = smooth_step(0.05, 0.25, event->progress);
	double away = 1.0 - smooth_step(0.85, 0.95, event->progress);
	for(int i = 0; i < bats; i++) {
		if(i / (double)bats > active) continue;
		double r1 = hill_random(event, i, 1), r2 = hill_random(event, i, 2), r3 = hill_random(event, i, 3);
		double u = fmod(event->t * (0.08 + 0.04 * r3) + r1, 1.0);
		double bx = cx + d * (mound_w * 0.1 + u * s * (0.35 + 0.6 * r2)) + sin(u * 9.0 + i) * s * 0.02 * u;
		double by = ground - mound_h * 0.3 - u * s * (0.25 + 0.5 * r1) + sin(u * 13.0 + i * 2.1) * s * 0.012;
		double vis = smooth_step(0.0, 0.05, u) * (1.0 - smooth_step(0.7, 1.0, u)) * away * event->fade;
		double size = s * (0.011 + 0.006 * r3) * (0.5 + 0.9 * u);
		double flap = sin(event->t * 13.0 + i * 1.7);
		Uint8 a = weapon_alpha(240.0 * vis);
		for(int side = -1; side <= 1; side += 2) {
			double wx = bx + side * size * 1.4, wy = by - flap * size * 0.9;
			thickLineRGBA(surface, (int)bx, (int)by, (int)wx, (int)wy, 2, 8, 10, 16, a);
			thickLineRGBA(surface, (int)wx, (int)wy, (int)(bx + side * size * 2.0), (int)(by - flap * size * 0.5 + size * 0.3),
				1, 8, 10, 16, a);
		}
		filledCircleRGBA(surface, (int)bx, (int)by, (int)(size * 0.35) + 1, 8, 10, 16, a);
	}
}

// A stone gate wakes: the arch fills with a swirling light and travellers step out of it.
static void hill_gate(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double cx = hill_side(event, 0.12);
	double ground = hill_ground(event, cx);
	double height = s * 0.12, half = s * 0.045, post = s * 0.015;
	double open = smooth_step(0.06, 0.16, event->progress) * (1.0 - smooth_step(0.9, 0.96, event->progress));
	double mid_y = ground - height * 0.5;
	static const Uint8 layers[5][3] = {
		{ 80, 50, 190 }, { 110, 90, 230 }, { 120, 150, 250 }, { 170, 210, 255 }, { 240, 250, 255 } };
	if(open > 0.0) {
		for(int k = 0; k < 5; k++) {
			double f = 1.0 - k * 0.17;
			filledEllipseRGBA(surface, (int)cx, (int)mid_y, (int)(half * f), (int)(height * 0.5 * f),
				layers[k][0], layers[k][1], layers[k][2], weapon_alpha(120.0 * open * event->fade));
		}
		for(int j = 0; j < 8; j++) {
			double a = event->t * 1.6 + j * 0.78;
			double rad = 0.15 + 0.85 * fmod(j * 0.37 + event->t * 0.2, 1.0);
			weapon_glow(surface, cx + cos(a) * half * 0.9 * rad, mid_y + sin(a) * height * 0.45 * rad, 1.5,
				layers[3], 200.0 * open * event->fade);
		}
	}
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	boxRGBA(surface, (int)(cx - half - post), (int)(ground - height), (int)(cx - half), (int)ground, 28, 32, 44, alpha);
	boxRGBA(surface, (int)(cx + half), (int)(ground - height), (int)(cx + half + post), (int)ground, 28, 32, 44, alpha);
	boxRGBA(surface, (int)(cx - half - post * 1.3), (int)(ground - height - post), (int)(cx + half + post * 1.3),
		(int)(ground - height), 34, 38, 52, alpha);
	weapon_glow(surface, cx, ground - height - post * 0.5, 2.0, layers[3], 200.0 * open * event->fade);
	for(int k = 0; k < 5; k++) {
		double u = (event->progress - (0.2 + 0.12 * k)) / 0.4;
		if(u <= 0.0 || u >= 1.0) continue;
		double x = cx + d * (half * 0.2 + u * s * 0.7);
		double vis = smooth_step(0.0, 0.08, u) * (1.0 - smooth_step(0.85, 1.0, u));
		hill_walker(surface, x, hill_ground(event, x), s * 0.036, d, event->t * 5.0 + k * 2.0, k % 2 == 0,
			event->fade * vis, event->t, k);
	}
}

// A striped lighthouse rises on the ridge; its lamp turns, sweeping two beams across the sky.
static void hill_lighthouse(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = hill_side(event, 0.1);
	double ground = hill_ground(event, cx);
	double h = s * 0.16 * smooth_step(0.0, 0.1, event->progress), w = s * 0.022;
	if(h < 4.0) return;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	double lit = smooth_step(0.1, 0.18, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
	double lamp_y = ground - h - w * 0.5;
	static const Uint8 warm[3] = { 255, 235, 170 };
	double a = event->t * 1.1;
	for(int band = 0; band < 6; band++) {
		double y0 = ground - h * band / 6.0, y1 = ground - h * (band + 1) / 6.0;
		double w0 = w * (1.0 - 0.45 * band / 6.0), w1 = w * (1.0 - 0.45 * (band + 1) / 6.0);
		Uint8 r = band % 2 ? 150 : 120, g = band % 2 ? 156 : 48, b = band % 2 ? 172 : 52;
		filledTrigonRGBA(surface, (int)(cx - w0), (int)y0, (int)(cx + w0), (int)y0, (int)(cx - w1), (int)y1, r, g, b, alpha);
		filledTrigonRGBA(surface, (int)(cx + w0), (int)y0, (int)(cx + w1), (int)y1, (int)(cx - w1), (int)y1, r, g, b, alpha);
	}
	boxRGBA(surface, (int)(cx - w * 0.75), (int)(lamp_y - w * 0.2), (int)(cx + w * 0.75), (int)(lamp_y + w * 0.5), 30, 34, 48, alpha);
	filledTrigonRGBA(surface, (int)(cx - w * 0.9), (int)(lamp_y - w * 0.2), (int)(cx + w * 0.9), (int)(lamp_y - w * 0.2),
		(int)cx, (int)(lamp_y - w * 1.0), 120, 48, 52, alpha);
	if(lit <= 0.0) return;
	double sweep = cos(a), reach = s * 0.9 * fabs(sweep);
	for(int side = -1; side <= 1; side += 2) {
		double ex = cx + side * (sweep >= 0.0 ? 1 : -1) * reach;
		filledTrigonRGBA(surface, (int)cx, (int)lamp_y, (int)ex, (int)(lamp_y - s * 0.02), (int)ex, (int)(lamp_y + s * 0.02),
			255, 245, 200, weapon_alpha(40.0 * lit * fabs(sweep) * event->fade));
	}
	weapon_glow(surface, cx, lamp_y + w * 0.15, w * (0.4 + 0.8 * pow(fabs(sin(a)), 6.0)), warm, 235.0 * lit * event->fade);
}

// A cherry tree grows and comes into blossom, then lets its petals fall in a slow drift.
static void hill_cherry(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double cx = hill_side(event, 0.13);
	double ground = hill_ground(event, cx);
	double grow = smooth_step(0.0, 0.18, event->progress);
	double bloom = smooth_step(0.12, 0.4, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
	double trunk = s * 0.075 * grow;
	if(trunk < 3.0) return;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	double top_x = cx + d * s * 0.01, top_y = ground - trunk;
	thickLineRGBA(surface, (int)cx, (int)ground, (int)top_x, (int)top_y, 5, 52, 38, 40, alpha);
	for(int b = -1; b <= 1; b += 2) {
		thickLineRGBA(surface, (int)top_x, (int)(top_y + trunk * 0.25), (int)(top_x + b * s * 0.04),
			(int)(top_y - trunk * 0.1), 3, 52, 38, 40, alpha);
	}
	for(int blob = 0; blob < 11; blob++) {
		double ang = blob * 0.57, rad = s * (0.018 + 0.02 * ((blob * 7) % 5) / 5.0);
		double bx = top_x + cos(ang * 2.1) * s * 0.045, by = top_y - trunk * 0.1 + sin(ang * 1.3) * s * 0.025;
		filledCircleRGBA(surface, (int)bx, (int)by, (int)(rad * bloom) + 1, (Uint8)(214 + blob % 3 * 12), (Uint8)(130 + blob % 4 * 14),
			(Uint8)(165 + blob % 3 * 10), weapon_alpha(200.0 * bloom * event->fade));
	}
	for(int petal = 0; petal < 34; petal++) {
		double r = hill_random(event, petal, 1), r2 = hill_random(event, petal, 2);
		double u = fmod(event->t * (0.07 + 0.05 * r2) + r, 1.0);
		double px = top_x + (r2 - 0.5) * s * 0.12 + u * d * s * (0.1 + 0.1 * r) + sin(event->t * 1.4 + petal) * s * 0.012;
		double py = top_y - s * 0.02 + u * (ground - top_y + s * 0.02);
		double vis = smooth_step(0.0, 0.1, u) * (1.0 - smooth_step(0.85, 1.0, u)) * bloom * event->fade;
		filledEllipseRGBA(surface, (int)px, (int)py, 2, 1, 255, 190, 214, weapon_alpha(230.0 * vis));
	}
}

// A wolf on a rock howls at a rising moon; rings of sound spread from its muzzle.
static void hill_wolf(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double cx = hill_side(event, 0.14);
	double ground = hill_ground(event, cx);
	double rise = smooth_step(0.0, 0.3, event->progress);
	double moon_x = cx + d * s * 0.16, moon_y = ground - s * 0.1 - rise * s * 0.22, moon_r = s * 0.05;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	filledCircleRGBA(surface, (int)moon_x, (int)moon_y, (int)(moon_r * 1.7), 190, 205, 235, weapon_alpha(26.0 * event->fade));
	filledCircleRGBA(surface, (int)moon_x, (int)moon_y, (int)moon_r, 226, 232, 240, weapon_alpha(215.0 * event->fade));
	filledCircleRGBA(surface, (int)(moon_x - moon_r * 0.3), (int)(moon_y + moon_r * 0.1), (int)(moon_r * 0.22), 196, 204, 216,
		weapon_alpha(200.0 * event->fade));
	double wy = ground - s * 0.03;
	filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(s * 0.05), (int)(s * 0.035), 18, 22, 32, alpha);
	double howl = smooth_step(0.0, 0.3, fmod(event->t, 6.0)) * (1.0 - smooth_step(3.6, 4.2, fmod(event->t, 6.0)));
	double neck_x = cx + d * s * 0.016, neck_y = wy - s * 0.016;
	double head_x = neck_x + d * s * (0.012 + 0.012 * howl), head_y = neck_y - s * (0.012 + 0.016 * howl);
	double nose_x = head_x + d * s * (0.014 - 0.002 * howl), nose_y = head_y - s * (0.004 + 0.012 * howl);
	filledEllipseRGBA(surface, (int)cx, (int)(wy - s * 0.008), (int)(s * 0.022), (int)(s * 0.01), 8, 10, 16, alpha);
	for(int leg = -1; leg <= 1; leg += 2) {
		thickLineRGBA(surface, (int)(cx + leg * s * 0.014), (int)(wy - s * 0.004), (int)(cx + leg * s * 0.016), (int)(wy + s * 0.012), 2, 8, 10, 16, alpha);
	}
	thickLineRGBA(surface, (int)(cx - d * s * 0.02), (int)(wy - s * 0.01), (int)(cx - d * s * 0.036), (int)(wy + s * 0.006), 3, 8, 10, 16, alpha);
	thickLineRGBA(surface, (int)(cx + d * s * 0.012), (int)(wy - s * 0.008), (int)head_x, (int)head_y, 5, 8, 10, 16, alpha);
	thickLineRGBA(surface, (int)head_x, (int)head_y, (int)nose_x, (int)nose_y, 4, 8, 10, 16, alpha);
	filledTrigonRGBA(surface, (int)head_x, (int)(head_y - s * 0.002), (int)(head_x - d * s * 0.004), (int)(head_y - s * 0.012),
		(int)(head_x + d * s * 0.002), (int)(head_y - s * 0.002), 8, 10, 16, alpha);
	for(int ring = 0; ring < 3; ring++) {
		double age = fmod(event->t, 6.0) - 0.4 - ring * 0.6;
		if(age <= 0.0 || age >= 2.2) continue;
		double u = age / 2.2;
		ellipseRGBA(surface, (int)nose_x, (int)nose_y, (int)(s * 0.01 + u * s * 0.07), (int)(s * 0.007 + u * s * 0.05),
			200, 215, 240, weapon_alpha(110.0 * (1.0 - u) * howl * event->fade));
	}
}

// A pumpkin patch: round pumpkins swell out of the ground and their carved faces light up.
static void hill_pumpkins(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int count = 9;
	for(int i = 0; i < count; i++) {
		double r = hill_random(event, i, 1);
		double x = surface->w * (0.06 + 0.88 * (i + 0.5 + (r - 0.5) * 0.5) / count);
		double grow = smooth_step(0.02 + i * 0.03, 0.14 + i * 0.03, event->progress);
		if(grow <= 0.0) continue;
		double ground = hill_ground(event, x);
		double rad = s * (0.017 + 0.012 * r) * grow;
		double lit = smooth_step(0.2 + i * 0.03, 0.28 + i * 0.03, event->progress) * (1.0 - smooth_step(0.9, 0.97, event->progress));
		double flicker = 0.8 + 0.2 * sin(event->t * 9.0 + i * 2.3);
		int cy = (int)(ground - rad * 0.8);
		Uint8 alpha = weapon_alpha(238.0 * event->fade);
		filledEllipseRGBA(surface, (int)x, cy, (int)rad + 1, (int)(rad * 0.82) + 1, 118, 54, 20, alpha);
		filledEllipseRGBA(surface, (int)(x - rad * 0.5), cy, (int)(rad * 0.5) + 1, (int)(rad * 0.8) + 1, 138, 66, 24, alpha);
		filledEllipseRGBA(surface, (int)(x + rad * 0.5), cy, (int)(rad * 0.5) + 1, (int)(rad * 0.8) + 1, 108, 48, 18, alpha);
		thickLineRGBA(surface, (int)x, (int)(cy - rad * 0.8), (int)(x + rad * 0.15), (int)(cy - rad * 1.2), 2, 52, 62, 30, alpha);
		if(lit <= 0.0 || rad < 5.0) continue;
		Uint8 glow = weapon_alpha(235.0 * lit * flicker * event->fade);
		filledCircleRGBA(surface, (int)x, cy, (int)(rad * 1.8), 255, 150, 50, weapon_alpha(26.0 * lit * flicker * event->fade));
		filledTrigonRGBA(surface, (int)(x - rad * 0.55), (int)(cy - rad * 0.1), (int)(x - rad * 0.15), (int)(cy - rad * 0.1),
			(int)(x - rad * 0.35), (int)(cy - rad * 0.4), 255, 205, 90, glow);
		filledTrigonRGBA(surface, (int)(x + rad * 0.15), (int)(cy - rad * 0.1), (int)(x + rad * 0.55), (int)(cy - rad * 0.1),
			(int)(x + rad * 0.35), (int)(cy - rad * 0.4), 255, 205, 90, glow);
		lineRGBA(surface, (int)(x - rad * 0.5), (int)(cy + rad * 0.25), (int)(x - rad * 0.2), (int)(cy + rad * 0.4), 255, 205, 90, glow);
		lineRGBA(surface, (int)(x - rad * 0.2), (int)(cy + rad * 0.4), (int)(x + rad * 0.2), (int)(cy + rad * 0.4), 255, 205, 90, glow);
		lineRGBA(surface, (int)(x + rad * 0.2), (int)(cy + rad * 0.4), (int)(x + rad * 0.5), (int)(cy + rad * 0.25), 255, 205, 90, glow);
	}
}

// A small castle with crenellated towers, lit windows, flickering gate torches and waving flags.
static void hill_castle(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double cx = hill_side(event, 0.14);
	double ground = hill_ground(event, cx) + 2.0;
	double build = smooth_step(0.0, 0.14, event->progress);
	double lit = smooth_step(0.14, 0.24, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
	if(build < 0.05) return;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	double keep_w = s * 0.05, keep_h = s * 0.09 * build, tower_w = s * 0.026, tower_h = s * 0.12 * build;
	boxRGBA(surface, (int)(cx - keep_w), (int)(ground - keep_h), (int)(cx + keep_w), (int)ground, 26, 30, 44, alpha);
	for(int side = -1; side <= 1; side += 2) {
		double tx = cx + side * (keep_w + tower_w * 0.6);
		boxRGBA(surface, (int)(tx - tower_w), (int)(ground - tower_h), (int)(tx + tower_w), (int)ground, 32, 36, 52, alpha);
		filledTrigonRGBA(surface, (int)(tx - tower_w * 1.2), (int)(ground - tower_h), (int)(tx + tower_w * 1.2), (int)(ground - tower_h),
			(int)tx, (int)(ground - tower_h - tower_w * 2.2), 70, 34, 44, alpha);
		double flag_top = ground - tower_h - tower_w * 2.2;
		thickLineRGBA(surface, (int)tx, (int)flag_top, (int)tx, (int)(flag_top - s * 0.02), 1, 20, 22, 30, alpha);
		double wave = sin(event->t * 4.0 + side) * s * 0.004;
		filledTrigonRGBA(surface, (int)tx, (int)(flag_top - s * 0.02), (int)tx, (int)(flag_top - s * 0.008),
			(int)(tx + d * s * 0.016), (int)(flag_top - s * 0.014 + wave), 200, 60, 70, alpha);
		boxRGBA(surface, (int)(tx - 2), (int)(ground - tower_h * 0.6), (int)(tx + 2), (int)(ground - tower_h * 0.6 + 6), 255, 214, 130,
			weapon_alpha(235.0 * lit * event->fade));
	}
	for(int m = 0; m < 6; m++) {
		double mx = cx - keep_w + (2.0 * keep_w) * (m + 0.5) / 6.0;
		boxRGBA(surface, (int)(mx - 3), (int)(ground - keep_h - 4), (int)(mx + 3), (int)(ground - keep_h), 26, 30, 44, alpha);
	}
	filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(keep_w * 0.22), (int)(keep_h * 0.3), 6, 8, 14, alpha);
	for(int win = 0; win < 3; win++) {
		boxRGBA(surface, (int)(cx - keep_w * 0.7 + win * keep_w * 0.7) - 1, (int)(ground - keep_h * 0.7), (int)(cx - keep_w * 0.7 + win * keep_w * 0.7) + 2,
			(int)(ground - keep_h * 0.7) + 6, 255, 214, 130, weapon_alpha(235.0 * lit * (0.85 + 0.15 * sin(event->t * 6.0 + win)) * event->fade));
	}
	for(int side = -1; side <= 1; side += 2) {
		double fx = cx + side * keep_w * 0.45, fy = ground - keep_h * 0.28;
		double flame = 0.8 + 0.2 * sin(event->t * 12.0 + side * 3.0);
		thickLineRGBA(surface, (int)fx, (int)(fy + 4), (int)fx, (int)(fy + 10), 2, 40, 30, 28, alpha);
		filledTrigonRGBA(surface, (int)(fx - 3), (int)(fy + 4), (int)(fx + 3), (int)(fy + 4), (int)fx, (int)(fy - 5 * flame), 255, 150, 50,
			weapon_alpha(230.0 * lit * event->fade));
		filledCircleRGBA(surface, (int)fx, (int)fy, 10, 255, 150, 60, weapon_alpha(26.0 * lit * flame * event->fade));
	}
}

// A space elevator: a thin cable climbs from a base station to the top of the sky, with lit climbers running along it.
static void hill_spaceelevator(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = hill_side(event, 0.17);
	double ground = hill_ground(event, cx);
	double reach = smooth_step(0.02, 0.2, event->progress);
	double top = ground - (ground + s * 0.05) * reach;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	static const Uint8 cyan[3] = { 150, 235, 255 };
	thickLineRGBA(surface, (int)cx, (int)ground, (int)cx, (int)top, 3, 120, 200, 230, weapon_alpha(24.0 * event->fade));
	lineRGBA(surface, (int)cx, (int)ground, (int)cx, (int)top, 215, 240, 255, weapon_alpha(200.0 * event->fade));
	filledTrigonRGBA(surface, (int)(cx - s * 0.03), (int)(ground + 2), (int)(cx + s * 0.03), (int)(ground + 2),
		(int)cx, (int)(ground - s * 0.025), 28, 34, 48, alpha);
	boxRGBA(surface, (int)(cx - s * 0.04), (int)(ground - 2), (int)(cx + s * 0.04), (int)(ground + 3), 22, 26, 38, alpha);
	if(fmod(event->t, 1.4) < 0.4) {
		static const Uint8 red[3] = { 255, 70, 60 };
		weapon_glow(surface, cx, ground - s * 0.028, 2.5, red, 235.0 * event->fade);
	}
	for(int car = 0; car < 4; car++) {
		double u = fmod(event->t * (0.045 + 0.012 * car) + car * 0.27, 1.0);
		if(car % 2) u = 1.0 - u;
		double y = ground - 8.0 - u * (ground - top - 16.0);
		if(reach < 0.98 && y < top) continue;
		boxRGBA(surface, (int)(cx - 3), (int)(y - 6), (int)(cx + 3), (int)(y + 6), 58, 66, 90, alpha);
		boxRGBA(surface, (int)(cx - 1), (int)(y - 4), (int)(cx + 1), (int)(y - 1), 255, 224, 150, alpha);
		weapon_glow(surface, cx, y + 7.0, 2.0, cyan, 200.0 * event->fade);
	}
}

// A small airstrip: runway lights come on in a row and a little plane lands, taxis and lifts off again.
static void hill_airstrip(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	static const Uint8 white[3] = { 255, 245, 210 };
	const int lights = 12;
	double lit = smooth_step(0.04, 0.14, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
	double x0 = surface->w * 0.12, x1 = surface->w * 0.88;
	for(int i = 0; i < lights; i++) {
		double x = x0 + (x1 - x0) * i / (lights - 1.0);
		double on = smooth_step(i / (double)lights * 0.5, i / (double)lights * 0.5 + 0.1, lit);
		weapon_glow(surface, x, hill_ground(event, x) - 1.0, 2.0, white, 220.0 * on * event->fade * (0.75 + 0.25 * sin(event->t * 4.0 + i)));
	}
	double u = (event->progress - 0.2) / 0.6;
	if(u <= 0.0 || u >= 1.0) return;
	double run = d > 0 ? u : 1.0 - u;
	double px = x0 + (x1 - x0) * run;
	double air = u < 0.5 ? pow(1.0 - u * 2.0, 2.0) : pow((u - 0.5) * 2.0, 2.0) * 0.0;
	double py = hill_ground(event, px) - s * 0.012 - air * s * 0.18;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	filledEllipseRGBA(surface, (int)px, (int)py, (int)(s * 0.02), (int)(s * 0.006), 30, 34, 48, alpha);
	thickLineRGBA(surface, (int)(px - d * s * 0.02), (int)py, (int)(px - d * s * 0.03), (int)(py - s * 0.01), 2, 30, 34, 48, alpha);
	thickLineRGBA(surface, (int)px, (int)(py - s * 0.004), (int)px, (int)(py + s * 0.004), 3, 60, 66, 86, alpha);
	weapon_glow(surface, px, py - s * 0.007, 2.0, white, 200.0 * event->fade);
}

// Hot springs: pools of glowing water on the ridge breathe slow clouds of steam.
static void hill_hotsprings(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int pools = 4;
	double warm = smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
	for(int i = 0; i < pools; i++) {
		double x = surface->w * (0.12 + 0.76 * (i + 0.5 * hill_random(event, i, 1)) / pools);
		double ground = hill_ground(event, x);
		double w = s * (0.03 + 0.015 * hill_random(event, i, 2));
		filledEllipseRGBA(surface, (int)x, (int)ground, (int)(w * 1.2), (int)(w * 0.2) + 1, 30, 36, 44, weapon_alpha(235.0 * event->fade));
		filledEllipseRGBA(surface, (int)x, (int)ground, (int)w, (int)(w * 0.14) + 1, 80, 190, 210,
			weapon_alpha(190.0 * warm * event->fade * (0.8 + 0.2 * sin(event->t * 1.5 + i))));
		for(int puff = 0; puff < 9; puff++) {
			double u = fmod(event->t / 5.0 + puff / 9.0 + i * 0.17, 1.0);
			double px = x + sin(u * 4.0 + puff + i) * w * 0.5 + event->direction * u * s * 0.03;
			double py = ground - u * s * 0.16;
			filledCircleRGBA(surface, (int)px, (int)py, (int)(s * (0.006 + 0.016 * u)), 215, 225, 235,
				weapon_alpha(70.0 * (1.0 - u) * warm * event->fade));
		}
	}
}

// A night market: stalls under striped awnings, strings of bulbs between poles, and a few shoppers.
static void hill_market(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int stalls = 6;
	double lit = smooth_step(0.05, 0.16, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
	Uint8 alpha = weapon_alpha(236.0 * event->fade);
	static const Uint8 bulbs[4][3] = { { 255, 210, 120 }, { 255, 140, 120 }, { 140, 230, 255 }, { 190, 255, 150 } };
	double prev_x = 0, prev_y = 0;
	for(int i = 0; i < stalls; i++) {
		double x = surface->w * (0.12 + 0.76 * (i + 0.5) / stalls);
		double ground = hill_ground(event, x);
		double rise = smooth_step(0.0 + i * 0.015, 0.1 + i * 0.015, event->progress);
		double w = s * 0.036, h = s * 0.034 * rise;
		boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x + w), (int)ground, 24, 28, 40, alpha);
		for(int stripe = 0; stripe < 4; stripe++) {
			double sx = x - w + stripe * w * 0.5;
			filledTrigonRGBA(surface, (int)sx, (int)(ground - h), (int)(sx + w * 0.5), (int)(ground - h), (int)(sx + w * 0.25), (int)(ground - h - s * 0.012),
				stripe % 2 ? 210 : 150, stripe % 2 ? 200 : 54, stripe % 2 ? 190 : 60, alpha);
		}
		filledCircleRGBA(surface, (int)x, (int)(ground - h * 0.45), 8, 255, 190, 100, weapon_alpha(34.0 * lit * event->fade));
		boxRGBA(surface, (int)(x - w * 0.5), (int)(ground - h * 0.6), (int)(x + w * 0.5), (int)(ground - h * 0.3), 255, 214, 130,
			weapon_alpha(220.0 * lit * event->fade * (0.88 + 0.12 * sin(event->t * 5.0 + i))));
		if(i > 0) {
			for(int k = 1; k < 6; k++) {
				double u = k / 6.0;
				double bx = prev_x + (x - prev_x) * u, by = prev_y + (ground - h - prev_y) * u + sin(3.14159 * u) * s * 0.012;
				weapon_glow(surface, bx, by, 1.6, bulbs[(i + k) % 4], 220.0 * lit * event->fade);
			}
		}
		prev_x = x;
		prev_y = ground - h - s * 0.012;
	}
}

// A pond of floating lotus lights: little flames drift across dark water and ripples spread around them.
static void hill_lotus(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * 0.5, ground = hill_ground(event, cx) + s * 0.008;
	double rw = s * 0.28, rh = s * 0.014;
	double fill = smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.93, 0.98, event->progress));
	filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(rw * fill), (int)(rh * fill) + 1, 16, 26, 40, weapon_alpha(220.0 * event->fade));
	ellipseRGBA(surface, (int)cx, (int)ground, (int)(rw * fill), (int)(rh * fill) + 1, 90, 130, 160, weapon_alpha(110.0 * event->fade));
	static const Uint8 pink[3] = { 255, 170, 200 };
	for(int i = 0; i < 16; i++) {
		double r1 = hill_random(event, i, 1), r2 = hill_random(event, i, 2);
		double u = fmod(event->t * (0.012 + 0.01 * r2) + r1, 1.0);
		double lx = cx - rw * 0.9 + rw * 1.8 * u;
		double ly = ground + (r2 - 0.5) * rh * 1.2;
		double vis = smooth_step(0.0, 0.1, u) * (1.0 - smooth_step(0.9, 1.0, u)) * fill;
		double ring = fmod(event->t * 0.6 + r1 * 3.0, 1.0);
		ellipseRGBA(surface, (int)lx, (int)ly, (int)(3 + ring * 10), (int)(1 + ring * 3), 190, 210, 235, weapon_alpha(90.0 * (1.0 - ring) * vis * event->fade));
		weapon_glow(surface, lx, ly - 2.0, 2.0, pink, 230.0 * vis * event->fade * (0.8 + 0.2 * sin(event->t * 6.0 + i)));
	}
}

// A bonfire festival: a big fire in the middle, a ring of dancers circling it, sparks rising.
static void hill_bonfire(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * 0.5, ground = hill_ground(event, cx);
	double build = smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.9, 0.97, event->progress));
	Uint8 alpha = weapon_alpha(236.0 * event->fade);
	filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(s * 0.16), (int)(s * 0.02), 255, 150, 60, weapon_alpha(30.0 * build * event->fade));
	double flare = 0.85 + 0.15 * sin(event->t * 9.0);
	for(int tongue = 0; tongue < 6; tongue++) {
		double h = s * (0.04 + 0.012 * (tongue % 3)) * build * (0.7 + 0.3 * sin(event->t * (8.0 + tongue) + tongue)) * flare;
		double fx = cx + (tongue - 2.5) * s * 0.006;
		filledTrigonRGBA(surface, (int)(fx - s * 0.01), (int)ground, (int)(fx + s * 0.01), (int)ground, (int)(fx + sin(event->t * 5.0 + tongue) * 3.0), (int)(ground - h), 250, 120, 40, weapon_alpha(215.0 * event->fade));
		filledTrigonRGBA(surface, (int)(fx - s * 0.005), (int)ground, (int)(fx + s * 0.005), (int)ground, (int)fx, (int)(ground - h * 0.7), 255, 220, 130, weapon_alpha(225.0 * event->fade));
	}
	for(int spark = 0; spark < 14; spark++) {
		double u = fmod(event->t / 2.2 + spark * 0.071, 1.0);
		filledCircleRGBA(surface, (int)(cx + sin(spark * 5.0 + event->t) * s * 0.03 * u), (int)(ground - s * 0.04 - u * s * 0.18), 1, 255, 210, 110,
			weapon_alpha(240.0 * (1.0 - u) * build * event->fade));
	}
	for(int dancer = 0; dancer < 8; dancer++) {
		double a = event->t * 0.7 + dancer * SCENE_TAU / 8.0;
		double px = cx + cos(a) * s * 0.11, depth = sin(a);
		double py = ground + depth * s * 0.012;
		double arms = sin(event->t * 6.0 + dancer) * s * 0.008;
		double hgt = s * 0.03 * build;
		Uint8 shade = (Uint8)(14 + 12 * (depth + 1.0));
		thickLineRGBA(surface, (int)px, (int)py, (int)px, (int)(py - hgt * 0.6), 3, shade, shade + 4, shade + 12, alpha);
		thickLineRGBA(surface, (int)(px - hgt * 0.4), (int)(py - hgt * 0.5 + arms), (int)(px + hgt * 0.4), (int)(py - hgt * 0.5 - arms), 1, shade, shade + 4, shade + 12, alpha);
		filledCircleRGBA(surface, (int)px, (int)(py - hgt * 0.75), (int)(hgt * 0.14) + 1, shade, shade + 4, shade + 12, alpha);
	}
}

// A waterfall spills down the near ridge, with mist at the bottom and a pale stream running away.
static void hill_waterfall(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = hill_side(event, 0.16), ground = hill_ground(event, cx);
	double flow = smooth_step(0.0, 0.1, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
	double h = s * 0.11, w = s * 0.014;
	Uint8 alpha = weapon_alpha(236.0 * event->fade);
	filledTrigonRGBA(surface, (int)(cx - s * 0.05), (int)(ground + 2), (int)(cx + s * 0.05), (int)(ground + 2), (int)cx, (int)(ground - h * 1.15), 24, 28, 40, alpha);
	for(int strand = 0; strand < 5; strand++) {
		double sx = cx + (strand - 2) * w * 0.4;
		for(int seg = 0; seg < 10; seg++) {
			double u = fmod(event->t * (0.9 + 0.1 * strand) + seg / 10.0 + strand * 0.13, 1.0);
			double py = ground - h + u * h;
			thickLineRGBA(surface, (int)sx, (int)py, (int)sx, (int)(py + h * 0.06), 2, 190, 225, 245, weapon_alpha(170.0 * flow * (0.4 + 0.6 * u) * event->fade));
		}
	}
	for(int puff = 0; puff < 8; puff++) {
		double u = fmod(event->t / 3.0 + puff / 8.0, 1.0);
		filledCircleRGBA(surface, (int)(cx + sin(puff * 2.3 + u * 4.0) * w * 2.0), (int)(ground - u * s * 0.06), (int)(s * (0.01 + 0.02 * u)), 215, 228, 240,
			weapon_alpha(70.0 * (1.0 - u) * flow * event->fade));
	}
	for(int k = 1; k < 12; k++) {
		double u = k / 12.0, rx = cx + event->direction * u * s * 0.3;
		thickLineRGBA(surface, (int)rx, (int)(hill_ground(event, rx) + 1.0), (int)(rx + 5), (int)(hill_ground(event, rx + 5) + 1.0), 2, 110, 180, 210,
			weapon_alpha(120.0 * (1.0 - u) * flow * event->fade));
	}
}

// Scarecrows stand in a field; crows land on them, flap off, and settle again.
static void hill_scarecrows(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int count = 4;
	Uint8 alpha = weapon_alpha(236.0 * event->fade);
	for(int i = 0; i < count; i++) {
		double x = surface->w * (0.1 + 0.8 * (i + 0.5) / count);
		double ground = hill_ground(event, x);
		double rise = smooth_step(0.0 + i * 0.03, 0.1 + i * 0.03, event->progress);
		double h = s * 0.05 * rise;
		thickLineRGBA(surface, (int)x, (int)ground, (int)x, (int)(ground - h), 2, 54, 40, 34, alpha);
		thickLineRGBA(surface, (int)(x - h * 0.4), (int)(ground - h * 0.75), (int)(x + h * 0.4), (int)(ground - h * 0.75), 2, 54, 40, 34, alpha);
		filledCircleRGBA(surface, (int)x, (int)(ground - h * 1.0), (int)(h * 0.1) + 1, 150, 120, 80, alpha);
		filledTrigonRGBA(surface, (int)(x - h * 0.16), (int)(ground - h * 1.04), (int)(x + h * 0.16), (int)(ground - h * 1.04), (int)x, (int)(ground - h * 1.3), 60, 44, 36, alpha);
		for(int c = 0; c < 2; c++) {
			double cycle = 7.0 + c * 2.3 + i;
			double local = fmod(event->t + i * 3.1 + c * 1.7, cycle) / cycle;
			double bx, by;
			if(local < 0.5) {
				bx = x + (c ? 1 : -1) * h * 0.4; by = ground - h * 0.8;
			} else {
				double f = (local - 0.5) / 0.5;
				bx = x + (c ? 1 : -1) * (h * 0.4 + f * s * 0.3 * event->direction); by = ground - h * 0.8 - f * s * 0.2;
				double flap = sin(event->t * 20.0 + c) * s * 0.006;
				thickLineRGBA(surface, (int)(bx - s * 0.008), (int)(by - flap), (int)bx, (int)by, 1, 8, 10, 16, weapon_alpha(235.0 * (1.0 - f) * event->fade));
				thickLineRGBA(surface, (int)bx, (int)by, (int)(bx + s * 0.008), (int)(by - flap), 1, 8, 10, 16, weapon_alpha(235.0 * (1.0 - f) * event->fade));
			}
			if(local < 0.5) filledEllipseRGBA(surface, (int)bx, (int)by, (int)(s * 0.006) + 1, (int)(s * 0.004) + 1, 8, 10, 16, alpha);
		}
	}
}

// A tiered pagoda rises on the ridge, floor by floor, its eaves hung with swaying lanterns.
static void hill_pagoda(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = hill_side(event, 0.13), ground = hill_ground(event, cx);
	const int tiers = 5;
	double lit = smooth_step(0.3, 0.42, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	double tier_h = s * 0.024;
	for(int t = 0; t < tiers; t++) {
		double rise = smooth_step(0.0 + t * 0.05, 0.08 + t * 0.05, event->progress);
		if(rise <= 0.0) break;
		double w = s * (0.04 - t * 0.006), y0 = ground - t * tier_h * 1.4, h = tier_h * rise;
		boxRGBA(surface, (int)(cx - w * 0.6), (int)(y0 - h), (int)(cx + w * 0.6), (int)y0, 70, 36, 40, alpha);
		filledTrigonRGBA(surface, (int)(cx - w * 1.2), (int)(y0 - h), (int)(cx + w * 1.2), (int)(y0 - h), (int)cx, (int)(y0 - h - tier_h * 0.7 * rise), 28, 32, 44, alpha);
		boxRGBA(surface, (int)(cx - 2), (int)(y0 - h * 0.7), (int)(cx + 2), (int)(y0 - h * 0.25), 255, 214, 130, weapon_alpha(230.0 * lit * event->fade));
		for(int side = -1; side <= 1; side += 2) {
			double lx = cx + side * w * 1.1, sway = sin(event->t * 1.8 + t + side) * 1.5;
			static const Uint8 red[3] = { 255, 120, 90 };
			weapon_glow(surface, lx + sway, y0 - h + 4.0, 1.8, red, 220.0 * lit * event->fade);
		}
	}
	double top = ground - tiers * tier_h * 1.4;
	thickLineRGBA(surface, (int)cx, (int)(top + tier_h * 0.5), (int)cx, (int)(top - tier_h * 0.8), 1, 28, 32, 44, weapon_alpha(235.0 * event->fade * smooth_step(0.2, 0.3, event->progress)));
}

// A frozen pond: figures glide in loops, leaving pale curved tracks on the ice.
static void hill_skaters(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * 0.5, ground = hill_ground(event, cx) + s * 0.006;
	double rw = s * 0.26, rh = s * 0.016;
	double freeze = smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.93, 0.98, event->progress));
	filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(rw * freeze), (int)(rh * freeze) + 1, 150, 190, 215, weapon_alpha(90.0 * event->fade));
	ellipseRGBA(surface, (int)cx, (int)ground, (int)(rw * freeze), (int)(rh * freeze) + 1, 200, 230, 245, weapon_alpha(150.0 * event->fade));
	Uint8 alpha = weapon_alpha(236.0 * event->fade * freeze);
	for(int i = 0; i < 3; i++) {
		double a = event->t * (0.5 + 0.15 * i) + i * 2.1;
		double px = cx + cos(a) * rw * 0.7, py = ground + sin(a) * rh * 0.6;
		for(int k = 1; k <= 12; k++) {
			double ta = a - k * 0.07;
			filledCircleRGBA(surface, (int)(cx + cos(ta) * rw * 0.7), (int)(ground + sin(ta) * rh * 0.6), 1, 230, 242, 250, weapon_alpha(120.0 * (1.0 - k / 13.0) * freeze * event->fade));
		}
		double lean = cos(a) * s * 0.006;
		thickLineRGBA(surface, (int)px, (int)py, (int)(px + lean), (int)(py - s * 0.024), 3, 18, 22, 32, alpha);
		filledCircleRGBA(surface, (int)(px + lean), (int)(py - s * 0.03), (int)(s * 0.005) + 1, 18, 22, 32, alpha);
		thickLineRGBA(surface, (int)(px - s * 0.006), (int)py, (int)(px + s * 0.006), (int)py, 1, 220, 235, 245, alpha);
	}
}

// A flock of sheep is walked along the ridge by a shepherd with a lantern and a small dog.
static void hill_shepherd(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double margin = s * 0.1, travel = surface->w + 2.0 * margin + s * 0.4;
	double lead = event->progress * travel - margin;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	for(int i = 0; i < 9; i++) {
		double off = s * 0.04 * (i % 3) + hill_random(event, i, 1) * s * 0.05 + (i / 3) * s * 0.045;
		double px = d > 0 ? lead - off : surface->w - lead + off;
		if(px < -30 || px > surface->w + 30) continue;
		double ground = hill_ground(event, px);
		double bob = fabs(sin(event->t * 3.0 + i)) * s * 0.002;
		double sz = s * 0.02;
		for(int leg = 0; leg < 2; leg++) {
			double lx = px + (leg ? 0.4 : -0.4) * sz;
			thickLineRGBA(surface, (int)lx, (int)(ground - sz * 0.4), (int)(lx + sin(event->t * 6.0 + leg + i) * sz * 0.15), (int)ground, 1, 24, 28, 38, alpha);
		}
		filledEllipseRGBA(surface, (int)px, (int)(ground - sz * 0.65 - bob), (int)(sz * 0.7), (int)(sz * 0.45), 150, 156, 170, alpha);
		filledCircleRGBA(surface, (int)(px + d * sz * 0.7), (int)(ground - sz * 0.65 - bob), (int)(sz * 0.22) + 1, 30, 34, 46, alpha);
	}
	double sx = d > 0 ? lead + s * 0.05 : surface->w - lead - s * 0.05;
	double sground = hill_ground(event, sx);
	hill_walker(surface, sx, sground, s * 0.042, d, event->t * 5.0, true, event->fade, event->t, 0);
	double dx = sx - d * s * 0.05 + sin(event->t * 3.0) * s * 0.02;
	filledEllipseRGBA(surface, (int)dx, (int)(hill_ground(event, dx) - s * 0.008), (int)(s * 0.01), (int)(s * 0.006), 14, 18, 26, alpha);
}

// An arched stone bridge over a stream; a lantern-carrying traveller crosses and the water glints beneath.
static void hill_bridge(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double cx = surface->w * 0.5, span = s * 0.2;
	double ground = hill_ground(event, cx);
	double rise = smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.94, 0.98, event->progress));
	Uint8 alpha = weapon_alpha(236.0 * event->fade);
	filledEllipseRGBA(surface, (int)cx, (int)(ground + s * 0.006), (int)(span * 1.2 * rise), (int)(s * 0.008 * rise) + 1, 40, 80, 110, weapon_alpha(200.0 * event->fade));
	for(int k = 0; k < 14; k++) {
		double gx = cx - span * 1.1 + fmod(k * 37.0 + event->t * 6.0, span * 2.2);
		pixelRGBA(surface, (int)gx, (int)(ground + s * 0.006 + sin(k * 3.0) * 2.0), 190, 225, 245, weapon_alpha(160.0 * rise * event->fade));
	}
	for(int i = 0; i < 24; i++) {
		double u0 = -1.0 + 2.0 * i / 24.0, u1 = -1.0 + 2.0 * (i + 1) / 24.0;
		double y0 = ground - s * 0.03 * rise * (1.0 - u0 * u0), y1 = ground - s * 0.03 * rise * (1.0 - u1 * u1);
		thickLineRGBA(surface, (int)(cx + u0 * span), (int)y0, (int)(cx + u1 * span), (int)y1, 4, 52, 56, 72, alpha);
	}
	for(int i = 0; i <= 8; i++) {
		double u = -1.0 + 2.0 * i / 8.0;
		lineRGBA(surface, (int)(cx + u * span), (int)(ground - s * 0.03 * rise * (1.0 - u * u)), (int)(cx + u * span), (int)(ground - s * 0.03 * rise * (1.0 - u * u) - s * 0.008), 30, 34, 48, alpha);
	}
	double u = (event->progress - 0.25) / 0.5;
	if(u > 0.0 && u < 1.0) {
		double px = cx + d * (-1.2 + 2.4 * u) * span;
		double nu = (px - cx) / span;
		double py = ground - s * 0.03 * (1.0 - fmin(1.0, nu * nu));
		hill_walker(surface, px, py, s * 0.036, d, event->t * 5.0, true, event->fade, event->t, 0);
	}
}

// Carved totem poles stand in a row; their eyes open one after another and glow.
static void hill_totems(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int poles = 5;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	static const Uint8 eye[3] = { 255, 190, 90 };
	for(int i = 0; i < poles; i++) {
		int order = event->direction > 0 ? i : poles - 1 - i;
		double x = surface->w * (0.1 + 0.8 * (i + 0.5) / poles);
		double ground = hill_ground(event, x);
		double rise = smooth_step(0.0 + i * 0.03, 0.1 + i * 0.03, event->progress);
		double h = s * (0.07 + 0.015 * (i % 3)) * rise, w = s * 0.012;
		double awake = smooth_step(0.2 + order * 0.08, 0.28 + order * 0.08, event->progress) * (1.0 - smooth_step(0.9, 0.97, event->progress));
		boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x + w), (int)ground, 54, 40, 40, alpha);
		boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x - w * 0.4), (int)ground, 74, 54, 50, alpha);
		for(int face = 0; face < 3; face++) {
			double fy = ground - h * (0.2 + 0.28 * face);
			boxRGBA(surface, (int)(x - w), (int)(fy - w * 0.15), (int)(x + w), (int)(fy + w * 0.15), 30, 22, 24, alpha);
			for(int side = -1; side <= 1; side += 2) {
				weapon_glow(surface, x + side * w * 0.45, fy - w * 0.5, 1.4, eye, 235.0 * awake * event->fade * (0.8 + 0.2 * sin(event->t * 3.0 + i + face)));
			}
		}
		filledTrigonRGBA(surface, (int)(x - w * 1.3), (int)(ground - h), (int)(x + w * 1.3), (int)(ground - h), (int)x, (int)(ground - h - w * 1.6), 100, 44, 46, alpha);
	}
}

// Snowmen build themselves ball by ball, then wave their twig arms as snow drifts down around them.
static void hill_snowmen(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int count = 4;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	for(int i = 0; i < count; i++) {
		double x = surface->w * (0.12 + 0.76 * (i + 0.5) / count);
		double ground = hill_ground(event, x);
		double r = s * (0.016 + 0.003 * (i % 2));
		double b1 = smooth_step(0.02 + i * 0.05, 0.08 + i * 0.05, event->progress);
		double b2 = smooth_step(0.08 + i * 0.05, 0.14 + i * 0.05, event->progress);
		double b3 = smooth_step(0.14 + i * 0.05, 0.2 + i * 0.05, event->progress);
		filledCircleRGBA(surface, (int)x, (int)(ground - r * b1), (int)(r * b1) + 1, 224, 234, 244, alpha);
		filledCircleRGBA(surface, (int)x, (int)(ground - r * 2.0 - r * 0.7 * b2), (int)(r * 0.75 * b2) + 1, 232, 240, 248, alpha);
		double hy = ground - r * 2.0 - r * 1.4 - r * 0.5 * b3;
		filledCircleRGBA(surface, (int)x, (int)hy, (int)(r * 0.55 * b3) + 1, 238, 244, 250, alpha);
		if(b3 < 0.9) continue;
		filledTrigonRGBA(surface, (int)(x + 1), (int)hy, (int)(x + r * 0.9), (int)(hy + 1), (int)x, (int)(hy + 2), 230, 120, 50, alpha);
		pixelRGBA(surface, (int)(x - r * 0.2), (int)(hy - r * 0.15), 20, 24, 32, alpha);
		pixelRGBA(surface, (int)(x + r * 0.2), (int)(hy - r * 0.15), 20, 24, 32, alpha);
		double wave = sin(event->t * 4.0 + i * 1.3) * r * 0.5;
		double sy = ground - r * 2.8;
		thickLineRGBA(surface, (int)(x - r * 0.6), (int)sy, (int)(x - r * 1.5), (int)(sy - r * 0.2 + wave), 1, 70, 50, 40, alpha);
		thickLineRGBA(surface, (int)(x + r * 0.6), (int)sy, (int)(x + r * 1.5), (int)(sy - r * 0.2 - wave), 1, 70, 50, 40, alpha);
	}
	for(int f = 0; f < 36; f++) {
		double r1 = hill_random(event, f, 1), r2 = hill_random(event, f, 2);
		double u = fmod(event->t * (0.12 + 0.08 * r2) + r1, 1.0);
		double fx = surface->w * r1 + sin(event->t + f) * s * 0.01, fy = surface->h * 0.55 + u * (surface->h * 0.4);
		filledCircleRGBA(surface, (int)fx, (int)fy, 1, 225, 235, 245, weapon_alpha(200.0 * (1.0 - u * 0.4) * event->fade));
	}
}

// A row of sunflowers grows up and every head slowly turns to follow the sun across the sky.
static void hill_sunflowers(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int count = 9;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	for(int i = 0; i < count; i++) {
		double x = surface->w * (0.08 + 0.84 * (i + 0.5) / count) + (hill_random(event, i, 1) - 0.5) * s * 0.02;
		double ground = hill_ground(event, x);
		double grow = smooth_step(0.0 + i * 0.015, 0.12 + i * 0.015, event->progress);
		double h = s * (0.055 + 0.02 * hill_random(event, i, 2)) * grow;
		double face = (event->progress - 0.5) * event->direction * 0.9;
		double sway = sin(event->t * 1.3 + i) * h * 0.03;
		double hx = x + sway + sin(face) * h * 0.12, hy = ground - h;
		double r = s * 0.011 * grow;
		thickLineRGBA(surface, (int)x, (int)ground, (int)hx, (int)hy, 2, 40, 86, 44, alpha);
		filledEllipseRGBA(surface, (int)(x + (hx - x) * 0.4 - r * 1.2), (int)(ground - h * 0.4), (int)(r * 1.1) + 1, (int)(r * 0.5) + 1, 52, 112, 56, alpha);
		for(int petal = 0; petal < 12; petal++) {
			double a = petal * 0.5236 + 0.26;
			double px = hx + cos(a) * r * 1.5, py = hy + sin(a) * r * 1.5 * (0.55 + 0.45 * cos(face));
			filledEllipseRGBA(surface, (int)px, (int)py, (int)(r * 0.5) + 1, (int)(r * 0.5) + 1, 244, 196, 40, alpha);
		}
		filledEllipseRGBA(surface, (int)(hx + sin(face) * r * 0.3), (int)hy, (int)(r * 0.95) + 1, (int)(r * 0.95 * (0.6 + 0.4 * cos(face))) + 1, 84, 54, 30, alpha);
	}
}

// Crystals push up out of the hillside, glowing in slow shifting colours, and sparkle at their tips.
static void hill_crystals(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int clusters = 6;
	for(int c = 0; c < clusters; c++) {
		double cx = surface->w * (0.1 + 0.8 * (c + 0.5) / clusters);
		double ground = hill_ground(event, cx);
		for(int k = 0; k < 5; k++) {
			double start = 0.03 + c * 0.04 + k * 0.01;
			double grow = smooth_step(start, start + 0.1, event->progress);
			if(grow <= 0.0) continue;
			double lean = (k - 2) * 0.28;
			double h = s * (0.03 + 0.04 * hill_random(event, c * 5 + k, 3)) * grow;
			double w = s * 0.007;
			double bx = cx + (k - 2) * s * 0.008;
			double tx = bx + sin(lean) * h, ty = ground - cos(lean) * h;
			double hue = event->t * 0.35 + c * 0.9 + k * 0.4;
			Uint8 r = (Uint8)(120 + 100 * sin(hue)), g = (Uint8)(150 + 90 * sin(hue + 2.1)), b = (Uint8)(200 + 50 * sin(hue + 4.2));
			Uint8 body = weapon_alpha(170.0 * event->fade);
			filledTrigonRGBA(surface, (int)(bx - w), (int)ground, (int)(bx + w), (int)ground, (int)tx, (int)ty, r, g, b, body);
			filledTrigonRGBA(surface, (int)(bx - w * 0.2), (int)ground, (int)(bx + w), (int)ground, (int)tx, (int)ty, 235, 240, 255, weapon_alpha(70.0 * event->fade));
			Uint8 tint[3] = { r, g, b };
			weapon_glow(surface, bx, ground - h * 0.4, w * 3.0, tint, 90.0 * grow * event->fade);
			double twinkle = sin(event->t * 3.0 + c * 2.0 + k * 1.7);
			if(twinkle > 0.85 && grow > 0.9) {
				Uint8 white[3] = { 255, 255, 255 };
				weapon_glow(surface, tx, ty, 2.5, white, 230.0 * event->fade * (twinkle - 0.85) / 0.15);
			}
		}
	}
}

// Giant glowing mushrooms bloom on the hill and puff drifting spores into the air.
static void hill_mushrooms(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int count = 7;
	Uint8 alpha = weapon_alpha(235.0 * event->fade);
	for(int i = 0; i < count; i++) {
		double x = surface->w * (0.1 + 0.8 * (i + 0.5) / count);
		double ground = hill_ground(event, x);
		double grow = smooth_step(0.02 + i * 0.03, 0.14 + i * 0.03, event->progress);
		double h = s * (0.03 + 0.03 * hill_random(event, i, 4)) * grow;
		double cap = h * 0.9;
		double pulse = 0.7 + 0.3 * sin(event->t * 1.6 + i * 1.1);
		Uint8 glow[3] = { (Uint8)(110 + 90 * (i % 2)), (Uint8)(200 - 60 * (i % 3)), 255 };
		boxRGBA(surface, (int)(x - h * 0.12), (int)(ground - h), (int)(x + h * 0.12), (int)ground, 214, 206, 222, alpha);
		weapon_glow(surface, x, ground - h, cap * 0.6, glow, 120.0 * pulse * grow * event->fade);
		filledEllipseRGBA(surface, (int)x, (int)(ground - h), (int)cap + 1, (int)(cap * 0.55) + 1, glow[0] / 2 + 40, glow[1] / 2 + 30, glow[2] / 2 + 40, alpha);
		for(int dot = 0; dot < 4; dot++) {
			double dx = (dot - 1.5) * cap * 0.4;
			filledCircleRGBA(surface, (int)(x + dx), (int)(ground - h - cap * 0.2 * (1.0 - fabs(dx) / cap)), (int)(cap * 0.12) + 1, 235, 245, 255, weapon_alpha(210.0 * pulse * event->fade));
		}
		for(int spore = 0; spore < 5; spore++) {
			double u = fmod(event->t * (0.08 + 0.04 * hill_random(event, i * 5 + spore, 5)) + hill_random(event, i * 5 + spore, 6), 1.0);
			double sx = x + sin(event->t * 0.8 + spore + i) * cap * (0.5 + u), sy = ground - h - u * s * 0.12;
			filledCircleRGBA(surface, (int)sx, (int)sy, 1, glow[0], glow[1], glow[2], weapon_alpha(200.0 * (1.0 - u) * grow * event->fade));
		}
	}
}

// Three tents pitch themselves around a camp; each lights up from inside and a plume of smoke rises between them.
static void hill_tents(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	static const Uint8 cloth[3][3] = { { 150, 78, 60 }, { 70, 100, 140 }, { 96, 120, 78 } };
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	static const Uint8 lamp[3] = { 255, 205, 120 };
	double mid = surface->w * (0.35 + 0.3 * hill_random(event, 0, 7));
	for(int i = 0; i < 3; i++) {
		double x = mid + (i - 1) * s * 0.17;
		double ground = hill_ground(event, x);
		double up = smooth_step(0.02 + i * 0.05, 0.1 + i * 0.05, event->progress);
		double w = s * 0.045 * up, h = s * 0.05 * up;
		filledTrigonRGBA(surface, (int)(x - w), (int)ground, (int)(x + w), (int)ground, (int)x, (int)(ground - h), cloth[i][0], cloth[i][1], cloth[i][2], alpha);
		filledTrigonRGBA(surface, (int)(x - w * 0.25), (int)ground, (int)(x + w * 0.25), (int)ground, (int)x, (int)(ground - h * 0.7), 20, 18, 24, alpha);
		double lit = smooth_step(0.2 + i * 0.06, 0.3 + i * 0.06, event->progress) * (0.85 + 0.15 * sin(event->t * 5.0 + i));
		filledTrigonRGBA(surface, (int)(x - w * 0.25), (int)ground, (int)(x + w * 0.25), (int)ground, (int)x, (int)(ground - h * 0.7), lamp[0], lamp[1], lamp[2], weapon_alpha(235.0 * lit * event->fade));
		weapon_glow(surface, x, ground - h * 0.3, w * 0.3, lamp, 110.0 * lit * event->fade);
		thickLineRGBA(surface, (int)x, (int)(ground - h), (int)x, (int)(ground - h - s * 0.01), 1, 60, 50, 44, alpha);
	}
	double fx = mid, fg = hill_ground(event, fx);
	double fire = smooth_step(0.15, 0.25, event->progress);
	weapon_glow(surface, fx, fg - s * 0.006, 2.5, lamp, 200.0 * fire * event->fade * (0.8 + 0.2 * sin(event->t * 9.0)));
	for(int p = 0; p < 12; p++) {
		double u = fmod(event->t * 0.2 + p / 12.0, 1.0);
		double px = fx + sin(event->t * 1.4 + p * 0.8) * s * 0.01 * u * 3.0, py = fg - u * s * 0.17;
		filledCircleRGBA(surface, (int)px, (int)py, (int)(2 + u * s * 0.008), 130, 132, 142, weapon_alpha(110.0 * (1.0 - u) * fire * event->fade));
	}
}

// A tiered fountain on the hill throws jets of water in slow shifting colours that fall back into the basin.
static void hill_fountain(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 7));
	double ground = hill_ground(event, cx);
	double rise = smooth_step(0.0, 0.1, event->progress);
	Uint8 alpha = weapon_alpha(235.0 * event->fade);
	double bw = s * 0.07, bh = s * 0.014;
	filledEllipseRGBA(surface, (int)cx, (int)(ground - bh * 0.4), (int)bw, (int)bh + 1, 70, 76, 92, alpha);
	filledEllipseRGBA(surface, (int)cx, (int)(ground - bh * 0.7), (int)(bw * 0.88), (int)(bh * 0.7) + 1, 40, 86, 120, alpha);
	boxRGBA(surface, (int)(cx - s * 0.006), (int)(ground - s * 0.05 * rise), (int)(cx + s * 0.006), (int)(ground - bh), 84, 90, 106, alpha);
	filledEllipseRGBA(surface, (int)cx, (int)(ground - s * 0.05 * rise), (int)(s * 0.02), (int)(s * 0.006) + 1, 84, 90, 106, alpha);
	double power = smooth_step(0.1, 0.2, event->progress);
	for(int jet = -4; jet <= 4; jet++) {
		double vx = jet * 0.075 * s, vy = (0.2 - 0.014 * abs(jet)) * s;
		double hue = event->t * 0.4 + jet * 0.35;
		Uint8 r = (Uint8)(150 + 90 * sin(hue)), g = (Uint8)(190 + 60 * sin(hue + 2.1)), b = (Uint8)(235 + 20 * sin(hue + 4.2));
		for(int d = 0; d < 14; d++) {
			double u = fmod(event->t * 0.55 + d / 14.0 + jet * 0.03, 1.0) * 0.8;
			double px = cx + vx * u, py = ground - s * 0.05 * rise - (vy * u - 0.5 * s * 0.5 * u * u) * power;
			if(py > ground - bh * 0.5) continue;
			filledCircleRGBA(surface, (int)px, (int)py, 1 + (d % 3 == 0), r, g, b, weapon_alpha(210.0 * (1.0 - u * 0.6) * power * event->fade));
		}
	}
	for(int ring = 0; ring < 3; ring++) {
		double u = fmod(event->t * 0.4 + ring / 3.0, 1.0);
		ellipseRGBA(surface, (int)cx, (int)(ground - bh * 0.7), (int)(bw * 0.8 * u) + 1, (int)(bh * 0.6 * u) + 1, 210, 230, 250, weapon_alpha(150.0 * (1.0 - u) * power * event->fade));
	}
}

// A harvester crawls along the ridge in the dark, its headlights lighting the crop and dust trailing behind.
static void hill_harvester(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double u = smooth_step(0.0, 1.0, event->progress);
	double x = d > 0 ? surface->w * (-0.1 + 1.2 * u) : surface->w * (1.1 - 1.2 * u);
	double ground = hill_ground(event, x);
	double tilt = (hill_ground(event, x + 6.0) - hill_ground(event, x - 6.0)) / 12.0;
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double w = s * 0.07, h = s * 0.03;
	static const Uint8 lamp[3] = { 255, 238, 170 };
	double wheel = s * 0.012;
	double base = ground - wheel;
	filledTrigonRGBA(surface, (int)(x + d * w * 0.5), (int)(base - h * 0.5), (int)(x + d * w * 3.2), (int)(base + s * 0.012 + tilt * w * 3), (int)(x + d * w * 3.2), (int)(base - s * 0.04 + tilt * w * 3), 255, 238, 170, weapon_alpha(36.0 * event->fade));
	boxRGBA(surface, (int)(x - w * 0.5), (int)(base - h * 0.9), (int)(x + w * 0.5), (int)base, 176, 60, 40, alpha);
	boxRGBA(surface, (int)(x + d * w * 0.05), (int)(base - h * 1.6), (int)(x + d * w * 0.4), (int)(base - h * 0.9), 60, 70, 86, alpha);
	boxRGBA(surface, (int)(x + d * w * 0.1), (int)(base - h * 1.5), (int)(x + d * w * 0.34), (int)(base - h * 1.0), 255, 214, 120, weapon_alpha(200.0 * event->fade));
	boxRGBA(surface, (int)(x + d * w * 0.5), (int)(base - h * 0.45), (int)(x + d * w * 0.8), (int)(base + wheel * 0.3), 150, 160, 70, alpha);
	for(int i = 0; i < 3; i++) {
		double wx = x + (i - 1) * w * 0.38;
		filledCircleRGBA(surface, (int)wx, (int)(ground - wheel * 0.9), (int)wheel, 20, 22, 28, alpha);
		double a = event->t * 6.0 * d + i;
		lineRGBA(surface, (int)wx, (int)(ground - wheel * 0.9), (int)(wx + cos(a) * wheel * 0.8), (int)(ground - wheel * 0.9 + sin(a) * wheel * 0.8), 120, 124, 130, alpha);
	}
	weapon_glow(surface, x + d * w * 0.62, base - h * 0.6, 2.2, lamp, 230.0 * event->fade);
	for(int p = 0; p < 14; p++) {
		double age = fmod(event->t * 0.8 + p / 14.0, 1.0);
		double px = x - d * (w * 0.6 + age * s * 0.12), py = ground - age * s * 0.03 - hill_random(event, p, 1) * s * 0.01;
		filledCircleRGBA(surface, (int)px, (int)py, (int)(2 + age * s * 0.01), 150, 130, 100, weapon_alpha(90.0 * (1.0 - age) * event->fade));
	}
}

// Small stone cairns stack themselves stone by stone; the top one of each lights up when it is done.
static void hill_cairns(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int count = 6;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	static const Uint8 rune[3] = { 130, 220, 255 };
	for(int i = 0; i < count; i++) {
		double x = surface->w * (0.1 + 0.8 * (i + 0.5) / count) + (hill_random(event, i, 1) - 0.5) * s * 0.03;
		double ground = hill_ground(event, x);
		double y = ground;
		int stones = 4 + i % 3;
		double r = s * 0.014;
		for(int k = 0; k < stones; k++) {
			double drop = smooth_step(0.02 + i * 0.03 + k * 0.05, 0.06 + i * 0.03 + k * 0.05, event->progress);
			if(drop <= 0.0) break;
			double rx = r * (1.0 - 0.14 * k), ry = rx * 0.5;
			double sx = x + (hill_random(event, i * 9 + k, 2) - 0.5) * r * 0.5;
			double sy = y - ry - (1.0 - drop) * s * 0.05;
			Uint8 shade = (Uint8)(130 + 14 * (k % 3));
			filledEllipseRGBA(surface, (int)sx, (int)sy, (int)rx + 1, (int)ry + 1, shade, shade - 4, shade + 6, alpha);
			filledEllipseRGBA(surface, (int)(sx - rx * 0.2), (int)(sy - ry * 0.3), (int)(rx * 0.6) + 1, (int)(ry * 0.4) + 1, shade + 36, shade + 32, shade + 40, weapon_alpha(150.0 * event->fade));
			if(drop >= 1.0) y -= ry * 1.7;
			if(k == stones - 1 && drop >= 1.0) {
				double lit = smooth_step(0.5, 0.6, event->progress) * (0.7 + 0.3 * sin(event->t * 2.0 + i));
				weapon_glow(surface, sx, sy - ry - 3.0, 2.0, rune, 200.0 * lit * event->fade);
			}
		}
	}
}

// A little watermill beside a stream: the wheel turns, its paddles drip, and the window is lit.
static void hill_watermill(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.4 + 0.2 * hill_random(event, 0, 8));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	double up = smooth_step(0.0, 0.12, event->progress);
	double bw = s * 0.055, bh = s * 0.05 * up;
	static const Uint8 lamp[3] = { 255, 205, 120 };
	boxRGBA(surface, (int)(cx - bw), (int)(ground - bh), (int)(cx + bw), (int)ground, 112, 84, 64, alpha);
	filledTrigonRGBA(surface, (int)(cx - bw * 1.2), (int)(ground - bh), (int)(cx + bw * 1.2), (int)(ground - bh), (int)cx, (int)(ground - bh - s * 0.035 * up), 74, 50, 46, alpha);
	boxRGBA(surface, (int)(cx - bw * 0.5), (int)(ground - bh * 0.7), (int)(cx - bw * 0.1), (int)(ground - bh * 0.35), lamp[0], lamp[1], lamp[2], weapon_alpha(220.0 * smooth_step(0.15, 0.25, event->progress) * event->fade));
	double wx = cx + bw * 1.15, wr = s * 0.038 * up, wy = ground - wr * 0.8;
	filledEllipseRGBA(surface, (int)(cx + bw * 0.6), (int)(ground + 1), (int)(bw * 1.5), (int)(s * 0.006) + 1, 60, 100, 150, weapon_alpha(200.0 * event->fade));
	for(int k = 0; k < 8; k++) {
		double a = event->t * 0.8 + k * 0.7854;
		double ex = wx + cos(a) * wr, ey = wy + sin(a) * wr;
		thickLineRGBA(surface, (int)wx, (int)wy, (int)ex, (int)ey, 1, 90, 66, 52, alpha);
		thickLineRGBA(surface, (int)(ex - sin(a) * 3), (int)(ey + cos(a) * 3), (int)(ex + sin(a) * 3), (int)(ey - cos(a) * 3), 2, 130, 98, 74, alpha);
		if(ey > wy + wr * 0.2 && ey < wy + wr * 0.9) {
			filledCircleRGBA(surface, (int)ex, (int)(ey + 4 + 2 * sin(event->t * 9.0 + k)), 1, 170, 210, 245, weapon_alpha(180.0 * event->fade));
		}
	}
	circleRGBA(surface, (int)wx, (int)wy, (int)wr, 110, 84, 66, alpha);
	filledCircleRGBA(surface, (int)wx, (int)wy, 2, 60, 46, 40, alpha);
	for(int seg = 0; seg < 10; seg++) {
		double u = fmod(event->t * 0.3 + seg / 10.0, 1.0);
		double px = cx - bw * 0.2 + u * s * 0.3 * event->direction, py = ground + 1 + sin(event->t * 3.0 + seg) * 1.0;
		filledCircleRGBA(surface, (int)px, (int)py, 1, 190, 220, 245, weapon_alpha(160.0 * (1.0 - u) * event->fade));
	}
}

// A reedy pond: cattails sway, ripples spread from rising fish, and two frog eyes glint at the edge.
static void hill_marsh(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.35 + 0.3 * hill_random(event, 0, 9));
	double ground = hill_ground(event, cx);
	double pw = s * 0.2, ph = s * 0.025;
	Uint8 alpha = weapon_alpha(235.0 * event->fade);
	static const Uint8 eye[3] = { 220, 255, 140 };
	double fill = smooth_step(0.0, 0.1, event->progress);
	filledEllipseRGBA(surface, (int)cx, (int)(ground - ph * 0.3), (int)(pw * fill) + 1, (int)(ph * fill) + 1, 26, 44, 66, weapon_alpha(225.0 * event->fade));
	ellipseRGBA(surface, (int)cx, (int)(ground - ph * 0.3), (int)(pw * fill) + 1, (int)(ph * fill) + 1, 90, 120, 150, weapon_alpha(120.0 * event->fade));
	for(int k = 0; k < 4; k++) {
		double u = fmod(event->t * 0.22 + k * 0.25 + hill_random(event, k, 3), 1.0);
		double rx = cx + (hill_random(event, k, 4) - 0.5) * pw * 1.2, ry = ground - ph * 0.3 + (hill_random(event, k, 5) - 0.5) * ph;
		ellipseRGBA(surface, (int)rx, (int)ry, (int)(u * s * 0.04) + 1, (int)(u * s * 0.012) + 1, 190, 215, 240, weapon_alpha(170.0 * (1.0 - u) * fill * event->fade));
	}
	for(int r = 0; r < 26; r++) {
		double side = r < 13 ? -1.0 : 1.0;
		double rx = cx + side * (pw * (0.78 + 0.28 * hill_random(event, r, 6))) + (hill_random(event, r, 7) - 0.5) * s * 0.03;
		double base = hill_ground(event, rx) + 1.0;
		double h = s * (0.035 + 0.04 * hill_random(event, r, 8)) * smooth_step(0.02 + r * 0.004, 0.14 + r * 0.004, event->progress);
		double sway = sin(event->t * 1.1 + r * 0.7) * h * 0.08;
		thickLineRGBA(surface, (int)rx, (int)base, (int)(rx + sway), (int)(base - h), 1, 52, 86, 52, alpha);
		if(r % 2 == 0) thickLineRGBA(surface, (int)(rx + sway), (int)(base - h), (int)(rx + sway), (int)(base - h - s * 0.012), 3, 100, 62, 40, alpha);
	}
	double blink = fmod(event->t, 4.0) > 3.6 ? 0.0 : 1.0;
	double ex = cx + pw * 0.55, ey = ground - ph * 0.9;
	weapon_glow(surface, ex, ey, 1.2, eye, 200.0 * blink * fill * event->fade);
	weapon_glow(surface, ex + 5, ey, 1.2, eye, 200.0 * blink * fill * event->fade);
}

// Three stargazers lie on the hill beside a telescope while shooting stars streak across the sky above them.
static void hill_stargazers(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 10));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double fin = smooth_step(0.0, 0.08, event->progress);
	double tx = cx + s * 0.05, th = s * 0.04 * fin;
	thickLineRGBA(surface, (int)tx, (int)(ground - th), (int)(tx - s * 0.015), (int)ground, 1, 120, 120, 130, alpha);
	thickLineRGBA(surface, (int)tx, (int)(ground - th), (int)(tx + s * 0.015), (int)ground, 1, 120, 120, 130, alpha);
	thickLineRGBA(surface, (int)tx, (int)(ground - th), (int)(tx - event->direction * s * 0.03), (int)(ground - th - s * 0.03), 4, 170, 176, 190, alpha);
	for(int p = 0; p < 3; p++) {
		double px = cx - p * s * 0.028, py = ground - s * 0.003;
		filledEllipseRGBA(surface, (int)px, (int)py, (int)(s * 0.014) + 1, (int)(s * 0.005) + 1, 190 - p * 30, 110 + p * 40, 100 + p * 50, alpha);
		filledCircleRGBA(surface, (int)(px + s * 0.015), (int)(py - s * 0.002), (int)(s * 0.005) + 1, 232, 196, 164, alpha);
	}
	int slot = (int)(event->t / 2.2);
	double age = event->t - slot * 2.2;
	if(age < 0.8) {
		double u = age / 0.8;
		double sx0 = surface->w * (0.1 + 0.8 * hill_random(event, slot, 11)), sy0 = surface->h * (0.04 + 0.12 * hill_random(event, slot, 12));
		double len = s * 0.3, dx = event->direction * len * u, dy = len * 0.35 * u;
		for(int k = 0; k < 14; k++) {
			double f = k / 14.0;
			double px = sx0 + dx - event->direction * len * 0.5 * f * (u > 0.5 ? 1.0 : u * 2.0), py = sy0 + dy - len * 0.175 * f * (u > 0.5 ? 1.0 : u * 2.0);
			filledCircleRGBA(surface, (int)px, (int)py, k < 3 ? 2 : 1, 235, 240, 255, weapon_alpha(230.0 * (1.0 - f) * (1.0 - u * 0.7) * fin * event->fade));
		}
		weapon_glow(surface, sx0 + dx, sy0 + dy, 1.8, (const Uint8[]){ 220, 235, 255 }, 230.0 * (1.0 - u) * event->fade);
	}
}

// A lit carousel turns slowly, its painted horses rising and falling and its rim bulbs chasing round.
static void hill_carousel(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 13));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	double up = smooth_step(0.0, 0.12, event->progress);
	double r = s * 0.06, h = s * 0.055 * up;
	filledEllipseRGBA(surface, (int)cx, (int)(ground - 2), (int)r, (int)(r * 0.18) + 1, 110, 90, 90, alpha);
	for(int k = 0; k < 6; k++) {
		double a = event->t * 0.7 + k * 1.0472;
		double px = cx + cos(a) * r * 0.8, depth = sin(a);
		double py = ground - 2 + depth * r * 0.12;
		double bob = sin(event->t * 2.4 + k * 1.3) * h * 0.08;
		thickLineRGBA(surface, (int)px, (int)py, (int)px, (int)(py - h * 0.85 + bob), 1, 220, 200, 120, alpha);
		static const Uint8 horse[3][3] = { { 240, 240, 236 }, { 200, 120, 70 }, { 150, 190, 230 } };
		const Uint8 *c = horse[k % 3];
		filledEllipseRGBA(surface, (int)px, (int)(py - h * 0.5 + bob), (int)(r * 0.14) + 1, (int)(r * 0.08) + 1, c[0], c[1], c[2], alpha);
		filledCircleRGBA(surface, (int)(px + cos(a + 1.57) * r * 0.11), (int)(py - h * 0.62 + bob), (int)(r * 0.05) + 1, c[0], c[1], c[2], alpha);
	}
	thickLineRGBA(surface, (int)cx, (int)(ground - 2), (int)cx, (int)(ground - h), 3, 200, 170, 90, alpha);
	filledTrigonRGBA(surface, (int)(cx - r * 1.05), (int)(ground - h), (int)(cx + r * 1.05), (int)(ground - h), (int)cx, (int)(ground - h - s * 0.03 * up), 190, 60, 70, alpha);
	for(int b = 0; b < 12; b++) {
		double bx = cx - r * 1.05 + b * r * 2.1 / 11.0;
		double on = 0.5 + 0.5 * sin(event->t * 6.0 - b * 0.9);
		weapon_glow(surface, bx, ground - h, 1.0, (const Uint8[]){ 255, 225, 150 }, 230.0 * on * up * event->fade);
	}
}

// A trebuchet hurls glowing stones in high arcs that burst into sparks where they land.
static void hill_trebuchet(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double bx = d > 0 ? surface->w * 0.22 : surface->w * 0.78;
	double ground = hill_ground(event, bx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double up = smooth_step(0.0, 0.1, event->progress);
	double h = s * 0.05 * up;
	thickLineRGBA(surface, (int)(bx - s * 0.03), (int)ground, (int)bx, (int)(ground - h), 3, 92, 66, 50, alpha);
	thickLineRGBA(surface, (int)(bx + s * 0.03), (int)ground, (int)bx, (int)(ground - h), 3, 92, 66, 50, alpha);
	double cycle = 3.6;
	double u = fmod(event->t, cycle) / cycle;
	double arm = -0.9 + 2.4 * smooth_step(0.0, 0.35, u);
	double ax = bx + d * sin(arm) * s * 0.045, ay = ground - h - cos(arm) * s * 0.045;
	double bxx = bx - d * sin(arm) * s * 0.02, byy = ground - h + cos(arm) * s * 0.02;
	thickLineRGBA(surface, (int)bxx, (int)byy, (int)ax, (int)ay, 3, 130, 96, 70, alpha);
	filledCircleRGBA(surface, (int)bxx, (int)(byy + 3), (int)(s * 0.008) + 1, 70, 70, 80, alpha);
	static const Uint8 fire[3] = { 255, 170, 80 };
	double range = d * s * 0.5;
	if(u > 0.28 && u < 0.85) {
		double f = (u - 0.28) / 0.57;
		double px = bx + range * f, py = ground - h - 4.0 * f * (1.0 - f) * s * 0.17;
		for(int k = 0; k < 8; k++) {
			double ff = f - k * 0.015;
			if(ff < 0) break;
			weapon_glow(surface, bx + range * ff, ground - h - 4.0 * ff * (1.0 - ff) * s * 0.17, 1.0, fire, 120.0 * (1.0 - k / 8.0) * event->fade);
		}
		weapon_glow(surface, px, py, 2.0, fire, 240.0 * event->fade);
	}
	if(u >= 0.85) {
		double f = (u - 0.85) / 0.15, lx = bx + range;
		double lg = hill_ground(event, lx);
		for(int k = 0; k < 10; k++) {
			double a = -3.14159 * (0.1 + 0.8 * k / 9.0);
			filledCircleRGBA(surface, (int)(lx + cos(a) * f * s * 0.04), (int)(lg + sin(a) * f * s * 0.05 + f * f * s * 0.02), 1, 255, 200, 110, weapon_alpha(220.0 * (1.0 - f) * event->fade));
		}
	}
}

// An orchard: apple trees fill with fruit and a few apples fall with a little glint.
static void hill_orchard(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int trees = 5;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	for(int i = 0; i < trees; i++) {
		double x = surface->w * (0.12 + 0.76 * (i + 0.5) / trees);
		double ground = hill_ground(event, x);
		double grow = smooth_step(0.0 + i * 0.03, 0.12 + i * 0.03, event->progress);
		double h = s * 0.045 * grow, cr = s * 0.032 * grow;
		thickLineRGBA(surface, (int)x, (int)ground, (int)x, (int)(ground - h), 3, 80, 56, 44, alpha);
		for(int blob = 0; blob < 4; blob++) {
			filledCircleRGBA(surface, (int)(x + (blob - 1.5) * cr * 0.5), (int)(ground - h - cr * 0.3 * (blob == 1 || blob == 2)), (int)(cr * 0.75) + 1, 42 + blob * 6, 100 + blob * 8, 54, alpha);
		}
		double fruit = smooth_step(0.3 + i * 0.03, 0.5 + i * 0.03, event->progress);
		for(int a = 0; a < 7; a++) {
			double ax = x + (hill_random(event, i * 7 + a, 14) - 0.5) * cr * 1.8, ay = ground - h - (hill_random(event, i * 7 + a, 15) - 0.3) * cr * 0.9;
			if(a < 7 * fruit) filledCircleRGBA(surface, (int)ax, (int)ay, 2, 220, 60, 56, alpha);
			if(a == 0 && fruit > 0.9) {
				double f = fmod(event->t * 0.35 + i * 0.2, 1.0);
				double fy = ay + f * f * (ground - ay);
				filledCircleRGBA(surface, (int)ax, (int)fy, 2, 230, 70, 60, weapon_alpha(235.0 * event->fade));
				if(f > 0.9) weapon_glow(surface, ax, ground - 1, 1.2, (const Uint8[]){ 255, 210, 200 }, 160.0 * event->fade);
			}
		}
	}
}

// A tall radio mast blinks its red lights and sends rings of signal rippling up into the sky.
static void hill_radiomast(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.25 + 0.5 * hill_random(event, 0, 16));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double h = s * 0.22 * smooth_step(0.0, 0.14, event->progress);
	double w = s * 0.012;
	thickLineRGBA(surface, (int)(cx - w), (int)ground, (int)cx, (int)(ground - h), 1, 160, 164, 172, alpha);
	thickLineRGBA(surface, (int)(cx + w), (int)ground, (int)cx, (int)(ground - h), 1, 160, 164, 172, alpha);
	for(int k = 1; k < 8; k++) {
		double f = k / 8.0;
		double hw = w * (1.0 - f);
		lineRGBA(surface, (int)(cx - hw), (int)(ground - h * f), (int)(cx + hw), (int)(ground - h * f * 0.96), 150, 154, 162, alpha);
	}
	double blink = fmod(event->t, 1.4) < 0.5 ? 1.0 : 0.15;
	weapon_glow(surface, cx, ground - h, 1.4, (const Uint8[]){ 255, 70, 60 }, 230.0 * blink * event->fade);
	weapon_glow(surface, cx, ground - h * 0.5, 1.0, (const Uint8[]){ 255, 70, 60 }, 200.0 * (1.2 - blink) * event->fade);
	double on = smooth_step(0.15, 0.25, event->progress);
	for(int ring = 0; ring < 4; ring++) {
		double u = fmod(event->t * 0.28 + ring / 4.0, 1.0);
		ellipseRGBA(surface, (int)cx, (int)(ground - h), (int)(u * s * 0.25) + 1, (int)(u * s * 0.1) + 1, 150, 220, 255, weapon_alpha(200.0 * (1.0 - u) * on * event->fade));
	}
}

// A glass greenhouse glows from within, its plants swaying and a little steam venting from the roof.
static void hill_greenhouse(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 17));
	double ground = hill_ground(event, cx);
	double up = smooth_step(0.0, 0.1, event->progress);
	double w = s * 0.07, h = s * 0.045 * up;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	static const Uint8 lamp[3] = { 190, 255, 170 };
	double lit = smooth_step(0.12, 0.22, event->progress);
	boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)ground, 70, 130, 100, weapon_alpha(120.0 * event->fade));
	filledTrigonRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)(ground - h), (int)cx, (int)(ground - h - s * 0.03 * up), 90, 150, 120, weapon_alpha(130.0 * event->fade));
	boxRGBA(surface, (int)(cx - w * 0.9), (int)(ground - h * 0.9), (int)(cx + w * 0.9), (int)(ground - 1), 150, 255, 150, weapon_alpha(70.0 * lit * event->fade));
	for(int k = 0; k <= 6; k++) {
		double fx = cx - w + k * w / 3.0;
		lineRGBA(surface, (int)fx, (int)(ground - h), (int)fx, (int)ground, 200, 230, 215, alpha);
	}
	rectangleRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)ground, 200, 230, 215, alpha);
	for(int p = 0; p < 9; p++) {
		double px = cx - w * 0.85 + p * w * 1.7 / 8.0, ph = h * (0.3 + 0.4 * hill_random(event, p, 18)) * lit;
		double sway = sin(event->t * 1.4 + p) * ph * 0.12;
		thickLineRGBA(surface, (int)px, (int)(ground - 1), (int)(px + sway), (int)(ground - ph), 1, 70, 170, 80, alpha);
		if(p % 3 == 0) filledCircleRGBA(surface, (int)(px + sway), (int)(ground - ph), 2, 255, 120, 150, weapon_alpha(230.0 * lit * event->fade));
	}
	weapon_glow(surface, cx, ground - h * 0.8, w * 0.5, lamp, 100.0 * lit * event->fade);
	for(int p = 0; p < 6; p++) {
		double u = fmod(event->t * 0.2 + p / 6.0, 1.0);
		filledCircleRGBA(surface, (int)(cx + sin(event->t + p) * s * 0.01), (int)(ground - h - s * 0.03 - u * s * 0.08), (int)(2 + u * 4), 220, 235, 235, weapon_alpha(100.0 * (1.0 - u) * lit * event->fade));
	}
}

// A swing set: two swings sway out of step, a child's silhouette on one of them.
static void hill_swings(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.25 + 0.5 * hill_random(event, 0, 19));
	double ground = hill_ground(event, cx);
	double up = smooth_step(0.0, 0.08, event->progress);
	double w = s * 0.05, h = s * 0.05 * up;
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	thickLineRGBA(surface, (int)(cx - w), (int)ground, (int)(cx - w * 0.7), (int)(ground - h), 2, 150, 90, 80, alpha);
	thickLineRGBA(surface, (int)(cx - w * 0.4), (int)ground, (int)(cx - w * 0.7), (int)(ground - h), 2, 150, 90, 80, alpha);
	thickLineRGBA(surface, (int)(cx + w), (int)ground, (int)(cx + w * 0.7), (int)(ground - h), 2, 150, 90, 80, alpha);
	thickLineRGBA(surface, (int)(cx + w * 0.4), (int)ground, (int)(cx + w * 0.7), (int)(ground - h), 2, 150, 90, 80, alpha);
	thickLineRGBA(surface, (int)(cx - w * 0.7), (int)(ground - h), (int)(cx + w * 0.7), (int)(ground - h), 3, 170, 100, 90, alpha);
	for(int k = 0; k < 2; k++) {
		double px = cx + (k - 0.5) * w * 0.7;
		double a = sin(event->t * 1.7 + k * 1.8) * (0.4 + 0.3 * smooth_step(0.1, 0.4, event->progress));
		double len = h * 0.85;
		double sx = px + sin(a) * len, sy = ground - h + cos(a) * len;
		lineRGBA(surface, (int)(px - 3), (int)(ground - h), (int)(sx - 3), (int)sy, 220, 210, 190, alpha);
		lineRGBA(surface, (int)(px + 3), (int)(ground - h), (int)(sx + 3), (int)sy, 220, 210, 190, alpha);
		thickLineRGBA(surface, (int)(sx - 4), (int)sy, (int)(sx + 4), (int)sy, 2, 60, 80, 120, alpha);
		if(k == 0) {
			thickLineRGBA(surface, (int)sx, (int)sy, (int)(sx + sin(a) * 2), (int)(sy - h * 0.25), 3, 20, 24, 36, alpha);
			filledCircleRGBA(surface, (int)(sx + sin(a) * 3), (int)(sy - h * 0.32), 3, 20, 24, 36, alpha);
		}
	}
}

// Beehives in a flowery meadow: glowing bees weave figure-eights around the boxes.
static void hill_beehives(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	const int count = 4;
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	static const Uint8 bee[3] = { 255, 215, 80 };
	for(int i = 0; i < count; i++) {
		double x = surface->w * (0.22 + 0.56 * (i + 0.5) / count);
		double ground = hill_ground(event, x);
		double up = smooth_step(0.0 + i * 0.04, 0.1 + i * 0.04, event->progress);
		double w = s * 0.016, h = s * 0.02 * up;
		boxRGBA(surface, (int)(x - w * 0.8), (int)(ground - h - w), (int)(x - w * 0.6), (int)ground, 90, 70, 60, alpha);
		boxRGBA(surface, (int)(x + w * 0.6), (int)(ground - h - w), (int)(x + w * 0.8), (int)ground, 90, 70, 60, alpha);
		boxRGBA(surface, (int)(x - w), (int)(ground - h * 1.8), (int)(x + w), (int)(ground - h), (i % 2) ? 230 : 240, 226, 200, alpha);
		boxRGBA(surface, (int)(x - w * 1.1), (int)(ground - h * 2.0), (int)(x + w * 1.1), (int)(ground - h * 1.8), 120, 90, 80, alpha);
		boxRGBA(surface, (int)(x - w * 0.3), (int)(ground - h * 1.3), (int)(x + w * 0.3), (int)(ground - h * 1.2), 30, 24, 24, alpha);
		for(int b = 0; b < 6; b++) {
			double a = event->t * (1.6 + 0.3 * (b % 3)) + b * 1.05 + i;
			double bx = x + sin(a) * s * 0.03, by = ground - h * 1.5 + sin(a * 2.0) * s * 0.012 - b * 1.0;
			weapon_glow(surface, bx, by, 0.8, bee, 190.0 * up * event->fade);
		}
	}
	for(int f = 0; f < 26; f++) {
		double fx = surface->w * (0.15 + 0.7 * hill_random(event, f, 20)), fy = hill_ground(event, fx) - 1.0;
		double grow = smooth_step(0.1 + f * 0.01, 0.25 + f * 0.01, event->progress);
		thickLineRGBA(surface, (int)fx, (int)fy, (int)fx, (int)(fy - s * 0.012 * grow), 1, 60, 140, 70, alpha);
		filledCircleRGBA(surface, (int)fx, (int)(fy - s * 0.012 * grow), 2, f % 2 ? 250 : 240, f % 3 ? 140 : 230, f % 2 ? 190 : 100, weapon_alpha(235.0 * grow * event->fade));
	}
}

// A cableway: two pylons hold a long cable and lit gondolas glide up and down it past each other.
static void hill_cableway(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double x1 = surface->w * 0.18, x2 = surface->w * 0.82;
	double g1 = hill_ground(event, x1), g2 = hill_ground(event, x2);
	double up = smooth_step(0.0, 0.12, event->progress);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double t1 = g1 - s * 0.12 * up, t2 = g2 - s * 0.2 * up;
	thickLineRGBA(surface, (int)x1, (int)g1, (int)x1, (int)t1, 2, 140, 144, 154, alpha);
	thickLineRGBA(surface, (int)x2, (int)g2, (int)x2, (int)t2, 2, 140, 144, 154, alpha);
	lineRGBA(surface, (int)x1, (int)t1, (int)x2, (int)t2, 190, 194, 204, alpha);
	lineRGBA(surface, (int)x1, (int)(t1 + 3), (int)x2, (int)(t2 + 3), 150, 154, 164, alpha);
	double move = smooth_step(0.15, 0.25, event->progress);
	for(int g = 0; g < 4; g++) {
		double u = fmod(event->t * 0.06 + g * 0.25, 1.0);
		if(g % 2) u = 1.0 - u;
		double gx = x1 + (x2 - x1) * u, gy = t1 + (t2 - t1) * u;
		thickLineRGBA(surface, (int)gx, (int)gy, (int)gx, (int)(gy + s * 0.012), 1, 150, 154, 164, alpha);
		boxRGBA(surface, (int)(gx - s * 0.009), (int)(gy + s * 0.012), (int)(gx + s * 0.009), (int)(gy + s * 0.026), 200, 70, 60, weapon_alpha(235.0 * move * event->fade));
		boxRGBA(surface, (int)(gx - s * 0.006), (int)(gy + s * 0.015), (int)(gx + s * 0.006), (int)(gy + s * 0.021), 255, 224, 150, weapon_alpha(235.0 * move * event->fade));
	}
}

// Old stone ruins: pillars light up with runes one after another and a few fragments float above them.
static void hill_ruins(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 21));
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	static const Uint8 rune[3] = { 120, 230, 255 };
	for(int i = 0; i < 6; i++) {
		double x = cx + (i - 2.5) * s * 0.05;
		double ground = hill_ground(event, x);
		double rise = smooth_step(0.0 + i * 0.03, 0.1 + i * 0.03, event->progress);
		double h = s * (0.03 + 0.025 * (i % 3) + 0.01 * hill_random(event, i, 22)) * rise;
		boxRGBA(surface, (int)(x - s * 0.009), (int)(ground - h), (int)(x + s * 0.009), (int)ground, 110, 108, 114, alpha);
		boxRGBA(surface, (int)(x - s * 0.009), (int)(ground - h), (int)(x - s * 0.004), (int)ground, 138, 136, 142, alpha);
		if(i % 3 != 2) boxRGBA(surface, (int)(x - s * 0.012), (int)(ground - h - s * 0.005), (int)(x + s * 0.012), (int)(ground - h), 126, 124, 130, alpha);
		double pulse = sin(event->t * 2.2 - i * 0.9);
		double on = smooth_step(0.2, 0.3, event->progress) * (pulse > 0 ? pulse : 0.0);
		weapon_glow(surface, x, ground - h * 0.55, 1.1, rune, 230.0 * on * event->fade);
		weapon_glow(surface, x, ground - h * 0.3, 0.8, rune, 200.0 * on * event->fade);
	}
	for(int k = 0; k < 4; k++) {
		double a = event->t * 0.6 + k * 1.6;
		double fx = cx + sin(a) * s * 0.07, fy = hill_ground(event, cx) - s * (0.12 + 0.02 * sin(a * 1.3 + k));
		filledEllipseRGBA(surface, (int)fx, (int)fy, (int)(s * 0.008) + 1, (int)(s * 0.005) + 1, 128, 126, 134, weapon_alpha(235.0 * smooth_step(0.25, 0.35, event->progress) * event->fade));
	}
}

// A geodesic dome whose panels light up in drifting patterns of colour.
static void hill_geodome(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 23));
	double ground = hill_ground(event, cx);
	double r = s * 0.06 * smooth_step(0.0, 0.1, event->progress);
	Uint8 alpha = weapon_alpha(238.0 * event->fade);
	if(r < 2) return;
	filledPieRGBA(surface, (int)cx, (int)ground, (int)r, 180, 360, 70, 80, 100, weapon_alpha(120.0 * event->fade));
	for(int ring = 1; ring <= 3; ring++) {
		double rr = r * ring / 3.0;
		int segs = 4 + ring * 3;
		for(int k = 0; k < segs; k++) {
			double a0 = 3.14159 + 3.14159 * k / segs, a1 = 3.14159 + 3.14159 * (k + 1) / segs;
			double am = (a0 + a1) * 0.5, rm = rr - r / 6.0;
			lineRGBA(surface, (int)(cx + cos(a0) * rr), (int)(ground + sin(a0) * rr), (int)(cx + cos(a1) * rr), (int)(ground + sin(a1) * rr), 180, 190, 210, alpha);
			lineRGBA(surface, (int)(cx + cos(a0) * rr), (int)(ground + sin(a0) * rr), (int)(cx + cos(a0) * (rr - r / 3.0)), (int)(ground + sin(a0) * (rr - r / 3.0)), 180, 190, 210, alpha);
			double hue = event->t * 0.6 + k * 0.7 + ring;
			double lit = 0.5 + 0.5 * sin(hue * 1.7);
			filledCircleRGBA(surface, (int)(cx + cos(am) * rm), (int)(ground + sin(am) * rm), (int)(r * 0.05) + 1, (Uint8)(150 + 100 * sin(hue)), (Uint8)(170 + 80 * sin(hue + 2.1)), (Uint8)(210 + 40 * sin(hue + 4.2)), weapon_alpha(230.0 * lit * smooth_step(0.15, 0.25, event->progress) * event->fade));
		}
	}
	lineRGBA(surface, (int)(cx - r), (int)ground, (int)(cx + r), (int)ground, 180, 190, 210, alpha);
}

// A log cabin: its windows light up one by one and smoke curls from the chimney.
static void hill_cabin(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 24));
	double ground = hill_ground(event, cx);
	double up = smooth_step(0.0, 0.1, event->progress);
	double w = s * 0.045, h = s * 0.035 * up;
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	static const Uint8 lamp[3] = { 255, 200, 110 };
	boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)ground, 104, 70, 52, alpha);
	for(int log = 1; log < 4; log++) lineRGBA(surface, (int)(cx - w), (int)(ground - h * log / 4.0), (int)(cx + w), (int)(ground - h * log / 4.0), 70, 46, 36, alpha);
	filledTrigonRGBA(surface, (int)(cx - w * 1.2), (int)(ground - h), (int)(cx + w * 1.2), (int)(ground - h), (int)cx, (int)(ground - h - s * 0.03 * up), 72, 50, 54, alpha);
	boxRGBA(surface, (int)(cx + w * 0.4), (int)(ground - h - s * 0.03 * up), (int)(cx + w * 0.65), (int)(ground - h - s * 0.008), 90, 80, 84, alpha);
	for(int win = 0; win < 2; win++) {
		double on = smooth_step(0.15 + win * 0.12, 0.22 + win * 0.12, event->progress) * (0.9 + 0.1 * sin(event->t * 7.0 + win));
		double wx = cx - w * 0.55 + win * w * 0.9;
		boxRGBA(surface, (int)(wx - w * 0.16), (int)(ground - h * 0.75), (int)(wx + w * 0.16), (int)(ground - h * 0.4), lamp[0], lamp[1], lamp[2], weapon_alpha(235.0 * on * event->fade));
		weapon_glow(surface, wx, ground - h * 0.58, w * 0.25, lamp, 70.0 * on * event->fade);
	}
	boxRGBA(surface, (int)(cx + w * 0.55), (int)(ground - h * 0.6), (int)(cx + w * 0.85), (int)ground, 58, 38, 32, alpha);
	for(int p = 0; p < 12; p++) {
		double u = fmod(event->t * 0.18 + p / 12.0, 1.0);
		double sx = cx + w * 0.52 + sin(event->t * 1.2 + p) * s * 0.008 * u * 3.0 + u * s * 0.03 * event->direction;
		double sy = ground - h - s * 0.035 - u * s * 0.14;
		filledCircleRGBA(surface, (int)sx, (int)sy, (int)(2 + u * s * 0.008), 150, 152, 160, weapon_alpha(120.0 * (1.0 - u) * smooth_step(0.1, 0.2, event->progress) * event->fade));
	}
}

// Wind chimes hang from three poles; the tubes swing and a faint ring spreads when they strike.
static void hill_windchimes(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	for(int pole = 0; pole < 3; pole++) {
		double px = surface->w * (0.25 + 0.25 * pole) + (hill_random(event, pole, 25) - 0.5) * s * 0.04;
		double ground = hill_ground(event, px);
		double h = s * 0.07 * smooth_step(0.0 + pole * 0.04, 0.1 + pole * 0.04, event->progress);
		thickLineRGBA(surface, (int)px, (int)ground, (int)px, (int)(ground - h), 2, 110, 84, 66, alpha);
		thickLineRGBA(surface, (int)(px - s * 0.02), (int)(ground - h), (int)(px + s * 0.02), (int)(ground - h), 2, 130, 100, 78, alpha);
		for(int tube = 0; tube < 5; tube++) {
			double tx = px - s * 0.016 + tube * s * 0.008;
			double swing = sin(event->t * (1.6 + 0.2 * tube) + pole + tube * 0.7) * 0.18;
			double len = s * (0.016 + 0.007 * ((tube + pole) % 3));
			double ex = tx + sin(swing) * len, ey = ground - h + cos(swing) * len;
			thickLineRGBA(surface, (int)tx, (int)(ground - h), (int)ex, (int)ey, 2, 210, 215, 225, alpha);
			if(fabs(swing) > 0.17) {
				double r = (fabs(swing) - 0.17) / 0.01 * s * 0.01;
				ellipseRGBA(surface, (int)ex, (int)ey, (int)(4 + r), (int)(2 + r * 0.5), 235, 245, 255, weapon_alpha(130.0 * event->fade));
			}
		}
	}
}

// A picnic blanket with a basket and two friends; a lantern at the middle lights their faces.
static void hill_picnic(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 26));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double fin = smooth_step(0.0, 0.1, event->progress);
	static const Uint8 lamp[3] = { 255, 205, 120 };
	double w = s * 0.05 * fin, flutter = sin(event->t * 3.0) * 1.0;
	for(int k = 0; k < 6; k++) {
		for(int j = 0; j < 2; j++) {
			Uint8 c = ((k + j) % 2) ? 200 : 120;
			boxRGBA(surface, (int)(cx - w + k * w / 3.0), (int)(ground - s * 0.006 * (j + 1) + (k == 5 ? flutter : 0)), (int)(cx - w + (k + 1) * w / 3.0), (int)(ground - s * 0.006 * j), c, (Uint8)(c / 2), (Uint8)(c / 3), alpha);
		}
	}
	boxRGBA(surface, (int)(cx + w * 0.4), (int)(ground - s * 0.018), (int)(cx + w * 0.7), (int)(ground - s * 0.006), 140, 100, 70, alpha);
	for(int p = 0; p < 2; p++) {
		double px = cx + (p ? 1.0 : -1.0) * w * 0.65;
		filledTrigonRGBA(surface, (int)(px - s * 0.008), (int)(ground - s * 0.006), (int)(px + s * 0.008), (int)(ground - s * 0.006), (int)px, (int)(ground - s * 0.026), p ? 60 : 150, 70, p ? 150 : 70, alpha);
		filledCircleRGBA(surface, (int)px, (int)(ground - s * 0.03), (int)(s * 0.005) + 1, 228, 190, 160, alpha);
	}
	weapon_glow(surface, cx, ground - s * 0.012, 2.2, lamp, 220.0 * smooth_step(0.1, 0.2, event->progress) * event->fade * (0.85 + 0.15 * sin(event->t * 6.0)));
}

// A wishing tree: ribbons and glowing paper tags hang from its branches and flutter in a breeze.
static void hill_wishtree(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 27));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double grow = smooth_step(0.0, 0.12, event->progress);
	double h = s * 0.09 * grow;
	thickLineRGBA(surface, (int)cx, (int)ground, (int)cx, (int)(ground - h), 4, 76, 56, 48, alpha);
	for(int b = 0; b < 6; b++) {
		double side = b % 2 ? 1.0 : -1.0;
		double bx = cx + side * s * 0.05 * (0.6 + 0.4 * (b / 2) / 2.0), by = ground - h * (0.55 + 0.1 * (b / 2));
		thickLineRGBA(surface, (int)cx, (int)(ground - h * (0.45 + 0.1 * (b / 2))), (int)bx, (int)by, 2, 84, 62, 52, alpha);
		for(int r = 0; r < 4; r++) {
			double rx = cx + (bx - cx) * (0.4 + 0.2 * r);
			double ry = (ground - h * (0.45 + 0.1 * (b / 2))) + (by - (ground - h * (0.45 + 0.1 * (b / 2)))) * (0.4 + 0.2 * r);
			double len = s * (0.02 + 0.01 * hill_random(event, b * 4 + r, 28)) * smooth_step(0.1 + b * 0.03, 0.25 + b * 0.03, event->progress);
			double sway = sin(event->t * 1.8 + b + r * 0.6) * len * 0.3;
			double hue = b * 0.9 + r;
			Uint8 cr = (Uint8)(190 + 60 * sin(hue)), cg = (Uint8)(150 + 80 * sin(hue + 2.1)), cb = (Uint8)(180 + 70 * sin(hue + 4.2));
			thickLineRGBA(surface, (int)rx, (int)ry, (int)(rx + sway), (int)(ry + len), 1, cr, cg, cb, alpha);
			double glow = 0.5 + 0.5 * sin(event->t * 2.0 + b * 1.3 + r);
			boxRGBA(surface, (int)(rx + sway - 1), (int)(ry + len), (int)(rx + sway + 2), (int)(ry + len + 4), 255, 240, 200, weapon_alpha((130.0 + 120.0 * glow) * event->fade));
		}
	}
}

// A mine mouth in the hillside: loaded ore carts roll out of the dark, their lamps sweeping the rails.
static void hill_mine(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 29));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double up = smooth_step(0.0, 0.1, event->progress);
	double r = s * 0.035 * up;
	filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(r * 1.5), (int)r, 60, 56, 62, alpha);
	filledEllipseRGBA(surface, (int)cx, (int)(ground + 1), (int)r, (int)(r * 0.75), 10, 10, 14, alpha);
	thickLineRGBA(surface, (int)(cx - r), (int)(ground - r * 0.9), (int)(cx - r), (int)ground, 2, 110, 76, 56, alpha);
	thickLineRGBA(surface, (int)(cx + r), (int)(ground - r * 0.9), (int)(cx + r), (int)ground, 2, 110, 76, 56, alpha);
	lineRGBA(surface, (int)cx, (int)(ground - 1), (int)(cx + d * s * 0.3), (int)(ground + 2), 130, 130, 140, alpha);
	static const Uint8 ore[3] = { 120, 240, 255 };
	for(int c = 0; c < 3; c++) {
		double u = fmod(event->t * 0.07 + c / 3.0, 1.0);
		double x = cx + d * s * 0.04 + d * u * s * 0.28, y = ground + u * 2.0 - s * 0.003;
		boxRGBA(surface, (int)(x - s * 0.009), (int)(y - s * 0.013), (int)(x + s * 0.009), (int)y, 100, 84, 70, weapon_alpha(235.0 * smooth_step(0.0, 0.1, u) * event->fade));
		filledCircleRGBA(surface, (int)(x - 3), (int)(y - s * 0.014), 2, 140, 220, 255, weapon_alpha(235.0 * smooth_step(0.0, 0.1, u) * event->fade));
		weapon_glow(surface, x + 2, y - s * 0.016, 1.0, ore, 170.0 * (0.6 + 0.4 * sin(event->t * 5.0 + c)) * smooth_step(0.0, 0.1, u) * event->fade);
		filledCircleRGBA(surface, (int)(x - s * 0.006), (int)y, 2, 30, 30, 36, weapon_alpha(235.0 * event->fade));
		filledCircleRGBA(surface, (int)(x + s * 0.006), (int)y, 2, 30, 30, 36, weapon_alpha(235.0 * event->fade));
	}
}

// Large glowing eggs lie in a nest; they crack in turn and small bright wyrmlings climb out and flutter up.
static void hill_eggs(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 30));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	for(int i = 0; i < 5; i++) {
		double x = cx + (i - 2) * s * 0.035;
		double y = ground - s * 0.012 + (i % 2) * 2.0;
		double pulse = 0.5 + 0.5 * sin(event->t * 1.5 + i);
		double hatch = smooth_step(0.35 + i * 0.07, 0.45 + i * 0.07, event->progress);
		Uint8 tint[3] = { (Uint8)(120 + 80 * (i % 3) / 2), 220, (Uint8)(190 + 40 * (i % 2)) };
		filledEllipseRGBA(surface, (int)x, (int)y, (int)(s * 0.012), (int)(s * 0.017), 96, 92, 110, alpha);
		filledEllipseRGBA(surface, (int)x, (int)(y - 1), (int)(s * 0.009), (int)(s * 0.013), tint[0], tint[1], tint[2], weapon_alpha((110.0 + 100.0 * pulse) * (1.0 - hatch * 0.6) * event->fade));
		if(hatch > 0.0) {
			lineRGBA(surface, (int)(x - s * 0.01), (int)y, (int)x, (int)(y - s * 0.006), 20, 20, 28, alpha);
			lineRGBA(surface, (int)x, (int)(y - s * 0.006), (int)(x + s * 0.01), (int)(y - s * 0.012), 20, 20, 28, alpha);
			double fly = smooth_step(0.0, 1.0, fmod(event->t * 0.15 + i * 0.2, 1.0)) * hatch;
			double wx = x + sin(event->t * 2.0 + i) * s * 0.02 * fly, wy = y - s * 0.02 - fly * s * 0.1;
			double flap = sin(event->t * 14.0 + i) * s * 0.006;
			thickLineRGBA(surface, (int)(wx - s * 0.007), (int)(wy - flap), (int)wx, (int)wy, 1, tint[0], tint[1], tint[2], weapon_alpha(235.0 * (1.0 - fly * 0.5) * event->fade));
			thickLineRGBA(surface, (int)(wx + s * 0.007), (int)(wy - flap), (int)wx, (int)wy, 1, tint[0], tint[1], tint[2], weapon_alpha(235.0 * (1.0 - fly * 0.5) * event->fade));
			weapon_glow(surface, wx, wy, 1.2, tint, 200.0 * (1.0 - fly * 0.5) * event->fade);
		}
	}
	for(int n = 0; n < 12; n++) {
		double a = n * 0.52;
		filledCircleRGBA(surface, (int)(cx + cos(a) * s * 0.1), (int)(ground + sin(a) * 1.5), 2, 90, 70, 50, alpha);
	}
}

// Two trees hold a hammock that sways gently, a sleeper in it and a lantern glowing on a branch.
static void hill_hammock(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 31));
	double span = s * 0.09;
	double g1 = hill_ground(event, cx - span), g2 = hill_ground(event, cx + span);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double grow = smooth_step(0.0, 0.1, event->progress);
	double h = s * 0.06 * grow;
	static const Uint8 lamp[3] = { 255, 205, 120 };
	for(int side = -1; side <= 1; side += 2) {
		double tx = cx + side * span, tg = side < 0 ? g1 : g2;
		thickLineRGBA(surface, (int)tx, (int)tg, (int)tx, (int)(tg - h), 3, 84, 62, 50, alpha);
		filledCircleRGBA(surface, (int)tx, (int)(tg - h - s * 0.012), (int)(s * 0.022 * grow) + 1, 48, 104, 62, alpha);
	}
	double sway = sin(event->t * 0.9) * s * 0.004;
	double px = cx - span, py = g1 - h * 0.55, qx = cx + span, qy = g2 - h * 0.55;
	double prevx = px, prevy = py;
	for(int i = 1; i <= 16; i++) {
		double f = i / 16.0;
		double hx = px + (qx - px) * f + sway * sin(f * 3.14159) * 2.0;
		double hy = py + (qy - py) * f + sin(f * 3.14159) * s * 0.02;
		thickLineRGBA(surface, (int)prevx, (int)prevy, (int)hx, (int)hy, 2, 210, 120, 100, alpha);
		prevx = hx;
		prevy = hy;
	}
	double mx = cx + sway * 2.0, my = (py + qy) / 2 + s * 0.02;
	filledEllipseRGBA(surface, (int)mx, (int)(my - s * 0.005), (int)(s * 0.016), (int)(s * 0.005) + 1, 24, 28, 40, alpha);
	filledCircleRGBA(surface, (int)(mx - s * 0.016), (int)(my - s * 0.008), (int)(s * 0.005) + 1, 24, 28, 40, alpha);
	weapon_glow(surface, cx - span * 0.9, g1 - h * 0.85, 2.0, lamp, 210.0 * smooth_step(0.1, 0.2, event->progress) * event->fade * (0.9 + 0.1 * sin(event->t * 4.0)));
}

// A line of telegraph poles runs along the ridge; pulses of light race down the wires between them.
static void hill_telegraph(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	const int poles = 7;
	double px[poles], py[poles];
	for(int i = 0; i < poles; i++) {
		px[i] = surface->w * (0.1 + 0.8 * i / (poles - 1));
		double g = hill_ground(event, px[i]);
		double h = s * 0.07 * smooth_step(0.0 + i * 0.03, 0.1 + i * 0.03, event->progress);
		py[i] = g - h;
		thickLineRGBA(surface, (int)px[i], (int)g, (int)px[i], (int)py[i], 2, 100, 78, 62, alpha);
		thickLineRGBA(surface, (int)(px[i] - s * 0.012), (int)(py[i] + 2), (int)(px[i] + s * 0.012), (int)(py[i] + 2), 2, 120, 94, 74, alpha);
	}
	static const Uint8 pulse[3] = { 150, 235, 255 };
	for(int i = 0; i + 1 < poles; i++) {
		for(int w = -1; w <= 1; w += 2) {
			double ax = px[i] + w * s * 0.01, ay = py[i] + 2, bx = px[i + 1] + w * s * 0.01, by = py[i + 1] + 2;
			double prevx = ax, prevy = ay;
			for(int k = 1; k <= 10; k++) {
				double f = k / 10.0;
				double nx = ax + (bx - ax) * f, ny = ay + (by - ay) * f + sin(f * 3.14159) * s * 0.012;
				lineRGBA(surface, (int)prevx, (int)prevy, (int)nx, (int)ny, 150, 150, 160, alpha);
				prevx = nx;
				prevy = ny;
			}
			double u = fmod(event->t * 0.5 + i * 0.37 + (w > 0 ? 0.0 : 0.5), 1.0) * 1.0;
			double dx = event->direction > 0 ? u : 1.0 - u;
			weapon_glow(surface, ax + (bx - ax) * dx, ay + (by - ay) * dx + sin(dx * 3.14159) * s * 0.012, 1.0, pulse, 230.0 * smooth_step(0.2, 0.3, event->progress) * event->fade);
		}
	}
}

// A chimney-lit village of tiny houses on a hillside: windows blink on at random, and a church bell swings.
static void hill_hamlet(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	static const Uint8 lamp[3] = { 255, 205, 120 };
	double start = surface->w * (0.2 + 0.2 * hill_random(event, 0, 40));
	for(int i = 0; i < 7; i++) {
		double x = start + i * s * 0.07;
		double ground = hill_ground(event, x);
		double up = smooth_step(0.0 + i * 0.03, 0.08 + i * 0.03, event->progress);
		double w = s * (0.016 + 0.006 * (i % 3)), h = s * (0.02 + 0.008 * ((i + 1) % 3)) * up;
		boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x + w), (int)ground, 74 + i * 6, 62, 70, alpha);
		filledTrigonRGBA(surface, (int)(x - w * 1.25), (int)(ground - h), (int)(x + w * 1.25), (int)(ground - h), (int)x, (int)(ground - h - w * 1.1), 120, 60, 56, alpha);
		double blink = fmod(event->t * 0.15 + hill_random(event, i, 41), 1.0);
		double on = smooth_step(0.2, 0.3, event->progress) * (blink < 0.7 ? 1.0 : 0.0);
		boxRGBA(surface, (int)(x - w * 0.3), (int)(ground - h * 0.7), (int)(x + w * 0.3), (int)(ground - h * 0.35), lamp[0], lamp[1], lamp[2], weapon_alpha(235.0 * on * event->fade));
		if(i % 3 == 1) {
			for(int p = 0; p < 5; p++) {
				double u = fmod(event->t * 0.2 + p / 5.0 + i * 0.1, 1.0);
				filledCircleRGBA(surface, (int)(x + w * 0.6 + sin(event->t + p) * 2.0 * u + u * event->direction * s * 0.02), (int)(ground - h - w - u * s * 0.06), (int)(1 + u * 3), 150, 152, 160, weapon_alpha(110.0 * (1.0 - u) * up * event->fade));
			}
		}
	}
	double cx = start + 7 * s * 0.07, cg = hill_ground(event, cx);
	double ch = s * 0.07 * smooth_step(0.1, 0.2, event->progress);
	boxRGBA(surface, (int)(cx - s * 0.01), (int)(cg - ch), (int)(cx + s * 0.01), (int)cg, 96, 90, 100, alpha);
	filledTrigonRGBA(surface, (int)(cx - s * 0.013), (int)(cg - ch), (int)(cx + s * 0.013), (int)(cg - ch), (int)cx, (int)(cg - ch - s * 0.02), 110, 70, 66, alpha);
	double swing = sin(event->t * 2.2) * s * 0.004;
	filledCircleRGBA(surface, (int)(cx + swing), (int)(cg - ch + s * 0.01), (int)(s * 0.003) + 1, 230, 190, 90, weapon_alpha(235.0 * event->fade));
}

// A row of glowing stepping stones lights up across a dark pond, one after another, as a traveller hops along them.
static void hill_stepstones(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * 0.5;
	double ground = hill_ground(event, cx);
	double pw = s * 0.26, ph = s * 0.03;
	static const Uint8 glow[3] = { 130, 220, 255 };
	double fill = smooth_step(0.0, 0.1, event->progress);
	filledEllipseRGBA(surface, (int)cx, (int)(ground - ph * 0.3), (int)(pw * fill) + 1, (int)(ph * fill) + 1, 22, 40, 62, weapon_alpha(225.0 * event->fade));
	const int stones = 8;
	for(int i = 0; i < stones; i++) {
		double f = (double)i / (stones - 1);
		double x = cx + (f - 0.5) * pw * 1.6 * event->direction;
		double y = ground - ph * 0.3 + sin(f * 6.28) * ph * 0.3;
		double on = smooth_step(0.1 + i * 0.05, 0.16 + i * 0.05, event->progress);
		filledEllipseRGBA(surface, (int)x, (int)y, (int)(s * 0.012) + 1, (int)(s * 0.004) + 1, 110, 116, 130, weapon_alpha(235.0 * on * event->fade));
		weapon_glow(surface, x, y, s * 0.004, glow, 150.0 * on * event->fade * (0.7 + 0.3 * sin(event->t * 2.0 + i)));
		double rip = fmod(event->t * 0.5 + i * 0.13, 1.0);
		ellipseRGBA(surface, (int)x, (int)y, (int)(s * 0.01 + rip * s * 0.02), (int)(s * 0.003 + rip * s * 0.006), 170, 210, 240, weapon_alpha(110.0 * (1.0 - rip) * on * event->fade));
	}
	double hop = fmod(event->t * 0.18, 1.0);
	double hf = hop * (stones - 1);
	int k = (int)hf;
	double u = hf - k;
	double f0 = (double)k / (stones - 1), f1 = (double)(k + 1 < stones ? k + 1 : k) / (stones - 1);
	double x0 = cx + (f0 - 0.5) * pw * 1.6 * event->direction, x1 = cx + (f1 - 0.5) * pw * 1.6 * event->direction;
	double px = x0 + (x1 - x0) * u, py = ground - ph * 0.3 - sin(u * 3.14159) * s * 0.02 - s * 0.012;
	hill_walker(surface, px, py + s * 0.012, s * 0.03, event->direction, event->t * 5.0, false, event->fade * smooth_step(0.3, 0.4, event->progress), event->t, 0);
}

// A seesaw rocks on the hill with two small figures; a gentle ripple of dust puffs at each landing.
static void hill_seesaw(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 42));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double fin = smooth_step(0.0, 0.08, event->progress);
	double half = s * 0.05;
	double tilt = sin(event->t * 1.8) * 0.28;
	double piv_y = ground - s * 0.012;
	filledTrigonRGBA(surface, (int)(cx - s * 0.01), (int)ground, (int)(cx + s * 0.01), (int)ground, (int)cx, (int)piv_y, 130, 100, 84, alpha);
	for(int side = -1; side <= 1; side += 2) {
		double ex = cx + side * cos(tilt) * half * fin, ey = piv_y - side * sin(tilt) * half * fin;
		thickLineRGBA(surface, (int)cx, (int)piv_y, (int)ex, (int)ey, 3, 190, 140, 90, alpha);
		double hy = ey - s * 0.004;
		thickLineRGBA(surface, (int)ex, (int)hy, (int)ex, (int)(hy - s * 0.014), 3, side < 0 ? 40 : 150, 60, side < 0 ? 140 : 60, alpha);
		filledCircleRGBA(surface, (int)ex, (int)(hy - s * 0.019), (int)(s * 0.004) + 1, 226, 190, 160, alpha);
		bool landing = (side * sin(tilt) < -0.25);
		if(landing) {
			double q = (fabs(sin(tilt)) - 0.25) / 0.03;
			if(q > 1.0) q = 1.0;
			for(int p = 0; p < 4; p++) {
				filledCircleRGBA(surface, (int)(ex + (p - 1.5) * s * 0.006 * q), (int)(ground - p % 2 * 2 - q * s * 0.006), 2, 150, 130, 100, weapon_alpha(110.0 * (1.0 - q * 0.5) * fin * event->fade));
			}
		}
	}
}

// A line of laundry strung between two poles: sheets and shirts billow in the wind and a lantern glows beneath.
static void hill_laundry(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 43));
	double span = s * 0.1;
	double g1 = hill_ground(event, cx - span), g2 = hill_ground(event, cx + span);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double up = smooth_step(0.0, 0.08, event->progress);
	double h = s * 0.055 * up;
	static const Uint8 lamp[3] = { 255, 205, 120 };
	thickLineRGBA(surface, (int)(cx - span), (int)g1, (int)(cx - span), (int)(g1 - h), 2, 100, 78, 64, alpha);
	thickLineRGBA(surface, (int)(cx + span), (int)g2, (int)(cx + span), (int)(g2 - h), 2, 100, 78, 64, alpha);
	double ax = cx - span, ay = g1 - h, bx = cx + span, by = g2 - h;
	double prevx = ax, prevy = ay;
	for(int i = 1; i <= 14; i++) {
		double f = i / 14.0;
		double nx = ax + (bx - ax) * f, ny = ay + (by - ay) * f + sin(f * 3.14159) * s * 0.01;
		lineRGBA(surface, (int)prevx, (int)prevy, (int)nx, (int)ny, 180, 180, 188, alpha);
		prevx = nx;
		prevy = ny;
	}
	double hang = smooth_step(0.1, 0.2, event->progress);
	for(int k = 0; k < 6; k++) {
		double f = (k + 1) / 7.0;
		double px = ax + (bx - ax) * f, py = ay + (by - ay) * f + sin(f * 3.14159) * s * 0.01;
		double billow = sin(event->t * 2.2 + k * 1.1) * s * 0.006;
		double w = s * (0.011 + 0.005 * (k % 2)), l = s * (0.022 + 0.01 * (k % 3)) * hang;
		static const Uint8 col[3][3] = { { 235, 235, 235 }, { 200, 120, 110 }, { 120, 160, 210 } };
		const Uint8 *c = col[k % 3];
		filledTrigonRGBA(surface, (int)(px - w), (int)py, (int)(px + w), (int)py, (int)(px + billow), (int)(py + l), c[0], c[1], c[2], alpha);
		filledTrigonRGBA(surface, (int)(px - w), (int)py, (int)(px + billow), (int)(py + l), (int)(px - w + billow * 1.5), (int)(py + l), c[0], c[1], c[2], alpha);
	}
	weapon_glow(surface, cx, (g1 + g2) * 0.5 - s * 0.006, 2.0, lamp, 190.0 * smooth_step(0.15, 0.25, event->progress) * event->fade * (0.85 + 0.15 * sin(event->t * 5.0)));
}

// A gentle wooden water-wheel pump by a well: a bucket rises on a rope, tips, and a silver trickle runs off down the hill.
static void hill_well(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 44));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double up = smooth_step(0.0, 0.1, event->progress);
	double w = s * 0.022, h = s * 0.02 * up;
	boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)ground, 118, 112, 120, alpha);
	boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)(ground - h + 2), 150, 144, 152, alpha);
	double post = s * 0.045 * up;
	thickLineRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx - w), (int)(ground - h - post), 3, 100, 74, 58, alpha);
	thickLineRGBA(surface, (int)(cx + w), (int)(ground - h), (int)(cx + w), (int)(ground - h - post), 3, 100, 74, 58, alpha);
	filledTrigonRGBA(surface, (int)(cx - w * 1.5), (int)(ground - h - post), (int)(cx + w * 1.5), (int)(ground - h - post), (int)cx, (int)(ground - h - post - s * 0.02), 120, 70, 60, alpha);
	double cycle = fmod(event->t * 0.25, 1.0);
	double lift = cycle < 0.6 ? cycle / 0.6 : 1.0;
	double by = ground - h - post * 0.15 - lift * post * 0.65;
	thickLineRGBA(surface, (int)cx, (int)(ground - h - post), (int)cx, (int)by, 1, 200, 190, 170, alpha);
	boxRGBA(surface, (int)(cx - s * 0.006), (int)by, (int)(cx + s * 0.006), (int)(by + s * 0.009), 140, 100, 70, alpha);
	if(cycle >= 0.6) {
		double pour = (cycle - 0.6) / 0.4;
		for(int d = 0; d < 8; d++) {
			double u = fmod(pour * 2.0 + d / 8.0, 1.0);
			filledCircleRGBA(surface, (int)(cx + event->direction * (s * 0.008 + u * s * 0.02)), (int)(by + s * 0.01 + u * u * (ground - by)), 1, 190, 225, 250, weapon_alpha(220.0 * (1.0 - u * 0.4) * smooth_step(0.0, 0.2, pour) * event->fade));
		}
	}
	for(int k = 0; k < 14; k++) {
		double u = fmod(event->t * 0.4 + k / 14.0, 1.0);
		double sx = cx + event->direction * (w + u * s * 0.2), sy = hill_ground(event, sx) - 1.0;
		filledCircleRGBA(surface, (int)sx, (int)sy, 1, 180, 215, 245, weapon_alpha(180.0 * (1.0 - u) * smooth_step(0.1, 0.2, event->progress) * event->fade));
	}
}

// A kiln on the hillside glows hot; potters carry glowing pots out one by one and set them in a row to cool.
static void hill_kiln(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double cx = surface->w * (d > 0 ? 0.25 : 0.75);
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double up = smooth_step(0.0, 0.1, event->progress);
	double r = s * 0.03 * up;
	static const Uint8 fire[3] = { 255, 150, 60 };
	filledEllipseRGBA(surface, (int)cx, (int)ground, (int)r, (int)(r * 1.1), 120, 80, 64, alpha);
	filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(r * 0.8), (int)(r * 0.9), 96, 64, 52, alpha);
	boxRGBA(surface, (int)(cx - r * 0.3), (int)(ground - r * 1.1 - s * 0.012), (int)(cx + r * 0.3), (int)(ground - r * 0.9), 104, 72, 58, alpha);
	double heat = smooth_step(0.08, 0.18, event->progress) * (0.85 + 0.15 * sin(event->t * 6.0));
	filledEllipseRGBA(surface, (int)cx, (int)(ground - r * 0.1), (int)(r * 0.35) + 1, (int)(r * 0.4) + 1, 255, 190, 90, weapon_alpha(235.0 * heat * event->fade));
	weapon_glow(surface, cx, ground - r * 0.1, r * 0.4, fire, 120.0 * heat * event->fade);
	for(int p = 0; p < 10; p++) {
		double u = fmod(event->t * 0.2 + p / 10.0, 1.0);
		filledCircleRGBA(surface, (int)(cx + sin(event->t + p) * s * 0.006 * u * 3.0), (int)(ground - r * 1.1 - s * 0.012 - u * s * 0.12), (int)(2 + u * 5), 130, 130, 140, weapon_alpha(100.0 * (1.0 - u) * heat * event->fade));
	}
	const int pots = 6;
	for(int i = 0; i < pots; i++) {
		double ready = 0.25 + i * 0.1;
		double carry = smooth_step(ready, ready + 0.08, event->progress);
		double rest = surface->w * (d > 0 ? 0.42 + i * 0.07 : 0.58 - i * 0.07);
		double px = cx + d * r * 1.2 + (rest - (cx + d * r * 1.2)) * carry;
		double pg = hill_ground(event, px);
		if(event->progress < ready - 0.02) continue;
		double cool = smooth_step(ready + 0.06, ready + 0.3, event->progress);
		double pr = s * 0.007;
		Uint8 cr = (Uint8)(255 - 150 * cool), cg = (Uint8)(150 - 50 * cool), cb = (Uint8)(60 + 30 * cool);
		filledEllipseRGBA(surface, (int)px, (int)(pg - pr * 1.2), (int)pr + 1, (int)(pr * 1.2) + 1, cr, cg, cb, alpha);
		boxRGBA(surface, (int)(px - pr * 0.5), (int)(pg - pr * 2.6), (int)(px + pr * 0.5), (int)(pg - pr * 2.2), cr, cg, cb, alpha);
		if(cool < 0.7) weapon_glow(surface, px, pg - pr * 1.2, pr, fire, 120.0 * (1.0 - cool) * event->fade);
	}
}

// A rope bridge sags between two rises; lanterns hang along it and sway as a traveller crosses.
static void hill_ropebridge(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double x1 = surface->w * 0.2, x2 = surface->w * 0.8;
	double g1 = hill_ground(event, x1), g2 = hill_ground(event, x2);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double up = smooth_step(0.0, 0.12, event->progress);
	static const Uint8 lamp[3] = { 255, 205, 120 };
	thickLineRGBA(surface, (int)x1, (int)g1, (int)x1, (int)(g1 - s * 0.05 * up), 3, 100, 76, 60, alpha);
	thickLineRGBA(surface, (int)x2, (int)g2, (int)x2, (int)(g2 - s * 0.05 * up), 3, 100, 76, 60, alpha);
	const int n = 28;
	double prevx = x1, prevy = g1 - s * 0.03 * up, prevy2 = g1 - s * 0.06 * up;
	for(int i = 1; i <= n; i++) {
		double f = (double)i / n;
		double nx = x1 + (x2 - x1) * f;
		double sag = sin(f * 3.14159) * s * 0.05 * up;
		double deck = g1 + (g2 - g1) * f - s * 0.03 * up + sag + sin(event->t * 1.3 - f * 8.0) * s * 0.0015;
		double rail = deck - s * 0.03 * up;
		thickLineRGBA(surface, (int)prevx, (int)prevy, (int)nx, (int)deck, 2, 130, 98, 76, alpha);
		lineRGBA(surface, (int)prevx, (int)prevy2, (int)nx, (int)rail, 170, 150, 120, alpha);
		if(i % 3 == 0) lineRGBA(surface, (int)nx, (int)deck, (int)nx, (int)rail, 150, 126, 100, (Uint8)(alpha * 0.7));
		if(i % 7 == 0) {
			double sw = sin(event->t * 1.8 + i) * s * 0.003;
			weapon_glow(surface, nx + sw, rail + s * 0.008, 1.6, lamp, 200.0 * smooth_step(0.15, 0.25, event->progress) * event->fade);
		}
		prevx = nx;
		prevy = deck;
		prevy2 = rail;
	}
	double cross = fmod(event->t * 0.04, 1.0);
	double f = event->direction > 0 ? cross : 1.0 - cross;
	double wx = x1 + (x2 - x1) * f;
	double wy = g1 + (g2 - g1) * f - s * 0.03 * up + sin(f * 3.14159) * s * 0.05 * up;
	hill_walker(surface, wx, wy, s * 0.03, event->direction, event->t * 5.0, true, event->fade * smooth_step(0.2, 0.3, event->progress), event->t, 0);
}

// A pottery-wheel-sized bell tower chimes: bells of different sizes swing in turn and each ring sends a pulse of light.
static void hill_belltower(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 45));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double up = smooth_step(0.0, 0.12, event->progress);
	double w = s * 0.034, h = s * 0.062 * up;
	static const Uint8 glow[3] = { 255, 225, 150 };
	boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)ground, 108, 98, 110, alpha);
	boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx - w * 0.4), (int)ground, 132, 122, 134, alpha);
	boxRGBA(surface, (int)(cx - w * 1.15), (int)(ground - h - s * 0.012), (int)(cx + w * 1.15), (int)(ground - h), 92, 84, 96, alpha);
	filledTrigonRGBA(surface, (int)(cx - w * 1.15), (int)(ground - h - s * 0.012), (int)(cx + w * 1.15), (int)(ground - h - s * 0.012), (int)cx, (int)(ground - h - s * 0.05), 120, 64, 60, alpha);
	boxRGBA(surface, (int)(cx - w * 0.6), (int)(ground - h * 0.55), (int)(cx + w * 0.6), (int)(ground - h * 0.35), 20, 18, 26, alpha);
	for(int b = 0; b < 3; b++) {
		double bx = cx + (b - 1) * w * 0.7, top = ground - h - s * 0.002;
		double phase = event->t * 1.2 - b * 0.9;
		double swing = sin(phase * 3.0) * 0.5 * smooth_step(0.15, 0.25, event->progress);
		double len = s * (0.012 + 0.003 * (2 - b));
		double ex = bx + sin(swing) * len, ey = top + cos(swing) * len;
		lineRGBA(surface, (int)bx, (int)top, (int)ex, (int)(ey - s * 0.004), 170, 160, 140, alpha);
		filledTrigonRGBA(surface, (int)(ex - s * 0.009), (int)(ey + s * 0.008), (int)(ex + s * 0.009), (int)(ey + s * 0.008), (int)ex, (int)(ey - s * 0.007), 240, 196, 90, alpha);
		if(fabs(swing) > 0.45) {
			double q = (fabs(swing) - 0.45) / 0.05;
			if(q > 1.0) q = 1.0;
			ellipseRGBA(surface, (int)ex, (int)ey, (int)(s * 0.01 + q * s * 0.03), (int)(s * 0.01 + q * s * 0.03), 255, 230, 160, weapon_alpha(150.0 * (1.0 - q * 0.5) * event->fade));
			weapon_glow(surface, ex, ey, 1.2, glow, 170.0 * event->fade);
		}
	}
}

// A hedge maze of dark walls; a tiny lantern wanders its paths and, at the end, the centre blooms with light.
static void hill_maze(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 46));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(235.0 * event->fade);
	double w = s * 0.11, h = s * 0.022;
	double grow = smooth_step(0.0, 0.12, event->progress);
	static const Uint8 lamp[3] = { 255, 215, 130 };
	static const Uint8 bloom[3] = { 255, 170, 200 };
	filledEllipseRGBA(surface, (int)cx, (int)(ground - 1), (int)(w * 1.1), (int)(h * 0.9) + 1, 24, 40, 30, weapon_alpha(200.0 * grow * event->fade));
	for(int ring = 0; ring < 3; ring++) {
		double rw = w * (1.0 - ring * 0.3), rh = h * (1.0 - ring * 0.3);
		for(int k = 0; k < 16; k++) {
			double a0 = k * 0.3927 + ring * 0.2, a1 = a0 + 0.3927;
			bool gap = ((k + ring * 5) % 7 == 0);
			if(gap) continue;
			double x0 = cx + cos(a0) * rw, y0 = ground - 1 + sin(a0) * rh;
			double x1 = cx + cos(a1) * rw, y1 = ground - 1 + sin(a1) * rh;
			double top = s * 0.022 * grow;
			Uint8 g = (Uint8)(50 + ring * 10);
			filledTrigonRGBA(surface, (int)x0, (int)y0, (int)x1, (int)y1, (int)x0, (int)(y0 - top), 34, g, 44, alpha);
			filledTrigonRGBA(surface, (int)x1, (int)y1, (int)x1, (int)(y1 - top), (int)x0, (int)(y0 - top), 40, (Uint8)(g + 14), 50, alpha);
		}
	}
	double trip = fmod(event->t * 0.1, 1.0);
	double a = trip * 6.2832 * 2.4, r = 1.0 - 0.9 * trip;
	double lx = cx + cos(a) * w * 0.85 * r, ly = ground - 1 + sin(a) * h * 0.85 * r - s * 0.006;
	weapon_glow(surface, lx, ly - s * 0.008, 2.6, lamp, 235.0 * smooth_step(0.15, 0.25, event->progress) * event->fade);
	double end = smooth_step(0.8, 0.95, trip);
	for(int p = 0; p < 8; p++) {
		double pa = p * 0.785 + event->t;
		weapon_glow(surface, cx + cos(pa) * s * 0.012 * end, ground - s * 0.012 + sin(pa) * s * 0.004 * end, 1.0, bloom, 190.0 * end * event->fade);
	}
}

// A fisherman sits on a rock beside a dark pool, casting a line; a fish rises, rings spread and the float bobs.
static void hill_fisher(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 47));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double fin = smooth_step(0.0, 0.1, event->progress);
	double pw = s * 0.12, ph = s * 0.02;
	filledEllipseRGBA(surface, (int)(cx + d * pw * 0.5), (int)(ground - ph * 0.3), (int)(pw * fin) + 1, (int)(ph * fin) + 1, 22, 40, 62, weapon_alpha(225.0 * event->fade));
	double rx = cx - d * s * 0.02;
	filledEllipseRGBA(surface, (int)rx, (int)(ground - s * 0.005), (int)(s * 0.016), (int)(s * 0.008) + 1, 96, 92, 100, alpha);
	double hx = rx, hy = ground - s * 0.012;
	thickLineRGBA(surface, (int)hx, (int)hy, (int)hx, (int)(hy - s * 0.016), 3, 120, 70, 60, alpha);
	filledCircleRGBA(surface, (int)hx, (int)(hy - s * 0.022), (int)(s * 0.005) + 1, 226, 190, 160, alpha);
	filledTrigonRGBA(surface, (int)(hx - d * s * 0.01), (int)(hy - s * 0.026), (int)(hx + d * s * 0.01), (int)(hy - s * 0.026), (int)hx, (int)(hy - s * 0.036), 150, 120, 80, alpha);
	double tipx = hx + d * s * 0.07, tipy = hy - s * 0.07;
	thickLineRGBA(surface, (int)(hx + d * s * 0.006), (int)(hy - s * 0.012), (int)tipx, (int)tipy, 1, 150, 110, 80, alpha);
	double fx = cx + d * pw * 0.55, fy = ground - ph * 0.3;
	double bite = fmod(event->t * 0.2, 1.0);
	double bob = bite > 0.8 ? sin((bite - 0.8) * 50.0) * s * 0.003 : sin(event->t * 1.5) * s * 0.0008;
	lineRGBA(surface, (int)tipx, (int)tipy, (int)fx, (int)(fy + bob), 210, 210, 220, (Uint8)(alpha * 0.8));
	filledCircleRGBA(surface, (int)fx, (int)(fy + bob), 2, 235, 80, 70, alpha);
	for(int r = 0; r < 3; r++) {
		double u = fmod(event->t * 0.4 + r / 3.0, 1.0);
		ellipseRGBA(surface, (int)fx, (int)fy, (int)(3 + u * s * 0.025), (int)(1 + u * s * 0.008), 190, 215, 240, weapon_alpha(150.0 * (1.0 - u) * fin * event->fade));
	}
	if(bite > 0.8) {
		double q = (bite - 0.8) / 0.2;
		double jx = fx + d * q * s * 0.012, jy = fy - sin(q * 3.14159) * s * 0.03;
		filledEllipseRGBA(surface, (int)jx, (int)jy, (int)(s * 0.006) + 1, (int)(s * 0.003) + 1, 200, 215, 235, weapon_alpha(230.0 * event->fade));
	}
}

// A lone cactus-like crystal tree blooms with tiny lanterns, each one lighting up and floating gently away.
static void hill_lanterntree(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 48));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double grow = smooth_step(0.0, 0.12, event->progress);
	double h = s * 0.08 * grow;
	thickLineRGBA(surface, (int)cx, (int)ground, (int)cx, (int)(ground - h), 5, 120, 100, 88, alpha);
	for(int b = 0; b < 4; b++) {
		double side = b % 2 ? 1.0 : -1.0;
		double by = ground - h * (0.4 + 0.15 * b);
		double ex = cx + side * s * (0.025 + 0.006 * b) * grow, ey = by - s * 0.02 * grow;
		thickLineRGBA(surface, (int)cx, (int)by, (int)ex, (int)ey, 3, 124, 104, 92, alpha);
	}
	for(int l = 0; l < 16; l++) {
		double u = fmod(event->t * 0.07 + hill_random(event, l, 49), 1.0);
		double born = smooth_step(0.0, 0.15, u), fade = 1.0 - smooth_step(0.75, 1.0, u);
		double side = (hill_random(event, l, 50) - 0.5) * 2.0;
		double lx = cx + side * s * 0.03 + sin(event->t * 0.5 + l) * s * 0.01 * u + u * event->direction * s * 0.06;
		double ly = ground - h * (0.7 + 0.3 * hill_random(event, l, 51)) - u * s * 0.18;
		Uint8 c[3] = { (Uint8)(255 - 40 * (l % 3)), (Uint8)(190 + 20 * (l % 2)), (Uint8)(110 + 40 * (l % 4)) };
		weapon_glow(surface, lx, ly, 3.2, c, 235.0 * born * fade * smooth_step(0.1, 0.2, event->progress) * event->fade);
	}
}

// A blacksmith's forge: the anvil rings, a hammer rises and falls, and a fan of orange sparks leaps from each strike.
static void hill_forge(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	int d = event->direction;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 52));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double up = smooth_step(0.0, 0.1, event->progress);
	static const Uint8 fire[3] = { 255, 160, 70 };
	double w = s * 0.03 * up, h = s * 0.03 * up;
	boxRGBA(surface, (int)(cx - d * s * 0.04 - w), (int)(ground - h), (int)(cx - d * s * 0.04 + w), (int)ground, 90, 70, 66, alpha);
	boxRGBA(surface, (int)(cx - d * s * 0.04 - w * 0.5), (int)(ground - h * 0.7), (int)(cx - d * s * 0.04 + w * 0.5), (int)(ground - h * 0.2), 255, 170, 80, weapon_alpha(235.0 * smooth_step(0.1, 0.2, event->progress) * (0.85 + 0.15 * sin(event->t * 8.0)) * event->fade));
	weapon_glow(surface, cx - d * s * 0.04, ground - h * 0.45, w * 0.7, fire, 100.0 * smooth_step(0.1, 0.2, event->progress) * event->fade);
	double ax = cx + d * s * 0.02, ay = ground - s * 0.012;
	boxRGBA(surface, (int)(ax - s * 0.018), (int)(ay - s * 0.009), (int)(ax + s * 0.018), (int)ay, 120, 124, 138, alpha);
	boxRGBA(surface, (int)(ax - s * 0.009), (int)ay, (int)(ax + s * 0.009), (int)ground, 100, 104, 116, alpha);
	double beat = fmod(event->t * 1.1, 1.0);
	double swing = beat < 0.7 ? smooth_step(0.0, 0.7, beat) : 1.0 - smooth_step(0.7, 0.78, beat);
	double handx = ax - d * s * 0.05, handy = ground - s * 0.03;
	double tipx = ax - d * s * 0.004, tipy = ay - s * 0.008 - (1.0 - swing) * s * 0.05;
	thickLineRGBA(surface, (int)handx, (int)ground, (int)handx, (int)(handy - s * 0.004), 4, 150, 120, 110, alpha);
	thickLineRGBA(surface, (int)handx, (int)handy, (int)tipx, (int)tipy, 3, 190, 150, 110, alpha);
	filledCircleRGBA(surface, (int)tipx, (int)tipy, (int)(s * 0.004) + 1, 150, 154, 168, alpha);
	filledCircleRGBA(surface, (int)handx, (int)(handy - s * 0.012), (int)(s * 0.006) + 1, 232, 196, 166, alpha);
	if(beat >= 0.7 && beat < 1.0) {
		double q = (beat - 0.7) / 0.3;
		for(int p = 0; p < 12; p++) {
			double a = -3.14159 * (0.1 + 0.8 * p / 11.0);
			double r = q * s * (0.025 + 0.02 * hill_random(event, p, 53));
			filledCircleRGBA(surface, (int)(ax + cos(a) * r), (int)(ay + sin(a) * r + q * q * s * 0.02), 1, 255, 200, 110, weapon_alpha(230.0 * (1.0 - q) * event->fade));
		}
		weapon_glow(surface, ax, ay - s * 0.004, 1.6, fire, 200.0 * (1.0 - q) * event->fade);
	}
}

// A great bonsai-like tree on a rock holds a nest of glowing fireflies-free "star seeds" that drift up one by one into the sky.
static void hill_seedtree(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 54));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double grow = smooth_step(0.0, 0.12, event->progress);
	static const Uint8 seed_c[3] = { 200, 230, 255 };
	filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(s * 0.03), (int)(s * 0.009) + 1, 96, 92, 104, alpha);
	double h = s * 0.055 * grow;
	thickLineRGBA(surface, (int)cx, (int)ground, (int)(cx + s * 0.006), (int)(ground - h * 0.6), 4, 120, 92, 76, alpha);
	thickLineRGBA(surface, (int)(cx + s * 0.006), (int)(ground - h * 0.6), (int)(cx - s * 0.012), (int)(ground - h), 3, 120, 92, 76, alpha);
	for(int c = 0; c < 5; c++) {
		double a = c * 1.25;
		filledCircleRGBA(surface, (int)(cx - s * 0.012 + cos(a) * s * 0.02 * grow), (int)(ground - h + sin(a) * s * 0.012 * grow), (int)(s * 0.014 * grow) + 1, 54, 116, 70, alpha);
	}
	for(int k = 0; k < 14; k++) {
		double u = fmod(event->t * 0.06 + hill_random(event, k, 55), 1.0);
		double born = smooth_step(0.0, 0.1, u), fade = 1.0 - smooth_step(0.8, 1.0, u);
		double sx = cx - s * 0.012 + sin(event->t * 0.7 + k) * s * 0.02 * u + (hill_random(event, k, 56) - 0.5) * s * 0.03;
		double sy = ground - h - u * s * 0.22;
		weapon_glow(surface, sx, sy, 2.4, seed_c, 230.0 * born * fade * smooth_step(0.1, 0.2, event->progress) * event->fade);
		if(u > 0.3) weapon_glow(surface, sx + sin(u * 20.0) * 1.5, sy + 6, 0.8, seed_c, 90.0 * born * fade * event->fade);
	}
}

// A stone circle on the hilltop: each standing stone takes a turn glowing with a vertical beam, then all pulse together.
static void hill_stonecircle(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 57));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	const int n = 8;
	double rx = s * 0.12, ry = s * 0.018;
	static const Uint8 beam[3] = { 150, 235, 255 };
	double all = smooth_step(0.6, 0.7, event->progress) * (0.6 + 0.4 * sin(event->t * 3.0));
	for(int i = 0; i < n; i++) {
		double a = i * 6.2832 / n + 0.3;
		double sx = cx + cos(a) * rx, sy = ground - 1 + sin(a) * ry;
		double up = smooth_step(0.0 + i * 0.02, 0.08 + i * 0.02, event->progress);
		double h = s * (0.035 + 0.012 * (i % 3)) * up, w = s * 0.007;
		boxRGBA(surface, (int)(sx - w), (int)(sy - h), (int)(sx + w), (int)sy, 96, 94, 104, alpha);
		boxRGBA(surface, (int)(sx - w), (int)(sy - h), (int)(sx - w * 0.2), (int)sy, 122, 120, 130, alpha);
		double turn = fmod(event->t * 0.5, (double)n);
		double lit = (turn >= i && turn < i + 1) ? 1.0 - fabs(turn - i - 0.5) * 2.0 : 0.0;
		lit = fmax(lit * smooth_step(0.15, 0.25, event->progress), all);
		if(lit > 0.02) {
			for(int k = 0; k < 14; k++) {
				double f = k / 14.0;
				double bw = 3.0 * (1.0 - f) + 1.0;
				boxRGBA(surface, (int)(sx - bw), (int)(sy - h - (f + 0.07) * s * 0.2), (int)(sx + bw), (int)(sy - h - f * s * 0.2), beam[0], beam[1], beam[2], weapon_alpha(150.0 * lit * (1.0 - f) * event->fade));
			}
			weapon_glow(surface, sx, sy - h, 3.0, beam, 235.0 * lit * event->fade);
		}
	}
}

// A sandcastle by a tide pool: turrets rise grain by grain, a tiny flag waves, and a wave washes in and trims one tower.
static void hill_sandcastle(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 58));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double build = smooth_step(0.0, 0.3, event->progress);
	double wave = fmod(event->t * 0.12, 1.0);
	double wash = wave > 0.7 ? sin((wave - 0.7) / 0.3 * 3.14159) : 0.0;
	for(int i = 0; i < 3; i++) {
		double tx = cx + (i - 1) * s * 0.03;
		double h = s * (0.022 + (i == 1 ? 0.018 : 0.0)) * build;
		double w = s * 0.011;
		boxRGBA(surface, (int)(tx - w), (int)(ground - h), (int)(tx + w), (int)ground, 214, 188, 140, alpha);
		boxRGBA(surface, (int)(tx - w), (int)(ground - h), (int)(tx - w * 0.3), (int)ground, 232, 206, 160, alpha);
		for(int c = -1; c <= 1; c += 2) boxRGBA(surface, (int)(tx + c * w * 0.7 - 1), (int)(ground - h - s * 0.004), (int)(tx + c * w * 0.7 + 1), (int)(ground - h), 214, 188, 140, alpha);
		boxRGBA(surface, (int)(tx - 1), (int)(ground - h * 0.6), (int)(tx + 1), (int)(ground - h * 0.35), 70, 58, 48, alpha);
	}
	double mx = cx, my = ground - s * 0.04 * build;
	lineRGBA(surface, (int)mx, (int)my, (int)mx, (int)(my - s * 0.015), 160, 150, 140, alpha);
	double flap = sin(event->t * 6.0) * s * 0.002;
	filledTrigonRGBA(surface, (int)mx, (int)(my - s * 0.015), (int)mx, (int)(my - s * 0.009), (int)(mx + event->direction * s * 0.01 + flap), (int)(my - s * 0.012), 230, 80, 80, alpha);
	filledEllipseRGBA(surface, (int)(cx + event->direction * s * 0.07), (int)(ground - 1), (int)(s * 0.035), (int)(s * 0.006) + 1, 40, 80, 120, weapon_alpha(210.0 * event->fade));
	if(wash > 0.0) {
		filledEllipseRGBA(surface, (int)(cx + event->direction * s * 0.03 * (1.0 - wash)), (int)(ground - 1), (int)(s * (0.05 + 0.06 * wash)), (int)(s * 0.008 * wash) + 1, 150, 200, 235, weapon_alpha(170.0 * wash * event->fade));
		for(int f = 0; f < 8; f++) filledCircleRGBA(surface, (int)(cx + (f - 3.5) * s * 0.012), (int)(ground - 2 - wash * 2.0), 1, 240, 248, 255, weapon_alpha(220.0 * wash * event->fade));
	}
}

// A music box on a stump opens: a tiny dancer turns on its lid and notes float up from it in glowing wisps.
static void hill_musicbox(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 59));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	double up = smooth_step(0.0, 0.1, event->progress);
	static const Uint8 note[3] = { 255, 225, 150 };
	double sh = s * 0.03 * up;
	filledEllipseRGBA(surface, (int)cx, (int)(ground - sh), (int)(s * 0.032), (int)(s * 0.009) + 1, 150, 118, 100, alpha);
	boxRGBA(surface, (int)(cx - s * 0.028), (int)(ground - sh), (int)(cx + s * 0.028), (int)ground, 128, 98, 82, alpha);
	double bh = s * 0.024 * up;
	boxRGBA(surface, (int)(cx - s * 0.026), (int)(ground - sh - bh), (int)(cx + s * 0.026), (int)(ground - sh), 190, 70, 84, alpha);
	boxRGBA(surface, (int)(cx - s * 0.026), (int)(ground - sh - bh - 3), (int)(cx + s * 0.026), (int)(ground - sh - bh), 225, 190, 100, alpha);
	double open = smooth_step(0.1, 0.2, event->progress);
	if(open > 0.0) {
		double dx = cx, dy = ground - sh - bh - s * 0.012 * open;
		double spin = event->t * 3.0;
		thickLineRGBA(surface, (int)dx, (int)(dy + s * 0.012), (int)(dx + cos(spin) * s * 0.004), (int)dy, 2, 245, 235, 245, alpha);
		filledCircleRGBA(surface, (int)(dx + cos(spin) * s * 0.003), (int)(dy - s * 0.004), (int)(s * 0.003) + 1, 245, 225, 210, alpha);
		double skirt = fabs(cos(spin)) * s * 0.008 + s * 0.002;
		filledTrigonRGBA(surface, (int)(dx - skirt), (int)(dy + s * 0.008), (int)(dx + skirt), (int)(dy + s * 0.008), (int)dx, (int)dy, 240, 160, 190, alpha);
		for(int n = 0; n < 8; n++) {
			double u = fmod(event->t * 0.15 + n / 8.0, 1.0);
			double nx = dx + sin(event->t * 1.2 + n * 1.4) * s * 0.02 * (0.3 + u), ny = dy - u * s * 0.16;
			weapon_glow(surface, nx, ny, 2.8, note, 235.0 * (1.0 - u) * open * event->fade);
			thickLineRGBA(surface, (int)(nx + 2), (int)ny, (int)(nx + 2), (int)(ny - 6), 1, note[0], note[1], note[2], weapon_alpha(200.0 * (1.0 - u) * open * event->fade));
		}
	}
}

// A tall spiral staircase of light winds up from a hilltop dais into the sky, steps lighting one after another.
static void hill_lightstairs(const HillEvent *event) {
	SDL_Surface *surface = event->surface;
	double s = event->scale;
	double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 60));
	double ground = hill_ground(event, cx);
	Uint8 alpha = weapon_alpha(240.0 * event->fade);
	static const Uint8 step_c[3] = { 190, 225, 255 };
	filledEllipseRGBA(surface, (int)cx, (int)(ground - 1), (int)(s * 0.07), (int)(s * 0.012) + 1, 140, 138, 152, alpha);
	const int steps = 40;
	double reach = smooth_step(0.05, 0.6, event->progress) * steps;
	for(int i = 0; i < steps; i++) {
		if(i > reach) break;
		double f = (double)i / steps;
		double a = i * 0.42 + event->t * 0.3;
		double sx = cx + cos(a) * s * 0.06, sy = ground - 4 - f * s * 0.34 + sin(a) * s * 0.014;
		bool front = sin(a) > 0;
		double pulse = 0.55 + 0.45 * sin(event->t * 3.0 - i * 0.5);
		double fade = 1.0 - f * 0.6;
		weapon_glow(surface, sx, sy, front ? 3.4 : 2.0, step_c, 235.0 * pulse * fade * (front ? 1.0 : 0.55) * event->fade);
		if(i % 4 == 0) lineRGBA(surface, (int)cx, (int)sy, (int)sx, (int)sy, 170, 205, 240, weapon_alpha(110.0 * fade * event->fade));
	}
	thickLineRGBA(surface, (int)cx, (int)(ground - 2), (int)cx, (int)(ground - 4 - s * 0.34 * smooth_step(0.05, 0.6, event->progress)), 2, 150, 190, 235, weapon_alpha(120.0 * event->fade));
}

// NEW_HILL_EVENTS_ABOVE (new hill_xxx() functions go above this line)

static void (*const hill_event_kinds[])(const HillEvent *) = {
	hill_village, hill_caravan, hill_turbines, hill_lanterns, hill_campfire,
	hill_bloom, hill_herd, hill_volcano, hill_beacons,
	hill_fireflies, hill_kites, hill_fireworks, hill_train, hill_rocket,
	hill_abduction, hill_geysers, hill_stones,
	hill_ferris, hill_observatory, hill_windmills, hill_lighttree, hill_meteorite,
	hill_dustdevils, hill_lasers, hill_tesla, hill_balloons, hill_giant,
	hill_clocktower, hill_drones, hill_bats, hill_gate,
	hill_lighthouse,
	hill_cherry,
	hill_wolf,
	hill_pumpkins,
	hill_castle,
	hill_spaceelevator,
	hill_airstrip,
	hill_hotsprings,
	hill_market,
	hill_lotus,
	hill_bonfire,
	hill_waterfall,
	hill_scarecrows,
	hill_pagoda,
	hill_skaters,
	hill_shepherd,
	hill_bridge,
	hill_totems,
	hill_snowmen,
	hill_sunflowers,
	hill_crystals,
	hill_mushrooms,
	hill_tents,
	hill_fountain,
	hill_harvester,
	hill_cairns,
	hill_watermill,
	hill_marsh,
	hill_stargazers,
	hill_carousel,
	hill_trebuchet,
	hill_orchard,
	hill_radiomast,
	hill_greenhouse,
	hill_swings,
	hill_beehives,
	hill_cableway,
	hill_ruins,
	hill_geodome,
	hill_cabin,
	hill_windchimes,
	hill_picnic,
	hill_wishtree,
	hill_mine,
	hill_eggs,
	hill_hammock,
	hill_telegraph,
	hill_hamlet,
	hill_stepstones,
	hill_seesaw,
	hill_laundry,
	hill_well,
	hill_kiln,
	hill_ropebridge,
	hill_belltower,
	hill_maze,
	hill_fisher,
	hill_lanterntree,
	hill_forge,
	hill_seedtree,
	hill_stonecircle,
	hill_sandcastle,
	hill_musicbox,
	hill_lightstairs,
	// NEW_HILL_KINDS_ABOVE
};
static_assert(sizeof(hill_event_kinds) / sizeof(hill_event_kinds[0]) == HILL_EVENT_KINDS,
	"HILL_EVENT_KINDS does not match hill_event_kinds[]");

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

// Three hot air balloons drift by, bobbing, their burners flaring now and then.
static void sky_event_43(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 colors[3][3] = { { 214, 92, 76 }, { 231, 184, 86 }, { 92, 147, 201 } };
	for(int balloon = 0; balloon < 3; balloon++) {
		const Uint8 *c = colors[balloon];
		int radius = (int)(size * (1.3 - 0.2 * balloon));
		int bx = center_x - direction * balloon * size * 5;
		int by = center_y + (int)(sin(progress * 9.4 + balloon * 1.7) * size) + balloon * size * 2;
		filledCircleRGBA(surface, bx, by, radius, c[0], c[1], c[2], alpha);
		filledTrigonRGBA(surface, bx - radius * 6 / 10, by + radius * 7 / 10, bx + radius * 6 / 10,
			by + radius * 7 / 10, bx, by + radius * 16 / 10, (Uint8)(c[0] * 0.8), (Uint8)(c[1] * 0.8),
			(Uint8)(c[2] * 0.8), alpha);
		ellipseRGBA(surface, bx, by, radius / 2, radius, 245, 235, 215, (Uint8)(alpha * 0.6));
		lineRGBA(surface, bx - radius / 2, by + radius * 11 / 10, bx - radius / 5, by + radius * 2, 90, 72, 60, alpha);
		lineRGBA(surface, bx + radius / 2, by + radius * 11 / 10, bx + radius / 5, by + radius * 2, 90, 72, 60, alpha);
		boxRGBA(surface, bx - radius / 5, by + radius * 2, bx + radius / 5, by + radius * 2 + radius / 4,
			105, 80, 60, alpha);
		if((phase / 140 + balloon) % 7 < 2) {
			filledCircleRGBA(surface, bx, by + radius * 3 / 2, 2, 255, 190, 90, alpha);
		}
	}
}

// A V of geese flaps past.
static void sky_event_44(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int goose = -3; goose <= 3; goose++) {
		int row = abs(goose);
		int gx = center_x - direction * row * size * 2;
		int gy = center_y + goose * size;
		double flap = sin(phase / 150.0 + row * 0.7 + (goose < 0 ? 1.0 : 0.0));
		int wing = size * 4 / 5;
		int tip_y = gy - (int)(flap * wing * 0.7);
		thickLineRGBA(surface, gx, gy, gx - wing, tip_y, 2, 60, 70, 85, alpha);
		thickLineRGBA(surface, gx, gy, gx + wing, tip_y, 2, 60, 70, 85, alpha);
		filledCircleRGBA(surface, gx + direction * size / 4, gy, 2, 60, 70, 85, alpha);
	}
}

// A small meteor shower: streaks cross the sky one after another.
static void sky_event_45(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int meteor = 0; meteor < 7; meteor++) {
		double begin = meteor * 0.11;
		double u = (progress - begin) / 0.25;
		if(u < 0.0 || u > 1.0) continue;
		unsigned int h = fx_hash(seed + meteor * 977u, 7);
		double sx = surface->w * (0.1 + 0.8 * (h & 255) / 255.0);
		double sy = surface->h * (0.03 + 0.2 * ((h >> 8) & 255) / 255.0);
		double reach = scale * 0.5;
		double hx = sx + direction * reach * u, hy = sy + reach * 0.45 * u;
		double tail = size * 5.0;
		Uint8 a = weapon_alpha(alpha * 1.4 * sin(3.14159265 * u));
		thickLineRGBA(surface, (int)hx, (int)hy, (int)(hx - direction * tail), (int)(hy - tail * 0.45), 2,
			205, 220, 240, (Uint8)(a * 0.5));
		thickLineRGBA(surface, (int)hx, (int)hy, (int)(hx - direction * tail * 0.5), (int)(hy - tail * 0.22), 1,
			255, 250, 235, a);
		filledCircleRGBA(surface, (int)hx, (int)hy, 2, 255, 250, 235, a);
	}
}

// Three lights in a triangle hang in the sky, turn slowly and dart sideways.
static void sky_event_46(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 white[3] = { 255, 240, 200 };
	double base_x = surface->w * (0.2 + 0.4 * ((seed >> 5) % 100) / 100.0);
	double dart = smooth_step(0.45, 0.5, progress) * surface->w * 0.12
		+ smooth_step(0.75, 0.78, progress) * surface->w * 0.3;
	double cx = base_x + direction * dart;
	double cy = center_y + sin(progress * 8.0) * size * 0.3;
	double radius = size * 1.4;
	int px[3], py[3];
	for(int k = 0; k < 3; k++) {
		double angle = progress * 2.0 + k * 2.0944;
		px[k] = (int)(cx + cos(angle) * radius);
		py[k] = (int)(cy + sin(angle) * radius * 0.6);
	}
	filledTrigonRGBA(surface, px[0], py[0], px[1], py[1], px[2], py[2], 20, 24, 34, (Uint8)(alpha * 0.4));
	for(int k = 0; k < 3; k++) {
		weapon_glow(surface, px[k], py[k], 2.5, white, alpha * 1.4);
	}
	if((phase / 400) % 2 == 0) {
		filledCircleRGBA(surface, (int)cx, (int)cy, 2, 255, 60, 50, alpha);
	}
}

// A small plane writes a loop in the sky with a white trail.
static void sky_event_47(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double radius = size * 4.0;
	double span = surface->w * 1.2;
	double start_x = direction > 0 ? -surface->w * 0.1 : surface->w * 1.1;
	double base_y = surface->h * 0.3 + (seed >> 12) % 100 * surface->h * 0.001;
	const int samples = 70;
	double last_x = 0.0, last_y = 0.0;
	bool have_last = false;
	for(int k = 0; k <= samples; k++) {
		double s = progress - 0.25 * (samples - k) / samples;
		if(s < 0.0) continue;
		double a = 6.2831853 * smooth_step(0.4, 0.6, s);
		double px = start_x + direction * (s * span - radius * sin(a));
		double py = base_y - radius * (1.0 - cos(a));
		if(have_last) {
			double recent = (double)k / samples;
			thickLineRGBA(surface, (int)last_x, (int)last_y, (int)px, (int)py, 3, 240, 244, 250,
				(Uint8)(alpha * 0.55 * recent));
		}
		last_x = px;
		last_y = py;
		have_last = true;
		if(k == samples) {
			double ahead = progress - 0.004;
			double a2 = 6.2831853 * smooth_step(0.4, 0.6, ahead);
			double qx = start_x + direction * (ahead * span - radius * sin(a2));
			double qy = base_y - radius * (1.0 - cos(a2));
			double dx = px - qx, dy = py - qy;
			double len = sqrt(dx * dx + dy * dy);
			if(len < 0.001) { dx = direction; dy = 0.0; len = 1.0; }
			dx /= len;
			dy /= len;
			thickLineRGBA(surface, (int)(px - dx * size), (int)(py - dy * size), (int)(px + dx * size),
				(int)(py + dy * size), 3, 230, 232, 238, alpha);
			thickLineRGBA(surface, (int)(px - dy * size * 0.9), (int)(py + dx * size * 0.9),
				(int)(px + dy * size * 0.9), (int)(py - dx * size * 0.9), 2, 200, 205, 215, alpha);
		}
	}
}

// A huge whale made of faint dusk colours glides through the sky, its belly dotted with lights.
static void sky_event_48(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const double profile[7] = { 1.0, 1.5, 1.6, 1.4, 1.0, 0.7, 0.45 };
	int whale_x[7], whale_y[7];
	for(int i = 6; i >= 0; i--) {
		whale_x[i] = center_x - direction * i * size * 8 / 5;
		whale_y[i] = center_y + (int)(sin(phase / 700.0 - i * 0.7) * size * 0.5);
		filledCircleRGBA(surface, whale_x[i], whale_y[i], (int)(size * profile[i]), 70, 85, 120,
			(Uint8)(alpha * 0.8));
	}
	for(int i = 1; i < 6; i++) {
		filledCircleRGBA(surface, whale_x[i], whale_y[i] + (int)(size * profile[i] * 0.55), 2,
			150, 220, 230, (Uint8)(alpha * (0.6 + 0.4 * sin(phase / 300.0 + i))));
	}
	int tail_x = whale_x[6] - direction * size / 2, tail_y = whale_y[6];
	double wave = sin(phase / 700.0 - 6 * 0.7 - 0.5);
	filledTrigonRGBA(surface, tail_x, tail_y, tail_x - direction * size, tail_y - size + (int)(wave * size / 2),
		tail_x - direction * size / 3, tail_y, 70, 85, 120, (Uint8)(alpha * 0.8));
	filledTrigonRGBA(surface, tail_x, tail_y, tail_x - direction * size, tail_y + size + (int)(wave * size / 2),
		tail_x - direction * size / 3, tail_y, 70, 85, 120, (Uint8)(alpha * 0.8));
	filledTrigonRGBA(surface, whale_x[2], whale_y[2] + size, whale_x[2] - direction * size,
		whale_y[2] + size * 2, whale_x[2] + direction * size / 4, whale_y[2] + size * 3 / 2, 55, 68, 100,
		(Uint8)(alpha * 0.8));
	filledCircleRGBA(surface, whale_x[0] + direction * size / 3, whale_y[0] - size / 4, 2, 230, 240, 250, alpha);
}

// A distant storm cell: a dark anvil cloud with rain below and sudden lightning bolts.
static void sky_event_49(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	bool flash = fx_hash(seed, phase / 170) % 7 == 0;
	unsigned int bolt_seed = fx_hash(seed, phase / 170 + 11);
	int r = size * 3 / 2;
	Uint8 cr = flash ? 200 : 66, cg = flash ? 210 : 72, cb = flash ? 250 : 92;
	for(int puff = 0; puff < 5; puff++) {
		filledCircleRGBA(surface, center_x + (puff - 2) * r * 7 / 10, center_y - (puff % 2) * r / 3,
			r * (puff == 2 ? 12 : 9) / 10, cr, cg, cb, (Uint8)(alpha * 0.85));
	}
	boxRGBA(surface, center_x - r * 2, center_y + r / 2, center_x + r * 2, center_y + r * 3 / 5,
		55, 60, 78, (Uint8)(alpha * 0.8));
	for(int drop = 0; drop < 14; drop++) {
		int rx = center_x - r * 3 / 2 + (drop * r * 3 / 13);
		int ry = center_y + r * 3 / 5 + ((phase / 40 + drop * 5) % 10) * size / 5;
		lineRGBA(surface, rx, ry, rx - 2, ry + size / 3, 140, 155, 185, (Uint8)(alpha * 0.45));
	}
	if(flash) {
		int bx = center_x + (int)(bolt_seed % 5) * r / 3 - r;
		int by = center_y + r / 2;
		for(int segment = 0; segment < 6; segment++) {
			int nx = bx + (int)((bolt_seed >> (segment * 3)) % 7) * size / 5 - size * 3 / 5;
			int ny = by + size * 4 / 5;
			thickLineRGBA(surface, bx, by, nx, ny, 3, 190, 205, 255, (Uint8)(alpha * 0.4));
			thickLineRGBA(surface, bx, by, nx, ny, 1, 255, 255, 255, alpha);
			bx = nx;
			by = ny;
		}
	}
}

// A total solar eclipse: a dark moon slides over the sun, a corona blooms at totality and
// a bright glint flares on the edge as it ends.
static void sky_event_50(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double cx = surface->w * (0.3 + 0.4 * ((seed >> 4) % 100) / 100.0);
	double radius = size * 1.6;
	double offset = (progress - 0.5) * radius * 5.0 * direction;
	double cover = 1.0 - smooth_step(0.0, radius * 1.2, fabs(offset));
	Uint8 solid = (Uint8)(fade * 235);
	filledCircleRGBA(surface, (int)cx, center_y, (int)radius, 255, 236, 170, solid);
	if(cover > 0.0) {
		for(int ray = 0; ray < 16; ray++) {
			double angle = ray * 0.3927 + progress * 0.5;
			double length = radius * (1.6 + 0.6 * sin(ray * 2.1 + phase / 500.0)) * cover;
			thickLineRGBA(surface, (int)(cx + cos(angle) * radius), (int)(center_y + sin(angle) * radius),
				(int)(cx + cos(angle) * (radius + length)), (int)(center_y + sin(angle) * (radius + length)), 1,
				235, 240, 255, (Uint8)(alpha * 0.8 * cover));
		}
		circleRGBA(surface, (int)cx, center_y, (int)(radius * 1.15), 255, 245, 215, (Uint8)(alpha * cover));
	}
	filledCircleRGBA(surface, (int)(cx + offset), center_y, (int)radius, 12, 14, 24, solid);
	if(fabs(offset) > radius * 0.03 && fabs(offset) < radius * 0.35) {
		static const Uint8 glint[3] = { 255, 245, 220 };
		weapon_glow(surface, cx - (offset > 0 ? 1 : -1) * radius * 0.95, center_y, radius * 0.3, glint, alpha * 1.6);
	}
}

// A long jade dragon undulates across the sky, with a mane of fins and whiskers streaming behind its snout.
static void sky_event_51(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	const int segments = 16;
	double wave = phase / 260.0;
	int px[segments + 1], py[segments + 1];
	for(int i = 0; i <= segments; i++) {
		px[i] = x - direction * i * size * 11 / 10;
		py[i] = y + (int)(sin(wave - i * 0.55) * size * (0.6 + 0.08 * i));
	}
	for(int i = segments; i >= 0; i--) {
		int r = (int)(size * (0.2 + 0.55 * (1.0 - (double)i / segments)));
		if(i == 0) r = size * 3 / 4;
		if(r < 2) r = 2;
		filledCircleRGBA(surface, px[i], py[i], r, 96, 170, 128, (Uint8)(alpha * 0.9));
		filledCircleRGBA(surface, px[i], py[i] + r / 3, r * 2 / 3, 200, 215, 150, (Uint8)(alpha * 0.45));
		if(i % 3 == 1) {
			filledTrigonRGBA(surface, px[i] - 2, py[i] - r, px[i] + 2, py[i] - r,
				px[i] - direction, py[i] - r - size / 2, 220, 160, 90, (Uint8)(alpha * 0.9));
		}
		if(i == 4 || i == 10) {
			thickLineRGBA(surface, px[i], py[i] + r, px[i] + direction * size / 3, py[i] + r + size / 2, 2,
				80, 140, 106, alpha);
		}
	}
	filledTrigonRGBA(surface, x + direction * size / 3, y - size / 3, x + direction * size / 3, y + size / 3,
		x + direction * size * 13 / 10, y + size / 10, 96, 170, 128, alpha);
	thickLineRGBA(surface, x - direction * size / 10, y - size * 7 / 10, x - direction * size * 6 / 5,
		y - size * 3 / 2, 2, 220, 180, 100, alpha);
	for(int whisker = -1; whisker <= 1; whisker += 2) {
		thickLineRGBA(surface, x + direction * size * 12 / 10, y + size / 10,
			x + direction * size * 2 + direction * size / 2, y + size / 3 + whisker * (int)(sin(wave * 2.0) * size * 0.5)
			+ whisker * size / 3, 1, 230, 220, 170, (Uint8)(alpha * 0.8));
	}
	filledCircleRGBA(surface, x + direction * size / 5, y - size / 5, 2, 255, 220, 120, alpha);
	filledTrigonRGBA(surface, px[segments], py[segments] - 2, px[segments], py[segments] + 2,
		px[segments] - direction * size, py[segments], 220, 160, 90, (Uint8)(alpha * 0.9));
}

// A small drift of jellyfish pulses through the sky, their tentacles trailing.
static void sky_event_52(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int j = 0; j < 5; j++) {
		int jx = x - direction * j * size * 2 + (j % 2) * size;
		int jy = y + (int)(sin(phase / 700.0 + j * 1.3) * size * 1.5) + (j % 3) * size;
		double pulse = 0.5 + 0.5 * sin(phase / 380.0 + j);
		int r = (int)(size * (0.9 + 0.15 * pulse));
		Uint8 cr = j % 2 ? 140 : 220, cg = j % 2 ? 210 : 150, cb = j % 2 ? 230 : 200;
		filledPieRGBA(surface, jx, jy, r, 180, 360, cr, cg, cb, (Uint8)(alpha * 0.55));
		ellipseRGBA(surface, jx, jy, r, r / 4 + 1, cr, cg, cb, (Uint8)(alpha * 0.8));
		filledCircleRGBA(surface, jx, jy - r / 3, r / 4 + 1, 255, 255, 255, (Uint8)(alpha * 0.35));
		for(int k = 0; k < 6; k++) {
			int tx = jx + (int)((k - 2.5) * r * 0.35), ty = jy;
			for(int seg = 1; seg <= 4; seg++) {
				int nx = tx + (int)(sin(phase / 300.0 - seg * 0.8 + k) * size * 0.25 * seg) / 2;
				int ny = ty + size / 2;
				lineRGBA(surface, tx, ty, nx, ny, cr, cg, cb, (Uint8)(alpha * (0.55 - 0.1 * seg)));
				tx = nx;
				ty = ny;
			}
		}
	}
}

// A train of satellites crosses the sky in a line, glints running along it.
static void sky_event_53(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 white[3] = { 240, 244, 255 };
	const int count = 14;
	int spacing = size * 6 / 5;
	for(int i = 0; i < count; i++) {
		double glint = 0.5 + 0.5 * sin(phase / 250.0 - i * 0.7);
		int sx = x - direction * i * spacing, sy = y + i * spacing / 8;
		double fade_tail = 1.0 - (double)i / (count * 1.4);
		filledCircleRGBA(surface, sx, sy, glint > 0.85 ? 2 : 1, white[0], white[1], white[2],
			weapon_alpha(alpha * (0.5 + 0.5 * glint) * fade_tail * 1.4));
		if(glint > 0.8) weapon_glow(surface, sx, sy, 2.0, white, alpha * glint * fade_tail);
	}
}

// A star explodes: it flares with spikes, then a coloured shock shell expands and thins out.
static void sky_event_54(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 white[3] = { 235, 240, 255 };
	double cx = surface->w * (0.2 + 0.6 * ((seed >> 5) % 100) / 100.0);
	double flare = smooth_step(0.1, 0.28, progress) * (1.0 - smooth_step(0.35, 0.7, progress));
	double glow = size * 0.2 + size * 2.2 * flare;
	weapon_glow(surface, cx, center_y, glow, white, 235.0 * fade * (0.25 + 0.75 * flare));
	double spike = size * (1.0 + 8.0 * flare);
	for(int k = 0; k < 4; k++) {
		double a = k * SCENE_PI / 4.0, len = k % 2 ? spike * 0.5 : spike;
		thickLineRGBA(surface, (int)(cx - cos(a) * len), (int)(center_y - sin(a) * len),
			(int)(cx + cos(a) * len), (int)(center_y + sin(a) * len), 1, 235, 240, 255,
			weapon_alpha(200.0 * flare * fade));
	}
	double shock = smooth_step(0.28, 1.0, progress);
	if(shock > 0.0) {
		double radius = shock * scale * 0.2;
		for(int k = 0; k < 3; k++) {
			Uint8 a = weapon_alpha(160.0 * (1.0 - shock) * fade * (1.0 - k * 0.25));
			circleRGBA(surface, (int)cx, center_y, (int)(radius * (1.0 - k * 0.12)),
				k == 1 ? 255 : 180, k == 1 ? 150 : 150, k == 1 ? 170 : 255, a);
		}
		filledCircleRGBA(surface, (int)cx, center_y, (int)(radius * 0.95), 180, 150, 255,
			weapon_alpha(10.0 * (1.0 - shock) * fade));
	}
}

// Dandelion seeds drift past on their little parachutes.
static void sky_event_55(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int i = 0; i < 9; i++) {
		int sx = x - direction * i * size * 9 / 5;
		int sy = y + (int)(sin(phase / 900.0 + i * 1.9) * size * 2.0) + (i % 3) * size * 6 / 5;
		double tilt = sin(phase / 700.0 + i) * 0.4;
		int stem_x = sx - (int)(sin(tilt) * size * 0.8), stem_y = sy + (int)(cos(tilt) * size * 0.8);
		lineRGBA(surface, sx, sy, stem_x, stem_y, 235, 235, 225, (Uint8)(alpha * 0.8));
		filledCircleRGBA(surface, stem_x, stem_y, 1, 200, 180, 140, alpha);
		for(int ray = 0; ray < 7; ray++) {
			double a = -SCENE_PI * (0.1 + 0.8 * ray / 6.0) + tilt;
			lineRGBA(surface, sx, sy, sx + (int)(cos(a) * size * 0.6), sy + (int)(sin(a) * size * 0.6),
				235, 235, 225, (Uint8)(alpha * 0.9));
		}
	}
}

// A double rainbow sweeps in across the sky and fades.
static void sky_event_56(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 band[7][3] = {
		{ 220, 70, 70 }, { 235, 150, 60 }, { 235, 215, 80 }, { 90, 190, 100 },
		{ 70, 140, 225 }, { 80, 90, 190 }, { 140, 90, 190 } };
	double cx = surface->w * (0.25 + 0.5 * ((seed >> 5) % 100) / 100.0);
	double base_y = surface->h * 0.58, radius = scale * 0.55;
	double reveal = smooth_step(0.0, 0.35, progress);
	int thick = scale / 120;
	if(thick < 3) thick = 3;
	const int segments = 40;
	for(int pass = 0; pass < 2; pass++) {
		for(int b = 0; b < 7; b++) {
			double r = pass == 0 ? radius - b * thick : radius * 1.25 + b * thick * 0.8;
			const Uint8 *c = band[pass == 0 ? b : 6 - b];
			double px = 0.0, py = 0.0;
			for(int k = 0; k <= segments; k++) {
				double u = reveal * k / segments;
				double a = SCENE_PI + u * SCENE_PI;
				double nx = cx + cos(a) * r, ny = base_y + sin(a) * r * 0.9;
				if(k > 0) {
					thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, thick, c[0], c[1], c[2],
						(Uint8)(alpha * (pass == 0 ? 0.42 : 0.18)));
				}
				px = nx;
				py = ny;
			}
		}
	}
}

// A sun halo: a pale ring with two bright sundogs on either side and a faint light pillar.
static void sky_event_57(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 warm[3] = { 255, 240, 200 };
	double cx = surface->w * (0.3 + 0.4 * ((seed >> 5) % 100) / 100.0);
	double R = scale * 0.2;
	double shimmer = 0.85 + 0.15 * sin(phase / 400.0);
	weapon_glow(surface, cx, center_y, size * 0.8, warm, 235.0 * fade);
	for(int k = 0; k < 3; k++) {
		circleRGBA(surface, (int)cx, center_y, (int)R + k, 255, 250, 225, weapon_alpha(70.0 * fade * shimmer - k * 18));
	}
	circleRGBA(surface, (int)cx, center_y, (int)R - 2, 255, 190, 170, weapon_alpha(40.0 * fade * shimmer));
	thickLineRGBA(surface, (int)cx, (int)(center_y - R * 1.4), (int)cx, (int)(center_y + R * 1.4), 2, 255, 245, 220,
		weapon_alpha(30.0 * fade));
	for(int side = -1; side <= 1; side += 2) {
		double dx = cx + side * R * 1.02;
		weapon_glow(surface, dx, center_y, size * 0.5, warm, 200.0 * fade * shimmer);
		for(int q = 0; q < 6; q++) {
			double f = q / 5.0;
			Uint8 cr = (Uint8)(255 - 85 * f), cg = (Uint8)(150 + 50 * f), cb = (Uint8)(120 + 135 * f);
			thickLineRGBA(surface, (int)(dx + side * q * size * 0.5), center_y,
				(int)(dx + side * (q + 1) * size * 0.5), center_y, 2, cr, cg, cb,
				weapon_alpha(110.0 * fade * (1.0 - f * 0.8)));
		}
	}
}

static double sky_track_x(const SkyEvent *event, double progress) {
	double margin = event->scale / 12.0;
	double span = event->surface->w + 2.0 * margin;
	return event->direction > 0 ? -margin + progress * span : event->surface->w + margin - progress * span;
}

// A small plane flies over and skydivers drop out, free-fall and open their canopies.
static void sky_event_58(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 canopy[4][3] = { { 190, 80, 70 }, { 80, 130, 190 }, { 220, 200, 90 }, { 110, 170, 110 } };
	filledEllipseRGBA(surface, x, y, size * 7 / 5, size * 7 / 20 + 1, 30, 34, 44, alpha);
	thickLineRGBA(surface, x - direction * size / 5, y, x + direction * size / 5, y - size / 2, 2, 30, 34, 44, alpha);
	filledTrigonRGBA(surface, x - direction * size, y, x - direction * size * 3 / 2, y - size / 2,
		x - direction * size * 6 / 5, y, 30, 34, 44, alpha);
	thickLineRGBA(surface, x + direction * size * 3 / 2, y - size / 3, x + direction * size * 3 / 2, y + size / 3, 1,
		190, 200, 210, (Uint8)(alpha * 0.6));
	for(int j = 0; j < 5; j++) {
		double drop = 0.2 + 0.07 * j;
		double dp = progress - drop;
		if(dp < 0.0) continue;
		double jx = sky_track_x(event, drop) + direction * scale * 0.05 * (1.0 - exp(-dp * 12.0));
		double fall = dp < 0.1 ? dp * dp * scale * 10.0 : 0.1 * 0.1 * scale * 10.0 + (dp - 0.1) * scale * 0.35;
		double jy = y + size / 3 + fall;
		if(dp < 0.1) {
			thickLineRGBA(surface, (int)jx, (int)jy, (int)(jx + direction * size / 2), (int)(jy + size / 5), 2,
				30, 34, 44, alpha);
			filledCircleRGBA(surface, (int)jx, (int)jy, 2, 30, 34, 44, alpha);
		} else {
			jx += sin(dp * 10.0 + j) * size * 0.3;
			const Uint8 *c = canopy[j % 4];
			int cr = size * 4 / 5 + 2;
			double open = smooth_step(0.1, 0.13, dp);
			int radius = (int)(cr * open) + 1;
			filledPieRGBA(surface, (int)jx, (int)jy - cr * 2, radius, 180, 360, c[0], c[1], c[2], alpha);
			lineRGBA(surface, (int)jx - radius, (int)jy - cr * 2, (int)jx, (int)jy, 30, 34, 44, (Uint8)(alpha * 0.7));
			lineRGBA(surface, (int)jx + radius, (int)jy - cr * 2, (int)jx, (int)jy, 30, 34, 44, (Uint8)(alpha * 0.7));
			filledCircleRGBA(surface, (int)jx, (int)jy, 2, 30, 34, 44, alpha);
		}
	}
}

// A flying carpet glides by with a turbaned rider and a trail of golden sparkles.
static void sky_event_59(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 gold[3] = { 255, 230, 150 };
	const int seg = 10;
	double half = size * 2.2, wave = phase / 220.0;
	int prev_x = 0, prev_y = 0, rider_x = x, rider_y = y;
	for(int i = 0; i <= seg; i++) {
		double u = (double)i / seg;
		int px = x + (int)(direction * (u - 0.5) * 2.0 * half);
		int py = y + (int)(sin(wave - u * 5.0) * size * 0.35);
		int thickness = size * 7 / 20 + 2;
		if(i > 0) {
			Uint8 cr = i % 2 ? 140 : 110, cg = i % 2 ? 50 : 40, cb = i % 2 ? 60 : 70;
			filledTrigonRGBA(surface, prev_x, prev_y, px, py, prev_x, prev_y + thickness, cr, cg, cb, alpha);
			filledTrigonRGBA(surface, px, py, px, py + thickness, prev_x, prev_y + thickness, cr, cg, cb, alpha);
			lineRGBA(surface, prev_x, prev_y, px, py, 210, 170, 90, alpha);
			lineRGBA(surface, prev_x, prev_y + thickness, px, py + thickness, 210, 170, 90, alpha);
		}
		if(i == 4) { rider_x = px; rider_y = py; }
		prev_x = px;
		prev_y = py;
	}
	for(int fringe = 0; fringe < 4; fringe++) {
		int fy = prev_y + fringe * size / 8;
		lineRGBA(surface, prev_x, fy, prev_x + direction * size / 4, fy + size / 8, 210, 170, 90, alpha);
	}
	filledCircleRGBA(surface, rider_x, rider_y - size * 7 / 10, size / 5 + 1, 30, 24, 30, alpha);
	filledCircleRGBA(surface, rider_x, rider_y - size * 4 / 5, size / 4, 220, 215, 200, alpha);
	filledTrigonRGBA(surface, rider_x - size / 4, rider_y, rider_x + size / 4, rider_y,
		rider_x, rider_y - size * 6 / 10, 40, 36, 50, alpha);
	for(int k = 0; k < 12; k++) {
		double age = k / 12.0;
		double tx = x - direction * (half + age * size * 7.0), ty = y + sin(wave * 0.8 + k) * size * 0.5 + age * size;
		weapon_glow(surface, tx, ty, 1.6, gold, alpha * (1.0 - age) * 1.4);
	}
}

// A glowing double helix of DNA turns as it flies across the sky.
static void sky_event_60(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	const int pairs = 22;
	double spin = phase / 380.0;
	int prev_a = 0, prev_b = 0, prev_x = 0;
	for(int i = 0; i < pairs; i++) {
		int ax = x - direction * i * size * 11 / 20;
		double s_ = sin(spin + i * 0.5), c_ = cos(spin + i * 0.5);
		int ya = y + (int)(s_ * size * 1.1), yb = y - (int)(s_ * size * 1.1);
		double tail = 1.0 - 0.6 * i / pairs;
		lineRGBA(surface, ax, ya, ax, yb, 200, 210, 240, (Uint8)(alpha * 0.35 * tail));
		if(i > 0) {
			lineRGBA(surface, prev_x, prev_a, ax, ya, 255, 140, 170, (Uint8)(alpha * 0.7 * tail));
			lineRGBA(surface, prev_x, prev_b, ax, yb, 130, 220, 255, (Uint8)(alpha * 0.7 * tail));
		}
		filledCircleRGBA(surface, ax, ya, c_ > 0 ? 3 : 2, 255, 140, 170, (Uint8)(alpha * tail));
		filledCircleRGBA(surface, ax, yb, c_ < 0 ? 3 : 2, 130, 220, 255, (Uint8)(alpha * tail));
		prev_x = ax;
		prev_a = ya;
		prev_b = yb;
	}
}

// A single huge snowflake spins slowly as it sails down across the sky.
static void sky_event_61(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 ice[3] = { 215, 238, 255 };
	double R = size * 2.6;
	double fy = y + progress * scale * 0.15;
	double rot = phase / 2600.0 * direction;
	for(int arm = 0; arm < 6; arm++) {
		double a = rot + arm * SCENE_PI / 3.0;
		double ux = cos(a), uy = sin(a);
		thickLineRGBA(surface, x, (int)fy, (int)(x + ux * R), (int)(fy + uy * R), 2, ice[0], ice[1], ice[2], alpha);
		for(int b = 0; b < 2; b++) {
			double at = b == 0 ? 0.45 : 0.75, len = R * (b == 0 ? 0.3 : 0.2);
			double bx = x + ux * R * at, by = fy + uy * R * at;
			for(int side = -1; side <= 1; side += 2) {
				double ba = a + side * 0.8;
				lineRGBA(surface, (int)bx, (int)by, (int)(bx + cos(ba) * len), (int)(by + sin(ba) * len),
					ice[0], ice[1], ice[2], alpha);
			}
		}
	}
	weapon_glow(surface, x, fy, size * 0.35, ice, alpha * 1.3);
	int sparkle = (int)(phase / 300) % 6;
	double sa = rot + sparkle * SCENE_PI / 3.0;
	weapon_glow(surface, x + cos(sa) * R, fy + sin(sa) * R, 2.0, ice, alpha * (0.5 + 0.5 * sin(phase / 90.0)));
}

// A pulsar: a small star whose twin beams sweep round like a lighthouse, with pulse rings.
static void sky_event_62(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 blue[3] = { 150, 200, 255 };
	double cx = surface->w * (0.2 + 0.6 * ((seed >> 5) % 100) / 100.0);
	double length = scale * 0.35, spin = phase / 420.0;
	for(int beam = 0; beam < 2; beam++) {
		double a = spin + beam * SCENE_PI;
		double ex = cx + cos(a) * length, ey = center_y + sin(a) * length;
		double wx = -sin(a) * size * 0.35, wy = cos(a) * size * 0.35;
		filledTrigonRGBA(surface, (int)cx, center_y, (int)(ex + wx), (int)(ey + wy), (int)(ex - wx), (int)(ey - wy),
			blue[0], blue[1], blue[2], (Uint8)(alpha * 0.18));
		lineRGBA(surface, (int)cx, center_y, (int)ex, (int)ey, 220, 235, 255, (Uint8)(alpha * 0.5));
	}
	double pulse = 0.5 + 0.5 * sin(spin * 3.0);
	weapon_glow(surface, cx, center_y, size * 0.45 * (1.0 + 0.2 * pulse), blue, 235.0 * fade);
	double ring = (phase % 1500) / 1500.0;
	circleRGBA(surface, (int)cx, center_y, (int)(ring * size * 3.0), blue[0], blue[1], blue[2],
		weapon_alpha(alpha * (1.0 - ring)));
}

// A swirling portal opens in the sky, throws sparks and shoots out a bright streak before it closes.
static void sky_event_63(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 swirl[4][3] = { { 150, 90, 255 }, { 120, 140, 255 }, { 100, 200, 255 }, { 190, 230, 255 } };
	double cx = surface->w * (0.2 + 0.6 * ((seed >> 5) % 100) / 100.0);
	double open = smooth_step(0.0, 0.2, progress) * (1.0 - smooth_step(0.8, 1.0, progress));
	double R = size * 2.4 * open;
	if(R < 2.0) return;
	double spin = phase / 500.0;
	filledEllipseRGBA(surface, (int)cx, center_y, (int)(R * 0.8), (int)R, 8, 6, 22, weapon_alpha(alpha * 1.3));
	for(int k = 0; k < 4; k++) {
		double r = R * (1.0 - 0.16 * k);
		double dir = k % 2 ? 1.0 : -1.0;
		for(int seg = 0; seg < 16; seg++) {
			double a0 = dir * spin * (1.0 + 0.3 * k) + seg * SCENE_TAU / 16.0, a1 = a0 + SCENE_TAU / 16.0 * 0.6;
			thickLineRGBA(surface, (int)(cx + cos(a0) * r * 0.8), (int)(center_y + sin(a0) * r),
				(int)(cx + cos(a1) * r * 0.8), (int)(center_y + sin(a1) * r), 2,
				swirl[k][0], swirl[k][1], swirl[k][2], weapon_alpha(alpha * 1.3));
		}
	}
	ellipseRGBA(surface, (int)cx, center_y, (int)(R * 0.85), (int)(R * 1.05), 170, 120, 255, weapon_alpha(alpha * 0.8));
	for(int j = 0; j < 10; j++) {
		double u = fmod(phase / 1800.0 + j / 10.0, 1.0), a = j * 2.4;
		weapon_glow(surface, cx + cos(a) * R * 0.8 * (0.5 + u * 1.1), center_y + sin(a) * R * (0.5 + u * 1.1), 1.6,
			swirl[3], alpha * (1.0 - u) * open);
	}
	double shot = (progress - 0.45) / 0.2;
	if(shot > 0.0 && shot < 1.0) {
		double sx = cx + direction * shot * scale * 0.6, sy = center_y - shot * scale * 0.05;
		thickLineRGBA(surface, (int)(sx - direction * size * 3), (int)(sy + size * 0.3), (int)sx, (int)sy, 2,
			200, 230, 255, weapon_alpha(alpha * (1.0 - shot)));
		weapon_glow(surface, sx, sy, 2.5, swirl[3], 235.0 * fade * (1.0 - shot));
	}
}

// A ghostly sailing ship, half transparent, crosses the sky with its mist wake and a green lantern.
static void sky_event_64(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 lantern[3] = { 150, 255, 200 };
	double flicker = 0.8 + 0.2 * sin(phase / 170.0);
	Uint8 a = weapon_alpha(alpha * 0.8 * flicker);
	int by = y + (int)(sin(phase / 900.0) * size * 0.25);
	// Hull and masts are placed in units of `size` along the heading.
	#define GHOST_X(u) (x + direction * (int)((u) * size))
	#define GHOST_Y(v) (by + (int)((v) * size))
	filledTrigonRGBA(surface, GHOST_X(-3.4), GHOST_Y(-0.6), GHOST_X(3.4), GHOST_Y(-0.3), GHOST_X(2.4), GHOST_Y(1.0),
		110, 140, 160, a);
	filledTrigonRGBA(surface, GHOST_X(-3.4), GHOST_Y(-0.6), GHOST_X(2.4), GHOST_Y(1.0), GHOST_X(-2.4), GHOST_Y(1.0),
		110, 140, 160, a);
	lineRGBA(surface, GHOST_X(-3.4), GHOST_Y(-0.6), GHOST_X(3.4), GHOST_Y(-0.3), 190, 215, 225, a);
	thickLineRGBA(surface, GHOST_X(3.4), GHOST_Y(-0.3), GHOST_X(4.8), GHOST_Y(-1.0), 1, 190, 215, 225, a);
	for(int p = 0; p < 4; p++) {
		filledCircleRGBA(surface, GHOST_X(-1.6 + p * 1.1), GHOST_Y(0.2), 1, 170, 255, 220, a);
	}
	static const double mast_u[3] = { -1.8, 0.2, 2.0 };
	static const double mast_h[3] = { 3.4, 4.4, 3.4 };
	for(int m = 0; m < 3; m++) {
		thickLineRGBA(surface, GHOST_X(mast_u[m]), GHOST_Y(-0.4), GHOST_X(mast_u[m]), GHOST_Y(-mast_h[m]), 1,
			190, 215, 225, a);
		for(int sail = 0; sail < 2; sail++) {
			double top = -mast_h[m] + 0.3 + sail * mast_h[m] * 0.45, bottom = top + mast_h[m] * 0.4;
			Uint8 sa = weapon_alpha(alpha * 0.4 * flicker);
			filledTrigonRGBA(surface, GHOST_X(mast_u[m]), GHOST_Y(top), GHOST_X(mast_u[m]), GHOST_Y(bottom),
				GHOST_X(mast_u[m] + 1.3), GHOST_Y((top + bottom) / 2.0), 200, 215, 230, sa);
		}
	}
	weapon_glow(surface, GHOST_X(-3.2), GHOST_Y(-0.9), 2.0, lantern, alpha * 1.5 * flicker);
	for(int puff = 0; puff < 6; puff++) {
		double age = fmod(phase / 2200.0 + puff / 6.0, 1.0);
		filledCircleRGBA(surface, GHOST_X(-3.4 - age * 4.0), GHOST_Y(0.6 - age * 0.6), (int)(size * (0.2 + 0.4 * age)),
			200, 215, 225, weapon_alpha(alpha * 0.35 * (1.0 - age)));
	}
	#undef GHOST_X
	#undef GHOST_Y
}

// Paper airplanes glide across the sky in a loose group, banking as they go.
static void sky_event_65(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int plane = 0; plane < 5; plane++) {
		double bank = sin(phase / 700.0 + plane * 1.3);
		int px = x - direction * (plane * size * 3 + (plane % 2) * size);
		int py = y + (int)(sin(phase / 900.0 + plane * 2.1) * size * 1.5) + plane * size;
		int nose_x = px + direction * size, tail_x = px - direction * size;
		int wing = (int)(size * (0.5 + 0.35 * fabs(bank)));
		int fold = (int)(bank * size * 0.25);
		filledTrigonRGBA(surface, nose_x, py, tail_x, py - wing, tail_x, py + fold, 238, 240, 244, alpha);
		filledTrigonRGBA(surface, nose_x, py, tail_x, py + wing, tail_x, py + fold, 188, 196, 212, alpha);
		lineRGBA(surface, nose_x, py, tail_x, py + fold, 110, 120, 140, alpha);
		for(int dot = 1; dot <= 5; dot++) {
			pixelRGBA(surface, tail_x - direction * dot * size / 2, py + fold, 220, 228, 240,
				(Uint8)(alpha * (1.0 - dot / 6.0)));
		}
	}
}

// Soap bubbles drift by: thin iridescent rings, each with a bright glint.
static void sky_event_66(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int bubble = 0; bubble < 8; bubble++) {
		int r = size * (4 + bubble % 3) / 6 + 3;
		int bx = x - direction * bubble * size * 3 / 2 + (int)(sin(phase / 800.0 + bubble) * size);
		int by = y + bubble * size / 2 + (int)(sin(phase / 600.0 + bubble * 1.7) * size);
		circleRGBA(surface, bx, by, r, 255, 140, 220, (Uint8)(alpha * 0.55));
		circleRGBA(surface, bx, by, r - 1, 120, 230, 255, (Uint8)(alpha * 0.45));
		circleRGBA(surface, bx, by, r - 2, 255, 240, 140, (Uint8)(alpha * 0.25));
		filledCircleRGBA(surface, bx, by, r - 2, 200, 220, 255, (Uint8)(alpha * 0.08));
		filledCircleRGBA(surface, bx - r / 3, by - r / 3, r / 6 + 1, 255, 255, 255, alpha);
	}
}

// A big banded ringed planet slides slowly across the sky.
static void sky_event_67(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	int r = size * 2;
	int ring_w = r * 22 / 10, ring_h = r * 6 / 10;
	ellipseRGBA(surface, x, y, ring_w, ring_h, 210, 190, 150, (Uint8)(alpha * 0.55));
	ellipseRGBA(surface, x, y, ring_w - 3, ring_h - 1, 190, 170, 130, (Uint8)(alpha * 0.4));
	ellipseRGBA(surface, x, y, ring_w * 8 / 10, ring_h * 8 / 10, 220, 200, 160, (Uint8)(alpha * 0.35));
	filledCircleRGBA(surface, x, y, r, 150, 118, 92, alpha);
	for(int band = -3; band <= 3; band++) {
		int by = y + band * r / 4;
		double dy = (double)(by - y) / r;
		int half = (int)(r * sqrt(1.0 - dy * dy));
		lineRGBA(surface, x - half, by, x + half, by, band % 2 ? 190 : 120, band % 2 ? 150 : 92, band % 2 ? 110 : 70,
			(Uint8)(alpha * 0.6));
	}
	filledCircleRGBA(surface, x - r / 3, y - r / 3, r / 2, 255, 235, 200, (Uint8)(alpha * 0.12));
	for(int step = 0; step < 40; step++) {
		double a0 = SCENE_PI * step / 40.0, a1 = SCENE_PI * (step + 1) / 40.0;
		lineRGBA(surface, x + (int)(cos(a0) * ring_w), y + (int)(sin(a0) * ring_h),
			x + (int)(cos(a1) * ring_w), y + (int)(sin(a1) * ring_h), 215, 195, 155, (Uint8)(alpha * 0.8));
	}
}

// A constellation draws itself: stars light up and thin lines join them one by one.
static void sky_event_68(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const double pts[7][2] = { { -3, 0 }, { -1.5, -1.5 }, { 0, -0.4 }, { 1.8, -1.8 }, { 3, 0.3 }, { 1.4, 1.6 }, { -0.8, 1.4 } };
	static const Uint8 star[3] = { 235, 240, 255 };
	double cx = surface->w * (0.2 + 0.6 * ((seed >> 5) % 100) / 100.0);
	int u = size * 2;
	double draw = smooth_step(0.1, 0.7, progress) * 6.0;
	for(int i = 0; i < 7; i++) {
		double sx = cx + pts[i][0] * u, sy = y + pts[i][1] * u;
		if(i < 6) {
			double f = fmin(draw - i, 1.0);
			if(f > 0.0) {
				double nx = cx + pts[i + 1][0] * u, ny = y + pts[i + 1][1] * u;
				lineRGBA(surface, (int)sx, (int)sy, (int)(sx + (nx - sx) * f), (int)(sy + (ny - sy) * f),
					170, 195, 235, (Uint8)(alpha * 0.7));
			}
		}
		double lit = smooth_step(i * 0.08, i * 0.08 + 0.1, progress);
		weapon_glow(surface, sx, sy, 1.5 + (i % 3) * 0.5, star, 235.0 * fade * lit * (0.7 + 0.3 * sin(phase / 300.0 + i)));
	}
}

// A flock of folded paper cranes flaps slowly across the sky.
static void sky_event_69(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int crane = 0; crane < 6; crane++) {
		int cx = x - direction * (crane * size * 2 + (crane % 2) * size);
		int cy = y + (int)(sin(phase / 800.0 + crane * 1.6) * size) + (crane % 3) * size;
		double flap = sin(phase / 260.0 + crane * 0.9);
		int wing = (int)(size * (0.8 + 0.4 * flap));
		Uint8 tint = (Uint8)(215 + crane * 6);
		filledTrigonRGBA(surface, cx, cy, cx - direction * size, cy - wing, cx - direction * size / 3, cy, tint, 228, 232, alpha);
		filledTrigonRGBA(surface, cx, cy, cx - direction * size, cy + wing / 2, cx - direction * size / 3, cy, 175, 185, 205, alpha);
		thickLineRGBA(surface, cx, cy, cx + direction * size, cy - size / 3, 1, 235, 120, 120, alpha);
		thickLineRGBA(surface, cx, cy, cx - direction * size * 3 / 2, cy + size / 4, 1, 215, 222, 232, alpha);
	}
}

// A string of glowing sky lanterns rises and drifts, each pulsing softly.
static void sky_event_70(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 warm[3] = { 255, 170, 80 };
	for(int lantern = 0; lantern < 9; lantern++) {
		double rise = progress * size * 3.0 + lantern * size * 0.6;
		double lx = x - direction * lantern * size * 2 + sin(phase / 900.0 + lantern) * size * 0.8;
		double ly = y + (lantern % 3) * size * 1.5 - rise * 0.4;
		double pulse = 0.7 + 0.3 * sin(phase / 250.0 + lantern * 1.7);
		weapon_glow(surface, lx, ly, size * 0.7, warm, 200.0 * fade * pulse);
		boxRGBA(surface, (int)lx - 2, (int)ly - 3, (int)lx + 2, (int)ly + 3, 255, 200, 120, (Uint8)(alpha * pulse));
	}
}

// A big owl glides silently across the moonless sky, wings spread wide.
static void sky_event_71(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	int by = y + (int)(sin(phase / 1100.0) * size * 0.6);
	double beat = sin(phase / 420.0);
	int span = size * 3, lift = (int)(beat * size * 0.8);
	for(int side = -1; side <= 1; side += 2) {
		int tip_x = x + side * span / 2 * 1, tip_y = by - lift;
		filledTrigonRGBA(surface, x, by - size / 4, tip_x, tip_y, x + side * size / 3, by + size / 3, 28, 24, 30, alpha);
		filledTrigonRGBA(surface, x, by - size / 4, tip_x, tip_y, x + side * span / 4, by - lift / 2 - size / 2, 40, 34, 40, alpha);
	}
	filledEllipseRGBA(surface, x, by, size / 2 + 1, size * 7 / 10, 36, 30, 34, alpha);
	filledCircleRGBA(surface, x, by - size * 3 / 4, size / 2, 44, 38, 42, alpha);
	filledTrigonRGBA(surface, x - size / 3, by - size, x - size / 6, by - size * 5 / 4, x - size / 10, by - size * 6 / 5, 44, 38, 42, alpha);
	filledTrigonRGBA(surface, x + size / 3, by - size, x + size / 6, by - size * 5 / 4, x + size / 10, by - size * 6 / 5, 44, 38, 42, alpha);
	filledCircleRGBA(surface, x - size / 5, by - size * 3 / 4, 2, 255, 210, 90, alpha);
	filledCircleRGBA(surface, x + size / 5, by - size * 3 / 4, 2, 255, 210, 90, alpha);
}

// A gust lifts a swirl of leaves that spiral across the sky and settle.
static void sky_event_72(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int leaf = 0; leaf < 16; leaf++) {
		double u = leaf / 16.0;
		double spin = phase / 400.0 + leaf * 0.9;
		double lx = x - direction * (leaf * size * 0.9) + cos(spin) * size * (1.0 + u * 2.0);
		double ly = y + sin(spin) * size * (0.6 + u) + u * size * 3.0;
		double turn = spin * 1.7;
		Uint8 r = (Uint8)(150 + (leaf * 17) % 80), g = (Uint8)(70 + (leaf * 29) % 70);
		filledEllipseRGBA(surface, (int)lx, (int)ly, (int)(2 + fabs(cos(turn)) * size * 0.4), (int)(1 + fabs(sin(turn)) * size * 0.3), r, g, 40, alpha);
	}
}

// A murmuration: a dense cloud of small birds twists and swells as it moves.
static void sky_event_73(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int bird = 0; bird < 70; bird++) {
		unsigned int h = fx_hash(seed, bird);
		double a = (h % 6283) / 1000.0, r = ((h >> 12) % 100) / 100.0;
		double shape = 1.0 + 0.5 * sin(phase / 700.0 + a * 2.0);
		double bx = x + cos(a + phase / 1500.0) * size * 4.0 * r * shape;
		double by = y + sin(a + phase / 1500.0) * size * 1.6 * r * (2.0 - shape) + sin(phase / 300.0 + bird) * 2.0;
		double flap = sin(phase / 90.0 + bird);
		thickLineRGBA(surface, (int)bx - 2, (int)(by + flap * 1.5), (int)bx, (int)by, 1, 14, 18, 28, alpha);
		thickLineRGBA(surface, (int)bx, (int)by, (int)bx + 2, (int)(by + flap * 1.5), 1, 14, 18, 28, alpha);
	}
}

// A single bright aurora curtain unfurls, hangs rippling and folds away.
static void sky_event_74(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double open = smooth_step(0.0, 0.25, progress) * (1.0 - smooth_step(0.75, 1.0, progress));
	const int columns = 36;
	double left = surface->w * 0.1, width = surface->w * 0.8;
	for(int c = 0; c < columns; c++) {
		double u = (double)c / columns;
		double top = surface->h * 0.08 + sin(u * 7.0 + phase / 900.0) * scale * 0.03;
		double length = scale * (0.12 + 0.08 * sin(u * 11.0 - phase / 600.0)) * open;
		double cx = left + width * u;
		Uint8 g = (Uint8)(190 + 60 * sin(u * 5.0)), b = (Uint8)(150 + 90 * u);
		for(int seg = 0; seg < 4; seg++) {
			double f0 = seg / 4.0, f1 = (seg + 1) / 4.0;
			thickLineRGBA(surface, (int)cx, (int)(top + length * f0), (int)cx, (int)(top + length * f1), 3, 90, g, b,
				weapon_alpha(alpha * (0.55 - 0.12 * seg) * 0.5));
		}
	}
}

// A great comet with a bright head and a two-part tail: a straight blue ion tail and a curved golden dust tail.
static void sky_event_75(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 head[3] = { 235, 245, 255 };
	int len = size * 12;
	for(int k = 0; k < 24; k++) {
		double u = k / 24.0;
		double ix = x - direction * u * len, iy = y - u * len * 0.18;
		double dx = x - direction * u * len * 0.8, dy = y + u * u * len * 0.35;
		filledCircleRGBA(surface, (int)ix, (int)iy, (int)(size * 0.25 * (1.0 - u)) + 1, 120, 180, 255, weapon_alpha(alpha * 0.5 * (1.0 - u)));
		filledCircleRGBA(surface, (int)dx, (int)dy, (int)(size * 0.45 * (0.4 + u * 0.6)) + 1, 255, 215, 140, weapon_alpha(alpha * 0.28 * (1.0 - u)));
	}
	weapon_glow(surface, x, y, size * 0.8, head, 235.0 * fade);
}

// A small flock of pterosaurs with long head crests glides past on wide, slow wing beats.
static void sky_event_76(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int p = 0; p < 4; p++) {
		int px = x - direction * p * size * 4, py = y + (p % 2) * size * 2 + (int)(sin(phase / 1000.0 + p) * size * 0.6);
		double beat = sin(phase / 520.0 + p * 0.8);
		int span = size * 2, lift = (int)(beat * size);
		for(int side = -1; side <= 1; side += 2) {
			int elbow_x = px + side * span / 2, elbow_y = py - lift / 2 - size / 3;
			int tip_x = px + side * span, tip_y = py + lift / 2;
			thickLineRGBA(surface, px, py, elbow_x, elbow_y, 2, 36, 30, 40, alpha);
			thickLineRGBA(surface, elbow_x, elbow_y, tip_x, tip_y, 2, 36, 30, 40, alpha);
			filledTrigonRGBA(surface, px, py, elbow_x, elbow_y, tip_x, tip_y, 50, 42, 52, (Uint8)(alpha * 0.8));
		}
		filledEllipseRGBA(surface, px, py, size / 2, size / 5 + 1, 42, 36, 46, alpha);
		thickLineRGBA(surface, px + direction * size / 2, py, px + direction * size, py - size / 6, 2, 42, 36, 46, alpha);
		thickLineRGBA(surface, px + direction * size / 2, py - size / 8, px - direction * size / 4, py - size / 2, 1, 150, 70, 60, alpha);
		thickLineRGBA(surface, px - direction * size / 2, py, px - direction * size, py + size / 5, 1, 36, 30, 40, alpha);
	}
}

// A swarm of luminous butterflies flutters across the sky in a loose, wandering cloud.
static void sky_event_77(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 tints[3][3] = { { 150, 220, 255 }, { 255, 170, 220 }, { 190, 255, 190 } };
	for(int b = 0; b < 14; b++) {
		unsigned int h = fx_hash(seed, b);
		double bx = x - direction * (h % 7) * size * 1.4 + sin(phase / 700.0 + b * 1.3) * size * 1.5;
		double by = y + ((h >> 8) % 7) * size * 0.6 + sin(phase / 530.0 + b * 2.1) * size;
		double flap = fabs(sin(phase / 110.0 + b * 0.7));
		int wing = (int)(1 + flap * size * 0.5);
		const Uint8 *c = tints[b % 3];
		filledEllipseRGBA(surface, (int)bx - wing, (int)by, wing, wing * 2 / 3 + 1, c[0], c[1], c[2], (Uint8)(alpha * 0.8));
		filledEllipseRGBA(surface, (int)bx + wing, (int)by, wing, wing * 2 / 3 + 1, c[0], c[1], c[2], (Uint8)(alpha * 0.8));
		weapon_glow(surface, bx, by, 1.2, c, 120.0 * fade);
	}
}

// A zeppelin cruises by sweeping a searchlight beam below it, its fins and gondola lit.
static void sky_event_78(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	int len = size * 7, h = size * 3 / 2;
	int zy = y + (int)(sin(phase / 1200.0) * size * 0.5);
	double aim = 1.2 + sin(phase / 800.0) * 0.5;
	int lx = x + direction * size, ly = zy + h;
	double reach = scale * 0.5;
	filledTrigonRGBA(surface, lx, ly, (int)(lx + direction * cos(aim - 0.07) * reach), (int)(ly + sin(aim - 0.07) * reach),
		(int)(lx + direction * cos(aim + 0.07) * reach), (int)(ly + sin(aim + 0.07) * reach), 255, 250, 215, (Uint8)(alpha * 0.2));
	filledEllipseRGBA(surface, x, zy, len / 2, h, 120, 126, 138, alpha);
	filledEllipseRGBA(surface, x, zy - h / 4, len / 2 - 2, h * 2 / 3, 160, 166, 178, (Uint8)(alpha * 0.8));
	for(int fin = -1; fin <= 1; fin += 2) {
		filledTrigonRGBA(surface, x - direction * len / 2, zy, x - direction * (len / 2 + size), zy + fin * h, x - direction * len / 3, zy + fin * h / 2, 96, 102, 114, alpha);
	}
	boxRGBA(surface, x + direction * size - size / 2, zy + h - 2, x + direction * size + size / 2, zy + h + size / 3, 70, 74, 88, alpha);
	if((phase / 500) % 2 == 0) filledCircleRGBA(surface, x - direction * len / 2, zy, 2, 255, 60, 50, alpha);
}

// Five small saucers hold a V formation, wheel together and wink their rim lights.
static void sky_event_79(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int u = 0; u < 5; u++) {
		int row = (u + 1) / 2, side = u == 0 ? 0 : (u % 2 ? -1 : 1);
		int ux = x - direction * row * size * 3;
		int uy = y + side * row * size * 2 + (int)(sin(phase / 600.0 + row) * size * 0.4);
		filledEllipseRGBA(surface, ux, uy, size, size / 3 + 1, 120, 132, 150, alpha);
		filledEllipseRGBA(surface, ux, uy - size / 4, size / 2, size / 3 + 1, 160, 215, 230, (Uint8)(alpha * 0.9));
		for(int light = -1; light <= 1; light++) {
			if(((phase / 160) + u + light + 3) % 3 == 0) filledCircleRGBA(surface, ux + light * size * 2 / 3, uy + size / 5, 1, 255, 240, 160, alpha);
		}
	}
}

// A ball of plasma drifts across the sky throwing crackling arcs of violet light.
static void sky_event_80(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 violet[3] = { 190, 140, 255 };
	weapon_glow(surface, x, y, size * 1.1, violet, 220.0 * fade);
	unsigned int tick = phase / 90;
	for(int arc = 0; arc < 7; arc++) {
		double a = fx_hash(seed + arc, tick) % 6283 / 1000.0;
		double len = size * (1.5 + (fx_hash(seed, tick + arc * 7) % 100) / 60.0);
		double px = x, py = y;
		for(int seg = 1; seg <= 5; seg++) {
			double f = seg / 5.0;
			double nx = x + cos(a) * len * f + ((int)(fx_hash(tick, arc * 9 + seg) % 7) - 3) * size * 0.12;
			double ny = y + sin(a) * len * f + ((int)(fx_hash(tick, arc * 11 + seg) % 7) - 3) * size * 0.12;
			thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 1, 235, 225, 255, (Uint8)(alpha * (1.0 - f * 0.6)));
			px = nx;
			py = ny;
		}
	}
}

// Red sprites: faint crimson jellyfish-shaped flashes blink high above a distant storm.
static void sky_event_81(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double cx = surface->w * (0.2 + 0.6 * ((seed >> 5) % 100) / 100.0);
	for(int burst = 0; burst < 3; burst++) {
		unsigned int step = phase / 380 + burst * 5;
		double life = fmod(phase / 380.0 + burst * 0.37, 1.0);
		if(fx_hash(seed, step) % 3 == 0) continue;
		double bx = cx + ((int)(fx_hash(seed, step + 3) % 200) - 100) * size * 0.05;
		double flash = (1.0 - life) * (1.0 - life);
		for(int tendril = -3; tendril <= 3; tendril++) {
			double top = y - size * 2.2 + abs(tendril) * size * 0.2;
			thickLineRGBA(surface, (int)(bx + tendril * size * 0.35), (int)top, (int)(bx + tendril * size * 0.2), (int)(top + size * 2.0 * flash + size * 0.4), 1, 255, 70, 80, weapon_alpha(alpha * 1.3 * flash));
		}
		filledEllipseRGBA(surface, (int)bx, (int)(y - size * 2.2), (int)(size * 1.5), (int)(size * 0.35), 255, 70, 90, weapon_alpha(alpha * 0.5 * flash));
	}
}

// A modular space station crosses the sky: truss, habitat tubes, big solar wings catching the light.
static void sky_event_82(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	int tilt = (int)(sin(phase / 1500.0) * size * 0.3);
	thickLineRGBA(surface, x - size * 4, y + tilt, x + size * 4, y - tilt, 2, 150, 158, 170, alpha);
	for(int mod = -1; mod <= 1; mod++) {
		int mx = x + mod * size * 2;
		filledEllipseRGBA(surface, mx, y - mod * tilt / 4, size * 3 / 4, size / 3 + 1, 178, 184, 194, alpha);
		thickLineRGBA(surface, mx, y - size * 3 / 4, mx, y + size * 3 / 4, 2, 120, 128, 140, alpha);
	}
	for(int side = -1; side <= 1; side += 2) {
		int wx = x + side * size * 4;
		boxRGBA(surface, wx - size / 4, y - size * 2, wx + size / 4, y + size * 2, 60, 105, 160, alpha);
		for(int cell = 1; cell < 4; cell++) lineRGBA(surface, wx - size / 4, y - size * 2 + cell * size, wx + size / 4, y - size * 2 + cell * size, 130, 180, 230, (Uint8)(alpha * 0.8));
	}
	double glint = 0.5 + 0.5 * sin(phase / 220.0);
	static const Uint8 white[3] = { 250, 252, 255 };
	weapon_glow(surface, x + size * 4, y - size, 1.8, white, 200.0 * fade * glint);
}

// A binary star: two suns, one gold and one blue, orbit each other and pass in front of one another.
static void sky_event_83(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 gold[3] = { 255, 215, 130 }, blue[3] = { 150, 200, 255 };
	double cx = surface->w * (0.25 + 0.5 * ((seed >> 5) % 100) / 100.0);
	double a = phase / 900.0, r = size * 1.6;
	double gx = cx + cos(a) * r, gy = y + sin(a) * r * 0.4;
	double bx = cx - cos(a) * r, by = y - sin(a) * r * 0.4;
	if(sin(a) >= 0.0) {
		weapon_glow(surface, bx, by, size * 0.55, blue, 235.0 * fade);
		weapon_glow(surface, gx, gy, size * 0.8, gold, 235.0 * fade);
	} else {
		weapon_glow(surface, gx, gy, size * 0.8, gold, 235.0 * fade);
		weapon_glow(surface, bx, by, size * 0.55, blue, 235.0 * fade);
	}
	ellipseRGBA(surface, (int)cx, y, (int)r, (int)(r * 0.4), 200, 210, 235, (Uint8)(alpha * 0.25));
}

// A hawk circles on still wings, then folds them and stoops in a steep dive.
static void sky_event_84(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double cx = surface->w * (0.25 + 0.5 * ((seed >> 5) % 100) / 100.0);
	double dive = smooth_step(0.6, 0.85, progress);
	double a = phase / 800.0;
	double hx = cx + cos(a) * size * 4.0 * (1.0 - dive) + dive * direction * size * 3.0;
	double hy = y + sin(a) * size * 1.2 * (1.0 - dive) + dive * dive * scale * 0.35;
	double span = size * (1.4 - 1.0 * dive);
	int dir = cos(a) > 0 ? -1 : 1;
	if(dive > 0.0) dir = direction;
	thickLineRGBA(surface, (int)(hx - span), (int)(hy + span * 0.15), (int)hx, (int)hy, 2, 40, 32, 30, alpha);
	thickLineRGBA(surface, (int)hx, (int)hy, (int)(hx + span), (int)(hy + span * 0.15), 2, 40, 32, 30, alpha);
	filledEllipseRGBA(surface, (int)hx, (int)hy, (int)(size * 0.45), (int)(size * (0.2 + 0.4 * dive)) + 1, 52, 40, 36, alpha);
	filledTrigonRGBA(surface, (int)hx, (int)(hy + size * 0.3), (int)(hx - size * 0.2), (int)(hy + size * 0.9), (int)(hx + size * 0.2), (int)(hy + size * 0.9), 70, 52, 44, alpha);
	filledCircleRGBA(surface, (int)(hx + dir * size * 0.4), (int)(hy - size * 0.1), (int)(size * 0.17) + 1, 60, 46, 40, alpha);
}

// A bunch of escaped party balloons floats up and away on their trailing strings.
static void sky_event_85(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 colors[5][3] = { { 230, 80, 90 }, { 240, 200, 80 }, { 90, 170, 230 }, { 130, 220, 140 }, { 200, 120, 230 } };
	for(int b = 0; b < 7; b++) {
		double sway = sin(phase / 700.0 + b * 1.4);
		int bx = x - direction * (b % 4) * size + (int)(sway * size * 0.5);
		int by = y - (int)(progress * size * 2.0) - (b / 4) * size * 2 + (b % 3) * size / 2;
		const Uint8 *c = colors[b % 5];
		int r = size * 2 / 3 + 2;
		double sx = bx, sy = by + r;
		for(int seg = 1; seg <= 6; seg++) {
			double nx = bx - sway * size * 0.2 * seg * 0.3 + sin(phase / 300.0 + seg + b) * 2.0, ny = by + r + seg * size * 0.5;
			lineRGBA(surface, (int)sx, (int)sy, (int)nx, (int)ny, 200, 205, 215, (Uint8)(alpha * 0.7));
			sx = nx;
			sy = ny;
		}
		filledEllipseRGBA(surface, bx, by, r * 4 / 5, r, c[0], c[1], c[2], alpha);
		filledTrigonRGBA(surface, bx - 2, by + r, bx + 2, by + r, bx, by + r + 3, c[0], c[1], c[2], alpha);
		filledEllipseRGBA(surface, bx - r / 3, by - r / 3, r / 5 + 1, r / 3 + 1, 255, 255, 255, (Uint8)(alpha * 0.5));
	}
}

// A golden haze of pollen drifts through the sky, each grain glinting as it turns.
static void sky_event_86(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 gold[3] = { 255, 225, 120 };
	for(int g = 0; g < 40; g++) {
		unsigned int h = fx_hash(seed, g);
		double gx = x - direction * (h % 12) * size * 0.9 + sin(phase / 900.0 + g * 0.7) * size;
		double gy = y + ((h >> 8) % 10) * size * 0.5 + cos(phase / 1100.0 + g * 1.1) * size * 0.8;
		double glint = 0.5 + 0.5 * sin(phase / 200.0 + g * 2.0);
		filledCircleRGBA(surface, (int)gx, (int)gy, 1, gold[0], gold[1], gold[2], (Uint8)(alpha * (0.5 + 0.5 * glint)));
		if(glint > 0.92) weapon_glow(surface, gx, gy, 1.5, gold, 180.0 * fade);
	}
}

// A serpent of pale mist winds across the sky, thickening in the middle, with two faint glowing eyes.
static void sky_event_87(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	const int segs = 20;
	for(int i = segs; i >= 0; i--) {
		double u = (double)i / segs;
		double sx = x - direction * i * size * 0.9;
		double sy = y + sin(phase / 500.0 - i * 0.45) * size * 1.2;
		double r = size * (0.3 + 0.9 * sin(3.14159 * (1.0 - u) * 0.9 + 0.1));
		filledCircleRGBA(surface, (int)sx, (int)sy, (int)r + 1, 200, 215, 235, (Uint8)(alpha * 0.25 * (1.0 - u * 0.6)));
		filledCircleRGBA(surface, (int)sx, (int)(sy - r * 0.2), (int)(r * 0.6) + 1, 235, 242, 250, (Uint8)(alpha * 0.18));
	}
	weapon_glow(surface, x + direction * size * 0.3, y - size * 0.15, 1.5, (const Uint8[]){ 255, 240, 170 }, 220.0 * fade);
	weapon_glow(surface, x + direction * size * 0.3, y + size * 0.25, 1.5, (const Uint8[]){ 255, 240, 170 }, 220.0 * fade);
}

// Two small sky-galleons with billowing sails sail past, lanterns glowing at their sterns.
static void sky_event_88(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 lamp[3] = { 255, 200, 120 };
	for(int ship = 0; ship < 2; ship++) {
		int sx = x - direction * ship * size * 7, sy = y + ship * size * 2 + (int)(sin(phase / 800.0 + ship * 2.0) * size * 0.5);
		int len = size * 3 - ship * size / 2;
		filledTrigonRGBA(surface, sx - len, sy, sx + len, sy - size / 3, sx + len * 3 / 4, sy + size * 2 / 3, 70, 46, 40, alpha);
		filledTrigonRGBA(surface, sx - len, sy, sx + len * 3 / 4, sy + size * 2 / 3, sx - len * 3 / 4, sy + size * 2 / 3, 70, 46, 40, alpha);
		for(int mast = 0; mast < 2; mast++) {
			int mx = sx - len / 3 + mast * len * 2 / 3;
			thickLineRGBA(surface, mx, sy, mx, sy - size * 3, 1, 50, 36, 32, alpha);
			double belly = sin(phase / 600.0 + mast + ship) * size * 0.2 + size * 0.7;
			filledTrigonRGBA(surface, mx, sy - size * 3 + 2, mx, sy - size / 2, mx + direction * (int)belly, sy - size * 3 / 2, 235, 228, 210, alpha);
		}
		weapon_glow(surface, sx - direction * len, sy - size / 4, 1.8, lamp, 210.0 * fade);
		for(int puff = 0; puff < 3; puff++) {
			double u = fmod(phase / 1500.0 + puff / 3.0, 1.0);
			filledCircleRGBA(surface, (int)(sx - direction * (len + u * size * 3)), sy + size / 3, (int)(size * 0.3 * (0.5 + u)) + 1, 215, 225, 240, (Uint8)(alpha * 0.3 * (1.0 - u)));
		}
	}
}

// A pale jellyfish drifts through the air, its bell pulsing and its tentacles trailing behind.
static void sky_event_89(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 glow[3] = { 190, 170, 255 };
	double pulse = 0.5 + 0.5 * sin(phase / 420.0);
	int bw = (int)(size * (1.5 + 0.35 * pulse)), bh = (int)(size * (1.0 - 0.2 * pulse));
	for(int t = -3; t <= 3; t++) {
		double sx = x + t * bw / 4.0, sy = y + bh / 2;
		for(int seg = 1; seg <= 7; seg++) {
			double nx = sx - direction * size * 0.15 + sin(phase / 380.0 - seg * 0.9 + t) * size * 0.25, ny = sy + size * 0.5;
			lineRGBA(surface, (int)sx, (int)sy, (int)nx, (int)ny, 210, 195, 255, (Uint8)(alpha * 0.6 * (1.0 - seg / 9.0)));
			sx = nx;
			sy = ny;
		}
	}
	filledEllipseRGBA(surface, x, y, bw, bh, 200, 185, 250, (Uint8)(alpha * 0.55));
	filledEllipseRGBA(surface, x, y + bh / 4, bw * 2 / 3, bh / 2 + 1, 235, 225, 255, (Uint8)(alpha * 0.4));
	weapon_glow(surface, x, y, size * 0.45, glow, 120.0 * fade);
}

// Two manta rays glide past, wingtips rippling, one a little behind and below the other.
static void sky_event_90(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int ray = 0; ray < 2; ray++) {
		double cx = x - direction * ray * size * 3.5, cy = y + ray * size * 1.6 + sin(phase / 900.0 + ray) * size * 0.4;
		double flap = sin(phase / 350.0 - ray * 0.8);
		for(int side = -1; side <= 1; side += 2) {
			double tipx = cx - direction * size * 0.4, tipy = cy + side * size * 1.8 - flap * size * 0.7;
			filledTrigonRGBA(surface, (int)(cx + direction * size * 0.8), (int)cy, (int)tipx, (int)tipy, (int)(cx - direction * size * 0.4), (int)cy, 96, 116, 148, alpha);
		}
		filledEllipseRGBA(surface, (int)cx, (int)cy, size, size / 3 + 1, 110, 132, 164, alpha);
		filledEllipseRGBA(surface, (int)(cx + direction * size / 4), (int)(cy - 1), size / 2, size / 6 + 1, 160, 180, 205, (Uint8)(alpha * 0.7));
		for(int seg = 1; seg <= 5; seg++) {
			lineRGBA(surface, (int)(cx - direction * (size * 0.8 + (seg - 1) * size * 0.4)), (int)(cy + sin(phase / 300.0 - seg) * 2), (int)(cx - direction * (size * 0.8 + seg * size * 0.4)), (int)(cy + sin(phase / 300.0 - seg - 1) * 2), 110, 132, 164, alpha);
		}
	}
}

// Perfect smoke rings roll across the sky, each one spreading and thinning as it goes.
static void sky_event_91(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int i = 0; i < 5; i++) {
		double u = fmod(phase / 4200.0 * 0.6 + i * 0.2, 1.0);
		double rx = x - direction * i * size * 2.2;
		double r = size * (0.5 + 1.1 * u), tilt = 0.45 + 0.1 * sin(phase / 700.0 + i);
		double a = alpha * 0.55 * (1.0 - u);
		ellipseRGBA(surface, (int)rx, y + (i % 2) * size, (int)(r * tilt) + 1, (int)r, 232, 232, 238, (Uint8)a);
		ellipseRGBA(surface, (int)rx, y + (i % 2) * size, (int)(r * tilt) + 2, (int)r + 1, 215, 218, 228, (Uint8)(a * 0.6));
		ellipseRGBA(surface, (int)rx, y + (i % 2) * size, (int)(r * tilt), (int)r - 1, 200, 205, 218, (Uint8)(a * 0.4));
	}
}

// Three hang gliders in coloured wings drift down in a loose line, banking on the air.
static void sky_event_92(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 wing[3][3] = { { 220, 90, 70 }, { 240, 190, 70 }, { 80, 150, 210 } };
	for(int g = 0; g < 3; g++) {
		double gx = x - direction * g * size * 3.5, gy = y + g * size * 1.2 + sin(phase / 1100.0 + g * 1.7) * size * 0.4;
		double bank = sin(phase / 1300.0 + g * 2.0) * 0.25;
		int span = size * 2;
		int lx = (int)(gx - cos(bank) * span), ly = (int)(gy + sin(bank) * span * 0.5);
		int rx = (int)(gx + cos(bank) * span), ry = (int)(gy - sin(bank) * span * 0.5);
		filledTrigonRGBA(surface, lx, ly, rx, ry, (int)(gx + direction * size * 0.4), (int)(gy - size * 0.55), wing[g][0], wing[g][1], wing[g][2], alpha);
		filledTrigonRGBA(surface, lx, ly, rx, ry, (int)(gx - direction * size * 0.2), (int)(gy + size * 0.05), wing[g][0] * 2 / 3, wing[g][1] * 2 / 3, wing[g][2] * 2 / 3, alpha);
		thickLineRGBA(surface, (int)gx, (int)gy, (int)gx, (int)(gy + size * 0.8), 1, 30, 30, 36, alpha);
		filledCircleRGBA(surface, (int)gx, (int)(gy + size * 0.9), size / 7 + 1, 30, 30, 36, alpha);
	}
}

// A flurry of maple seeds spins down through the sky, each a single whirling wing.
static void sky_event_93(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int i = 0; i < 18; i++) {
		unsigned int h = fx_hash(seed, i);
		double u = fmod(phase / 3600.0 + (h % 100) / 100.0, 1.0);
		double sx = x - direction * ((h >> 8) % 14) * size * 0.8 + sin(phase / 700.0 + i) * size * 0.4;
		double sy = y - size * 1.5 + u * size * 5.0;
		double spin = phase / 90.0 + i * 1.3;
		double len = size * 0.6;
		double ex = sx + cos(spin) * len, ey = sy + sin(spin) * len * 0.35;
		thickLineRGBA(surface, (int)sx, (int)sy, (int)ex, (int)ey, 2, 190, 150, 90, (Uint8)(alpha * (1.0 - u * 0.5)));
		filledCircleRGBA(surface, (int)sx, (int)sy, 1, 120, 90, 60, (Uint8)(alpha * (1.0 - u * 0.5)));
	}
}

// Three koi swim through the air in lazy S-curves, their tails fanning behind them.
static void sky_event_94(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 body[3][3] = { { 255, 140, 60 }, { 245, 245, 240 }, { 230, 70, 60 } };
	for(int k = 0; k < 3; k++) {
		double hx = x - direction * k * size * 3.2, hy = y + k * size * 1.1;
		for(int seg = 6; seg >= 0; seg--) {
			double sx = hx - direction * seg * size * 0.55;
			double sy = hy + sin(phase / 450.0 - seg * 0.7 + k * 1.3) * size * 0.45;
			double r = size * (0.5 - seg * 0.045);
			filledCircleRGBA(surface, (int)sx, (int)sy, (int)r + 1, body[k][0], body[k][1], body[k][2], alpha);
			if(seg % 2 == 1) filledCircleRGBA(surface, (int)sx, (int)(sy - r * 0.3), (int)(r * 0.5) + 1, 230, 80, 60, (Uint8)(alpha * 0.6));
		}
		double tx = hx - direction * 7 * size * 0.55, ty = hy + sin(phase / 450.0 - 7 * 0.7 + k * 1.3) * size * 0.45;
		double fan = sin(phase / 300.0 + k) * size * 0.4;
		filledTrigonRGBA(surface, (int)(tx + direction * 2), (int)ty, (int)(tx - direction * size), (int)(ty - size * 0.7 + fan), (int)(tx - direction * size), (int)(ty + size * 0.7 + fan), body[k][0], body[k][1], body[k][2], (Uint8)(alpha * 0.7));
		filledCircleRGBA(surface, (int)(hx + direction * size * 0.25), (int)(hy - size * 0.1), 1, 20, 20, 24, alpha);
	}
}

// A flock of bright pinwheels spins across the sky on the wind.
static void sky_event_95(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 col[5][3] = { { 240, 80, 80 }, { 250, 200, 60 }, { 80, 190, 240 }, { 120, 220, 120 }, { 220, 120, 230 } };
	for(int p = 0; p < 5; p++) {
		double px = x - direction * p * size * 2.4, py = y + sin(phase / 800.0 + p * 1.4) * size * 0.9 + (p % 2) * size;
		double spin = phase / (160.0 + p * 20.0) * direction;
		double r = size * (0.75 - 0.06 * p);
		for(int blade = 0; blade < 4; blade++) {
			double a = spin + blade * 1.5708;
			filledTrigonRGBA(surface, (int)px, (int)py, (int)(px + cos(a) * r), (int)(py + sin(a) * r), (int)(px + cos(a + 0.9) * r * 0.7), (int)(py + sin(a + 0.9) * r * 0.7), col[p][0], col[p][1], col[p][2], blade % 2 ? alpha : (Uint8)(alpha * 0.75));
		}
		filledCircleRGBA(surface, (int)px, (int)py, 1, 250, 250, 250, alpha);
	}
}

// Two jets cross the sky on opposite diagonals, each drawing a long white contrail that slowly spreads and fades.
static void sky_event_96(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int jet = 0; jet < 2; jet++) {
		double dir = jet ? -direction : direction;
		double slope = jet ? 0.22 : -0.18;
		double hx = jet ? x + direction * size * 4 : x, hy = y + (jet ? size * 3 : 0);
		for(int i = 0; i < 40; i++) {
			double back = i * size * 0.5;
			double tx = hx - dir * back, ty = hy - slope * back;
			double age = i / 40.0;
			filledCircleRGBA(surface, (int)tx, (int)ty, (int)(2 + age * size * 0.3), 250, 250, 255, (Uint8)(alpha * 0.95 * (1.0 - age)));
		}
		filledTrigonRGBA(surface, (int)(hx + dir * size * 0.5), (int)hy, (int)(hx - dir * size * 0.2), (int)(hy - size * 0.3), (int)(hx - dir * size * 0.2), (int)(hy + size * 0.3), 220, 224, 232, alpha);
		pixelRGBA(surface, (int)(hx + dir * size * 0.5), (int)hy, 255, 255, 255, alpha);
	}
}

// A long ribbon of changing colour twists through the sky as if held by an invisible dancer.
static void sky_event_97(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double px = x, py = y;
	for(int i = 1; i <= 36; i++) {
		double nx = x - direction * i * size * 0.45;
		double ny = y + sin(phase / 380.0 - i * 0.38) * size * (0.4 + i * 0.05) + cos(phase / 710.0 - i * 0.2) * size * 0.4;
		double hue = phase / 900.0 + i * 0.18;
		Uint8 r = (Uint8)(190 + 60 * sin(hue)), g = (Uint8)(160 + 80 * sin(hue + 2.1)), b = (Uint8)(190 + 60 * sin(hue + 4.2));
		thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 3 - i / 16, r, g, b, (Uint8)(alpha * (1.0 - i / 40.0)));
		px = nx;
		py = ny;
	}
	weapon_glow(surface, x, y, 1.6, (const Uint8[]){ 255, 240, 220 }, 200.0 * fade);
}

// A giant moth with eye-spotted wings beats slowly across the sky, glowing softly.
static void sky_event_98(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 spot[3] = { 255, 220, 140 };
	double beat = cos(phase / 260.0);
	double wing = size * (0.4 + 1.5 * fabs(beat));
	double bob = sin(phase / 700.0) * size * 0.5;
	double cy = y + bob;
	for(int side = -1; side <= 1; side += 2) {
		double wy = cy - side * size * 0.9 * (0.4 + 0.6 * fabs(beat));
		filledTrigonRGBA(surface, x, (int)cy, (int)(x - direction * size * 1.2), (int)(wy), (int)(x + direction * size * 0.8), (int)(wy + side * size * 0.2), 205, 170, 140, alpha);
		filledEllipseRGBA(surface, (int)(x - direction * size * 0.4), (int)(cy - side * wing * 0.45), (int)(size * 0.9), (int)(wing * 0.45) + 1, 225, 185, 150, (Uint8)(alpha * 0.8));
		weapon_glow(surface, x - direction * size * 0.5, cy - side * wing * 0.5, size * 0.18, spot, 190.0 * fade);
	}
	filledEllipseRGBA(surface, (int)(x - direction * size * 0.1), (int)cy, (int)(size * 0.9), (int)(size * 0.22) + 1, 96, 76, 64, alpha);
	filledCircleRGBA(surface, (int)(x + direction * size * 0.8), (int)cy, (int)(size * 0.2) + 1, 90, 70, 60, alpha);
	lineRGBA(surface, (int)(x + direction * size), (int)cy, (int)(x + direction * size * 1.5), (int)(cy - size * 0.4), 200, 190, 170, alpha);
	lineRGBA(surface, (int)(x + direction * size), (int)cy, (int)(x + direction * size * 1.5), (int)(cy + size * 0.1), 200, 190, 170, alpha);
}

// A phoenix of living flame crosses the sky, wings beating, shedding sparks that drift down behind it.
static void sky_event_99(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 flame[3] = { 255, 170, 60 };
	double beat = sin(phase / 240.0);
	for(int k = 18; k >= 1; k--) {
		double u = k / 18.0;
		double px = x - direction * k * size * 0.45, py = y + sin(phase / 500.0 - k * 0.3) * size * 0.4 + u * size * 0.8;
		filledCircleRGBA(surface, (int)px, (int)py, (int)(size * 0.35 * (1.0 - u * 0.7)) + 1, 255, (Uint8)(190 - 140 * u), (Uint8)(70 - 40 * u), (Uint8)(alpha * 0.7 * (1.0 - u)));
	}
	for(int side = -1; side <= 1; side += 2) {
		double wy = y - side * size * 1.6 * (0.3 + 0.7 * fabs(beat)) - (beat > 0 ? 0 : -side * size * 0.3);
		filledTrigonRGBA(surface, x, y, (int)(x - direction * size * 1.8), (int)wy, (int)(x + direction * size * 0.5), (int)(wy + side * size * 0.3), 255, 120, 40, alpha);
		filledTrigonRGBA(surface, x, y, (int)(x - direction * size * 1.1), (int)((y + wy) / 2), (int)(x + direction * size * 0.3), (int)((y + wy) / 2), 255, 215, 90, (Uint8)(alpha * 0.8));
	}
	filledEllipseRGBA(surface, x, y, (int)(size * 0.9), (int)(size * 0.3) + 1, 255, 150, 50, alpha);
	filledCircleRGBA(surface, (int)(x + direction * size * 0.9), (int)(y - size * 0.15), (int)(size * 0.25) + 1, 255, 190, 80, alpha);
	weapon_glow(surface, x, y, size * 0.5, flame, 120.0 * fade);
}

// A drift of diamond dust: tiny ice crystals that flash into four-point glints one after another.
static void sky_event_100(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int g = 0; g < 46; g++) {
		unsigned int h = fx_hash(seed, g);
		double gx = x - direction * (h % 16) * size * 0.8 + sin(phase / 1200.0 + g) * size * 0.3;
		double gy = y + ((h >> 8) % 12) * size * 0.45 + phase / 40.0 * 0.0;
		double cyc = fmod(phase / 1400.0 + (h >> 16 & 255) / 255.0, 1.0);
		double flash = cyc < 0.4 ? sin(cyc / 0.4 * 3.14159) : 0.0;
		filledCircleRGBA(surface, (int)gx, (int)gy, 2, 215, 235, 255, (Uint8)(alpha * 1.4));
		if(flash > 0.05) {
			double l = size * (0.15 + 0.4 * flash);
			Uint8 a = (Uint8)(fmin(255.0, alpha * 1.7) * flash);
			thickLineRGBA(surface, (int)(gx - l), (int)gy, (int)(gx + l), (int)gy, 1, 240, 248, 255, a);
			thickLineRGBA(surface, (int)gx, (int)(gy - l), (int)gx, (int)(gy + l), 1, 240, 248, 255, a);
			weapon_glow(surface, gx, gy, 1.5, (const Uint8[]){ 200, 225, 255 }, 160.0 * flash * fade);
		}
	}
}

// A solar sailer glides by: a big bright square sail on a thin mast catching sunlight in bands, a tiny hull below.
static void sky_event_101(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double tilt = sin(phase / 1500.0) * 0.2;
	double sw = size * 2.2;
	double lx = x - sw, rx = x + sw;
	double ly = y - size * 1.4 + tilt * size, ry = y - size * 1.4 - tilt * size;
	for(int b = 0; b < 6; b++) {
		double f0 = b / 6.0, f1 = (b + 1) / 6.0;
		double x0 = lx + (rx - lx) * f0, x1 = lx + (rx - lx) * f1;
		double y0 = ly + (ry - ly) * f0, y1 = ly + (ry - ly) * f1;
		Uint8 shade = (Uint8)(150 + 90 * (0.5 + 0.5 * sin(phase / 700.0 - b * 0.9)));
		filledTrigonRGBA(surface, (int)x0, (int)y0, (int)x1, (int)y1, (int)x0, (int)(y0 + size * 2.4), shade, (Uint8)(shade - 20), (Uint8)(shade - 70), alpha);
		filledTrigonRGBA(surface, (int)x1, (int)y1, (int)x1, (int)(y1 + size * 2.4), (int)x0, (int)(y0 + size * 2.4), shade, (Uint8)(shade - 20), (Uint8)(shade - 70), alpha);
	}
	for(int c = 0; c < 4; c++) lineRGBA(surface, (int)x, (int)(y + size * 1.0), (int)(lx + (rx - lx) * (c + 0.5) / 4.0), (int)(ly + size * 2.4), 150, 150, 160, (Uint8)(alpha * 0.5));
	filledEllipseRGBA(surface, x, (int)(y + size * 1.1), (int)(size * 0.5), (int)(size * 0.25) + 1, 190, 195, 205, alpha);
	weapon_glow(surface, lx + (rx - lx) * (0.5 + 0.5 * sin(phase / 900.0)), ly + size * 0.8, 1.8, (const Uint8[]){ 255, 245, 210 }, 210.0 * fade);
}

// A small gas giant with three moons that circle it while the whole system drifts across the sky.
static void sky_event_102(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double R = size * 1.1;
	filledCircleRGBA(surface, x, y, (int)R, 150, 110, 150, alpha);
	for(int band = -2; band <= 2; band++) {
		double by = band * R * 0.32;
		double hw = sqrt(fmax(0.0, R * R - by * by));
		boxRGBA(surface, (int)(x - hw), (int)(y + by - R * 0.07), (int)(x + hw), (int)(y + by + R * 0.07), 190 - band * 12, 140, 170 + band * 10, (Uint8)(alpha * 0.8));
	}
	filledCircleRGBA(surface, (int)(x - R * 0.3), (int)(y - R * 0.3), (int)(R * 0.65), 255, 235, 240, (Uint8)(alpha * 0.18));
	for(int m = 0; m < 3; m++) {
		double a = phase / (900.0 + m * 500.0) + m * 2.1;
		double orbit = R * (1.8 + m * 0.7);
		double mx = x + cos(a) * orbit, my = y + sin(a) * orbit * 0.3;
		bool front = sin(a) > 0;
		ellipseRGBA(surface, x, y, (int)orbit, (int)(orbit * 0.3), 200, 200, 220, (Uint8)(alpha * 0.12));
		if(front || fabs(mx - x) > R) filledCircleRGBA(surface, (int)mx, (int)my, (int)(size * 0.22) + 1, 220 - m * 20, 215 - m * 25, 205 - m * 15, alpha);
	}
}

// Four dragonflies zip and hover across the sky, their wings shimmering.
static void sky_event_103(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int d = 0; d < 4; d++) {
		double burst = sin(phase / 700.0 + d * 1.7);
		double px = x - direction * d * size * 2.0 + burst * size * 0.9 * direction, py = y + (d - 1.5) * size * 0.9 + sin(phase / 330.0 + d) * size * 0.25;
		double flick = 0.5 + 0.5 * sin(phase / 28.0 + d);
		for(int pair = 0; pair < 2; pair++) {
			double ox = px - direction * size * (0.1 + pair * 0.25);
			filledEllipseRGBA(surface, (int)ox, (int)(py - size * 0.35), (int)(size * 0.14) + 1, (int)(size * (0.5 + 0.2 * flick)), 200, 235, 250, (Uint8)(alpha * (0.35 + 0.3 * flick)));
			filledEllipseRGBA(surface, (int)ox, (int)(py + size * 0.35), (int)(size * 0.14) + 1, (int)(size * (0.5 + 0.2 * (1.0 - flick))), 200, 235, 250, (Uint8)(alpha * (0.35 + 0.3 * (1.0 - flick))));
		}
		thickLineRGBA(surface, (int)(px + direction * size * 0.3), (int)py, (int)(px - direction * size * 1.0), (int)py, 2, 40, 150, 190, alpha);
		filledCircleRGBA(surface, (int)(px + direction * size * 0.35), (int)py, (int)(size * 0.14) + 1, 30, 120, 160, alpha);
	}
}

// A swirl of pink petals is lifted up and whirls across the sky, turning as it goes.
static void sky_event_104(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int p = 0; p < 34; p++) {
		unsigned int h = fx_hash(seed, p);
		double a = phase / 600.0 + p * 0.55;
		double r = size * (0.4 + (h % 100) / 100.0 * 2.2);
		double px = x - direction * (h >> 8 & 15) * size * 0.35 + cos(a) * r, py = y + sin(a) * r * 0.45 + (p - 17) * size * 0.04;
		double turn = phase / 150.0 + p;
		filledEllipseRGBA(surface, (int)px, (int)py, (int)(size * 0.22 * (0.4 + 0.6 * fabs(cos(turn)))) + 1, (int)(size * 0.14) + 1, 255, (Uint8)(170 + 40 * (p % 3)), 200, (Uint8)(alpha * (0.6 + 0.4 * sin(a))));
	}
}

// A great ring of twelve glowing signs turns slowly across the sky inside a thinner counter-turning ring.
static void sky_event_105(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 sign[3] = { 255, 225, 150 };
	double R = size * 2.6;
	ellipseRGBA(surface, x, y, (int)R, (int)(R * 0.5), 220, 200, 150, (Uint8)(alpha * 0.5));
	ellipseRGBA(surface, x, y, (int)(R * 0.7), (int)(R * 0.35), 200, 190, 160, (Uint8)(alpha * 0.4));
	for(int k = 0; k < 12; k++) {
		double a = phase / 2000.0 + k * 0.5236;
		double px = x + cos(a) * R, py = y + sin(a) * R * 0.5;
		double lit = 0.5 + 0.5 * sin(phase / 400.0 + k);
		weapon_glow(surface, px, py, 1.0 + lit, sign, 130.0 * (0.4 + lit) * fade);
		double a2 = -phase / 1500.0 + k * 0.5236;
		lineRGBA(surface, (int)(x + cos(a2) * R * 0.7), (int)(y + sin(a2) * R * 0.35), (int)px, (int)py, 230, 210, 160, (Uint8)(alpha * 0.18));
		filledCircleRGBA(surface, (int)(x + cos(a2) * R * 0.7), (int)(y + sin(a2) * R * 0.35), 1, 240, 225, 190, (Uint8)(alpha * 0.8));
	}
}

// A small spiral galaxy turns as it drifts, its arms made of thousands of faint stars around a warm core.
static void sky_event_106(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 core[3] = { 255, 230, 190 };
	double rot = phase / 1800.0;
	for(int arm = 0; arm < 2; arm++) {
		for(int i = 0; i < 70; i++) {
			unsigned int h = fx_hash(seed + arm * 31, i);
			double t = i / 70.0;
			double a = rot + arm * 3.14159 + t * 5.2 + ((h % 100) - 50) / 400.0;
			double r = size * (0.3 + 2.8 * t) * (0.9 + (h >> 8 & 31) / 150.0);
			double px = x + cos(a) * r, py = y + sin(a) * r * 0.42;
			filledCircleRGBA(surface, (int)px, (int)py, t < 0.5 ? 1 : 0 + 1, (Uint8)(200 + 55 * (1.0 - t)), (Uint8)(210 + 30 * (1.0 - t)), 255, (Uint8)(alpha * (1.2 - t)));
		}
	}
	weapon_glow(surface, x, y, size * 0.4, core, 200.0 * fade);
}

// A hummingbird darts through the sky, hovering and shooting forward in bursts, its wings a blur.
static void sky_event_107(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double hover = sin(phase / 900.0);
	double hx = x + direction * hover * size * 1.2, hy = y + sin(phase / 130.0) * 1.5 + cos(phase / 600.0) * size * 0.5;
	double flap = sin(phase / 24.0);
	for(int w = 0; w < 2; w++) {
		double ang = (w ? -1.0 : 1.0) * (0.9 + 0.5 * flap);
		filledEllipseRGBA(surface, (int)(hx - direction * size * 0.2), (int)(hy - size * 0.5 * cos(ang)), (int)(size * 0.14) + 1, (int)(size * 0.55), 190, 215, 235, (Uint8)(alpha * (w ? 0.3 : 0.55)));
	}
	filledEllipseRGBA(surface, (int)hx, (int)hy, (int)(size * 0.5), (int)(size * 0.25) + 1, 40, 160, 120, alpha);
	filledEllipseRGBA(surface, (int)(hx + direction * size * 0.2), (int)(hy + size * 0.1), (int)(size * 0.3), (int)(size * 0.14) + 1, 220, 90, 100, (Uint8)(alpha * 0.9));
	filledCircleRGBA(surface, (int)(hx + direction * size * 0.55), (int)(hy - size * 0.1), (int)(size * 0.2) + 1, 30, 130, 100, alpha);
	thickLineRGBA(surface, (int)(hx + direction * size * 0.7), (int)(hy - size * 0.1), (int)(hx + direction * size * 1.4), (int)(hy - size * 0.05), 1, 230, 230, 235, alpha);
	filledTrigonRGBA(surface, (int)(hx - direction * size * 0.4), (int)hy, (int)(hx - direction * size * 0.9), (int)(hy - size * 0.2), (int)(hx - direction * size * 0.9), (int)(hy + size * 0.3), 30, 110, 100, alpha);
}

// A bunch of coloured umbrellas floats down through the sky, each rocking on the air.
static void sky_event_108(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 col[5][3] = { { 220, 70, 80 }, { 240, 200, 70 }, { 70, 150, 220 }, { 130, 200, 110 }, { 190, 110, 220 } };
	for(int u = 0; u < 5; u++) {
		double ux = x - direction * u * size * 2.4, uy = y + u * size * 0.6 + sin(phase / 700.0 + u * 1.3) * size * 0.5;
		double rock = sin(phase / 500.0 + u) * 0.25;
		int r = (int)(size * (0.9 - 0.06 * u));
		double hx = ux + sin(rock) * size * 1.4, hy = uy + cos(rock) * size * 1.4;
		lineRGBA(surface, (int)ux, (int)uy, (int)hx, (int)hy, 60, 50, 50, alpha);
		lineRGBA(surface, (int)hx, (int)hy, (int)(hx - 3), (int)(hy + 2), 60, 50, 50, alpha);
		filledPieRGBA(surface, (int)ux, (int)uy, r, 180 + (int)(rock * 57.3), 360 + (int)(rock * 57.3), col[u][0], col[u][1], col[u][2], alpha);
		for(int rib = -1; rib <= 1; rib++) lineRGBA(surface, (int)ux, (int)uy, (int)(ux + rib * r * 0.6 + sin(rock) * 0), (int)(uy - r * 0.8), 255, 255, 255, (Uint8)(alpha * 0.3));
	}
}

// Two swans fly low in perfect step, necks stretched, wings beating in unison.
static void sky_event_109(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double beat = sin(phase / 340.0);
	for(int sw = 0; sw < 2; sw++) {
		double sx = x - direction * sw * size * 2.4, sy = y + sw * size * 0.9 + sin(phase / 1200.0 + sw) * size * 0.2;
		for(int side = -1; side <= 1; side += 2) {
			double tipy = sy - side * size * (0.4 + 1.4 * fabs(beat)) * (beat > 0 ? 1.0 : 0.6);
			filledTrigonRGBA(surface, (int)(sx - direction * size * 0.3), (int)sy, (int)(sx + direction * size * 0.4), (int)sy, (int)(sx - direction * size * 0.9), (int)tipy, 240, 242, 248, (Uint8)(alpha * (side > 0 ? 1.0 : 0.7)));
		}
		filledEllipseRGBA(surface, (int)sx, (int)sy, (int)(size * 0.9), (int)(size * 0.28) + 1, 250, 250, 252, alpha);
		for(int seg = 0; seg < 5; seg++) {
			double nx = sx + direction * (size * 0.8 + seg * size * 0.28), ny = sy - sin(seg * 0.6) * size * 0.2 - seg * size * 0.02;
			filledCircleRGBA(surface, (int)nx, (int)ny, (int)(size * 0.1) + 1, 250, 250, 252, alpha);
		}
		filledCircleRGBA(surface, (int)(sx + direction * size * 2.3), (int)(sy - size * 0.35), (int)(size * 0.14) + 1, 250, 250, 252, alpha);
		filledTrigonRGBA(surface, (int)(sx + direction * size * 2.4), (int)(sy - size * 0.38), (int)(sx + direction * size * 2.75), (int)(sy - size * 0.32), (int)(sx + direction * size * 2.4), (int)(sy - size * 0.28), 240, 150, 60, alpha);
	}
}

// A school of silver flying fish leaps in long arcs across the sky, one after another.
static void sky_event_110(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int k = 0; k < 7; k++) {
		double u = fmod(phase / 1700.0 + k * 0.14, 1.0);
		double fx = x - direction * (k * size * 1.0) + direction * (u - 0.5) * size * 5.0;
		double fy = y + (k % 3) * size * 0.5 - sin(u * 3.14159) * size * 2.2;
		double ang = atan2(-cos(u * 3.14159) * 2.2 * 3.14159, 5.0);
		double dx = direction * cos(ang), dy = sin(ang) * direction;
		filledEllipseRGBA(surface, (int)fx, (int)fy, (int)(size * 0.5) + 1, (int)(size * 0.16) + 1, 205, 220, 235, alpha);
		filledTrigonRGBA(surface, (int)(fx - dx * size * 0.5), (int)(fy - dy * size * 0.5), (int)(fx - dx * size * 0.9), (int)(fy - dy * size * 0.5 - size * 0.3), (int)(fx - dx * size * 0.9), (int)(fy - dy * size * 0.5 + size * 0.3), 170, 195, 225, alpha);
		filledTrigonRGBA(surface, (int)fx, (int)fy, (int)(fx - direction * size * 0.3), (int)(fy - size * 0.7), (int)(fx + direction * size * 0.1), (int)(fy - size * 0.1), 215, 235, 255, (Uint8)(alpha * 0.7));
	}
}

// A small floating island with a tree, a lamp-lit hut and a thin waterfall that falls away into the dark.
static void sky_event_111(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double bob = sin(phase / 900.0) * size * 0.3;
	double iy = y + bob, w = size * 2.4;
	filledTrigonRGBA(surface, (int)(x - w), (int)iy, (int)(x + w), (int)iy, (int)(x + sin(phase / 2000.0) * size * 0.2), (int)(iy + size * 2.6), 96, 80, 84, alpha);
	filledEllipseRGBA(surface, x, (int)iy, (int)w, (int)(size * 0.35) + 1, 80, 150, 90, alpha);
	thickLineRGBA(surface, (int)(x - w * 0.4), (int)iy, (int)(x - w * 0.4), (int)(iy - size * 1.1), 2, 96, 70, 56, alpha);
	filledCircleRGBA(surface, (int)(x - w * 0.4), (int)(iy - size * 1.3), (int)(size * 0.6) + 1, 60, 140, 80, alpha);
	boxRGBA(surface, (int)(x + w * 0.2), (int)(iy - size * 0.7), (int)(x + w * 0.5), (int)iy, 200, 180, 150, alpha);
	filledTrigonRGBA(surface, (int)(x + w * 0.15), (int)(iy - size * 0.7), (int)(x + w * 0.55), (int)(iy - size * 0.7), (int)(x + w * 0.35), (int)(iy - size * 1.2), 150, 70, 70, alpha);
	weapon_glow(surface, x + w * 0.35, iy - size * 0.35, 0.8, (const Uint8[]){ 255, 210, 130 }, 210.0 * fade);
	for(int d = 0; d < 14; d++) {
		double u = fmod(phase / 900.0 + d / 14.0, 1.0);
		filledCircleRGBA(surface, (int)(x + w * 0.85 + sin(d) * 1.5), (int)(iy + u * size * 4.0), 1, 190, 220, 250, (Uint8)(alpha * (1.0 - u)));
	}
}

// A lens-shaped cloud hovers, layered like a stack of plates, its rim glowing in slow pastel pulses.
static void sky_event_112(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int l = 0; l < 5; l++) {
		double w = size * (3.2 - l * 0.45), yy = y + l * size * 0.28;
		filledEllipseRGBA(surface, x, (int)yy, (int)w, (int)(size * 0.28) + 1, 190 - l * 10, 190 - l * 12, 215 - l * 8, (Uint8)(alpha * 0.8));
		double hue = phase / 700.0 + l * 0.8;
		ellipseRGBA(surface, x, (int)yy, (int)w, (int)(size * 0.28) + 1, (Uint8)(210 + 40 * sin(hue)), (Uint8)(190 + 50 * sin(hue + 2.1)), (Uint8)(230 + 25 * sin(hue + 4.2)), (Uint8)(alpha * 0.9));
	}
	for(int k = 0; k < 8; k++) {
		double a = phase / 800.0 + k * 0.785;
		weapon_glow(surface, x + cos(a) * size * 3.0, y + size * 0.7 + sin(a) * size * 0.3, 0.8, (const Uint8[]){ 230, 210, 255 }, 120.0 * (0.5 + 0.5 * sin(a)) * fade);
	}
}

// Three dolphins leap through the sky in graceful arcs, one after another.
static void sky_event_113(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int k = 0; k < 3; k++) {
		double u = fmod(phase / 2300.0 + k * 0.28, 1.0);
		double span = size * 6.0, rise = size * 2.4;
		double hx = x - direction * k * size * 4.0 + direction * (u - 0.5) * span;
		double hy = y + k * size * 0.5 - sin(u * 3.14159) * rise;
		double vx = direction * span, vy = -cos(u * 3.14159) * rise * 3.14159;
		double vl = sqrt(vx * vx + vy * vy);
		double dx = vx / vl, dy = vy / vl;
		for(int seg = 0; seg < 6; seg++) {
			double r = size * (0.33 - fabs(seg - 1.5) * 0.04 - seg * 0.01);
			filledCircleRGBA(surface, (int)(hx - dx * seg * size * 0.4), (int)(hy - dy * seg * size * 0.4), (int)r + 1, 120, 160, 200, alpha);
		}
		filledTrigonRGBA(surface, (int)(hx - dx * size * 0.8), (int)(hy - dy * size * 0.8 - size * 0.1), (int)(hx - dx * size * 1.2), (int)(hy - dy * size * 1.2 - size * 0.55), (int)(hx - dx * size * 1.5), (int)(hy - dy * size * 1.5), 90, 130, 170, alpha);
		double tx = hx - dx * size * 2.5, ty = hy - dy * size * 2.5;
		thickLineRGBA(surface, (int)(tx - size * 0.1), (int)(ty - size * 0.4), (int)(tx + dx * size * 0.1), (int)ty, 2, 100, 140, 180, alpha);
		thickLineRGBA(surface, (int)(tx - size * 0.1), (int)(ty + size * 0.4), (int)(tx + dx * size * 0.1), (int)ty, 2, 100, 140, 180, alpha);
		filledCircleRGBA(surface, (int)(hx + dx * size * 0.35), (int)(hy + dy * size * 0.35 - size * 0.04), (int)(size * 0.2) + 1, 160, 195, 225, alpha);
	}
}

// A pinwheel firework spins in the sky, throwing curved arms of gold sparks.
static void sky_event_114(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double spin = phase / 140.0;
	for(int arm = 0; arm < 6; arm++) {
		for(int i = 0; i < 12; i++) {
			double a = spin - i * 0.22 + arm * 1.0472;
			double r = size * (0.4 + i * 0.22);
			filledCircleRGBA(surface, (int)(x + cos(a) * r), (int)(y + sin(a) * r), i < 3 ? 2 : 1, 255, (Uint8)(220 - i * 12), (Uint8)(120 - i * 8), (Uint8)(alpha * 1.4 * (1.0 - i / 12.0)));
		}
	}
	weapon_glow(surface, x, y, size * 0.5, (const Uint8[]){ 255, 210, 120 }, 200.0 * fade);
}

// A thunderbird glides overhead, its wide wings edged with crackling blue lightning.
static void sky_event_115(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double beat = sin(phase / 500.0);
	for(int side = -1; side <= 1; side += 2) {
		double tipx = x - direction * size * 1.2, tipy = y - side * size * (1.6 + 1.2 * fabs(beat));
		filledTrigonRGBA(surface, x, y, (int)tipx, (int)tipy, (int)(x + direction * size * 0.6), (int)(y + side * size * 0.1), 120, 110, 175, alpha);
		double bx = x, by = y;
		unsigned int h = fx_hash(seed, (unsigned int)(phase / 90));
		for(int seg = 1; seg <= 6; seg++) {
			double f = seg / 6.0;
			double nx = x + (tipx - x) * f + ((int)((h >> seg * 3) & 7) - 3) * size * 0.08;
			double ny = y + (tipy - y) * f + ((int)((h >> (seg * 3 + 1)) & 7) - 3) * size * 0.08;
			lineRGBA(surface, (int)bx, (int)by, (int)nx, (int)ny, 150, 200, 255, (Uint8)(alpha * 1.6 > 255 ? 255 : alpha * 1.6));
			bx = nx;
			by = ny;
		}
	}
	filledEllipseRGBA(surface, x, y, (int)(size * 0.9), (int)(size * 0.28) + 1, 85, 80, 130, alpha);
	filledCircleRGBA(surface, (int)(x + direction * size), y, (int)(size * 0.22) + 1, 85, 80, 130, alpha);
	weapon_glow(surface, x + direction * size * 1.1, y - size * 0.05, 0.9, (const Uint8[]){ 255, 230, 120 }, 230.0 * fade);
	weapon_glow(surface, x, y, size * 0.5, (const Uint8[]){ 130, 180, 255 }, 90.0 * fade);
}

// A slim crescent moon sails across the sky with a tiny boat hung from its tip, fishing for a star.
static void sky_event_116(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double R = size * 2.6;
	for(int i = 0; i <= 40; i++) {
		double a = -1.2 + 2.4 * i / 40.0;
		double px = x + R * 0.6 - cos(a) * R, py = y - sin(a) * R;
		double r = size * 0.55 * cos(a / 1.2 * 1.5708) + 1.0;
		filledCircleRGBA(surface, (int)px, (int)py, (int)r, 245, 235, 200, alpha);
	}
	double bx = x + R * 0.6 - cos(1.2) * R, by = y - sin(1.2) * R;
	double sway = sin(phase / 500.0) * size * 0.3;
	lineRGBA(surface, (int)bx, (int)by, (int)(bx + sway), (int)(by + size * 1.2), 200, 200, 200, alpha);
	filledTrigonRGBA(surface, (int)(bx + sway - size * 0.4), (int)(by + size * 1.2), (int)(bx + sway + size * 0.4), (int)(by + size * 1.2), (int)(bx + sway), (int)(by + size * 1.5), 120, 80, 60, alpha);
	filledTrigonRGBA(surface, (int)(bx + sway), (int)(by + size * 1.2), (int)(bx + sway), (int)(by + size * 0.6), (int)(bx + sway + size * 0.35), (int)(by + size * 1.15), 235, 230, 220, alpha);
	lineRGBA(surface, (int)(bx + sway), (int)(by + size * 1.3), (int)(bx + sway - size * 0.3), (int)(by + size * 2.6), 200, 200, 200, (Uint8)(alpha * 0.7));
	weapon_glow(surface, bx + sway - size * 0.3, by + size * 2.6, 1.2, (const Uint8[]){ 255, 245, 190 }, 220.0 * (0.6 + 0.4 * sin(phase / 250.0)) * fade);
}

// A big crystal sphere rolls through the sky with a slow storm of gold and violet turning inside it.
static void sky_event_117(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double R = size * 1.5;
	filledCircleRGBA(surface, x, y, (int)R, 70, 80, 130, (Uint8)(alpha * 0.5));
	for(int i = 0; i < 60; i++) {
		double t = i / 60.0;
		double a = phase / 700.0 + t * 12.0;
		double r = R * 0.85 * t;
		double px = x + cos(a) * r, py = y + sin(a) * r * 0.9;
		filledCircleRGBA(surface, (int)px, (int)py, t < 0.4 ? 2 : 1, (Uint8)(255 * (1.0 - t * 0.4)), (Uint8)(200 - 100 * t), (Uint8)(120 + 120 * t), (Uint8)(alpha * 1.6 * (1.0 - t * 0.4)));
	}
	circleRGBA(surface, x, y, (int)R, 200, 215, 255, (Uint8)(alpha * 1.1));
	filledEllipseRGBA(surface, (int)(x - R * 0.4), (int)(y - R * 0.5), (int)(R * 0.25), (int)(R * 0.12) + 1, 255, 255, 255, (Uint8)(alpha * 0.8));
	weapon_glow(surface, x, y, R * 0.3, (const Uint8[]){ 255, 215, 150 }, 120.0 * fade);
}

// A wavy musical staff unrolls across the sky and glowing notes rise and fall along it.
static void sky_event_118(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int line = 0; line < 5; line++) {
		double px = x, py = y + line * size * 0.3;
		for(int i = 1; i <= 30; i++) {
			double nx = x - direction * i * size * 0.5;
			double ny = y + line * size * 0.3 + sin(phase / 600.0 - i * 0.35) * size * 0.8;
			lineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 225, 225, 240, (Uint8)(alpha * 0.55 * (1.0 - i / 32.0)));
			px = nx;
			py = ny;
		}
	}
	for(int n = 0; n < 8; n++) {
		double i = 2.0 + n * 3.6;
		double nx = x - direction * i * size * 0.5;
		double ny = y + ((n * 7) % 5) * size * 0.3 + sin(phase / 600.0 - i * 0.35) * size * 0.8;
		double lit = 0.6 + 0.4 * sin(phase / 300.0 + n);
		filledEllipseRGBA(surface, (int)nx, (int)ny, (int)(size * 0.18) + 1, (int)(size * 0.13) + 1, 255, 225, 150, (Uint8)(alpha * 1.4 * lit));
		thickLineRGBA(surface, (int)(nx + size * 0.15), (int)ny, (int)(nx + size * 0.15), (int)(ny - size * 0.8), 1, 255, 225, 150, (Uint8)(alpha * 1.2 * lit));
	}
}

// A big diamond kite flies on a long string, its tail of paper bows whipping in the wind.
static void sky_event_119(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double tilt = sin(phase / 700.0) * 0.2;
	double w = size * 0.9, h = size * 1.3;
	double cx = x, cy = y;
	filledTrigonRGBA(surface, (int)cx, (int)(cy - h), (int)(cx - w), (int)cy, (int)cx, (int)(cy + h * 0.2), 235, 90, 80, alpha);
	filledTrigonRGBA(surface, (int)cx, (int)(cy - h), (int)(cx + w), (int)cy, (int)cx, (int)(cy + h * 0.2), 250, 200, 80, alpha);
	filledTrigonRGBA(surface, (int)(cx - w), (int)cy, (int)cx, (int)(cy + h * 0.2), (int)cx, (int)(cy + h), 80, 150, 220, alpha);
	filledTrigonRGBA(surface, (int)(cx + w), (int)cy, (int)cx, (int)(cy + h * 0.2), (int)cx, (int)(cy + h), 100, 200, 130, alpha);
	lineRGBA(surface, (int)cx, (int)(cy - h), (int)cx, (int)(cy + h), 255, 255, 255, (Uint8)(alpha * 0.5));
	lineRGBA(surface, (int)(cx - w), (int)cy, (int)(cx + w), (int)cy, 255, 255, 255, (Uint8)(alpha * 0.5));
	double tx = cx, ty = cy + h;
	for(int i = 1; i <= 12; i++) {
		double nx = cx - direction * i * size * 0.4 + sin(phase / 250.0 - i * 0.8) * size * 0.4 * (i / 12.0) + tilt * i;
		double ny = cy + h + i * size * 0.28;
		lineRGBA(surface, (int)tx, (int)ty, (int)nx, (int)ny, 230, 230, 235, alpha);
		if(i % 2 == 0) {
			filledTrigonRGBA(surface, (int)nx, (int)ny, (int)(nx - size * 0.3), (int)(ny - size * 0.18), (int)(nx - size * 0.3), (int)(ny + size * 0.18), i % 4 ? 240 : 100, 120, i % 4 ? 100 : 230, alpha);
			filledTrigonRGBA(surface, (int)nx, (int)ny, (int)(nx + size * 0.3), (int)(ny - size * 0.18), (int)(nx + size * 0.3), (int)(ny + size * 0.18), i % 4 ? 240 : 100, 120, i % 4 ? 100 : 230, alpha);
		}
		tx = nx;
		ty = ny;
	}
}

// Two comets circle one another as they travel, their tails braiding into a bright double spiral.
static void sky_event_120(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int c = 0; c < 2; c++) {
		for(int i = 0; i < 26; i++) {
			double a = phase / 450.0 + c * 3.14159 - i * 0.16;
			double r = size * 1.2;
			double px = x - direction * i * size * 0.18 + cos(a) * r, py = y + sin(a) * r * 0.8;
			double f = 1.0 - i / 26.0;
			filledCircleRGBA(surface, (int)px, (int)py, (int)(1 + 2 * f), c ? 150 : 255, c ? 200 : 190, c ? 255 : 120, (Uint8)(alpha * 1.4 * f));
		}
		double a = phase / 450.0 + c * 3.14159;
		weapon_glow(surface, x + cos(a) * size * 1.2, y + sin(a) * size * 0.96, 1.4, c ? (const Uint8[]){ 160, 210, 255 } : (const Uint8[]){ 255, 200, 130 }, 230.0 * fade);
	}
}

// A curtain of glowing glass beads falls in long strands, swaying as if hung on invisible threads.
static void sky_event_121(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int strand = 0; strand < 14; strand++) {
		unsigned int h = fx_hash(seed, strand);
		double sx = x - direction * strand * size * 0.7;
		double top = y - size * 1.0 + ((h >> 4) & 7) * size * 0.1;
		for(int b = 0; b < 9; b++) {
			double v = fmod(phase / 2600.0 + b / 9.0 + (h & 63) / 64.0, 1.0);
			double by = top + v * size * 6.0, bx = sx + sin(phase / 800.0 + strand * 0.5 + v * 3.0) * size * 0.3;
			double hue = strand * 0.5 + b * 0.7;
			Uint8 c[3] = { (Uint8)(190 + 60 * sin(hue)), (Uint8)(190 + 60 * sin(hue + 2.1)), (Uint8)(215 + 40 * sin(hue + 4.2)) };
			filledCircleRGBA(surface, (int)bx, (int)by, 2, c[0], c[1], c[2], (Uint8)(alpha * 1.4 * sin(v * 3.14159)));
		}
	}
}

// A great sea turtle paddles slowly through the sky, its shell patterned with softly glowing hexagons.
static void sky_event_122(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 shell_glow[3] = { 120, 240, 200 };
	double bob = sin(phase / 1100.0) * size * 0.3;
	double cy = y + bob;
	double stroke = sin(phase / 700.0);
	for(int side = -1; side <= 1; side += 2) {
		double fx = x + direction * size * 0.7, fy = cy + side * size * 0.6;
		double tipx = fx + direction * size * 0.5 - direction * stroke * size * 0.5, tipy = cy + side * size * (1.6 + 0.4 * stroke);
		filledTrigonRGBA(surface, (int)fx, (int)fy, (int)(fx - direction * size * 0.5), (int)(fy + side * size * 0.1), (int)tipx, (int)tipy, 70, 130, 110, alpha);
		double bx = x - direction * size * 0.9, by = cy + side * size * 0.45;
		filledTrigonRGBA(surface, (int)bx, (int)by, (int)(bx - direction * size * 0.4), (int)by, (int)(bx - direction * size * 0.6), (int)(by + side * size * (0.7 + 0.2 * stroke)), 70, 130, 110, alpha);
	}
	filledEllipseRGBA(surface, x, (int)cy, (int)(size * 1.5), (int)(size * 1.0), 70, 120, 90, alpha);
	filledEllipseRGBA(surface, x, (int)cy, (int)(size * 1.2), (int)(size * 0.78), 96, 150, 110, alpha);
	for(int k = 0; k < 7; k++) {
		double a = k * 0.8976;
		double hx = k == 0 ? x : x + cos(a) * size * 0.62, hy = k == 0 ? cy : cy + sin(a) * size * 0.45;
		double lit = 0.5 + 0.5 * sin(phase / 500.0 + k * 0.9);
		weapon_glow(surface, hx, hy, size * 0.14, shell_glow, 110.0 * lit * fade);
		for(int e = 0; e < 6; e++) {
			double a1 = e * 1.0472, a2 = (e + 1) * 1.0472;
			lineRGBA(surface, (int)(hx + cos(a1) * size * 0.3), (int)(hy + sin(a1) * size * 0.22), (int)(hx + cos(a2) * size * 0.3), (int)(hy + sin(a2) * size * 0.22), 200, 245, 225, (Uint8)(alpha * 0.55));
		}
	}
	filledCircleRGBA(surface, (int)(x + direction * size * 1.6), (int)(cy - size * 0.05), (int)(size * 0.38), 90, 150, 120, alpha);
	filledCircleRGBA(surface, (int)(x + direction * size * 1.75), (int)(cy - size * 0.15), 1, 20, 24, 30, alpha);
}

// A line of prayer flags streams across the sky on an unseen cord, each flag rippling in turn.
static void sky_event_123(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 col[5][3] = { { 70, 130, 220 }, { 240, 240, 240 }, { 220, 70, 70 }, { 80, 190, 100 }, { 245, 205, 70 } };
	double px = x, py = y;
	for(int i = 1; i <= 24; i++) {
		double nx = x - direction * i * size * 0.55;
		double ny = y + sin(phase / 900.0 - i * 0.3) * size * 0.5 + i * size * 0.04;
		lineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 200, 200, 205, (Uint8)(alpha * 0.8));
		if(i % 2 == 0) {
			double ripple = sin(phase / 180.0 - i * 0.9) * size * 0.2;
			const Uint8 *c = col[(i / 2) % 5];
			double fw = size * 0.45, fh = size * 0.6;
			filledTrigonRGBA(surface, (int)nx, (int)ny, (int)(nx + fw * 0.9 * -direction), (int)(ny + ripple), (int)nx, (int)(ny + fh), c[0], c[1], c[2], (Uint8)(alpha * (1.0 - i / 30.0)));
			filledTrigonRGBA(surface, (int)(nx + fw * 0.9 * -direction), (int)(ny + ripple), (int)(nx + fw * 0.9 * -direction), (int)(ny + fh + ripple), (int)nx, (int)(ny + fh), c[0], c[1], c[2], (Uint8)(alpha * (1.0 - i / 30.0)));
		}
		px = nx;
		py = ny;
	}
}

// A small spinning top of light floats across the sky, shedding a spiral of sparks from its tip.
static void sky_event_124(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 tint[3] = { 255, 190, 120 };
	double wob = sin(phase / 300.0) * size * 0.15;
	double spin = phase / 90.0;
	filledTrigonRGBA(surface, (int)(x - size * 0.7 + wob), y, (int)(x + size * 0.7 + wob), y, (int)x, (int)(y + size * 1.2), 235, 120, 80, alpha);
	for(int band = 0; band < 3; band++) {
		double w = size * (0.55 - band * 0.15), yy = y + band * size * 0.22 + size * 0.1;
		double shift = cos(spin + band) * w * 0.7;
		lineRGBA(surface, (int)(x - w + wob * (1.0 - band * 0.3)), (int)yy, (int)(x + w + wob * (1.0 - band * 0.3)), (int)yy, 255, 235, 190, alpha);
		filledCircleRGBA(surface, (int)(x + shift), (int)yy, 1, 255, 255, 230, alpha);
	}
	thickLineRGBA(surface, (int)(x + wob), y, (int)(x + wob * 0.5), (int)(y - size * 0.5), 2, 220, 190, 140, alpha);
	for(int i = 0; i < 26; i++) {
		double a = spin * 0.6 - i * 0.4;
		double r = size * (0.15 + i * 0.07);
		double px = x - direction * i * size * 0.15 + cos(a) * r, py = y + size * 1.2 + sin(a) * r * 0.4;
		double f = 1.0 - i / 26.0;
		filledCircleRGBA(surface, (int)px, (int)py, f > 0.6 ? 2 : 1, 255, (Uint8)(220 * f + 30), 120, (Uint8)(alpha * 1.4 * f));
	}
	weapon_glow(surface, x, y + size * 1.2, 1.4, tint, 210.0 * fade);
}

// A feather drifts down through the sky, tumbling gently, leaving a faint shimmering trail.
static void sky_event_125(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 sheen[3] = { 255, 245, 230 };
	for(int i = 14; i >= 0; i--) {
		double age = i * 70.0;
		double ph = phase - age;
		double fx = x - direction * i * size * 0.25 + sin(ph / 600.0) * size * 1.0;
		double fy = y + ph / 500.0 * 0.0 + cos(ph / 900.0) * size * 0.4 + i * size * 0.15;
		filledCircleRGBA(surface, (int)fx, (int)fy, 1, 255, 245, 230, (Uint8)(alpha * 0.6 * (1.0 - i / 15.0)));
	}
	double sway = sin(phase / 600.0), turn = phase / 700.0;
	double fx = x + sway * size * 1.0, fy = y + cos(phase / 900.0) * size * 0.4;
	double len = size * 1.6, ang = sin(turn) * 0.9 + 0.3;
	double tx = fx + cos(ang) * len, ty = fy + sin(ang) * len * 0.6;
	double bx = fx - cos(ang) * len, by = fy - sin(ang) * len * 0.6;
	for(int v = 0; v < 12; v++) {
		double f = (v + 0.5) / 12.0;
		double mx = bx + (tx - bx) * f, my = by + (ty - by) * f;
		double w = size * 0.5 * sin(f * 3.14159) * (0.5 + 0.5 * fabs(cos(turn)));
		double nx = -sin(ang) * 0.6, ny = cos(ang);
		lineRGBA(surface, (int)mx, (int)my, (int)(mx + nx * w), (int)(my + ny * w + size * 0.1), 240, 236, 230, alpha);
		lineRGBA(surface, (int)mx, (int)my, (int)(mx - nx * w), (int)(my - ny * w + size * 0.1), 240, 236, 230, alpha);
	}
	thickLineRGBA(surface, (int)bx, (int)by, (int)tx, (int)ty, 1, 200, 190, 180, alpha);
	weapon_glow(surface, tx, ty, 1.0, sheen, 90.0 * fade);
}

// A school of glowing lantern-fish drifts through the sky, each with a bright bulb dangling in front of its mouth.
static void sky_event_126(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 bulb[3] = { 150, 255, 220 };
	for(int f = 0; f < 6; f++) {
		unsigned int h = fx_hash(seed, f);
		double fx = x - direction * f * size * 3.0 + sin(phase / 800.0 + f) * size * 0.3;
		double fy = y + (f % 3 - 1) * size * 1.3 + ((int)(h % 3) - 1) * size * 0.3 + sin(phase / 600.0 + f * 1.7) * size * 0.3;
		double len = size * (0.8 + 0.1 * (h % 4));
		filledEllipseRGBA(surface, (int)fx, (int)fy, (int)len, (int)(len * 0.45) + 1, 90, 120, 170, alpha);
		filledEllipseRGBA(surface, (int)fx, (int)(fy - len * 0.1), (int)(len * 0.8), (int)(len * 0.25) + 1, 140, 175, 220, (Uint8)(alpha * 0.8));
		double wag = sin(phase / 260.0 + f) * len * 0.3;
		filledTrigonRGBA(surface, (int)(fx - direction * len * 0.8), (int)fy, (int)(fx - direction * len * 1.5), (int)(fy - len * 0.4 + wag), (int)(fx - direction * len * 1.5), (int)(fy + len * 0.4 + wag), 100, 135, 185, alpha);
		double mx = fx + direction * len * 0.9;
		filledCircleRGBA(surface, (int)(mx - direction * len * 0.3), (int)(fy - len * 0.1), 1, 240, 250, 255, alpha);
		double bx = mx + direction * len * 0.4, by = fy - len * 0.9 + sin(phase / 400.0 + f) * len * 0.15;
		lineRGBA(surface, (int)(fx + direction * len * 0.4), (int)(fy - len * 0.4), (int)bx, (int)by, 150, 190, 200, (Uint8)(alpha * 0.8));
		weapon_glow(surface, bx, by, 2.6, bulb, 230.0 * (0.7 + 0.3 * sin(phase / 300.0 + f)) * fade);
	}
}

// A ring of paper cutout stars spins slowly like a mobile, each star turning on its own thread.
static void sky_event_127(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 col[4][3] = { { 255, 220, 120 }, { 255, 160, 170 }, { 150, 210, 255 }, { 190, 240, 170 } };
	double R = size * 2.2;
	double turn = phase / 1800.0;
	lineRGBA(surface, x, (int)(y - size * 2.4), x, y, 210, 210, 220, (Uint8)(alpha * 0.6));
	ellipseRGBA(surface, x, y, (int)R, (int)(R * 0.22), 210, 210, 220, (Uint8)(alpha * 0.7));
	for(int k = 0; k < 6; k++) {
		double a = turn + k * 1.0472;
		double sx = x + cos(a) * R, sy = y + sin(a) * R * 0.22;
		double drop = size * (0.9 + 0.35 * (k % 3));
		lineRGBA(surface, (int)sx, (int)sy, (int)sx, (int)(sy + drop), 200, 200, 210, (Uint8)(alpha * 0.6));
		double cy = sy + drop, spin = phase / 400.0 * (k % 2 ? 1.0 : -1.0) + k;
		double squash = fabs(cos(spin * 0.5)) * 0.7 + 0.3;
		const Uint8 *c = col[k % 4];
		for(int p = 0; p < 5; p++) {
			double a1 = p * 1.2566 - 1.5708, a2 = a1 + 0.6283, a3 = a1 + 1.2566;
			double r1 = size * 0.5, r2 = size * 0.2;
			filledTrigonRGBA(surface, (int)(sx + cos(a1) * r1 * squash), (int)(cy + sin(a1) * r1), (int)(sx + cos(a2) * r2 * squash), (int)(cy + sin(a2) * r2), (int)sx, (int)cy, c[0], c[1], c[2], alpha);
			filledTrigonRGBA(surface, (int)(sx + cos(a2) * r2 * squash), (int)(cy + sin(a2) * r2), (int)(sx + cos(a3) * r1 * squash), (int)(cy + sin(a3) * r1), (int)sx, (int)cy, c[0], c[1], c[2], alpha);
		}
	}
}

// A rolling wave of cloud curls over like surf, spray flying off its crest in fine droplets.
static void sky_event_128(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double R = size * 2.4;
	double sweep = 0.75 + 0.15 * sin(phase / 900.0);
	// A long swell rises from the left and curls over into a spiral in front.
	for(int i = 0; i < 60; i++) {
		double t = i / 59.0;
		double theta = 3.14159 - t * 4.4 * sweep * 1.3;
		double r = R * (1.0 - 0.6 * t);
		double px = x + direction * cos(theta) * r, py = y - sin(theta) * r * 0.8;
		filledCircleRGBA(surface, (int)px, (int)py, (int)(size * (0.5 - 0.3 * t)) + 1, 225, 232, 245, (Uint8)(alpha * (0.9 - 0.4 * t)));
	}
	for(int i = 0; i < 14; i++) {
		double f = i / 13.0;
		filledCircleRGBA(surface, (int)(x - direction * (R * 1.0 + (1.0 - f) * size * 3.0)), (int)(y + R * 0.2 + f * size * 0.1 - (1.0 - f) * size * 0.2), (int)(size * 0.45) + 1, 190, 205, 230, (Uint8)(alpha * 0.45 * f));
	}
	filledEllipseRGBA(surface, x, (int)(y + R * 0.95), (int)(R * 1.5), (int)(size * 0.3) + 1, 170, 190, 220, (Uint8)(alpha * 0.4));
	for(int d = 0; d < 22; d++) {
		unsigned int h = fx_hash(seed, d);
		double u = fmod(phase / 1100.0 + (h % 100) / 100.0, 1.0);
		double sx = x + direction * (R * 0.2 + u * size * 2.4), sy = y - R * 0.75 - sin(u * 3.14159) * size * 1.0 + u * u * size * 2.0;
		double jitter = ((int)((h >> 8) % 7) - 3) * size * 0.08;
		filledCircleRGBA(surface, (int)(sx + jitter), (int)sy, 1, 245, 250, 255, (Uint8)(alpha * 1.5 * (1.0 - u)));
	}
}

// A great white heron glides across the sky with slow wingbeats, long legs trailing, a ripple of light under each stroke.
static void sky_event_129(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double bob = cos(phase / 520.0) * size * 0.15;
	double cy = y + bob;
	for(int wing = 1; wing >= 0; wing--) {
		// Far wing (wing 1) is dimmer and a little behind; both beat together.
		double lag = wing ? 0.12 : 0.0;
		double lift = sin(phase / 520.0 - lag * 3.0);
		double ox = wing ? direction * size * 0.25 : 0.0;
		double tipx = x + ox - direction * size * (1.6 + 0.5 * fabs(lift)), tipy = cy - size * 2.0 * lift - size * 0.3;
		double armx = x + ox - direction * size * 0.2, army = cy - size * 1.2 * lift - size * 0.15;
		Uint8 a = (Uint8)(alpha * (wing ? 0.65 : 1.0));
		filledTrigonRGBA(surface, (int)(x + ox + direction * size * 0.6), (int)cy, (int)(x + ox - direction * size * 0.7), (int)cy, (int)armx, (int)army, 245, 246, 250, a);
		filledTrigonRGBA(surface, (int)(x + ox - direction * size * 0.7), (int)cy, (int)armx, (int)army, (int)tipx, (int)tipy, 228, 232, 244, a);
		lineRGBA(surface, (int)armx, (int)army, (int)tipx, (int)tipy, 200, 206, 224, a);
	}
	filledEllipseRGBA(surface, x, (int)cy, (int)(size * 0.9), (int)(size * 0.24) + 1, 250, 250, 252, alpha);
	for(int seg = 0; seg < 5; seg++) {
		double f = seg / 4.0;
		double nx = x + direction * (size * 0.8 + f * size * 0.7), ny = cy - size * 0.1 - sin(f * 3.14159) * size * 0.45 + f * size * 0.05;
		filledCircleRGBA(surface, (int)nx, (int)ny, (int)(size * 0.1) + 1, 250, 250, 252, alpha);
	}
	filledCircleRGBA(surface, (int)(x + direction * size * 1.6), (int)(cy - size * 0.05), (int)(size * 0.16) + 1, 250, 250, 252, alpha);
	filledTrigonRGBA(surface, (int)(x + direction * size * 1.7), (int)(cy - size * 0.1), (int)(x + direction * size * 2.4), (int)(cy - size * 0.02), (int)(x + direction * size * 1.7), (int)(cy + size * 0.03), 235, 190, 80, alpha);
	for(int leg = 0; leg < 2; leg++) {
		thickLineRGBA(surface, (int)(x - direction * size * 0.7), (int)(cy + size * 0.05 * leg), (int)(x - direction * size * (2.0 + 0.15 * leg)), (int)(cy + size * 0.35 + size * 0.05 * leg), 1, 60, 60, 66, alpha);
	}
	for(int r = 0; r < 3; r++) {
		double u = fmod(phase / 1040.0 + r / 3.0, 1.0);
		ellipseRGBA(surface, (int)(x - direction * u * size * 0.6), (int)(cy + size * 1.6), (int)(size * (0.3 + u * 1.0)), (int)(size * (0.06 + u * 0.18)) + 1, 220, 230, 245, (Uint8)(alpha * 0.45 * (1.0 - u)));
	}
}

// A flight of paper boats with glowing candle-flames sails through the sky on a slow river of mist.
static void sky_event_130(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 flame[3] = { 255, 200, 110 };
	for(int m = 0; m < 14; m++) {
		double mx = x - direction * m * size * 0.9, my = y + size * 1.2 + sin(phase / 700.0 - m * 0.5) * size * 0.35;
		filledCircleRGBA(surface, (int)mx, (int)my, (int)(size * 0.7), 190, 205, 230, (Uint8)(alpha * 0.2));
	}
	for(int b = 0; b < 5; b++) {
		double bx = x - direction * b * size * 2.5, by = y + sin(phase / 700.0 - b * 1.1) * size * 0.35 - (b % 2) * size * 0.2;
		double rock = sin(phase / 500.0 + b) * 0.12;
		double w = size * (0.8 - 0.05 * b);
		filledTrigonRGBA(surface, (int)(bx - w), (int)(by - w * 0.1 * rock * 8), (int)(bx + w), (int)(by + w * 0.1 * rock * 8), (int)bx, (int)(by + w * 0.45), 245, 240, 228, alpha);
		filledTrigonRGBA(surface, (int)bx, (int)(by - w * 0.9), (int)bx, (int)by, (int)(bx - direction * w * 0.7), (int)by, 235, 228, 214, alpha);
		filledTrigonRGBA(surface, (int)bx, (int)(by - w * 0.6), (int)bx, (int)by, (int)(bx + direction * w * 0.6), (int)by, 250, 246, 236, alpha);
		weapon_glow(surface, bx, by - w * 0.15, 1.0 + w * 0.05, flame, 220.0 * (0.75 + 0.25 * sin(phase / 130.0 + b * 2.0)) * fade);
	}
}

// A glowing hourglass of drifting sand turns slowly end over end, a thin bright thread falling through its waist.
static void sky_event_131(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 sand[3] = { 255, 215, 130 };
	double turn = phase / 1500.0;
	double flip = fmod(turn, 2.0);
	double ang = flip < 1.0 ? 0.0 : 3.14159 * (flip - 1.0);
	double ca = cos(ang), sa = sin(ang);
	double hh = size * 1.6, hw = size * 0.8;
	#define HG(px, py) (int)(x + (px) * ca - (py) * sa), (int)(y + (px) * sa + (py) * ca)
	filledTrigonRGBA(surface, HG(-hw, -hh), HG(hw, -hh), HG(0.0, 0.0), 120, 140, 190, (Uint8)(alpha * 0.45));
	filledTrigonRGBA(surface, HG(-hw, hh), HG(hw, hh), HG(0.0, 0.0), 120, 140, 190, (Uint8)(alpha * 0.45));
	double fill = flip < 1.0 ? flip : 1.0;
	double top = 1.0 - fill, bot = fill;
	filledTrigonRGBA(surface, HG(-hw * top, -hh * 0.2 - hh * 0.8 * (1.0 - top)), HG(hw * top, -hh * 0.2 - hh * 0.8 * (1.0 - top)), HG(0.0, 0.0), sand[0], sand[1], sand[2], (Uint8)(alpha * 1.1));
	filledTrigonRGBA(surface, HG(-hw * bot, hh - hh * 0.5 * bot), HG(hw * bot, hh - hh * 0.5 * bot), HG(0.0, hh - hh * 0.9 * bot), sand[0], sand[1], sand[2], (Uint8)(alpha * 1.1));
	lineRGBA(surface, HG(0.0, 0.0), HG(0.0, hh * 0.8 * (flip < 1.0 ? 1.0 : 0.0)), 255, 240, 190, alpha);
	lineRGBA(surface, HG(-hw, -hh), HG(hw, -hh), 220, 220, 235, alpha);
	lineRGBA(surface, HG(-hw, hh), HG(hw, hh), 220, 220, 235, alpha);
	#undef HG
	weapon_glow(surface, x, y, 1.6, sand, 150.0 * fade);
}

// A drift of glowing spores rises like champagne bubbles and bursts at the top into tiny rings of pale light.
static void sky_event_132(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int b = 0; b < 30; b++) {
		unsigned int h = fx_hash(seed, b);
		double u = fmod(phase / 2800.0 + (h % 100) / 100.0, 1.0);
		double bx = x - direction * ((h >> 8) % 16) * size * 0.5 + sin(phase / 700.0 + b) * size * 0.25;
		double by = y + size * 2.5 - u * size * 4.0;
		double r = size * (0.12 + 0.1 * ((h >> 16) % 3));
		if(u < 0.88) {
			filledCircleRGBA(surface, (int)bx, (int)by, (int)r + 1, 180, 230, 255, (Uint8)(alpha * 0.5));
			circleRGBA(surface, (int)bx, (int)by, (int)r + 1, 230, 250, 255, (Uint8)(alpha * 1.1));
			pixelRGBA(surface, (int)(bx - r * 0.3), (int)(by - r * 0.3), 255, 255, 255, alpha);
		} else {
			double q = (u - 0.88) / 0.12;
			ellipseRGBA(surface, (int)bx, (int)by, (int)(r + q * size * 0.5) + 1, (int)(r + q * size * 0.5) + 1, 220, 245, 255, (Uint8)(alpha * 1.3 * (1.0 - q)));
		}
	}
}

// A lone figure on a glowing disc surfs the air on a long ribbon of light, carving slow S-curves.
static void sky_event_133(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 trail[3] = { 140, 220, 255 };
	for(int i = 1; i <= 30; i++) {
		double px = x - direction * i * size * 0.5;
		double py = y + sin(phase / 500.0 - i * 0.28) * size * 0.9;
		double f = 1.0 - i / 31.0;
		filledCircleRGBA(surface, (int)px, (int)py, (int)(1 + 3 * f), trail[0], trail[1], trail[2], (Uint8)(alpha * 1.3 * f));
	}
	double bank = cos(phase / 500.0) * 0.5;
	double sx = x, sy = y + sin(phase / 500.0) * size * 0.9;
	thickLineRGBA(surface, (int)(sx - size * 1.0), (int)(sy + size * 0.1 + bank * size * 0.4), (int)(sx + size * 1.0), (int)(sy + size * 0.1 - bank * size * 0.4), 3, 230, 240, 255, alpha);
	weapon_glow(surface, sx, sy + size * 0.1, size * 0.35, trail, 150.0 * fade);
	thickLineRGBA(surface, (int)sx, (int)(sy + size * 0.1), (int)(sx + bank * size * 0.5), (int)(sy - size * 1.1), 4, 235, 225, 210, alpha);
	filledCircleRGBA(surface, (int)(sx + bank * size * 0.6), (int)(sy - size * 1.4), (int)(size * 0.3) + 1, 245, 235, 220, alpha);
	thickLineRGBA(surface, (int)(sx + bank * size * 0.4), (int)(sy - size * 0.8), (int)(sx - size * 0.9 + bank * size * 0.4), (int)(sy - size * 0.5 - bank * size * 0.6), 3, 235, 225, 210, alpha);
	thickLineRGBA(surface, (int)(sx + bank * size * 0.4), (int)(sy - size * 0.8), (int)(sx + size * 0.9 + bank * size * 0.4), (int)(sy - size * 0.5 + bank * size * 0.6), 3, 235, 225, 210, alpha);
}

// A bright aurora-coloured ribbon of birds wheels across the sky in a slow figure-eight, trailing glimmers.
static void sky_event_134(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	for(int b = 0; b < 16; b++) {
		double t = phase / 1500.0 - b * 0.2;
		double ex = sin(t) * size * 5.0, ey = sin(t * 2.0) * size * 1.8;
		double px = x + ex, py = y + ey;
		double vx = cos(t) * size * 5.0, vy = cos(t * 2.0) * size * 3.6;
		double l = sqrt(vx * vx + vy * vy) + 0.001;
		double dx = vx / l, dy = vy / l;
		double hue = b * 0.4 + phase / 800.0;
		Uint8 r = (Uint8)(150 + 90 * sin(hue)), g = (Uint8)(190 + 60 * sin(hue + 2.1)), bl = (Uint8)(215 + 40 * sin(hue + 4.2));
		double flap = sin(phase / 140.0 + b) * size * 0.3;
		thickLineRGBA(surface, (int)(px - dx * size * 0.3 - dy * (size * 0.6 + flap)), (int)(py - dy * size * 0.3 + dx * (size * 0.6 + flap)), (int)px, (int)py, 2, r, g, bl, (Uint8)(alpha * 1.4));
		thickLineRGBA(surface, (int)(px - dx * size * 0.3 + dy * (size * 0.6 + flap)), (int)(py - dy * size * 0.3 - dx * (size * 0.6 + flap)), (int)px, (int)py, 2, r, g, bl, (Uint8)(alpha * 1.4));
		filledCircleRGBA(surface, (int)px, (int)py, 2, 255, 255, 255, alpha);
		for(int k = 1; k <= 3; k++) filledCircleRGBA(surface, (int)(px - dx * k * size * 0.35), (int)(py - dy * k * size * 0.35), 1, r, g, bl, (Uint8)(alpha * 0.5 / k));
	}
}

// A small orrery drifts by: a bright sun with three planets circling it on faint rings, one with its own moon.
static void sky_event_135(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 sun[3] = { 255, 220, 140 };
	static const Uint8 planet[3][3] = { { 150, 190, 255 }, { 255, 160, 120 }, { 190, 255, 190 } };
	for(int p = 0; p < 3; p++) {
		double orbit = size * (1.8 + p * 1.3);
		ellipseRGBA(surface, x, y, (int)orbit, (int)(orbit * 0.4), 200, 205, 225, (Uint8)(alpha * 0.35));
		double a = phase / (500.0 + p * 380.0) + p * 2.0;
		double px = x + cos(a) * orbit, py = y + sin(a) * orbit * 0.4;
		filledCircleRGBA(surface, (int)px, (int)py, (int)(size * (0.32 + 0.08 * p)) + 1, planet[p][0], planet[p][1], planet[p][2], (Uint8)(alpha * 1.4));
		if(p == 1) {
			double ma = phase / 160.0;
			filledCircleRGBA(surface, (int)(px + cos(ma) * size * 0.45), (int)(py + sin(ma) * size * 0.2), 1, 235, 235, 240, alpha);
		}
	}
	weapon_glow(surface, x, y, size * 0.6, sun, 210.0 * fade);
}

// A lace of frost spreads across the sky like a window pane: branching ice ferns grow outward and glitter.
static void sky_event_136(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 ice[3] = { 210, 240, 255 };
	double grow = smooth_step(0.0, 1.0, fmod(phase / 3200.0, 1.0));
	for(int arm = 0; arm < 6; arm++) {
		double a = arm * 1.0472 + phase / 6000.0;
		double len = size * 3.2 * grow;
		double ex = x + cos(a) * len, ey = y + sin(a) * len * 0.8;
		thickLineRGBA(surface, x, y, (int)ex, (int)ey, 2, ice[0], ice[1], ice[2], alpha);
		for(int k = 1; k <= 6; k++) {
			double f = k / 7.0;
			double bx = x + cos(a) * len * f, by = y + sin(a) * len * 0.8 * f;
			double bl = size * 1.0 * (1.0 - f) * grow;
			for(int side = -1; side <= 1; side += 2) {
				lineRGBA(surface, (int)bx, (int)by, (int)(bx + cos(a + side * 0.9) * bl), (int)(by + sin(a + side * 0.9) * bl * 0.8), ice[0], ice[1], ice[2], (Uint8)(alpha * 0.9));
			}
		}
		double tw = sin(phase / 200.0 + arm * 1.3);
		if(tw > 0.5) weapon_glow(surface, ex, ey, 1.4, ice, 220.0 * (tw - 0.5) * 2.0 * fade);
	}
	weapon_glow(surface, x, y, size * 0.3, ice, 160.0 * fade);
}

// A string of glowing fairy lights unspools across the sky, each bulb twinkling in a travelling wave.
static void sky_event_137(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	double px = x, py = y;
	for(int i = 1; i <= 26; i++) {
		double nx = x - direction * i * size * 0.6;
		double ny = y + sin(phase / 900.0 - i * 0.35) * size * 0.8 + sin(i * 0.9) * size * 0.2;
		lineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 190, 190, 200, (Uint8)(alpha * 0.6 * (1.0 - i / 30.0)));
		double wave = 0.5 + 0.5 * sin(phase / 250.0 - i * 0.7);
		double hue = i * 0.5;
		Uint8 c[3] = { (Uint8)(220 + 35 * sin(hue)), (Uint8)(190 + 60 * sin(hue + 2.1)), (Uint8)(170 + 80 * sin(hue + 4.2)) };
		weapon_glow(surface, nx, ny + size * 0.15, 1.2 + wave, c, 230.0 * (0.3 + 0.7 * wave) * (1.0 - i / 30.0) * fade);
		px = nx;
		py = ny;
	}
}

// A lone hot-air lantern balloon rises through a swarm of tiny sparks that spiral up around it like a chimney draught.
static void sky_event_138(const SkyEvent *event) {
	SKY_EVENT_UNPACK(event);
	static const Uint8 warm[3] = { 255, 190, 100 };
	double bx = x, by = y + sin(phase / 800.0) * size * 0.4;
	double r = size * 1.1;
	filledEllipseRGBA(surface, (int)bx, (int)by, (int)(r * 0.8), (int)r, 235, 120, 80, alpha);
	filledEllipseRGBA(surface, (int)(bx - r * 0.2), (int)(by - r * 0.2), (int)(r * 0.4), (int)(r * 0.6), 255, 190, 140, (Uint8)(alpha * 0.8));
	for(int k = -1; k <= 1; k++) lineRGBA(surface, (int)(bx + k * r * 0.5), (int)(by + r * 0.7), (int)(bx + k * r * 0.2), (int)(by + r * 1.6), 200, 190, 180, alpha);
	boxRGBA(surface, (int)(bx - r * 0.25), (int)(by + r * 1.6), (int)(bx + r * 0.25), (int)(by + r * 1.9), 130, 90, 70, alpha);
	weapon_glow(surface, bx, by + r * 1.5, 1.6, warm, 230.0 * (0.8 + 0.2 * sin(phase / 90.0)) * fade);
	for(int i = 0; i < 40; i++) {
		double u = fmod(phase / 2200.0 + i / 40.0, 1.0);
		double a = u * 14.0 + i;
		double sx = bx + cos(a) * r * (0.5 + 0.7 * u), sy = by + r * 1.5 - u * size * 5.0 + sin(a) * 2.0;
		filledCircleRGBA(surface, (int)sx, (int)sy, 1, 255, 210, 140, (Uint8)(alpha * 1.5 * (1.0 - u)));
	}
}

// NEW_SKY_EVENTS_ABOVE (new sky_event_NN() functions go above this line)

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
	sky_event_38, sky_event_39, sky_event_40, sky_event_41, sky_event_42,
	sky_event_43, sky_event_44, sky_event_45, sky_event_46, sky_event_47,
	sky_event_48, sky_event_49, sky_event_50, sky_event_51, sky_event_52,
	sky_event_53, sky_event_54, sky_event_55, sky_event_56, sky_event_57,
	sky_event_58, sky_event_59, sky_event_60, sky_event_61, sky_event_62,
	sky_event_63, sky_event_64,
	sky_event_65,
	sky_event_66,
	sky_event_67,
	sky_event_68,
	sky_event_69,
	sky_event_70,
	sky_event_71,
	sky_event_72,
	sky_event_73,
	sky_event_74,
	sky_event_75,
	sky_event_76,
	sky_event_77,
	sky_event_78,
	sky_event_79,
	sky_event_80,
	sky_event_81,
	sky_event_82,
	sky_event_83,
	sky_event_84,
	sky_event_85,
	sky_event_86,
	sky_event_87,
	sky_event_88,
	sky_event_89,
	sky_event_90,
	sky_event_91,
	sky_event_92,
	sky_event_93,
	sky_event_94,
	sky_event_95,
	sky_event_96,
	sky_event_97,
	sky_event_98,
	sky_event_99,
	sky_event_100,
	sky_event_101,
	sky_event_102,
	sky_event_103,
	sky_event_104,
	sky_event_105,
	sky_event_106,
	sky_event_107,
	sky_event_108,
	sky_event_109,
	sky_event_110,
	sky_event_111,
	sky_event_112,
	sky_event_113,
	sky_event_114,
	sky_event_115,
	sky_event_116,
	sky_event_117,
	sky_event_118,
	sky_event_119,
	sky_event_120,
	sky_event_121,
	sky_event_122,
	sky_event_123,
	sky_event_124,
	sky_event_125,
	sky_event_126,
	sky_event_127,
	sky_event_128,
	sky_event_129,
	sky_event_130,
	sky_event_131,
	sky_event_132,
	sky_event_133,
	sky_event_134,
	sky_event_135,
	sky_event_136,
	sky_event_137,
	sky_event_138,
	// NEW_SKY_KINDS_ABOVE
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
