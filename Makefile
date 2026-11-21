CXX?=g++
FONTDIR=$(DESTDIR)/usr/share/gluqlo
# Source files use C++ features (bool, mixed declarations), so compile with -x c++.
CXXFLAGS?=-O2
CXXFLAGS+=-Wall $(shell pkg-config --cflags sdl3 sdl3-ttf fontconfig) -DFONT='"$(FONTDIR)/gluqlo.ttf"'
LDFLAGS=$(shell pkg-config --libs sdl3 sdl3-ttf fontconfig) -lm

SRCS=gluqlo.c src/gfx_primitives.c src/scene_core.c src/ships_battles.c \
	src/hill_events_01.c src/hill_events_02.c src/sky_events_01.c \
	src/sky_events_02.c src/backdrop.c src/clock_render.c
OBJS=$(SRCS:.c=.o)

all: gluqlo

gluqlo: $(OBJS)
	$(CXX) -o gluqlo $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CXX) -x c++ $(CXXFLAGS) -c $< -o $@

install:
	strip gluqlo
	install -o root -m 0755 -D gluqlo $(DESTDIR)/usr/libexec/xscreensaver/gluqlo
	install -o root -m 0644 -D gluqlo.ttf $(FONTDIR)/gluqlo.ttf
	install -o root -m 0644 -D gluqlo.png $(DESTDIR)/usr/share/pixmaps/gluqlo.png
	install -o root -m 0644 -D gluqlo.xml $(DESTDIR)/usr/share/xscreensaver/config/gluqlo.xml
	install -o root -m 0644 -D gluqlo.desktop $(DESTDIR)/usr/share/applications/screensavers/gluqlo.desktop

uninstall:
	rm -f $(DESTDIR)/usr/share/xscreensaver/config/gluqlo.xml $(DESTDIR)/usr/share/applications/screensavers/gluqlo.desktop \
		$(DESTDIR)/usr/libexec/xscreensaver/gluqlo $(FONTDIR)/gluqlo.ttf $(DESTDIR)/usr/share/pixmaps/gluqlo.png

clean:
	rm -f gluqlo $(OBJS)
