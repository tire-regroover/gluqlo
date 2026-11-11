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

const SDL_Color FONT_COLOR = { 0xda, 0xda, 0xe2 };
const SDL_Color DATE_COLOR = { 0xc6, 0xc2, 0xcf };
const SDL_Color BACKGROUND_COLOR = { 0x12, 0x14, 0x1d };
const SDL_Color CARD_BORDER_COLOR = { 0x37, 0x3d, 0x4e };

SDL_Surface *screen;

SDL_Rect hourBackground;
SDL_Rect minBackground;

SDL_Rect bgrect;
SDL_Surface *bg;
static SDL_Surface *backdrop_base;
static SDL_Surface *digit_glyphs[10];
static SDL_Surface *ampm_glyphs[2];

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
	SDL_Rect coords;
	SDL_Surface *ampm = ampm_glyphs[pm ? 1 : 0];
	if(!ampm) return;
	int offset = rect->h * 0.127;
	coords.x = rect->x + rect->h * 0.07;
	coords.y = rect->y + (pm ? rect->h - offset - ampm->h : offset);
	SDL_BlitSurface(ampm, 0, surface, &coords);
}

// Pre-render the clock digits and AM/PM labels once, since the time glyphs
// don't change between ambient frames. TTF_RenderGlyph_Blended is expensive
// to call every frame.
static void init_glyph_cache() {
	for(int i = 0; i < 10; i++) {
		digit_glyphs[i] = TTF_RenderGlyph_Blended(font_time, '0' + i, FONT_COLOR);
	}
	ampm_glyphs[0] = TTF_RenderText_Blended(font_mode, "AM", FONT_COLOR);
	ampm_glyphs[1] = TTF_RenderText_Blended(font_mode, "PM", FONT_COLOR);
}

static void free_glyph_cache() {
	for(int i = 0; i < 10; i++) {
		if(digit_glyphs[i]) SDL_FreeSurface(digit_glyphs[i]);
		digit_glyphs[i] = NULL;
	}
	for(int i = 0; i < 2; i++) {
		if(ampm_glyphs[i]) SDL_FreeSurface(ampm_glyphs[i]);
		ampm_glyphs[i] = NULL;
	}
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
	SceneWeather scene = { { 0.0, 0.0, 0.0, 0.0 },
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, { 0.0, 0.0, 0.0 } };

	Uint32 season_elapsed = ticks % (season_duration * 4);
	int season = season_elapsed / season_duration;
	Uint32 season_phase = season_elapsed % season_duration;
	double season_blend = smooth_step(season_duration - season_transition,
		season_duration, season_phase);
	scene.season[season] = 1.0 - season_blend;
	scene.season[(season + 1) % 4] = season_blend;

	// Each season draws from its own pool of presets so that rain, snow and
	// clear skies stay tied to the season instead of appearing at random.
	static const int season_weather_pool[4][4] = {
		{ 0, 2, 3, 9 },  // spring: clear, cloudy, light rain, fog
		{ 0, 1, 4, 5 },  // summer: clear, sunset, heavy rain, thunderstorm
		{ 2, 3, 4, 9 },  // autumn: cloudy, drizzle, rain, fog
		{ 6, 7, 2, 8 }   // winter: light snow, blizzard, overcast, aurora
	};
	const int pool_size = 4;
	int dominant = 0;
	for(int i = 1; i < 4; i++) {
		if(scene.season[i] > scene.season[dominant]) dominant = i;
	}
	const int *pool = season_weather_pool[dominant];

	Uint32 weather_slot = ticks / weather_duration;
	Uint32 current_seed = weather_slot * 2654435761u + 0x9e3779b9u;
	Uint32 previous_seed = (weather_slot - 1) * 2246822519u + 3266489917u;
	int current = pool[current_seed % pool_size];
	int previous = pool[previous_seed % pool_size];
	if(previous == current) {
		previous = pool[(previous_seed % pool_size + 1
			+ (current_seed >> 8) % (pool_size - 1)) % pool_size];
	}
	Uint32 weather_phase = ticks % weather_duration;
	double weather_blend = smooth_step(0.0, weather_transition, weather_phase);
	scene.cloud = weather_levels[previous][0]
		+ (weather_levels[current][0] - weather_levels[previous][0]) * weather_blend;
	scene.rain = weather_levels[previous][1]
		+ (weather_levels[current][1] - weather_levels[previous][1]) * weather_blend;
	scene.snow = weather_levels[previous][2]
		+ (weather_levels[current][2] - weather_levels[previous][2]) * weather_blend;
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

static void render_weather_effects(SDL_Surface *surface, Uint32 ticks,
	const SceneWeather *weather, int scale) {
	if(weather->aurora > 0.01) {
		for(int band = 0; band < 3; band++) {
			int previous_x = 0;
			int previous_y = surface->h * (19 + band * 12) / 100;
			for(int segment = 1; segment <= 24; segment++) {
				int x = surface->w * segment / 24;
				double nx = (double)x / surface->w;
				int y = surface->h * (19 + band * 12) / 100
					+ (int)(sin(nx * 6.28318530718 + ticks / (1100.0 + band * 270)
						+ band * 1.9) * scale * (0.03 + band * 0.008));
				Uint8 alpha = (Uint8)(weather->aurora * (28 - band * 5));
				thickLineRGBA(surface, previous_x, previous_y, x, y, 2,
					(Uint8)(70 + band * 24), (Uint8)(164 + band * 18),
					(Uint8)(137 + band * 30), alpha);
				previous_x = x;
				previous_y = y;
			}
		}
	}

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

	// Rainbow: appears when rain and sunshine overlap (sunshower).
	if(weather->rain > 0.15 && weather->sunshine > 0.25) {
		double rainbow_strength = weather->rain * weather->sunshine;
		int rainbow_x = surface->w / 2;
		int rainbow_y = surface->h * 68 / 100;
		int base_radius = scale / 3;
		static const Uint8 rainbow_colors[6][3] = {
			{ 226, 66, 60 }, { 240, 150, 54 }, { 244, 218, 74 },
			{ 96, 190, 96 }, { 74, 148, 226 }, { 150, 96, 210 }
		};
		for(int band = 0; band < 6; band++) {
			int radius = base_radius + band * scale / 60;
			Uint8 alpha = (Uint8)(rainbow_strength * (26 - band * 2));
			arcRGBA(surface, rainbow_x, rainbow_y, radius, 180, 360,
				rainbow_colors[band][0], rainbow_colors[band][1],
				rainbow_colors[band][2], alpha);
		}
	}

	// Hail: heavy rain produces fast-falling white pellets.
	if(weather->rain > 0.55) {
		int hail_count = (int)(surface->w * surface->h / 9000.0 * weather->rain);
		if(hail_count > 220) hail_count = 220;
		unsigned int hail_seed = ticks / 900 * 2654435761u + 0x9e3779b9u;
		for(int i = 0; i < hail_count; i++) {
			hail_seed = hail_seed * 1664525u + 1013904223u;
			int start_x = hail_seed % surface->w;
			hail_seed = hail_seed * 1664525u + 1013904223u;
			int start_y = hail_seed % surface->h;
			int speed = 14 + i % 6;
			double travel = ticks * speed / 90.0;
			int y = (start_y + (int)travel) % (surface->h + 8);
			int x = (start_x + (int)(travel * 0.10)) % surface->w;
			Uint8 alpha = (Uint8)(90 + (hail_seed >> 16) % 60);
			filledCircleRGBA(surface, x, y, 1 + i % 2,
				226, 232, 240, alpha);
		}
	}
}

// A distant bird, drawn as a shallow gull silhouette: two thin wing strokes
// meeting at the body, with the tips settling slightly below the mid-wing
// joint. At this scale any extra detail (head, beak, tail) collapses into a
// blob, so the shape is deliberately minimal and anti-aliased.
static void draw_bird(SDL_Surface *surface, int x, int y, int size, int wing_lift) {
	const Uint8 shade = 26, shade_g = 33, shade_b = 47;
	if(size < 2) size = 2;
	if(wing_lift < 0) wing_lift = 0;

	int span = size + size / 2;
	int lift = wing_lift + size / 3;
	int joint_x = span * 45 / 100;
	int joint_y = y - lift * 3 / 4;
	int tip_y = y - lift / 2;

	aalineRGBA(surface, x, y, x - joint_x, joint_y, shade, shade_g, shade_b, 165);
	aalineRGBA(surface, x - joint_x, joint_y, x - span, tip_y, shade, shade_g, shade_b, 145);
	aalineRGBA(surface, x, y, x + joint_x, joint_y, shade, shade_g, shade_b, 165);
	aalineRGBA(surface, x + joint_x, joint_y, x + span, tip_y, shade, shade_g, shade_b, 145);
}

static void render_fullscreen_event(SDL_Surface *surface, Uint32 ticks, int scale) {
	const Uint32 cycle = 90000;
	Uint32 slot = ticks / cycle;
	Uint32 slot_phase = ticks % cycle;
	Uint32 seed = slot * 2654435761u + 0x9e3779b9u;
	Uint32 duration = 6500 + (seed >> 9) % 4000;
	Uint32 start = (seed >> 16) % (cycle - duration);
	int kind = (seed >> 5) % 6;
	if(slot_phase < start || slot_phase >= start + duration || kind >= 6) return;
	Uint32 phase = slot_phase - start;
	double progress = (double)phase / duration;

	double fade = smooth_step(0.0, 1300.0, phase)
		* (1.0 - smooth_step(duration - 1200.0, duration, phase));
	if(kind == 0) {
		boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
			62, 94, 132, (Uint8)(10 * fade));
		int segments = 20;
		for(int band = 0; band < 3; band++) {
			int previous_x = 0;
			int previous_y = surface->h * (18 + band * 12) / 100;
			for(int i = 1; i <= segments; i++) {
				int x = surface->w * i / segments;
				double nx = (double)x / surface->w;
				int y = surface->h * (18 + band * 12) / 100
					+ (int)(sin(nx * 6.28318530718 + ticks / 900.0 + band * 1.7)
						* scale * (0.025 + band * 0.006));
				Uint8 alpha = (Uint8)((30 - band * 5) * fade);
				thickLineRGBA(surface, previous_x, previous_y, x, y,
					2, (Uint8)(82 + band * 28), (Uint8)(166 + band * 15),
					(Uint8)(167 + band * 24), alpha);
				previous_x = x;
				previous_y = y;
			}
		}
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
	} else if(kind == 3) {
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
	} else if(kind == 4) {
		// Solar eclipse: the sky darkens and a corona appears around the moon.
		boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
			8, 10, 18, (Uint8)(70 * fade));
		int eclipse_x = surface->w * (30 + (seed >> 12) % 40) / 100;
		int eclipse_y = surface->h * (20 + (seed >> 20) % 20) / 100;
		int sun_radius = scale / 9;
		int moon_radius = sun_radius * 9 / 10;
		double cover = smooth_step(0.15, 0.55, progress)
			* (1.0 - smooth_step(0.75, 0.95, progress));
		int moon_offset = (int)((0.5 - cover) * sun_radius * 2.2);
		filledCircleRGBA(surface, eclipse_x, eclipse_y, sun_radius + scale / 20,
			255, 200, 90, (Uint8)(60 * fade));
		filledCircleRGBA(surface, eclipse_x, eclipse_y, sun_radius,
			255, 220, 130, (Uint8)(120 * fade));
		filledCircleRGBA(surface, eclipse_x + moon_offset, eclipse_y, moon_radius,
			10, 12, 22, (Uint8)(230 * fade));
		for(int ray = 0; ray < 16; ray++) {
			double angle = ray * 0.39269908169 + ticks / 4000.0;
			int end_x = eclipse_x + (int)(cos(angle) * sun_radius * 1.6);
			int end_y = eclipse_y + (int)(sin(angle) * sun_radius * 1.6);
			thickLineRGBA(surface, eclipse_x, eclipse_y, end_x, end_y, 1,
				255, 235, 180, (Uint8)(14 * fade));
		}
	} else {
		// Meteor shower: streaks of light falling across the sky.
		boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
			10, 12, 24, (Uint8)(30 * fade));
		int meteor_count = 18 + (seed >> 8) % 10;
		for(int meteor = 0; meteor < meteor_count; meteor++) {
			unsigned int meteor_seed = (unsigned int)meteor * 2246822519u + seed;
			double meteor_progress = fmod((meteor_seed >> 8) / 4294967295.0
				+ progress * (0.7 + (meteor_seed >> 16) % 40 / 100.0), 1.0);
			int start_x = (int)(meteor_seed % surface->w);
			int start_y = (int)((meteor_seed >> 12) % (surface->h * 45 / 100));
			int length = scale / (14 + (meteor_seed >> 20) % 10);
			int dx = length / 3;
			int dy = length;
			int meteor_x = start_x + (int)(meteor_progress * surface->w * 0.6);
			int meteor_y = start_y + (int)(meteor_progress * surface->h * 0.5);
			Uint8 meteor_alpha = (Uint8)((60 + (meteor_seed >> 24) % 60) * fade
				* (1.0 - meteor_progress));
			thickLineRGBA(surface, meteor_x - dx, meteor_y - dy, meteor_x, meteor_y,
				1, 220, 230, 245, meteor_alpha);
			filledCircleRGBA(surface, meteor_x, meteor_y, 1,
				255, 255, 250, (Uint8)(meteor_alpha * 1.2));
		}
	}
}

// Defined further down; declared here so the long scenes can use the
// soft-light helpers without reordering the file.
static void soft_glow(SDL_Surface *surface, int cx, int cy, int radius,
	Uint8 red, Uint8 green, Uint8 blue, int peak_alpha);
static void set_pixel_glow(SDL_Surface *surface, int x, int y,
	Uint8 red, Uint8 green, Uint8 blue, int alpha);
static void soft_band(SDL_Surface *surface, int y0, int y1,
	Uint8 red, Uint8 green, Uint8 blue, int peak_alpha);
static void soft_column(SDL_Surface *surface, int cx, int base_y, int height,
	int half_width, Uint8 red, Uint8 green, Uint8 blue, int peak_alpha);

static void render_long_scene(SDL_Surface *surface, Uint32 ticks, int scale) {
	const Uint32 cycle = 210000;
	Uint32 slot = ticks / cycle;
	Uint32 slot_phase = ticks % cycle;
	Uint32 seed = slot * 2246822519u + 3266489917u;
	if(seed % 5 != 0) return;
	seed ^= seed >> 15;
	seed *= 2246822519u;
	seed ^= seed >> 13;
	Uint32 duration = 72000 + (seed >> 8) % 28000;
	Uint32 start = (seed >> 16) % (cycle - duration);
	if(slot_phase < start || slot_phase >= start + duration) return;
	Uint32 phase = slot_phase - start;
	double progress = (double)phase / duration;
	double fade = smooth_step(0.0, 6000.0, phase)
		* (1.0 - smooth_step(duration - 7000.0, duration, phase));
	int kind = seed % 7;

	if(kind == 0) {
		// Ridges emerging from haze as the light comes up. Each rank is drawn
		// as a filled silhouette with a soft top edge, so the hills dissolve
		// into the air instead of ending in a hard line.
		for(int rank = 0; rank < 4; rank++) {
			int base_y = surface->h * (66 + rank * 7) / 100;
			Uint8 shade = (Uint8)(30 + rank * 14);
			Uint8 alpha = (Uint8)((150 - rank * 22) * fade);
			int previous_x = 0;
			int previous_y = base_y;
			for(int segment = 1; segment <= 40; segment++) {
				double t = (double)segment / 40;
				int px = (int)(t * surface->w);
				double nx = t;
				// Two octaves of noise give a ridge that reads as terrain
				// rather than a sine wave.
				double ridge = smooth_noise(nx * 5.0 + rank * 3.1, rank * 1.7, 1123)
					* 0.65 + smooth_noise(nx * 13.0, rank * 2.3, 1451) * 0.35;
				int py = base_y - (int)((ridge - 0.35) * scale * 0.16);
				filledTrigonRGBA(surface, previous_x, previous_y, px, py,
					px, surface->h, shade, (Uint8)(shade + 7),
					(Uint8)(shade + 15), alpha);
				filledTrigonRGBA(surface, previous_x, previous_y,
					px, surface->h, previous_x, surface->h,
					shade, (Uint8)(shade + 7), (Uint8)(shade + 15), alpha);
				previous_x = px;
				previous_y = py;
			}
			// Haze pooling at the foot of each rank.
			soft_band(surface, base_y - scale / 30, base_y + scale / 12,
				150, 164, 186, (int)((9 + rank * 3) * fade));
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
		// A wildfire burning somewhere beyond the ridge. Only the glow and the
		// smoke are visible; no flame is ever resolved at this distance.
		boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
			135, 57, 27, (Uint8)(17 * fade));
		int fire_x = surface->w * (24 + (seed >> 8) % 52) / 100;
		double nx = (double)fire_x / surface->w;
		double ridge = 0.79
			+ (smooth_noise(nx * 5.2 + 8.0, 2.0, 1013) - 0.5) * 0.10;
		int fire_y = (int)(surface->h * ridge);
		// Slow breathing rather than a flicker, so it reads as a large fire
		// seen from far away.
		double breathe = 0.72 + 0.28 * sin(ticks / 3400.0);
		soft_glow(surface, fire_x, fire_y, scale / 4,
			226, 118, 58, (int)(52 * fade * breathe));
		soft_glow(surface, fire_x, fire_y, scale / 9,
			246, 168, 92, (int)(44 * fade * breathe));
		soft_glow(surface, fire_x, fire_y - scale / 40, scale / 22,
			255, 214, 150, (int)(30 * fade * breathe));
		// Smoke lifting off the fire and drifting with the wind.
		for(int plume = 0; plume < 7; plume++) {
			double t = (double)plume / 6;
			int px = fire_x + (int)(sin(t * 2.6 + ticks / 5200.0)
				* scale * 0.05 * t);
			int py = fire_y - (int)(t * scale * 0.30);
			soft_glow(surface, px, py, (int)(scale / 16 * (1.0 + t * 0.8)),
				74, 66, 62, (int)(26 * fade * (1.0 - t * 0.5)));
		}
		// The lit underside of the smoke, close to the source.
		soft_band(surface, fire_y - scale / 12, fire_y + scale / 40,
			198, 122, 74, (int)(30 * fade * breathe));
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
	} else if(kind == 4) {
		// Mist filling a valley at dawn: layered veils that thicken toward the
		// floor, with the far ridge dissolving into them. No hard edges.
		int ridge_y = surface->h * 62 / 100;
		double drift = sin(ticks / 9000.0) * scale * 0.012;
		for(int layer = 0; layer < 5; layer++) {
			int top = ridge_y + (int)(layer * scale * 0.045 + drift * layer);
			int bottom = surface->h * (74 + layer * 5) / 100;
			soft_band(surface, top, bottom, 196, 208, 220,
				(int)((26 + layer * 9) * fade));
		}
		// A low sun behind the mist, only its glow reaching through.
		int sun_x = surface->w * (30 + (seed >> 8) % 40) / 100;
		soft_glow(surface, sun_x, ridge_y - scale / 14, scale / 6,
			246, 226, 196, (int)(34 * fade));
		soft_glow(surface, sun_x, ridge_y - scale / 14, scale / 16,
			252, 240, 214, (int)(26 * fade));
	} else if(kind == 5) {
		// A slow curtain of rain crossing the valley, lit from behind so it
		// reads as a sheet of water rather than individual drops.
		int curtain_x = surface->w * (10 + (int)(progress * 80)) / 100;
		int curtain_w = scale / 2;
		int top_y = surface->h * 18 / 100;
		int bottom_y = surface->h * 92 / 100;
		// The veil: a wide, faint column so the rain has body before any
		// individual streak is drawn.
		soft_column(surface, curtain_x, bottom_y, bottom_y - top_y,
			curtain_w / 2, 168, 186, 206, (int)(26 * fade));
		soft_band(surface, top_y, bottom_y, 172, 190, 210, (int)(14 * fade));
		int streak_len = scale / 22;
		for(int streak = 0; streak < 260; streak++) {
			unsigned int s = (unsigned int)streak * 1664525u + seed;
			int sx = curtain_x - curtain_w / 2
				+ (int)((s % 1000) / 1000.0 * curtain_w);
			int span = bottom_y - top_y;
			int sy = top_y + (int)((s >> 10) % (Uint32)span);
			// Fall, wrapping within the curtain.
			sy = top_y + ((sy - top_y + (int)(ticks / 9.0)) % span + span) % span;
			// Fade the streaks at the curtain edges so it has no hard sides.
			double edge = 1.0 - fabs((double)(sx - curtain_x) / (curtain_w / 2.0));
			if(edge < 0.0) edge = 0.0;
			thickLineRGBA(surface, sx, sy, sx - 2, sy + streak_len, 1,
				186, 202, 220, (Uint8)(44 * fade * edge));
		}
	} else {
		// A distant forest at dusk. The canopy is a noise-driven silhouette
		// rather than a row of triangles: individual trees are never resolved,
		// only the ragged edge of the mass, which is what a treeline actually
		// looks like from a distance.
		//
		// Each rank is filled as ONE polygon. Drawing it as a chain of adjacent
		// alpha-blended triangles double-blends every shared edge, which shows
		// up as a vertical seam every few pixels.
		boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
			17, 20, 32, (Uint8)(34 * fade));
		enum { CANOPY_STEPS = 160 };
		static Sint16 canopy_x[CANOPY_STEPS + 3];
		static Sint16 canopy_y[CANOPY_STEPS + 3];
		for(int rank = 0; rank < 4; rank++) {
			int base_y = surface->h * (70 + rank * 5) / 100;
			Uint8 shade = (Uint8)(40 + rank * 18);
			Uint8 alpha = (Uint8)((185 - rank * 28) * fade);
			for(int segment = 0; segment <= CANOPY_STEPS; segment++) {
				double t = (double)segment / CANOPY_STEPS;
				// Three octaves: broad clumps, individual crowns, fine twigs.
				double canopy =
					smooth_noise(t * 9.0 + rank * 4.7, rank * 2.3, 2111) * 0.55
					+ smooth_noise(t * 34.0 + rank * 1.9, rank * 3.1, 2237) * 0.30
					+ smooth_noise(t * 90.0, rank * 1.3, 2371) * 0.15;
				canopy_x[segment] = (Sint16)(t * surface->w);
				canopy_y[segment] = (Sint16)(base_y
					- (canopy - 0.30) * scale * 0.13 * (1.0 - rank * 0.10));
			}
			// Close the polygon down to the bottom of the frame.
			canopy_x[CANOPY_STEPS + 1] = (Sint16)(surface->w - 1);
			canopy_y[CANOPY_STEPS + 1] = (Sint16)(surface->h - 1);
			canopy_x[CANOPY_STEPS + 2] = 0;
			canopy_y[CANOPY_STEPS + 2] = (Sint16)(surface->h - 1);
			filledPolygonRGBA(surface, canopy_x, canopy_y, CANOPY_STEPS + 3,
				shade, (Uint8)(shade + 7), (Uint8)(shade + 16), alpha);
			// Haze pooling along the base of each rank, so the layers separate
			// by depth rather than by outline.
			soft_band(surface, base_y - scale / 26, base_y + scale / 10,
				150, 164, 186, (int)((8 + rank * 3) * fade));
		}
	}
}

// --- Atmospheric light helpers -------------------------------------------
// Hard-edged primitives at low alpha read as sticks and stripes rather than
// air. These accumulate many faint passes so edges dissolve into a gradient,
// which is what makes distant light look like weather instead of geometry.

// A soft radial falloff. The centre accumulates every pass, the rim only one,
// so brightness decays smoothly with no visible disc edge.
static void soft_glow(SDL_Surface *surface, int cx, int cy, int radius,
	Uint8 red, Uint8 green, Uint8 blue, int peak_alpha) {
	if(radius < 2 || peak_alpha < 1) return;
	const int layers = 16;
	int step_alpha = peak_alpha / 5;
	if(step_alpha < 2) step_alpha = 2;
	for(int layer = layers; layer >= 1; layer--) {
		int r = radius * layer / layers;
		if(r < 1) continue;
		filledCircleRGBA(surface, cx, cy, r, red, green, blue,
			(Uint8)step_alpha);
	}
}

// A single lit point with a one-pixel falloff, cheap enough to use in loops.
static void set_pixel_glow(SDL_Surface *surface, int x, int y,
	Uint8 red, Uint8 green, Uint8 blue, int alpha) {
	if(alpha < 1) return;
	pixelRGBA(surface, x, y, red, green, blue, (Uint8)alpha);
	pixelRGBA(surface, x + 1, y, red, green, blue, (Uint8)(alpha / 3));
	pixelRGBA(surface, x - 1, y, red, green, blue, (Uint8)(alpha / 3));
	pixelRGBA(surface, x, y + 1, red, green, blue, (Uint8)(alpha / 3));
	pixelRGBA(surface, x, y - 1, red, green, blue, (Uint8)(alpha / 3));
}

// A horizontal veil with soft top and bottom edges, built from thin strips of
// varying alpha. Used for fog, low cloud and snow walls.
static void soft_band(SDL_Surface *surface, int y0, int y1,
	Uint8 red, Uint8 green, Uint8 blue, int peak_alpha) {
	if(y1 <= y0 || peak_alpha < 1) return;
	const int strips = 24;
	int strip_h = (y1 - y0) / strips;
	if(strip_h < 1) strip_h = 1;
	for(int i = 0; i < strips; i++) {
		double t = (double)i / (strips - 1);
		// sine envelope: transparent at both edges, strongest in the middle
		double envelope = sin(t * 3.14159265359);
		int a = (int)(peak_alpha * envelope * 0.30);
		if(a < 1) continue;
		int y = y0 + (int)(t * (y1 - y0));
		boxRGBA(surface, 0, y, surface->w - 1, y + strip_h,
			red, green, blue, (Uint8)a);
	}
}

// A vertical light column (light pillars, aurora sheets) fading out with
// height, built the same way so it has no hard top.
static void soft_column(SDL_Surface *surface, int cx, int base_y, int height,
	int half_width, Uint8 red, Uint8 green, Uint8 blue, int peak_alpha) {
	if(height < 2 || peak_alpha < 1) return;
	const int strips = 28;
	for(int i = 0; i < strips; i++) {
		double t = (double)i / (strips - 1);
		double envelope = 1.0 - t;
		envelope *= envelope;
		int a = (int)(peak_alpha * envelope * 0.30);
		if(a < 1) continue;
		int y = base_y - (int)(t * height);
		int w = (int)(half_width * (0.35 + 0.65 * (1.0 - t)));
		if(w < 1) w = 1;
		boxRGBA(surface, cx - w, y, cx + w, y + height / strips + 1,
			red, green, blue, (Uint8)a);
	}
}

static void render_rare_sky_event(SDL_Surface *surface, Uint32 ticks, int scale) {
	const Uint32 event_cycle = 48000;
	Uint32 slot = ticks / event_cycle;
	Uint32 slot_phase = ticks % event_cycle;
	Uint32 seed = slot * 2246822519u + 3266489917u;
	seed ^= seed >> 15;
	seed *= 2246822519u;
	seed ^= seed >> 13;
	Uint32 event_duration = 4200 + (seed >> 8) % 3200;
	Uint32 event_start = (seed >> 16) % (event_cycle - event_duration);
	if(slot_phase < event_start || slot_phase >= event_start + event_duration
		|| seed % 3 != 0) return;
	Uint32 phase = slot_phase - event_start;
	double progress = (double)phase / event_duration;
	double fade = smooth_step(0.0, 350.0, phase)
		* (1.0 - smooth_step(event_duration - 400.0, event_duration, phase));
	int direction = (seed & 1) ? 1 : -1;
	int y = surface->h * (8 + (seed >> 8) % 39) / 100;
	int margin = scale / 12;
	int x = direction > 0
		? -margin + (int)(progress * (surface->w + 2.0 * margin))
		: surface->w + margin - (int)(progress * (surface->w + 2.0 * margin));
	int size = scale / (26 + (seed >> 16) % 24);
	if(size < 4) size = 4;
	Uint8 alpha = (Uint8)(150 * fade);
	int kind = (seed >> 24) % 63;
	if(kind >= 23) {
		unsigned int rare_gate = slot * 2654435761u + 0x9e3779b9u;
		if(rare_gate % 7 != 0) return;
	}
	if(kind >= 23) {
		if(kind == 23) {
			// Aurora curtains: horizontal sheets of light that drift and fold,
			// fading out toward the ridge. Drawn as stacked translucent strokes
			// so the bands overlap into a glow instead of reading as lines.
			double strength = smooth_step(0.0, 1200.0, phase)
				* (1.0 - smooth_step(event_duration - 1400.0, event_duration, phase));
			for(int sheet = 0; sheet < 4; sheet++) {
				int base_y = surface->h * (10 + sheet * 7) / 100;
				int previous_x = 0;
				int previous_y = base_y;
				for(int segment = 1; segment <= 30; segment++) {
					double t = (double)segment / 30;
					int px = (int)(t * surface->w);
					int py = base_y
						+ (int)(sin(t * 4.1 + ticks / (2600.0 + sheet * 430)
							+ sheet * 1.6) * scale * (0.030 + sheet * 0.008))
						+ (int)(sin(t * 11.0 + ticks / 1700.0) * scale * 0.006);
					int alpha_here = (int)(strength * (26 - sheet * 4));
					if(alpha_here < 2) alpha_here = 2;
					// three overlapping widths give the band a soft, feathered edge
					thickLineRGBA(surface, previous_x, previous_y, px, py,
						7 + sheet, (Uint8)(64 + sheet * 12), (Uint8)(150 + sheet * 10),
						(Uint8)(136 + sheet * 14), (Uint8)(alpha_here / 4));
					thickLineRGBA(surface, previous_x, previous_y, px, py,
						3 + sheet, (Uint8)(78 + sheet * 12), (Uint8)(176 + sheet * 8),
						(Uint8)(150 + sheet * 12), (Uint8)(alpha_here / 2));
					thickLineRGBA(surface, previous_x, previous_y, px, py,
						1, (Uint8)(96 + sheet * 10), (Uint8)(198 + sheet * 6),
						(Uint8)(164 + sheet * 10), (Uint8)alpha_here);
					previous_x = px;
					previous_y = py;
				}
			}
		} else if(kind == 24) {
			// Distant storm: a low bank of cloud lit from within, far enough
			// away that no bolt is ever resolved.
			int bank_y = surface->h * 62 / 100;
			for(int layer = 0; layer < 4; layer++) {
				int layer_y = bank_y + layer * scale / 90;
				Uint8 layer_alpha = (Uint8)(alpha * (0.42 - layer * 0.07));
				boxRGBA(surface, 0, layer_y, surface->w - 1, layer_y + scale / 70,
					58, 66, 88, layer_alpha);
			}
			Uint32 pulse_phase = ticks % 9000;
			double pulse = 0.0;
			if(pulse_phase < 260) pulse = 1.0 - pulse_phase / 260.0;
			else if(pulse_phase > 1400 && pulse_phase < 1620)
				pulse = 0.45 * (1.0 - (pulse_phase - 1400) / 220.0);
			if(pulse > 0.0) {
				int glow_x = surface->w * (24 + (seed >> 12) % 52) / 100;
				filledCircleRGBA(surface, glow_x, bank_y + scale / 60,
					scale / 9, 132, 148, 186, (Uint8)(alpha * 0.55 * pulse));
				filledCircleRGBA(surface, glow_x, bank_y + scale / 60,
					scale / 22, 196, 208, 232, (Uint8)(alpha * 0.75 * pulse));
			}
		} else if(kind == 25) {
			// A single satellite: one steady point of light crossing the sky,
			// with a faint trail fading behind it.
			int sat_y = surface->h * (14 + (seed >> 8) % 26) / 100;
			int sat_x = direction > 0
				? -scale / 20 + (int)(progress * (surface->w + scale / 10.0))
				: surface->w + scale / 20 - (int)(progress * (surface->w + scale / 10.0));
			int trail = scale / 14;
			for(int step = 1; step <= 6; step++) {
				int trail_x = sat_x - direction * trail * step / 6;
				Uint8 trail_alpha = (Uint8)(alpha * 0.30 * (1.0 - step / 7.0));
				thickLineRGBA(surface, trail_x, sat_y, trail_x + direction, sat_y,
					1, 198, 212, 232, trail_alpha);
			}
			filledCircleRGBA(surface, sat_x, sat_y, 1, 236, 242, 250, alpha);
		} else if(kind == 26) {
			// Meteor: a brief streak that burns out rather than travelling.
			int meteor_y = surface->h * (10 + (seed >> 8) % 30) / 100;
			int meteor_x = surface->w * (20 + (seed >> 12) % 60) / 100;
			int length = scale / (10 + (seed >> 20) % 8);
			double burn = sin(progress * 3.14159265359);
			int head_x = meteor_x + (int)(progress * length * 1.6) * direction;
			int head_y = meteor_y + (int)(progress * length * 0.9);
			Uint8 meteor_alpha = (Uint8)(alpha * burn);
			thickLineRGBA(surface, head_x - direction * length,
				head_y - length * 9 / 16, head_x, head_y,
				1, 214, 226, 244, (Uint8)(meteor_alpha * 0.55));
			thickLineRGBA(surface, head_x - direction * length / 2,
				head_y - length * 9 / 32, head_x, head_y,
				1, 238, 244, 252, meteor_alpha);
			filledCircleRGBA(surface, head_x, head_y, 1, 255, 255, 255, meteor_alpha);
		} else if(kind == 27) {
			// Comet: a slow, wide-tailed body that holds its place in the sky.
			int comet_x = surface->w * (30 + (seed >> 12) % 40) / 100;
			int comet_y = surface->h * (16 + (seed >> 8) % 22) / 100;
			int tail = scale / 5;
			for(int layer = 0; layer < 5; layer++) {
				double spread = (layer - 2) * 0.16;
				int tail_x = comet_x - direction * tail * (1.0 - layer * 0.08);
				int tail_y = comet_y + (int)(tail * spread);
				Uint8 tail_alpha = (Uint8)(alpha * (0.16 - layer * 0.02));
				thickLineRGBA(surface, tail_x, tail_y, comet_x, comet_y,
					2, 176, 200, 226, tail_alpha);
			}
			filledCircleRGBA(surface, comet_x, comet_y, scale / 150 + 1,
				226, 234, 246, alpha);
			filledCircleRGBA(surface, comet_x, comet_y, scale / 400 + 1,
				255, 255, 255, alpha);
		} else if(kind == 28) {
			// Moonrise: a large, low moon with a soft halo, rising slowly.
			int moon_x = surface->w * (24 + (seed >> 12) % 52) / 100;
			int moon_y = surface->h * (58 - (int)(progress * 16)) / 100;
			int moon_radius = scale / 16;
			for(int halo = 4; halo >= 1; halo--) {
				filledCircleRGBA(surface, moon_x, moon_y,
					moon_radius + halo * scale / 90,
					186, 198, 216, (Uint8)(alpha * 0.05));
			}
			filledCircleRGBA(surface, moon_x, moon_y, moon_radius,
				214, 220, 228, (Uint8)(alpha * 0.55));
			filledCircleRGBA(surface, moon_x - moon_radius / 4,
				moon_y - moon_radius / 5, moon_radius / 3,
				232, 236, 242, (Uint8)(alpha * 0.30));
		} else if(kind == 29) {
			// Distant city: a warm band of light along the horizon, hazed by the
			// atmosphere so no individual building is ever legible.
			int city_x = surface->w * (20 + (seed >> 12) % 60) / 100;
			int city_y = surface->h * 76 / 100;
			double strength = sin(progress * 3.14159265359);
			soft_glow(surface, city_x, city_y, scale / 4,
				204, 152, 96, (int)(58 * strength));
			soft_band(surface, surface->h * 70 / 100, surface->h * 80 / 100,
				188, 142, 92, (int)(70 * strength));
			for(int light = 0; light < 30; light++) {
				unsigned int s = (unsigned int)light * 2246822519u + seed;
				int lx = (int)((s % 1000) / 1000.0 * surface->w);
				int ly = city_y - (int)((s >> 10) % (unsigned int)(scale / 50 + 1));
				set_pixel_glow(surface, lx, ly, 240, 206, 148,
					(int)(90 * strength));
			}
		} else if(kind == 30) {
			// Fog bank: a low drift of haze swallowing the ridge. Soft edges
			// only; a hard-edged band looks like a stripe, not weather.
			int drift = (int)(sin(ticks / 5200.0) * scale * 0.010);
			soft_band(surface, surface->h * 62 / 100 + drift,
				surface->h * 78 / 100 + drift, 138, 150, 166, 74);
			soft_band(surface, surface->h * 70 / 100 + drift,
				surface->h * 88 / 100 + drift, 150, 160, 176, 48);
		} else if(kind == 31) {
			// Nebula bloom: a slow-breathing patch of deep-sky colour with a
			// dense core and no discernible boundary.
			int nebula_x = surface->w * (24 + (seed >> 12) % 52) / 100;
			int nebula_y = surface->h * (14 + (seed >> 8) % 22) / 100;
			double breathe = 0.55 + 0.45 * sin(ticks / 6400.0);
			int peak = (int)(alpha * 0.55 * breathe);
			if(peak < 4) peak = 4;
			soft_glow(surface, nebula_x, nebula_y, scale / 7,
				104, 78, 152, peak);
			soft_glow(surface, nebula_x + scale / 40, nebula_y - scale / 60,
				scale / 14, 138, 96, 168, peak * 2 / 3);
			soft_glow(surface, nebula_x - scale / 50, nebula_y + scale / 70,
				scale / 26, 176, 150, 210, peak / 2);
			for(int star = 0; star < 22; star++) {
				unsigned int s = (unsigned int)star * 1664525u + seed;
				int sx = nebula_x
					+ (int)(s % (unsigned int)(scale / 5 + 1)) - scale / 10;
				int sy = nebula_y
					+ (int)((s >> 8) % (unsigned int)(scale / 8 + 1)) - scale / 16;
				pixelRGBA(surface, sx, sy, 228, 234, 248,
					(Uint8)(alpha * 0.45));
			}
		} else if(kind == 32) {
			// Distant thunderhead: a column of soft, overlapping masses. Built
			// from glows so it reads as depth rather than a stack of discs.
			int cloud_x = surface->w * (26 + (seed >> 12) % 48) / 100;
			int base_y = surface->h * 72 / 100;
			int height = scale / 4;
			for(int layer = 0; layer < 6; layer++) {
				double t = (double)layer / 5;
				int layer_y = base_y - (int)(t * height);
				int layer_x = cloud_x
					+ (int)(sin(t * 2.4 + ticks / 6200.0) * scale * 0.014);
				int radius = (int)(scale / 14 * (1.0 - t * 0.40));
				soft_glow(surface, layer_x, layer_y, radius,
					60, 68, 92, 34 - layer * 3);
			}
			Uint32 glow_phase = ticks % 11000;
			if(glow_phase < 420) {
				double glow = 1.0 - glow_phase / 420.0;
				soft_glow(surface, cloud_x, base_y - height / 2,
					scale / 12, 150, 166, 204, (int)(70 * glow));
			}
		} else if(kind == 33) {
			// A ringed planet hanging low, far too large to be near.
			int planet_x = surface->w * (22 + (seed >> 12) % 56) / 100;
			int planet_y = surface->h * (34 + (seed >> 8) % 12) / 100;
			int radius = scale / 9;
			for(int ring = 0; ring < 3; ring++) {
				int rx = radius * (16 + ring * 3) / 10;
				int ry = rx / 4;
				ellipseRGBA(surface, planet_x, planet_y, rx, ry,
					(Uint8)(150 + ring * 14), (Uint8)(140 + ring * 12),
					(Uint8)(120 + ring * 10), (Uint8)(alpha * (0.26 - ring * 0.07)));
			}
			filledCircleRGBA(surface, planet_x, planet_y, radius + scale / 40,
				44, 52, 74, (Uint8)(alpha * 0.28));
			filledCircleRGBA(surface, planet_x, planet_y, radius,
				58, 66, 90, (Uint8)(alpha * 0.50));
			// Night side: a dark disc nudged off-centre leaves a lit crescent.
			filledCircleRGBA(surface, planet_x + radius / 3,
				planet_y + radius / 6, radius, 16, 20, 34, (Uint8)(alpha * 0.55));
		} else if(kind == 34) {
			// Two moons at different distances, drifting slowly apart.
			int moon_y = surface->h * (16 + (seed >> 8) % 18) / 100;
			int base_x = surface->w * (30 + (seed >> 12) % 40) / 100;
			int separation = (int)(progress * scale * 0.16);
			int near_radius = scale / 34;
			int far_radius = scale / 52;
			int near_x = base_x - separation;
			int far_x = base_x + separation + scale / 20;
			filledCircleRGBA(surface, far_x, moon_y - scale / 60,
				far_radius + scale / 120, 190, 198, 212, (Uint8)(alpha * 0.08));
			filledCircleRGBA(surface, far_x, moon_y - scale / 60, far_radius,
				206, 212, 224, (Uint8)(alpha * 0.40));
			filledCircleRGBA(surface, near_x, moon_y,
				near_radius + scale / 90, 198, 206, 220, (Uint8)(alpha * 0.10));
			filledCircleRGBA(surface, near_x, moon_y, near_radius,
				220, 226, 234, (Uint8)(alpha * 0.58));
		} else if(kind == 35) {
			// The galactic band, brightening as the air settles.
			double rise = sin(progress * 3.14159265359);
			for(int band = 0; band < 3; band++) {
				int previous_x = 0;
				int previous_y = 0;
				for(int segment = 0; segment <= 22; segment++) {
					double t = (double)segment / 22;
					int px = (int)(t * surface->w);
					int py = surface->h * (8 + band * 5) / 100
						+ (int)(t * surface->h * 0.34)
						+ (int)(sin(t * 2.2 + band) * scale * 0.03);
					if(segment > 0) {
						thickLineRGBA(surface, previous_x, previous_y, px, py,
							band == 1 ? 5 : 3, 176, 186, 214,
							(Uint8)(alpha * 0.16 * rise));
					}
					previous_x = px;
					previous_y = py;
				}
			}
			for(int star = 0; star < 90; star++) {
				unsigned int s = (unsigned int)star * 1664525u + seed;
				double t = (s % 1000) / 1000.0;
				int sx = (int)(t * surface->w);
				int sy = surface->h * (8 + (s >> 10) % 30) / 100
					+ (int)(t * surface->h * 0.30);
				pixelRGBA(surface, sx, sy, 214, 222, 240,
					(Uint8)(alpha * 0.45 * rise));
			}
		} else if(kind == 36) {
			// Sheet lightning beyond the range: the far side lights up, but no
			// bolt ever resolves. The lit cloud bank is always present; the
			// pulses only brighten it, so the sky is never empty.
			int glow_x = surface->w * (20 + (seed >> 12) % 60) / 100;
			int ridge_y = surface->h * 66 / 100;
			for(int layer = 3; layer >= 1; layer--) {
				filledCircleRGBA(surface, glow_x, ridge_y,
					scale / (6 + layer * 2), 104, 118, 154,
					(Uint8)(alpha * 0.16));
			}
			boxRGBA(surface, 0, ridge_y - scale / 50, surface->w - 1,
				ridge_y + scale / 40, 88, 100, 132, (Uint8)(alpha * 0.14));
			Uint32 pulse = phase % 2600;
			double flash = 0.0;
			if(pulse < 180) flash = 1.0 - pulse / 180.0;
			else if(pulse > 700 && pulse < 860)
				flash = 0.6 * (1.0 - (pulse - 700) / 160.0);
			if(flash > 0.0) {
				for(int layer = 3; layer >= 1; layer--) {
					filledCircleRGBA(surface, glow_x, ridge_y,
						scale / (7 + layer * 2), 128, 146, 186,
						(Uint8)(alpha * 0.62 * flash));
				}
				boxRGBA(surface, 0, ridge_y - scale / 60, surface->w - 1,
					ridge_y, 112, 128, 168, (Uint8)(alpha * 0.30 * flash));
			}
		} else if(kind == 37) {
			// High cirrus: soft smears of thin cloud, no visible strokes.
			for(int wisp = 0; wisp < 7; wisp++) {
				unsigned int s = (unsigned int)wisp * 2246822519u + seed;
				int wy = surface->h * (10 + (s >> 8) % 28) / 100;
				int wx = (int)((s % 1000) / 1000.0 * surface->w);
				int drift = (int)(sin(ticks / 9000.0 + wisp) * scale * 0.010);
				soft_glow(surface, wx + drift, wy,
					scale / (16 + wisp % 3 * 4), 168, 176, 196, 22);
				soft_glow(surface, wx + drift + scale / 12, wy + scale / 90,
					scale / (22 + wisp % 2 * 5), 176, 184, 202, 14);
			}
		} else if(kind == 38) {
			// Volumetric shafts breaking through high cloud: a soft source with
			// wide, very faint wedges beneath it.
			int source_x = surface->w * (26 + (seed >> 12) % 48) / 100;
			int source_y = surface->h * 6 / 100;
			double strength = sin(progress * 3.14159265359);
			soft_glow(surface, source_x, source_y, scale / 14,
				228, 220, 192, (int)(52 * strength));
			for(int ray = 0; ray < 7; ray++) {
				double angle = 1.57079632679 + (ray - 3) * 0.105;
				int end_x = source_x + (int)(cos(angle) * scale * 0.70);
				int end_y = source_y + (int)(sin(angle) * scale * 0.70);
				thickLineRGBA(surface, source_x, source_y, end_x, end_y,
					11, 214, 206, 178, (Uint8)(alpha * 0.045 * strength));
				thickLineRGBA(surface, source_x, source_y, end_x, end_y,
					4, 222, 214, 186, (Uint8)(alpha * 0.075 * strength));
			}
		} else if(kind == 39) {
			// Space elevator: a hairline from the ridge into the sky, with a
			// single slow climber.
			int base_x = surface->w * (18 + (seed >> 12) % 64) / 100;
			int ridge_y = surface->h * 70 / 100;
			thickLineRGBA(surface, base_x, ridge_y, base_x, -scale / 40, 1,
				146, 158, 180, (Uint8)(alpha * 0.55));
			int climber_y = ridge_y - (int)(progress * (ridge_y + scale / 40));
			filledCircleRGBA(surface, base_x, climber_y, scale / 180 + 1,
				198, 214, 236, (Uint8)(alpha * 0.45));
			filledCircleRGBA(surface, base_x, climber_y, 1,
				232, 238, 248, (Uint8)(alpha * 0.75));
		} else if(kind == 40) {
			// A faint orbital arc crossing the whole sky.
			int previous_x = 0;
			int previous_y = surface->h * 62 / 100;
			for(int segment = 1; segment <= 26; segment++) {
				double t = (double)segment / 26;
				int px = (int)(t * surface->w);
				int py = (int)(surface->h * 62 / 100
					- sin(t * 3.14159265359) * surface->h * 0.44);
				thickLineRGBA(surface, previous_x, previous_y, px, py, 1,
					152, 164, 190, (Uint8)(alpha * 0.38));
				previous_x = px;
				previous_y = py;
			}
		} else if(kind == 41) {
			// Light pollution: the dome a distant city throws onto low cloud.
			// Only the glow is visible, never the source.
			int dome_x = surface->w * (24 + (seed >> 12) % 52) / 100;
			int dome_y = surface->h * 74 / 100;
			double strength = sin(progress * 3.14159265359);
			soft_glow(surface, dome_x, dome_y, scale / 5,
				206, 158, 104, (int)(46 * strength));
			soft_glow(surface, dome_x, dome_y, scale / 11,
				228, 186, 132, (int)(34 * strength));
			for(int lamp = 0; lamp < 14; lamp++) {
				unsigned int s = (unsigned int)lamp * 2246822519u + seed;
				int lx = dome_x
					+ (int)(s % (unsigned int)(scale / 8 + 1)) - scale / 16;
				int ly = dome_y
					+ (int)((s >> 9) % (unsigned int)(scale / 60 + 1));
				pixelRGBA(surface, lx, ly, 236, 200, 140,
					(Uint8)(alpha * 0.30 * strength));
			}
		} else if(kind == 42) {
			// A derelict station tumbling slowly, lit on one edge only.
			int body = size * 3;
			double tumble = progress * 1.2;
			int ring_rx = (int)(body * (1.0 + 0.4 * fabs(cos(tumble))));
			ellipseRGBA(surface, x, y, ring_rx, body / 3,
				62, 70, 86, (Uint8)(alpha * 0.70));
			filledCircleRGBA(surface, x, y, body / 5, 44, 50, 64,
				(Uint8)(alpha * 0.80));
			arcRGBA(surface, x, y, body / 5, 200, 340, 176, 186, 204,
				(Uint8)(alpha * 0.75));
		} else if(kind == 43) {
			// Only the running lights of something moving out there. No hull,
			// no shape, just points holding formation.
			int fleet_y = surface->h * (18 + (seed >> 8) % 20) / 100;
			for(int light = 0; light < 9; light++) {
				unsigned int s = (unsigned int)light * 2246822519u + seed;
				int lx = (int)((s % 1000) / 1000.0 * surface->w)
					+ direction * (int)(progress * surface->w * 0.35);
				lx = ((lx % surface->w) + surface->w) % surface->w;
				int ly = fleet_y
					+ (int)((s >> 10) % (unsigned int)(scale / 16 + 1)) - scale / 32;
				double flicker = 0.6 + 0.4 * sin(ticks / 1600.0 + light * 1.7);
				soft_glow(surface, lx, ly, scale / 45, 202, 216, 240,
					(int)(120 * flicker));
			}
		} else if(kind == 44) {
			// The aurora again, but only as it appears in the mist below: the
			// sky event is over, its light is still in the air.
			int mist_y = surface->h * 70 / 100;
			double strength = sin(progress * 3.14159265359);
			for(int band = 0; band < 5; band++) {
				int previous_x = 0;
				int previous_y = mist_y + band * scale / 50;
				for(int segment = 1; segment <= 24; segment++) {
					int px = surface->w * segment / 24;
					int py = mist_y + band * scale / 50
						+ (int)(sin(segment * 0.45 + ticks / 3200.0 + band)
							* scale * 0.012);
					thickLineRGBA(surface, previous_x, previous_y, px, py, 6,
						88, 170, 148, (Uint8)(strength * 14));
					thickLineRGBA(surface, previous_x, previous_y, px, py, 2,
						120, 198, 172, (Uint8)(strength * 26));
					previous_x = px;
					previous_y = py;
				}
			}
		} else if(kind == 45) {
			// A shower rather than a single streak. One continuous tapered tail
			// per meteor: stacking short segments made the tail read as dashes.
			double burst = sin(progress * 3.14159265359);
			for(int meteor = 0; meteor < 16; meteor++) {
				unsigned int s = (unsigned int)meteor * 2246822519u + seed;
				double t = fmod((s % 1000) / 1000.0 + progress * 0.75, 1.0);
				int sx = (int)(t * surface->w);
				int sy = (int)(t * surface->h * 0.50);
				double life = 1.0 - t;
				int head = (int)(150 * burst * life);
				if(head < 3) continue;
				int tail_len = scale / 14;
				// three nested widths give a taper without visible steps
				thickLineRGBA(surface, sx - tail_len, sy - tail_len * 3 / 4,
					sx, sy, 4, 206, 218, 242, (Uint8)(burst * life * 12));
				thickLineRGBA(surface, sx - tail_len / 2, sy - tail_len * 3 / 8,
					sx, sy, 3, 216, 228, 248, (Uint8)(burst * life * 26));
				thickLineRGBA(surface, sx - tail_len / 4, sy - tail_len * 3 / 16,
					sx, sy, 1, 236, 244, 255, (Uint8)(burst * life * 44));
				soft_glow(surface, sx, sy, scale / 80, 228, 236, 252, head);
			}
		} else if(kind == 46) {
			// False dawn: a wide, low wash of light lifting off the horizon,
			// brightening the sky before any source is visible.
			int apex_x = surface->w * (24 + (seed >> 12) % 52) / 100;
			double rise = sin(progress * 3.14159265359);
			int ridge_y = surface->h * 76 / 100;
			soft_band(surface, surface->h * 58 / 100, ridge_y + scale / 20,
				196, 184, 156, (int)(80 * rise));
			soft_glow(surface, apex_x, ridge_y, scale / 5,
				206, 194, 164, (int)(46 * rise));
			soft_glow(surface, apex_x, ridge_y - scale / 40, scale / 12,
				220, 208, 178, (int)(30 * rise));
		} else if(kind == 47) {
			// Light pillar: a column of ice-crystal glow above a bright point.
			int pillar_x = surface->w * (20 + (seed >> 12) % 60) / 100;
			int ground_y = surface->h * 76 / 100;
			int height = scale / 3;
			soft_column(surface, pillar_x, ground_y, height, scale / 60,
				226, 220, 202, 90);
			soft_glow(surface, pillar_x, ground_y, scale / 22,
				240, 232, 204, 60);
		} else if(kind == 48) {
			// A moon with its halo: the ice ring is the subject, not the moon.
			int halo_x = surface->w * (26 + (seed >> 12) % 48) / 100;
			int halo_y = surface->h * (16 + (seed >> 8) % 16) / 100;
			int halo_r = scale / 8;
			for(int ring = 0; ring < 4; ring++) {
				circleRGBA(surface, halo_x, halo_y, halo_r + ring * 2,
					202, 210, 226, (Uint8)(alpha * (0.22 - ring * 0.045)));
			}
			filledCircleRGBA(surface, halo_x, halo_y, scale / 46,
				226, 230, 238, (Uint8)(alpha * 0.52));
		} else if(kind == 49) {
			// A sea of cloud filling the valley: soft, layered, catching light.
			int sea_y = surface->h * 64 / 100;
			int drift = (int)(sin(ticks / 7000.0) * scale * 0.006);
			soft_band(surface, sea_y + drift, surface->h * 82 / 100 + drift,
				176, 186, 204, 62);
			soft_band(surface, sea_y + scale / 20 + drift,
				surface->h * 92 / 100 + drift, 190, 198, 214, 44);
		} else if(kind == 50) {
			// A snow squall crossing the valley. Built from soft glows along a
			// feathered leading edge plus a broad veil. Stepping the alpha one
			// column at a time produced visible vertical striping, and hard
			// top/bottom edges made it read as a grey rectangle.
			double reach = sin(progress * 3.14159265359);
			int edge_x = direction > 0
				? (int)(reach * surface->w * 1.15) - surface->w / 6
				: surface->w + surface->w / 6
					- (int)(reach * surface->w * 1.15);
			int top_y = surface->h * 8 / 100;
			int bottom_y = surface->h * 88 / 100;
			int span_y = bottom_y - top_y;
			if(span_y < 1) span_y = 1;
			// The body of the squall, trailing behind the leading edge.
			int body_x = edge_x - direction * surface->w / 4;
			soft_column(surface, body_x, bottom_y, span_y,
				surface->w / 5, 206, 212, 224, (int)(alpha * 0.10));
			// The leading edge: overlapping glows, so the boundary dissolves
			// into the air instead of ending in a wall.
			for(int puff = 0; puff < 14; puff++) {
				double t = (double)puff / 13;
				int py = top_y + (int)(t * span_y);
				int jitter = (int)(sin(t * 7.0 + ticks / 2600.0)
					* scale * 0.02);
				soft_glow(surface, edge_x + jitter, py,
					(int)(scale * (0.10 + 0.05 * sin(t * 5.0))),
					210, 216, 228, (int)(alpha * 0.16));
			}
			// Flakes inside the squall, falling and drifting.
			for(int flake = 0; flake < 90; flake++) {
				unsigned int s = (unsigned int)flake * 1664525u + seed;
				int fx = edge_x - direction
					* (int)((s % 1000) / 1000.0 * surface->w / 3);
				int fy = top_y + (int)((s >> 10) % (Uint32)span_y);
				fy = top_y + ((fy - top_y + (int)(ticks / 14.0)) % span_y);
				pixelRGBA(surface, fx, fy, 226, 232, 242,
					(Uint8)(alpha * 0.30));
			}
		} else if(kind == 51) {
			// Heat lightning: the horizon pulses, nothing else moves. A dim
			// warm haze sits on the ridge permanently so the scene reads even
			// between flashes.
			int glow_x = surface->w * (20 + (seed >> 8) % 60) / 100;
			int span = surface->w * (30 + (seed >> 12) % 40) / 100;
			int ridge_y = surface->h * 76 / 100;
			for(int layer = 5; layer >= 1; layer--) {
				filledCircleRGBA(surface, glow_x, ridge_y,
					span / (4 + layer), 92, 88, 96, (Uint8)(alpha * 0.10));
			}
			Uint32 pulse = phase % 1700;
			double flash = pulse < 240 ? 1.0 - pulse / 240.0 : 0.0;
			if(flash > 0.0) {
				for(int layer = 4; layer >= 1; layer--) {
					filledCircleRGBA(surface, glow_x, ridge_y,
						span / (4 + layer), 126, 138, 176,
						(Uint8)(alpha * 0.42 * flash));
				}
			}
		} else if(kind == 52) {
			// Diamond ring: totality ending, one point of photosphere left.
			int sun_x = surface->w * (28 + (seed >> 12) % 44) / 100;
			int sun_y = surface->h * (16 + (seed >> 8) % 16) / 100;
			int sun_r = scale / 26;
			filledCircleRGBA(surface, sun_x, sun_y, sun_r + scale / 30,
				226, 220, 198, (Uint8)(alpha * 0.05));
			filledCircleRGBA(surface, sun_x, sun_y, sun_r, 14, 16, 26,
				(Uint8)(alpha * 0.60));
			double edge = smooth_step(0.15, 0.6, progress);
			int diamond_x = sun_x
				+ (int)(sun_r * 1.05 * cos(progress * 6.28318530718));
			int diamond_y = sun_y
				+ (int)(sun_r * 1.05 * sin(progress * 6.28318530718));
			filledCircleRGBA(surface, diamond_x, diamond_y, scale / 150 + 1,
				255, 250, 232, (Uint8)(alpha * 0.80 * edge));
			for(int ray = 0; ray < 6; ray++) {
				double a = ray * 1.0471975512 + progress;
				thickLineRGBA(surface, diamond_x, diamond_y,
					diamond_x + (int)(cos(a) * scale * 0.05),
					diamond_y + (int)(sin(a) * scale * 0.05), 1,
					255, 246, 220, (Uint8)(alpha * 0.12 * edge));
			}
		} else if(kind == 53) {
			// A planet crossing a distant sun: a small dark disc, bright rim.
			int sun_x = surface->w * (30 + (seed >> 12) % 40) / 100;
			int sun_y = surface->h * (14 + (seed >> 8) % 14) / 100;
			int sun_r = scale / 30;
			filledCircleRGBA(surface, sun_x, sun_y, sun_r + scale / 22,
				238, 228, 196, (Uint8)(alpha * 0.06));
			filledCircleRGBA(surface, sun_x, sun_y, sun_r, 246, 236, 206,
				(Uint8)(alpha * 0.28));
			double transit = smooth_step(0.2, 0.8, progress);
			int planet_x = sun_x - sun_r + (int)(transit * sun_r * 2);
			int planet_y = sun_y + sun_r / 5;
			filledCircleRGBA(surface, planet_x, planet_y, sun_r / 5,
				26, 28, 38, (Uint8)(alpha * 0.70));
		} else if(kind == 54) {
			// A comet whose nucleus is coming apart, fragments trailing wide.
			int comet_x = surface->w * (26 + (seed >> 12) % 48) / 100;
			int comet_y = surface->h * (16 + (seed >> 8) % 20) / 100;
			for(int frag = 0; frag < 7; frag++) {
				double t = (double)frag / 6;
				int fx = comet_x - direction * (int)(t * scale * 0.10);
				int fy = comet_y + (int)(t * scale * 0.03);
				Uint8 fa = (Uint8)(alpha * (0.42 - t * 0.05));
				filledCircleRGBA(surface, fx, fy, 1 + frag % 2,
					226, 232, 244, fa);
				if(frag > 0) {
					thickLineRGBA(surface, fx, fy,
						fx + direction * scale / 40, fy - scale / 90, 1,
						190, 204, 228, (Uint8)(fa * 0.55));
				}
			}
		} else if(kind == 55) {
			// A monolith on the plain: the only straight line for miles. The
			// face catches a little skylight so the slab reads as a solid, not
			// as an edge highlight floating in the dark.
			int mono_x = surface->w * (22 + (seed >> 12) % 56) / 100;
			double nx = (double)mono_x / surface->w;
			double ridge = 0.88
				+ (smooth_noise(nx * 8.0 - 3.0, 3.0, 1123) - 0.5) * 0.09;
			int base_y = (int)(surface->h * ridge);
			int height = scale / 6;
			int half = height / 6;
			if(half < 3) half = 3;
			// body: a very dark blue-grey, just above the sky value
			boxRGBA(surface, mono_x - half, base_y - height,
				mono_x + half, base_y, 26, 31, 42, 235);
			// lit vertical edge and top
			boxRGBA(surface, mono_x + half - 1, base_y - height,
				mono_x + half, base_y, 96, 112, 138, 235);
			boxRGBA(surface, mono_x - half, base_y - height,
				mono_x + half, base_y - height + 1, 74, 88, 112, 235);
			soft_glow(surface, mono_x + half, base_y - height / 2,
				scale / 26, 116, 132, 160, 22);
		} else if(kind == 56) {
			// Distant turbines: large enough to read as silhouettes against the
			// last light, slow enough that the motion is barely perceptible.
			for(int turbine = 0; turbine < 3; turbine++) {
				int tx = surface->w * (20 + turbine * 30) / 100;
				double nx = (double)tx / surface->w;
				double ridge = 0.88
					+ (smooth_noise(nx * 8.0 - 3.0, 3.0, 1123) - 0.5) * 0.09;
				int base_y = (int)(surface->h * ridge);
				int th = scale / 9;
				thickLineRGBA(surface, tx, base_y, tx, base_y - th, 3,
					8, 10, 15, 245);
				double spin = ticks / (5200.0 + turbine * 460) + turbine;
				for(int blade = 0; blade < 3; blade++) {
					double a = spin + blade * 2.09439510239;
					thickLineRGBA(surface, tx, base_y - th,
						tx + (int)(cos(a) * th * 0.62),
						base_y - th + (int)(sin(a) * th * 0.62),
						3, 8, 10, 15, 230);
				}
			}
		} else if(kind == 57) {
			// A bridge reduced to its lights, reflected in still water.
			int water_y = surface->h * 74 / 100;
			for(int light = 0; light < 15; light++) {
				int lx = surface->w * (14 + light * 72 / 15) / 100;
				int ly = water_y - scale / 18
					- (int)(sin(light * 3.14159265359 / 14) * scale / 34);
				soft_glow(surface, lx, ly, scale / 70, 236, 206, 152, 60);
				for(int r = 1; r <= 5; r++) {
					if((light * 7 + r * 3) % 4 == 0) continue;
					pixelRGBA(surface, lx, ly + r * 3, 220, 192, 144,
						(Uint8)(alpha * (0.34 - r * 0.05)));
				}
			}
			soft_band(surface, water_y, surface->h * 88 / 100,
				36, 44, 62, 40);
		} else if(kind == 58) {
			// A substorm: the aurora brightens sharply, then falls back.
			double surge = smooth_step(0.3, 0.5, progress)
				* (1.0 - smooth_step(0.6, 0.85, progress));
			for(int band = 0; band < 4; band++) {
				int previous_x = 0;
				int previous_y = surface->h * (14 + band * 8) / 100;
				for(int segment = 1; segment <= 20; segment++) {
					int px = surface->w * segment / 20;
					double nx = (double)px / surface->w;
					int py = surface->h * (14 + band * 8) / 100
						+ (int)(sin(nx * 5.0 + ticks / (1400.0 + band * 220) + band)
							* scale * (0.02 + surge * 0.03));
					thickLineRGBA(surface, previous_x, previous_y, px, py, 2,
						(Uint8)(96 + band * 16), (Uint8)(186 + band * 12),
						(Uint8)(150 + band * 18),
						(Uint8)(alpha * (0.22 + surge * 0.30)));
					previous_x = px;
					previous_y = py;
				}
			}
		} else if(kind == 59) {
			// Moon dogs: two faint bright spots flanking a moon.
			int moon_x = surface->w * (30 + (seed >> 12) % 40) / 100;
			int moon_y = surface->h * (18 + (seed >> 8) % 14) / 100;
			int radius = scale / 9;
			filledCircleRGBA(surface, moon_x, moon_y, scale / 40,
				226, 230, 238, (Uint8)(alpha * 0.46));
			for(int side = -1; side <= 1; side += 2) {
				int dog_x = moon_x + side * radius;
				filledCircleRGBA(surface, dog_x, moon_y, scale / 90,
					206, 214, 230, (Uint8)(alpha * 0.10));
				filledCircleRGBA(surface, dog_x, moon_y, scale / 200 + 1,
					226, 232, 242, (Uint8)(alpha * 0.20));
			}
		} else if(kind == 60) {
			// A dust devil on the plain, seen from far off: a soft tapering
			// column of lifted dust, leaning slightly.
			int devil_x = surface->w * (24 + (seed >> 12) % 52) / 100;
			double nx = (double)devil_x / surface->w;
			double ridge = 0.88
				+ (smooth_noise(nx * 8.0 - 3.0, 3.0, 1123) - 0.5) * 0.09;
			int base_y = (int)(surface->h * ridge);
			int height = scale / 5;
			double strength = sin(progress * 3.14159265359);
			for(int segment = 0; segment < 12; segment++) {
				double t = (double)segment / 11;
				int sway = (int)(sin(t * 3.0 + ticks / 1200.0)
					* scale * 0.014 * t);
				int gy = base_y - (int)(t * height);
				soft_glow(surface, devil_x + sway, gy,
					(int)(scale / 26 * (1.0 - t * 0.5)),
					180, 168, 148, (int)(46 * strength * (1.0 - t * 0.5)));
			}
		} else if(kind == 61) {
			// Slow parallax: the sky turning, near stars sliding faster.
			for(int star = 0; star < 130; star++) {
				unsigned int s = (unsigned int)star * 1664525u + seed;
				int depth = 1 + (int)((s >> 20) % 3);
				int sx = (int)((s % 1000) / 1000.0 * surface->w
					+ ticks / (4000.0 * depth));
				sx = ((sx % surface->w) + surface->w) % surface->w;
				int sy = surface->h * (4 + (s >> 8) % 46) / 100;
				pixelRGBA(surface, sx, sy, 226, 234, 250,
					(Uint8)(alpha * (0.45 + depth * 0.30)));
				if(star % 11 == 0) {
					soft_glow(surface, sx, sy, scale / 110, 214, 228, 248, 60);
				}
			}
		} else {
			// A single craft high up: a bright point with a soft halo and a
			// short wake, the only moving thing in the frame.
			int craft_x = direction > 0
				? -scale / 20
					+ (int)(progress * (surface->w + scale / 10.0))
				: surface->w + scale / 20
					- (int)(progress * (surface->w + scale / 10.0));
			int craft_y = surface->h * (12 + (seed >> 8) % 22) / 100
				+ (int)(sin(progress * 3.14159265359) * scale * 0.02);
			for(int step = 1; step <= 14; step++) {
				int wx = craft_x - direction * step * scale / 45;
				int wy = craft_y + step * step / 4;
				thickLineRGBA(surface, wx, wy, wx - direction, wy, 3,
					210, 220, 240,
					(Uint8)(alpha * 0.42 * (1.0 - step / 15.0)));
			}
			soft_glow(surface, craft_x, craft_y, scale / 40, 222, 232, 250, 160);
			filledCircleRGBA(surface, craft_x, craft_y, 2, 250, 252, 255, 245);
		}
		return;
	}

	// The common events are the ones seen most often, so they carry the tone:
	// a point of light, a few birds, a meteor, a wisp of high cloud. Nothing
	// here has a hull, a cockpit or a face.
	switch(kind % 4) {
		case 0: {
			// A single point of light crossing the sky, with a soft halo and a
			// short fading wake. No shape is ever resolved.
			soft_glow(surface, x, y, size * 3, 214, 226, 246,
				(int)(alpha * 0.30));
			soft_glow(surface, x, y, size, 236, 242, 252,
				(int)(alpha * 0.55));
			for(int step = 1; step <= 8; step++) {
				int wx = x - direction * step * size;
				thickLineRGBA(surface, wx, y, wx - direction, y, 1,
					206, 218, 240,
					(Uint8)(alpha * 0.30 * (1.0 - step / 9.0)));
			}
			filledCircleRGBA(surface, x, y, 1, 250, 252, 255, alpha);
			break;
		}
		case 1: {
			// Three gulls crossing in loose formation, each beating at its own
			// rate so the group ripples rather than pulsing together.
			for(int bird = 0; bird < 3; bird++) {
				int bird_x = x - direction * bird * size * 3;
				int bird_y = y + (bird - 1) * size * 2
					+ (int)(sin(ticks / (900.0 + bird * 170) + bird * 2.1)
						* size * 0.8);
				double beat = sin(ticks / (150.0 + bird * 13) + bird * 2.3);
				int flap = (int)((0.5 + 0.5 * beat) * (size + 1));
				draw_bird(surface, bird_x, bird_y, size, flap);
			}
			break;
		}
		case 2: {
			// A meteor: one continuous tapered streak that burns out, rather
			// than a stack of segments that would read as dashes.
			int length = size * 6;
			double burn = sin(progress * 3.14159265359);
			int head_x = x + direction * (int)(progress * length);
			int head_y = y + (int)(progress * length * 9 / 16);
			thickLineRGBA(surface, head_x - direction * length,
				head_y - length * 9 / 16, head_x, head_y, 3,
				198, 212, 238, (Uint8)(alpha * 0.16 * burn));
			thickLineRGBA(surface, head_x - direction * length / 2,
				head_y - length * 9 / 32, head_x, head_y, 2,
				216, 228, 248, (Uint8)(alpha * 0.34 * burn));
			thickLineRGBA(surface, head_x - direction * length / 4,
				head_y - length * 9 / 64, head_x, head_y, 1,
				238, 244, 255, (Uint8)(alpha * 0.60 * burn));
			soft_glow(surface, head_x, head_y, size * 2, 228, 236, 252,
				(int)(alpha * 0.40 * burn));
			break;
		}
		default: {
			// A wisp of high cloud drifting across, lit from one side. Soft
			// edges only, so it never reads as a shape.
			for(int puff = 0; puff < 5; puff++) {
				int px = x - direction * puff * size * 3;
				int py = y + (int)(sin(puff * 1.7 + ticks / 5200.0) * size);
				soft_glow(surface, px, py, size * (4 + puff % 2),
					186, 196, 216, (int)(alpha * 0.26));
				soft_glow(surface, px + size, py - size / 2,
					size * (2 + puff % 2), 202, 210, 228,
					(int)(alpha * 0.18));
			}
			break;
		}
	}
}

static void render_foreground_event(SDL_Surface *surface, Uint32 ticks, int scale) {
	const Uint32 cycle = 180000;
	const Uint32 duration = 6500;
	Uint32 slot = ticks / cycle;
	Uint32 phase = ticks % cycle;
	Uint32 seed = slot * 2654435761u + 0x9e3779b9u;
	if(seed % 2 != 0 || phase >= duration) return;
	seed ^= seed >> 15;
	seed *= 2246822519u;
	seed ^= seed >> 13;
	int kind = seed % 4;
	double progress = (double)phase / duration;
	double fade = smooth_step(0.0, 650.0, phase)
		* (1.0 - smooth_step(duration - 900.0, duration, phase));
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
	const Uint32 cycle = 300000;
	const Uint32 duration = 12000;
	Uint32 slot = ticks / cycle;
	Uint32 phase = ticks % cycle;
	Uint32 seed = slot * 2654435761u + 0x9e3779b9u;
	if(seed % 4 != 0) return;
	Uint32 start = (seed >> 16) % (cycle - duration);
	if(phase < start || phase >= start + duration) return;

	Uint32 event_phase = phase - start;
	double fade = smooth_step(0.0, 1800.0, event_phase)
		* (1.0 - smooth_step(duration - 1800.0, duration, event_phase));
	Uint8 color = (seed & 0x80) ? 255 : 0;
	boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1,
		color, color, color, (Uint8)(255 * fade));
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

				// Ridge lines need enough horizontal frequency to undulate. At
				// 3-4 cells across the frame they collapse into a straight
				// horizontal seam, which is what made the horizon read as a
				// banded slab.
				double far_edge = 0.68
					+ (smooth_noise(nx * 11.0, 1.0, 907) - 0.5) * 0.16
					+ (smooth_noise(nx * 29.0, 4.0, 941) - 0.5) * 0.05;
				double middle_edge = 0.79
					+ (smooth_noise(nx * 14.0 + 8.0, 2.0, 1013) - 0.5) * 0.13
					+ (smooth_noise(nx * 37.0, 5.0, 1051) - 0.5) * 0.04;
				double near_edge = 0.88
					+ (smooth_noise(nx * 18.0 - 3.0, 3.0, 1123) - 0.5) * 0.11
					+ (smooth_noise(nx * 47.0, 6.0, 1153) - 0.5) * 0.035;
				// Two octaves of ground texture. A single low-frequency field
				// stretched over the lower third reads as a flat slab, so the
				// fine octave carries the visible grain.
				double ground_coarse = smooth_noise(nx * 22.0, ny * 26.0, 1237);
				double ground_fine = smooth_noise(nx * 90.0, ny * 110.0, 1291);
				double ground_noise = ground_coarse * 0.55 + ground_fine * 0.45;
				// Depth: the ground darkens toward the bottom of the frame, so
				// the foreground recedes instead of ending in a flat wall.
				double depth = smooth_step(0.62, 1.0, ny);
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
				// Apply the depth falloff only where ground is present, so the
				// sky is untouched.
				double ground_present = far_cover;
				red = blend_channel(red, (Uint8)(red * 0.45), depth * ground_present);
				green = blend_channel(green, (Uint8)(green * 0.45), depth * ground_present);
				blue = blend_channel(blue, (Uint8)(blue * 0.50), depth * ground_present);

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
	render_fullscreen_event(surface, ticks, scale);
	render_weather_effects(surface, ticks, &weather, scale);
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
			int bird_span = scale / 95;
			if(bird_span < 3) bird_span = 3;
			int formation_width = 27 * bird_span;
			int exit_margin = bird_span * 2;
			int route_length = surface->w + formation_width + exit_margin * 2;
			int travel = (int)((Uint64)ticks * route_length / 90000);
			int route_start = route_length * bird_group_x[group] / 100;
			int group_x = (route_start + travel) % route_length - exit_margin;
			int group_y = surface->h * bird_group_y[group] / 100
				+ (int)(sin(ticks / 1700.0 + group * 2.1) * scale * 0.008);
			// A loose, uneven V. Offsets are hand-tuned rather than generated so
			// the formation never looks mechanically spaced.
			static const int formation_x[] = { 0, -3, -6, -9, -12, -15, -18, -21 };
			static const int formation_y[] = { 0, 3, -2, 5, -5, 1, 6, -3 };
			for(int bird = 0; bird < 8; bird++) {
				int bird_x = group_x + formation_x[bird] * bird_span;
				int bird_y = group_y + formation_y[bird] * bird_span
					+ (int)(sin(ticks / (900.0 + bird * 130) + group * 1.8 + bird * 2.3)
						* bird_span * 0.5);
				// Each bird beats at its own rate and phase, so the flock
				// ripples instead of pulsing in unison.
				double beat = sin(ticks / (150.0 + bird * 11) + group * 1.8 + bird * 2.1);
				int flap = (int)((0.5 + 0.5 * beat) * (bird_span + 1));
				int bird_size = bird_span - (bird % 3 == 0 ? 1 : 0);
				draw_bird(surface, bird_x, bird_y, bird_size, flap);
			}
		}

	render_rare_sky_event(surface, ticks, scale);
	render_long_scene(surface, ticks, scale);

	for(int i = 0; i < 10; i++) {
		unsigned int mote = (unsigned int)i * 2246822519u + 3266489917u;
		int base_x = (mote % surface->w + ticks / (45 + i % 4 * 13)) % surface->w;
		double nx = (double)base_x / surface->w;
		double ridge = 0.68 + (smooth_noise(nx * 3.6, 1.0, 907) - 0.5) * 0.13;
		int y = surface->h * (ridge + 0.035) + (int)(sin(ticks / (520.0 + i * 47) + i) * scale * 0.018);
		filledCircleRGBA(surface, base_x, y, 1, 200, 206, 164,
			(Uint8)(35 + 30 * (0.5 + 0.5 * sin(ticks / (230.0 + i * 31) + i))));
	}

	// Autumn: leaf litter drifting on the wind. Drawn as small soft motes with
	// a faint warm tint rather than bright dots, so they read as debris in the
	// air instead of embers.
	double autumn_strength = weather.season[2];
	int autumn = (int)(autumn_strength * 34);
	for(int i = 0; i < autumn; i++) {
		unsigned int seed = (unsigned int)i * 2246822519u + 3266489917u;
		int drift = (int)(ticks / (26 + i % 5 * 7));
		int fall = (int)(ticks * (3 + i % 4) / 46);
		int x = (seed + drift) % surface->w;
		int y = (seed / 11 + fall) % surface->h;
		int sway = (int)(sin(ticks / (300.0 + i % 7 * 40) + i * 1.7) * scale * 0.016);
		// Faint, and dimmer the further it is from the viewer.
		Uint8 alpha = (Uint8)(26 + 44 * autumn_strength);
		Uint8 lr = (Uint8)(132 + (i * 37) % 46);
		Uint8 lg = (Uint8)(74 + (i * 23) % 34);
		Uint8 lb = (Uint8)(44 + (i * 17) % 24);
		int radius = 1 + i % 2;
		// A soft mote: a dim core with a single faint halo pixel, no hard edge.
		filledCircleRGBA(surface, x + sway, y, radius, lr, lg, lb, alpha);
		pixelRGBA(surface, x + sway + radius, y, lr, lg, lb,
			(Uint8)(alpha / 3));
		pixelRGBA(surface, x + sway, y + radius, lr, lg, lb,
			(Uint8)(alpha / 3));
	}

	unsigned int rain_seed = 0x7f4a7c15u;
	double storm_strength = smooth_step(0.25, 0.75, weather.lightning);
	double rain_share = 1.0 - weather.snow * 0.45;
	int rain_count = (int)(surface->w * surface->h / 1650.0 * weather.rain
		* (1.0 + storm_strength * 2.0) * rain_share);
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

	double snow_share = 1.0 - weather.rain * 0.45;
	int snow_count = (int)(surface->w * surface->h / 3400.0 * weather.snow * snow_share);
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
	// The cached glyphs are rendered at full FONT_COLOR. The flip animation
	// passes a dimmed color for the scaling half, so render those on the fly.
	bool use_cache = color.r == FONT_COLOR.r && color.g == FONT_COLOR.g
		&& color.b == FONT_COLOR.b;

	if(digits[1]) {
		int gap = spc + spc / 2;
		SDL_Surface *first_glyph = use_cache ? digit_glyphs[digits[0] - '0']
			: TTF_RenderGlyph_Blended(font_time, digits[0], color);
		SDL_Surface *second_glyph = use_cache ? digit_glyphs[digits[1] - '0']
			: TTF_RenderGlyph_Blended(font_time, digits[1], color);
		if(!first_glyph || !second_glyph) {
			fprintf(stderr, "Unable to render clock digits: %s\n", TTF_GetError());
			if(!use_cache) {
				if(first_glyph) SDL_FreeSurface(first_glyph);
				if(second_glyph) SDL_FreeSurface(second_glyph);
			}
			return;
		}
		int total_width = first_glyph->w + gap + second_glyph->w;
		coords.x = rect->x + (rect->w - total_width) / 2;
		coords.y = rect->y + (rect->h - first_glyph->h) / 2;
		SDL_BlitSurface(first_glyph, 0, surface, &coords);
		coords.x += first_glyph->w + gap;
		coords.y = rect->y + (rect->h - second_glyph->h) / 2;
		SDL_BlitSurface(second_glyph, 0, surface, &coords);
		if(!use_cache) {
			SDL_FreeSurface(first_glyph);
			SDL_FreeSurface(second_glyph);
		}
	} else {
		// single digit
		glyph = use_cache ? digit_glyphs[digits[0] - '0']
			: TTF_RenderGlyph_Blended(font_time, digits[0], color);
		if(!glyph) {
			fprintf(stderr, "Unable to render clock digits: %s\n", TTF_GetError());
			return;
		}
		coords.x = rect->x + (rect->w - glyph->w) / 2;
		coords.y = rect->y + (rect->h - glyph->h) / 2;
		SDL_BlitSurface(glyph, 0, surface, &coords);
		if(!use_cache) SDL_FreeSurface(glyph);
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
	if(moved || ambient_frame) {
		Uint32 backdrop_start = profile_rendering ? SDL_GetTicks() : 0;
		render_backdrop(screen, SDL_GetTicks());
		if(profile_rendering) backdrop_elapsed = SDL_GetTicks() - backdrop_start;
	}
	bool time_changed = _time->tm_hour != past_h || _time->tm_min != past_m;
	bool redraw = ambient_frame || (moved && !time_changed);
	int render_steps = redraw ? 20 : maxsteps;
	int render_step = redraw ? 19 : step;

	// draw hours
	if(_time->tm_hour != past_h || redraw || moved) {
		int h = twentyfourh ? _time->tm_hour : (_time->tm_hour + 11) % 12 + 1;
		// past_h is stored in 24-hour form; convert it the same way as h
		// so the flip animation shows the previous 12-hour value.
		int prev_h = twentyfourh ? past_h : (past_h + 11) % 12 + 1;
		int hour_steps = render_steps;
		int hour_step = render_step;
		if(moved && _time->tm_hour == past_h) {
			hour_steps = 20;
			hour_step = 19;
		}
		if(leadingzero) {
			snprintf(buffer, 3, "%02d", h);
			snprintf(buffer2, 3, "%02d", redraw ? h : prev_h);
		} else {
			snprintf(buffer, 3, "%d", h);
			snprintf(buffer2, 3, "%d", redraw ? h : prev_h);
		}
		render_digits(screen, &hourBackground, buffer, buffer2, hour_steps, hour_step);
		// draw am/pm
		if(!twentyfourh) render_ampm(screen, &hourBackground, _time->tm_hour >= 12);
	}

	// draw minutes
	if(_time->tm_min != past_m || redraw) {
		snprintf(buffer, 3, "%02d", _time->tm_min);
		snprintf(buffer2, 3, "%02d", redraw ? _time->tm_min : past_m);
		render_digits(screen, &minBackground, buffer, buffer2, render_steps, render_step);
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
	if((redraw || moved || _time->tm_yday != past_yday)
		&& date_surface_cache && date_surface_yday == _time->tm_yday) {
		SDL_Rect date_coords;
		date_coords.x = (screen->w - date_surface_cache->w) / 2 + burnin_x;
		date_coords.y = date_y + burnin_y;
		SDL_BlitSurface(date_surface_cache, NULL, screen, &date_coords);
		past_yday = _time->tm_yday;
	}

	SceneWeather weather = get_scene_weather(SDL_GetTicks());
	SDL_Rect *clock_cards[] = { &hourBackground, &minBackground };
	for(size_t card_index = 0; card_index < sizeof(clock_cards) / sizeof(clock_cards[0]); card_index++) {
		SDL_Rect *card = clock_cards[card_index];
		SDL_SetClipRect(screen, card);
		int drops = (int)(card->w / 42.0 * weather.rain);
		for(int i = 0; i < drops; i++) {
			Uint32 seed = (Uint32)(i + card_index * 97) * 2246822519u + 3266489917u;
			int drop_x = card->x + seed % card->w;
			int length = 4 + (seed >> 24) % 7;
			int speed = 7 + (seed >> 12) % 7;
			int drop_y = card->y
				+ (int)((seed + SDL_GetTicks() * speed / 90) % card->h);
			Uint8 alpha = (Uint8)(24 + (seed >> 18) % 30);
			thickLineRGBA(screen, drop_x, drop_y, drop_x - 1, drop_y - length,
				1, 151, 174, 211, alpha);
		}
		int flakes = (int)(card->w / 50.0 * weather.snow);
		for(int i = 0; i < flakes; i++) {
			Uint32 seed = (Uint32)(i + card_index * 61) * 1664525u + 1013904223u;
			int flake_x = card->x + seed % card->w;
			int flake_y = card->y
				+ (int)((seed + SDL_GetTicks() * (2 + (seed >> 16) % 4) / 80) % card->h);
			pixelRGBA(screen, flake_x, flake_y, 208, 221, 236, 110);
		}
	}
	SDL_SetClipRect(screen, NULL);
	int scene_scale = screen->h < screen->w ? screen->h : screen->w;
	render_foreground_event(screen, SDL_GetTicks(), scene_scale);
	render_rare_screen_fade(screen, SDL_GetTicks());

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

int main(int argc, char** argv ) {
	char *wid_env;
	static char sdlwid[100];
	double display_scale_factor = 1;
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
			char *resolution = argv[i+1];
			char *val = strtok(resolution, "x");
			width = atoi(val);
			val = strtok(NULL, "x");
			height = atoi(val);
			i++;
		} else if(strcmp("-w", argv[i]) == 0) {
			width = atoi(argv[i+1]);
			i++;
		} else if(strcmp("-h", argv[i]) == 0) {
			height = atoi(argv[i+1]);
			i++;
		} else if(strcmp("-s", argv[i]) == 0) {
			display_scale_factor = atof(argv[i+1]);
			i++;
		} else if(strcmp("-window-id", argv[i]) == 0) {
			wid = strtol(argv[i+1], (char **) NULL, 0);
			i++;
		} else {
			printf("Invalid option -- %s\n", argv[i]);
			printf("Try --help for more information.\n");
			return 0;
		}
	}

	/* If no window argument, check environment */
	if(wid == 0) {
		if ((wid_env = getenv("XSCREENSAVER_WINDOW")) != NULL ) {
			wid = strtol(wid_env, (char **) NULL, 0); /* Base 0 autodetects hex/dec */
		}
	}

	/* Get win attrs if we've been given a window, otherwise we'll use our own */
	if(wid != 0) {
		if ((display = XOpenDisplay(NULL)) != NULL) { /* Use the default display */
			XGetWindowAttributes(display, (Window) wid, &windowAttributes);
			XCloseDisplay(display);
			snprintf(sdlwid, 100, "SDL_WINDOWID=0x%X", wid);
			putenv(sdlwid); /* Tell SDL to use this window */
			width = windowAttributes.width;
			height = windowAttributes.height;
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
	init_glyph_cache();

	// Draw the layered forest before placing the clock over it.
	render_backdrop(screen, SDL_GetTicks());

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
		if(date_surface_cache) SDL_FreeSurface(date_surface_cache);
		free_glyph_cache();
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
	if(date_surface_cache) SDL_FreeSurface(date_surface_cache);
	free_glyph_cache();

	TTF_CloseFont(font_time);
	TTF_CloseFont(font_mode);
	TTF_CloseFont(font_date);

	return 0;
}
