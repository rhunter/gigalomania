#include "stdafx.h"

#include "qt_screen.h"
#include "utils.h"
#include "game.h"
#include "gamestate.h"

#include <QtGui>

Screen::Screen() {

}

Screen::~Screen() {
}

bool Screen::canOpenFullscreen(int width, int height) {
    return true;
}

bool Screen::open(int screen_width, int screen_height, bool fullscreen) {
    mainWindow.setOrientation(MainWindow::ScreenOrientationLockLandscape);
    mainWindow.showExpanded();
    pixmap = QPixmap(screen_width, screen_height);
    return true;
}

void Screen::setTitle(const char *title) {
    this->mainWindow.setWindowTitle(title);
}

void Screen::clear() {
    //LOG("clear\n");
    //this->fillRect(0, 0, this->mainWindow.width(), this->mainWindow.height(), 0, 255, 0);
    this->fillRect(0, 0, this->pixmap.width(), this->pixmap.height(), 0, 0, 0);
}

void Screen::refresh() {
    this->mainWindow.repaint();
}

void Screen::fillRect(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b) {
    QPainter painter(&this->pixmap);
    QColor color(r, g, b);
    QBrush brush(color);
    painter.setBrush(brush);
    painter.fillRect(x, y, w, h, brush);
}

void Screen::fillRectWithAlpha(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha) {
    QPainter painter(&this->pixmap);
    painter.setOpacity((qreal)(alpha/255.0));
    QColor color(r, g, b);
    QBrush brush(color);
    painter.setBrush(brush);
    painter.fillRect(x, y, w, h, brush);
}

void Screen::getMouseCoords(int *m_x, int *m_y) {
    this->mainWindow.getMouseXY(m_x, m_y);
}

bool Screen::getMouseState(int *m_x, int *m_y, bool *m_left, bool *m_middle, bool *m_right) {
    /*Qt::MouseButtons m_b = qApp->mouseButtons();
    *m_left = ( m_b & Qt::LeftButton );
    *m_middle = ( m_b & Qt::MiddleButton );
    *m_right = ( m_b & Qt::RightButton );*/
    *m_middle = false;
    *m_right = false;
    this->mainWindow.getMouseButtons(m_left);
    this->mainWindow.getMouseXY(m_x, m_y);
    //qDebug("%d, %d: %d, %d, %d", *m_x, *m_y, *m_left, *m_middle, *m_right);
    return ( *m_left || *m_middle || *m_right );
    //return false;
}

int Application::getTicks() const {
    //return elapsed_time;
    return static_cast<int>(elapsed_timer.elapsed());
}

void Application::update() {
    // draw screen
    /*if( !paused )
        gamestate->draw();*/
    drawGame();

    //elapsed_time += time_per_frame_c;
    int new_time = this->getTicks();
    if( !paused ) {
        //updateTime(time_per_frame_c);
        updateTime(new_time - elapsed_time);
    }
    else
        updateTime(0);
    elapsed_time = new_time;
    /*int new_time = application->getTicks();
    if( !paused ) {
        updateTime(new_time - elapsed_time);
    }
    else
        updateTime(0);
    elapsed_time = new_time;*/

    updateGame();
}

void Application::runMainLoop() {
    LOG("run main loop...\n");
    const int time_per_frame_c = 1000/25;
    //const int time_per_frame_c = 200; // test
    QObject::connect(&timer, SIGNAL(timeout()), screen->getMainWindow(), SLOT(updateScene()));
    timer.start(time_per_frame_c);
    elapsed_timer.start();

    //elapsed_time = 0;
    elapsed_time = this->getTicks();
    qApp->exec();
}

void Application::processEvents() {
    qApp->processEvents();
}
