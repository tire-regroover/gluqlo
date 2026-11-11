CXX?=g++
FONTDIR=$(DESTDIR)/usr/share/gluqlo
# gluqlo.c is C++ (it uses bool and mixed declarations), so say so explicitly.
CXXFLAGS?=-O2
CXXFLAGS+=-Wall -x c++ `sdl-config --cflags` -DFONT='"$(FONTDIR)/gluqlo.ttf"'
LDFLAGS=`sdl-config --libs` -lX11 -lSDL_ttf -lSDL_gfx

all: gluqlo

gluqlo: gluqlo.c
	$(CXX) $(CXXFLAGS) -o gluqlo gluqlo.c $(LDFLAGS)

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
	rm -f gluqlo
