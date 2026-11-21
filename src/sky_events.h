#ifndef GLUQLO_SKY_EVENTS_H
#define GLUQLO_SKY_EVENTS_H

#include "gluqlo.h"

typedef struct {
  RenderTarget *surface;
  Uint32 ticks;
  int scale;  // shorter screen side
  Uint32 seed;
  Uint32 phase;     // ms since the event started
  double progress;  // 0..1 through the event
  double fade;      // 0..1 fade in/out
  int direction;    // 1: moves left to right, -1: right to left
  int x, y;         // current position of the object
  int size;
  Uint8 alpha;  // 150 * fade
} SkyEvent;

extern void (*const sky_event_kinds[])(const SkyEvent *);

void render_rare_sky_event(RenderTarget *surface, Uint32 ticks, int scale);
void render_foreground_event(RenderTarget *surface, Uint32 ticks, int scale);
void render_rare_screen_fade(RenderTarget *surface, Uint32 ticks);

#endif
