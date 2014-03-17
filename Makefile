CC=g++
CCFLAGS=-mmacosx-version-min=10.5 -arch x86_64 -O0 -g -Wall
CFILES=game.cpp gamestate.cpp gui.cpp image.cpp main.cpp panel.cpp player.cpp resources.cpp screen.cpp sector.cpp sound.cpp utils.cpp
HFILES=game.h gamestate.h gui.h image.h panel.h player.h resources.h screen.h sector.h sound.h utils.h common.h stdafx.h
OFILES=game.o gamestate.o gui.o image.o panel.o player.o resources.o screen.o sector.o sound.o utils.o main.o
APP=gigalomania
INC=-I ~/Library/Frameworks/SDL2.framework/Headers -I ~/Library/Frameworks/SDL2_image.framework/Headers -I ~/Library/Frameworks/SDL2_mixer.framework/Headers
LINKPATH=-F ~/Library/Frameworks -rpath @loader_path/../Frameworks

LIBS=-framework SDL2 -framework SDL2_image -framework SDL2_mixer

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

dist_macosx: Gigalomania.app/Contents/MacOS/gigalomania Gigalomania.app/Contents/Resources/gfx Gigalomania.app/Contents/MacOS/gigalomania.launcher.sh Gigalomania.app/Contents/Resources/islands Gigalomania.app/Contents/Resources/sound Gigalomania.app/Contents/Resources/gamemusic.ogg Gigalomania.app/Contents/Info.plist Gigalomania.app/Contents/Resources/icon1.icns Gigalomania.app/Contents/Frameworks

Gigalomania.app/Contents/MacOS/gigalomania: Gigalomania.app/Contents/MacOS $(OFILES) $(HFILES) $(CFILES)
	mkdir -p $<
	$(CC) $(OFILES) $(CCFLAGS) $(LINKPATH) $(LIBS) -o $@


Gigalomania.app/Contents/Info.plist: macosx/app_bundle_template/Contents/Info.plist
	mkdir -p Gigalomania.app/Contents
	cp -r $< $@
Gigalomania.app/Contents/Resources/icon1.icns: macosx/app_bundle_template/Contents/Resources/icon1.icns
	mkdir -p Gigalomania.app/Contents/Resources
	cp -r $< $@
Gigalomania.app/Contents/MacOS:
Gigalomania.app/Contents/Resources: Gigalomania.app/Contents/Resources/gfx Gigalomania.app/Contents/Resources/islands Gigalomania.app/Contents/Resources/sound Gigalomania.app/Contents/Resources/gamemusic.ogg

Gigalomania.app/Contents/MacOS/gigalomania.launcher.sh: macosx/app_bundle_template/Contents/MacOS/gigalomania.launcher.sh
	cp -a $< $@
# TODO: I'm sure there's a way to collapse all these into one rule
Gigalomania.app/Contents/Resources/gfx: gfx
	mkdir -p Gigalomania.app/Contents/Resources
	cp -r $< $@
Gigalomania.app/Contents/Resources/islands: islands
	mkdir -p Gigalomania.app/Contents/Resources
	cp -r $< $@
Gigalomania.app/Contents/Resources/sound: sound/
	mkdir -p Gigalomania.app/Contents/Resources
	cp -r $< $@
Gigalomania.app/Contents/Resources/gamemusic.ogg: gamemusic.ogg
	mkdir -p Gigalomania.app/Contents/Resources
	cp -r $< $@

Gigalomania.app/Contents/Frameworks:
	mkdir -p $@
	cp -r ~/Library/Frameworks/SDL2.framework ~/Library/Frameworks/SDL2_image.framework ~/Library/Frameworks/SDL2_mixer.framework $@

Gigalomania.app: macosx/bundle_template
	cp -r macosx/bundle_template $@

clean:
	rm -rf *.o
	rm -f $(APP)
