rd /S /Q c:\temp\gigalomania_android\

mkdir c:\temp\gigalomania_android\

set src="."
set dst="c:\temp\gigalomania_android"

REM Qt stuff:

copy %src%\gigalomania.pro %dst%
copy %src%\deployment.pri %dst%
copy %src%\gigalomania.qrc %dst%

REM General:

copy %src%\gpl.txt %dst%

copy %src%\game.cpp %dst%
copy %src%\gamestate.cpp %dst%
copy %src%\gui.cpp %dst%
copy %src%\main.cpp %dst%
copy %src%\panel.cpp %dst%
copy %src%\player.cpp %dst%
copy %src%\resources.cpp %dst%
copy %src%\sector.cpp %dst%
copy %src%\utils.cpp %dst%
copy %src%\stdafx.cpp %dst%

copy %src%\game.h %dst%
copy %src%\gamestate.h %dst%
copy %src%\gui.h %dst%
copy %src%\panel.h %dst%
copy %src%\player.h %dst%
copy %src%\resources.h %dst%
copy %src%\sector.h %dst%
copy %src%\utils.h %dst%
copy %src%\common.h %dst%
copy %src%\stdafx.h %dst%

REM Qt specific files:

copy %src%\mainwindow.cpp %dst%
copy %src%\qt_image.cpp %dst%
copy %src%\qt_screen.cpp %dst%
copy %src%\qt_sound.cpp %dst%

copy %src%\mainwindow.h %dst%
copy %src%\qt_image.h %dst%
copy %src%\qt_screen.h %dst%
copy %src%\qt_sound.h %dst%

REM Android specific files:

mkdir %dst%\android\
copy %src%\android\ %dst%\android\

REM also copy data folders, as Linux users won't have downloaded the Windows binary archive!

mkdir %dst%\islands\
copy %src%\islands\ %dst%\islands\

mkdir %dst%\gfx
copy %src%\gfx\ %dst%\gfx\
REM copy %src%\gfx_2x\ %dst%\gfx\

mkdir %dst%\sound
copy %src%\sound\ %dst%\sound\

REM exit
