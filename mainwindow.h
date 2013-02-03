#pragma once

#include <QtGlobal> // need this to get Q_OS_ANDROID #define, which we need before we include anything else!

#ifndef Q_OS_ANDROID
#include <QSystemScreenSaver>
QTM_USE_NAMESPACE
#endif

#include <QtGui/QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT

    int m_x, m_y;
    bool m_left;
#ifndef Q_OS_ANDROID
    QSystemScreenSaver *screen_saver;
#endif

protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    //virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void paintEvent(QPaintEvent *event);
public slots:
    void updateScene();
public:

    enum ScreenOrientation {
        ScreenOrientationLockPortrait,
        ScreenOrientationLockLandscape,
        ScreenOrientationAuto
    };

    explicit MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();

    // Note that this will only have an effect on Symbian and Fremantle.
    void setOrientation(ScreenOrientation orientation);

    void showExpanded();

    void getMouseXY(int *m_x, int *m_y) const {
        *m_x = this->m_x;
        *m_y = this->m_y;
    }
    void getMouseButtons(bool *m_left) const {
        *m_left = this->m_left;
    }
};
