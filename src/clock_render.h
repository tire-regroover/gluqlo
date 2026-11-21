#ifndef GLUQLO_CLOCK_RENDER_H
#define GLUQLO_CLOCK_RENDER_H

#include "gluqlo.h"

extern bool twentyfourh;
extern bool leadingzero;
extern bool animate;
extern bool profile_rendering;

extern int width;
extern int height;
extern SDL_Rect hourBackground;
extern SDL_Rect minBackground;
extern int past_h, past_m;

extern SDL_Texture *date_texture_cache;

void fill_rounded_box_b(SDL_Surface *dst, SDL_Rect *coords, int r, SDL_Color color, Uint8 alpha);
void destroy_clock_caches(void);
void layout_clock(double display_scale_factor);
void render_clock(int maxsteps, int step, bool ambient_frame);
void render_animation(void);

#endif
