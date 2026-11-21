#ifndef GLUQLO_H
#define GLUQLO_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RARE_SKY_KINDS 297
#define RARE_SKY_CLASSIC_KINDS 23  // kinds below this use the four classic sky objects
#define HILL_EVENT_KINDS 297       // things that happen on the hills (see render_hill_event)
#define SCENE_PI 3.14159265359
#define SCENE_TAU 6.28318530718

enum { RIDGE_FAR, RIDGE_MIDDLE, RIDGE_NEAR };

// SDL3 RenderTarget abstraction: keeps existing draw functions clean and lets
// both the main GPU renderer and offscreen software harnesses share one path.
typedef struct {
  SDL_Renderer *renderer;
  int w;
  int h;
} RenderTarget;

// Global renderer & clock state shared across modules
extern SDL_Renderer *renderer;
extern RenderTarget *screen;
extern TTF_Font *font_time;
extern TTF_Font *font_mode;
extern TTF_Font *font_date;
extern const SDL_Color FONT_COLOR;
extern const SDL_Color DATE_COLOR;
extern const SDL_Color BACKGROUND_COLOR;
extern const SDL_Color CARD_BORDER_COLOR;

// Noise / math / terrain helpers (defined in src/scene_core.c)
double hash_noise(int x, int y, Uint32 seed);
double smooth_noise(double x, double y, Uint32 seed);
Uint8 blend_channel(Uint8 from, Uint8 to, double amount);
double smooth_step(double edge0, double edge1, double value);
double ridge_height(int layer, double nx);

// Shared utility helpers
unsigned int fx_hash(unsigned int a, unsigned int b);
Uint8 weapon_alpha(double alpha);
void weapon_glow(RenderTarget *surface, double x, double y, double radius, const Uint8 color[3],
                 double alpha);
double ground_y(RenderTarget *surface, int layer, double x);

#endif
