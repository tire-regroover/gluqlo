// Draw single rare_sky kinds offscreen, mid-screen and at a fixed size, so they can be checked
// without waiting for the scheduler.
//   build/run: dev/tools/draw_sky.sh FIRST LAST   (writes /tmp/kindtest/out/skyharness-N.png)
#define main gluqlo_main
#include "../../gluqlo.c"
#undef main
#include <stdlib.h>

int main(int argc, char **argv) {
	int first = argc > 1 ? atoi(argv[1]) : RARE_SKY_CLASSIC_KINDS;
	int last = argc > 2 ? atoi(argv[2]) : first;
	const char *dir = argc > 3 ? argv[3] : "/tmp/kindtest/out";
	for(int kind = first; kind <= last; kind++) {
		SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 240, 32, 0xff0000, 0xff00, 0xff, 0);
		SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 14, 20, 36));
		SkyEvent event;
		event.surface = surface;
		event.ticks = 0;
		event.scale = 768;
		event.seed = 12345u + kind * 77u;
		event.phase = 2600;
		event.progress = 0.5;
		event.fade = 1.0;
		event.direction = 1;
		event.x = 360;
		event.y = 90;
		event.size = 20;
		event.alpha = 150;
		if(kind >= RARE_SKY_CLASSIC_KINDS) sky_event_kinds[kind - RARE_SKY_CLASSIC_KINDS](&event);
		char name[256];
		snprintf(name, sizeof name, "%s/skyharness-%d.bmp", dir, kind);
		SDL_SaveBMP(surface, name);
		SDL_FreeSurface(surface);
	}
	return 0;
}
