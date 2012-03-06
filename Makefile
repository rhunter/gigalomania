CC=g++
CCFLAGS=-O2 -Wall
CFILES=game.cpp gamestate.cpp gui.cpp image.cpp main.cpp panel.cpp player.cpp resources.cpp screen.cpp sector.cpp sound.cpp utils.cpp
HFILES=game.h gamestate.h gui.h image.h panel.h player.h resources.h screen.h sector.h sound.h utils.h common.h stdafx.h
OFILES=game.o gamestate.o gui.o image.o panel.o player.o resources.o screen.o sector.o sound.o utils.o main.o
APP=gigalomania
INC=`sdl-config --cflags`
LINKPATH=`sdl-config --libs` -L/usr/X11R6/lib/ -L/usr/lib

LIBS=-lSDL_image -lSDL_mixer

all: $(APP)

$(APP): $(OFILES) $(HFILES) $(CFILES)
	$(CC) $(OFILES) $(CCFLAGS) $(LINKPATH) $(LIBS) -o $(APP)

.cpp.o:
	$(CC) $(CCFLAGS) -O2 $(INC) -c $<

# REMEMBER to update debian/dirs if the system directories that we use are changed!!!
install: $(APP)
	mkdir -p $(DESTDIR)/opt/gigalomania # -p so we don't fail if folder already exists
	cp $(APP) $(DESTDIR)/opt/gigalomania
	cp readme.html $(DESTDIR)/opt/gigalomania
	cp -a gfx/ $(DESTDIR)/opt/gigalomania # -a need to copy permissions etc
	cp -a islands/ $(DESTDIR)/opt/gigalomania
	cp -a sound/ $(DESTDIR)/opt/gigalomania
	cp gigalomania.desktop $(DESTDIR)/usr/share/applications/
	cp gigalomania_fullscreen.desktop $(DESTDIR)/usr/share/applications/
	cp gigalomania64.png $(DESTDIR)/usr/share/pixmaps/
# REMEMBER to update debian/dirs if the system directories that we use are changed!!!

uninstall:
	rm -rf $(DESTDIR)/opt/gigalomania # -f so we don't fail if folder doesn't exist
	rm -f $(DESTDIR)/usr/share/applications/gigalomania.desktop
	rm -f $(DESTDIR)/usr/share/applications/gigalomania_fullscreen.desktop
	rm -f $(DESTDIR)/usr/share/pixmaps/gigalomania64.png

install_maemo: $(APP)
	mkdir -p $(DESTDIR)/opt/gigalomania # -p so we don't fail if folder already exists
	cp $(APP) $(DESTDIR)/opt/gigalomania
	cp -a gfx/ $(DESTDIR)/opt/gigalomania # -a need to copy permissions etc
	cp -a islands/ $(DESTDIR)/opt/gigalomania
	cp -a sound/ $(DESTDIR)/opt/gigalomania
	mkdir -p $(DESTDIR)/usr/share/applications/hildon/
	cp gigalomania_maemo.desktop $(DESTDIR)/usr/share/applications/hildon/
	mkdir -p $(DESTDIR)/usr/share/pixmaps
	cp gigalomania48.png $(DESTDIR)/usr/share/pixmaps/
	chmod a+x gigalomania_mobile.sh # workaround for permissions not set in zip file!
	mkdir -p $(DESTDIR)/usr/bin/
	cp gigalomania_mobile.sh $(DESTDIR)/usr/bin/gigalomania_mobile.sh

uninstall_maemo:
	rm -rf $(DESTDIR)/opt/gigalomania # -f so we don't fail if folder doesn't exist
	rm -f $(DESTDIR)/usr/share/applications/hildon/gigalomania_maemo.desktop
	rm -f $(DESTDIR)/usr/share/pixmaps/gigalomania48.png
	rm -f $(DESTDIR)/usr/bin/gigalomania_mobile.sh

install_meego: $(APP)
	mkdir -p $(DESTDIR)/opt/gigalomania # -p so we don't fail if folder already exists
	cp $(APP) $(DESTDIR)/opt/gigalomania
	cp -a gfx/ $(DESTDIR)/opt/gigalomania # -a need to copy permissions etc
	cp -a islands/ $(DESTDIR)/opt/gigalomania
	cp -a sound/ $(DESTDIR)/opt/gigalomania
	cp gigalomania_maemo.desktop $(DESTDIR)/usr/share/applications/
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/48x48/apps/
	cp gigalomania48.png $(DESTDIR)/usr/share/icons/hicolor/48x48/apps/
	chmod a+x gigalomania_mobile.sh # workaround for permissions not set in zip file!
	mkdir -p $(DESTDIR)/usr/bin/
	cp gigalomania_mobile.sh $(DESTDIR)/usr/bin/gigalomania_mobile.sh

uninstall_meego:
	rm -rf $(DESTDIR)/opt/gigalomania # -f so we don't fail if folder doesn't exist
	rm -f $(DESTDIR)/usr/share/applications/gigalomania_maemo.desktop
	rm -f $(DESTDIR)/usr/share/icons/hicolor/48x48/apps/gigalomania48.png
	rm -f $(DESTDIR)/usr/bin/gigalomania_mobile.sh

clean:
	rm -rf *.o
	rm -f $(APP)
