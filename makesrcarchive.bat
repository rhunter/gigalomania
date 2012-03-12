rd /S /Q c:\temp\gigalomaniasrc\

mkdir c:\temp\gigalomaniasrc\

set src="."
set docsrc="docs\"
set dst="c:\temp\gigalomaniasrc"

copy %src%\makearchive.bat %dst%
copy %src%\makesrcarchive.bat %dst%
copy %src%\makesymbianfolder.bat %dst%
copy %src%\makeandroidfolder.bat %dst%
copy %src%\gigalomania_source.txt %dst%
copy %docsrc%\comp_gigalomania.html %dst%\readme.html

REM Windows Visual C++ stuff:

copy %src%\gigalomania.vcxproj %dst%
copy %src%\gigalomania.vcxproj.filters %dst%
copy %src%\gigalomania.sln %dst%
copy %src%\resource.rc %dst%
copy %src%\gigalomania.ico %dst%

REM Linux stuff:

copy %src%\Makefile %dst%
copy %src%\gigalomania_fullscreen.sh %dst%
copy %src%\gigalomania.desktop %dst%
copy %src%\gigalomania_fullscreen.desktop %dst%
copy %src%\gigalomania_maemo.desktop %dst%
copy %src%\gigalomania_mobile.sh %dst%
copy %src%\gigalomania64.png %dst%
copy %src%\gigalomania48.png %dst%

REM Qt stuff:

copy %src%\gigalomania.pro %dst%
copy %src%\deployment.pri %dst%
copy %src%\gigalomania.qrc %dst%
copy %src%\gigalomania.svg %dst%

REM Not really needed, but just to have backup of the source for gigalomania.svg:

copy %src%\gigalomania_symbian.png %dst%

REM General:

copy %src%\gpl.txt %dst%

copy %src%\game.cpp %dst%
copy %src%\gamestate.cpp %dst%
copy %src%\gui.cpp %dst%
copy %src%\image.cpp %dst%
copy %src%\main.cpp %dst%
copy %src%\panel.cpp %dst%
copy %src%\player.cpp %dst%
copy %src%\resources.cpp %dst%
copy %src%\screen.cpp %dst%
copy %src%\sector.cpp %dst%
copy %src%\sound.cpp %dst%
copy %src%\utils.cpp %dst%
copy %src%\stdafx.cpp %dst%

copy %src%\game.h %dst%
copy %src%\gamestate.h %dst%
copy %src%\gui.h %dst%
copy %src%\image.h %dst%
copy %src%\panel.h %dst%
copy %src%\player.h %dst%
copy %src%\resources.h %dst%
copy %src%\screen.h %dst%
copy %src%\sector.h %dst%
copy %src%\sound.h %dst%
copy %src%\utils.h %dst%
copy %src%\common.h %dst%
copy %src%\stdafx.h %dst%

copy %src%\SDL_main.c %dst%

REM Qt specific files:

copy %src%\mainwindow.cpp %dst%
copy %src%\qt_image.cpp %dst%
copy %src%\qt_screen.cpp %dst%
copy %src%\qt_sound.cpp %dst%

copy %src%\mainwindow.h %dst%
copy %src%\qt_image.h %dst%
copy %src%\qt_screen.h %dst%
copy %src%\qt_sound.h %dst%

REM also copy data folders, as Linux users won't have downloaded the Windows binary archive!

mkdir %dst%\islands\
copy %src%\islands\ %dst%\islands\

mkdir %dst%\gfx
copy %src%\gfx\ %dst%\gfx\

REM used by Symbian deployment, though has to replace gfx/ manually
mkdir %dst%\gfx_2x
copy %src%\gfx_2x\ %dst%\gfx_2x\

mkdir %dst%\sound
copy %src%\sound\ %dst%\sound\

mkdir %dst%\debian
copy %src%\debian\ %dst%\debian\

mkdir %dst%\debian_maemo
copy %src%\debian_maemo\ %dst%\debian_maemo\

REM exit
