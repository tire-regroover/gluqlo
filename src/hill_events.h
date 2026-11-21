#ifndef GLUQLO_HILL_EVENTS_H
#define GLUQLO_HILL_EVENTS_H

#include "gluqlo.h"

typedef struct {
  RenderTarget *surface;
  Uint32 ticks;
  int scale;
  Uint32 seed;
  double t;
  double progress;
  double fade;
  int direction;  // 1: things go left to right, -1: right to left
} HillEvent;

extern void (*const hill_event_kinds[])(const HillEvent *);

void render_long_scene(RenderTarget *surface, Uint32 ticks, int scale);
void render_hill_event(RenderTarget *surface, Uint32 ticks, int scale);

#endif
