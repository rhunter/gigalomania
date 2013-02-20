#include "stdafx.h"

#ifndef USING_QT
// n.b., need SDL even though we don't apparently call SDL in this file, so that main() is redefined with SDL's version
#if defined(__linux) || defined(__MORPHOS__)
#include <SDL/SDL.h>
#else
#include <sdl.h>
#endif
#endif

#include "game.h"

#ifdef USING_QT

#include <QtGui/QApplication>

#include "qt_screen.h"

class MyApplication : public QApplication {

protected:
    bool event(QEvent *event);

public:
    MyApplication(int argc, char *argv[]) : QApplication(argc, argv)/*, is_active(true)*/ {
    }
};

bool MyApplication::event(QEvent *event) {
    if( event->type() == QEvent::ApplicationActivate ) {
        qDebug("application activated");
        if( application != NULL ) {
            application->activateTimers(true);
        }
    }
    else if( event->type() == QEvent::ApplicationDeactivate ) {
        qDebug("application deactivated");
        keypressP(); // automatically pause when application goes inactive
        if( application != NULL ) {
            application->activateTimers(false);
        }
    }
    return false;
}

#endif

int main(int argc, char *argv[])
{
#ifdef USING_QT
    qDebug("Hello!");
    MyApplication app(argc, argv);
#endif

    playGame(argc, argv);
    return 0;
}
