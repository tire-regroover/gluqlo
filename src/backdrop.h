#ifndef GLUQLO_BACKDROP_H
#define GLUQLO_BACKDROP_H

#include "gluqlo.h"

extern SDL_Texture *backdrop_base;

void invalidate_backdrop(void);
void render_backdrop(RenderTarget *surface, Uint32 ticks);

#endif
