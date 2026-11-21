#ifndef GLUQLO_SHIPS_BATTLES_H
#define GLUQLO_SHIPS_BATTLES_H

#include "gluqlo.h"

#define SHIP_OPACITY 235

enum { SHIP_SMALL, SHIP_MID, SHIP_BIG };

extern SDL_Texture *ship_target_tex;

void draw_ship_role(RenderTarget *surface, int x, int y, int role, unsigned int variety, int length,
                    int direction, int team, Uint8 opacity, double tilt = 0.0);
void render_battle_flyby(RenderTarget *surface, Uint32 ticks, int scale);
void render_dogfight_effect(RenderTarget *surface, Uint32 ticks, int scale);
void render_background_battle(RenderTarget *surface, Uint32 ticks, int scale);
void render_fleet_battle(RenderTarget *surface, Uint32 ticks, int scale);

#endif
