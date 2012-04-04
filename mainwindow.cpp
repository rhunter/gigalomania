#include "mainwindow.h"
#include "utils.h"
#include "gamestate.h"
#include "game.h"
#include "qt_screen.h"

#include <QtCore/QCoreApplication>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_x(0), m_y(0), m_left(false)
{
#ifndef Q_OS_ANDROID
    // prevent screen dimming/blanking
    // on Android, the screen blanking is disabled in the QtActivity.java file
    this->screen_saver = new QSystemScreenSaver(this);
    this->screen_saver->setScreenSaverInhibit();
#endif
}

MainWindow::~MainWindow()
{
}

void MainWindow::setOrientation(ScreenOrientation orientation)
{
#if defined(Q_OS_SYMBIAN)
    // If the version of Qt on the device is < 4.7.2, that attribute won't work
    if (orientation != ScreenOrientationAuto) {
        const QStringList v = QString::fromAscii(qVersion()).split(QLatin1Char('.'));
        if (v.count() == 3 && (v.at(0).toInt() << 16 | v.at(1).toInt() << 8 | v.at(2).toInt()) < 0x040702) {
            qWarning("Screen orientation locking only supported with Qt 4.7.2 and above");
            return;
        }
    }
#endif // Q_OS_SYMBIAN

    Qt::WidgetAttribute attribute;
    switch (orientation) {
#if QT_VERSION < 0x040702
    // Qt < 4.7.2 does not yet have the Qt::WA_*Orientation attributes
    case ScreenOrientationLockPortrait:
        attribute = static_cast<Qt::WidgetAttribute>(128);
        break;
    case ScreenOrientationLockLandscape:
        attribute = static_cast<Qt::WidgetAttribute>(129);
        break;
    default:
    case ScreenOrientationAuto:
        attribute = static_cast<Qt::WidgetAttribute>(130);
        break;
#else // QT_VERSION < 0x040702
    case ScreenOrientationLockPortrait:
        attribute = Qt::WA_LockPortraitOrientation;
        break;
    case ScreenOrientationLockLandscape:
        attribute = Qt::WA_LockLandscapeOrientation;
        break;
    default:
    case ScreenOrientationAuto:
        attribute = Qt::WA_AutoOrientation;
        break;
#endif // QT_VERSION < 0x040702
    };
    setAttribute(attribute, true);
}

void MainWindow::showExpanded()
{
#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR)
    showFullScreen();
#elif defined(Q_WS_MAEMO_5)
    showMaximized();
#else
    show();
#endif
    // n.b., showFullScreen() causes crash on Android?! We set fullscreen via the AndroidManifest.xml instead
}

void MainWindow::updateScene() {
    //qDebug("MainWindow::updateScene()");
    //qApp->beep();
    application->update();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    qDebug("key press");
    /*if( event->key() == Qt::Key_VolumeDown ) {
        qApp->beep();
    }
    else if( event->key() == Qt::Key_VolumeUp ) {
        qApp->beep();
    }*/
    if( event->key() == Qt::Key_Escape ) {
        keypressEscape();
    }
    else if( event->key() == Qt::Key_P ) {
        keypressP();
    }
    else if( event->key() == Qt::Key_Q ) {
        keypressQ();
    }
}

/*void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    //qDebug("mouse move");
    m_x = event->x();
    m_y = event->y();
    //LOG("mouse: %d, %d\n", m_x, m_y);
}*/

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if( event->button() == Qt::LeftButton ) {
        m_x = event->x();
        m_y = event->y();
        m_left = true;
        //qApp->beep();

        if( paused ) {
            // click automatically unpaused (needed to work without keyboard!)
            paused = false;
        }
        else {
            //qApp->beep();
            mouseClick(m_x, m_y, m_left, false, false, true);
        }
    }
    //qDebug("mouse press: %d, %d", m_x, m_y);
    //LOG("mouse: %d, %d\n", m_x, m_y);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    if( event->button() == Qt::LeftButton ) {
        m_left = false;
    }
}

void MainWindow::paintEvent(QPaintEvent *event) {
    //qDebug("size: %d x %d\n", this->width(), this->height());
    QPainter painter(this);
    //painter.fillRect(0, 0, width(), height(), Qt::red);
    painter.drawPixmap(0, 0, screen->getPixmap());
}
