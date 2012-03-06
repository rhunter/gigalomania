#pragma once

/** Class for managing the display. There shouldn't be anything specific to a
*   particlar game in this class.
*/

#include <QtGui>

#include "mainwindow.h"

/*class MainWindow;

class TGamestate : public QMainWindow {
    Q_OBJECT

protected:
    MainWindow *mainWindow;
public:
    TGamestate(MainWindow *mainWindow) : mainWindow(mainWindow) {
    }
    virtual ~TGamestate() {
    }

};*/

class Application /*: public QObject*/ {
    //Q_OBJECT

    //bool quit;
    QTimer timer;
    QElapsedTimer elapsed_timer;

    int elapsed_time;
    bool was_m_left;

/*public slots:
    void update();*/

public:
    Application(/*QWidget *parent = 0*/) : /*quit(false),*/ elapsed_time(0) {
    }

    ~Application() {
    }

    bool init() {
        // TODO
        return true;
    }

    int getTicks() const;
    void delay(int time) {
        // TODO
    }

    void runMainLoop();
    void processEvents();

    void setQuit() {
        //quit = true;
        qApp->quit();
    }
    void update();
    void activateTimers(bool activate) {
        activate ? timer.start() : timer.stop();
    }
};

class Screen {
    MainWindow mainWindow;
    QPixmap pixmap;
public:
	Screen();
	~Screen();

	static bool canOpenFullscreen(int width, int height);
	bool open(int screen_width, int screen_height, bool fullscreen);
	void setTitle(const char *title);
	void clear();
	void refresh();
    int getWidth() const {
        return pixmap.width();
    }
    int getHeight() const {
        return pixmap.height();
    }
	void fillRect(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b);
    void fillRectWithAlpha(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);

	void getMouseCoords(int *m_x, int *m_y);
	bool getMouseState(int *m_x, int *m_y, bool *m_left, bool *m_middle, bool *m_right);

    // Qt specific
    MainWindow *getMainWindow() {
        return &mainWindow;
    }
    const MainWindow *getMainWindow() const {
        return &mainWindow;
    }
    QPixmap& getPixmap() {
        return pixmap;
    }
    /*const QPixmap getPixmap() const {
        return pixmap;
    }*/
};
