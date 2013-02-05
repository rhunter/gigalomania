#include <QtCore/QCoreApplication>

#include <QDir>
#include <QImage>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QDir src_dir("../gfx/");
    QDir dst_dir("c:/temp/");
    dst_dir.mkdir("gfx_2x/");
    dst_dir.cd("gfx_2x/");

    QRgb filter_rgb = qRgba(240, 0, 0, 255);
    QRgb mask_rgb = qRgba(255, 0, 255, 255);
    QRgb shadow_rgb = qRgba(127, 0, 127, 255);

    QStringList list = src_dir.entryList();
    foreach(const QString file, list) {
        qDebug("found file: %s", file.toStdString().c_str());
        QImage image(src_dir.filePath(file));
        if( !image.isNull() ) {
            if( image.width() % 2 != 0 ) {
                throw "image width not multiple of 2";
            }
            if( image.height() % 2 != 0 ) {
                throw "image height not multiple of 2";
            }
            int new_width = image.width() / 2;
            int new_height = image.height() / 2;
            QImage new_image(new_width, new_height, image.format());
            new_image.fill(0);
            for(int y=0;y<new_height;y++) {
                for(int x=0;x<new_width;x++) {
                    QRgb rgb00 = image.pixel(2*x, 2*y);
                    QRgb rgb10 = image.pixel(2*x+1, 2*y);
                    QRgb rgb01 = image.pixel(2*x, 2*y);
                    QRgb rgb11 = image.pixel(2*x+1, 2*y+1);
                    if( rgb00 == filter_rgb || rgb10 == filter_rgb || rgb01 == filter_rgb || rgb11 == filter_rgb ) {
                        rgb00 = filter_rgb;
                        rgb10 = filter_rgb;
                        rgb01 = filter_rgb;
                        rgb11 = filter_rgb;
                    }
                    else if( rgb00 == mask_rgb || rgb10 == mask_rgb || rgb01 == mask_rgb || rgb11 == mask_rgb ) {
                        rgb00 = mask_rgb;
                        rgb10 = mask_rgb;
                        rgb01 = mask_rgb;
                        rgb11 = mask_rgb;
                    }
                    else if( rgb00 == shadow_rgb || rgb10 == shadow_rgb || rgb01 == shadow_rgb || rgb11 == shadow_rgb ) {
                        rgb00 = shadow_rgb;
                        rgb10 = shadow_rgb;
                        rgb01 = shadow_rgb;
                        rgb11 = shadow_rgb;
                    }
                    /*if( rgb00 == mask_rgb ) {
                        rgb00 = qRgba(0, 0, 0, 0);
                    }
                    if( rgb10 == mask_rgb ) {
                        rgb10 = qRgba(0, 0, 0, 0);
                    }
                    if( rgb01 == mask_rgb ) {
                        rgb01 = qRgba(0, 0, 0, 0);
                    }
                    if( rgb11 == mask_rgb ) {
                        rgb11 = qRgba(0, 0, 0, 0);
                    }
                    if( rgb00 == shadow_rgb ) {
                        rgb00 = qRgba(0, 0, 0, 0);
                    }
                    if( rgb10 == shadow_rgb ) {
                        rgb10 = qRgba(0, 0, 0, 0);
                    }
                    if( rgb01 == shadow_rgb ) {
                        rgb01 = qRgba(0, 0, 0, 0);
                    }
                    if( rgb11 == shadow_rgb ) {
                        rgb11 = qRgba(0, 0, 0, 0);
                    }*/
                    int irgb00[] = {qRed(rgb00), qGreen(rgb00), qBlue(rgb00), qAlpha(rgb00)};
                    int irgb10[] = {qRed(rgb10), qGreen(rgb10), qBlue(rgb10), qAlpha(rgb10)};
                    int irgb01[] = {qRed(rgb01), qGreen(rgb01), qBlue(rgb01), qAlpha(rgb01)};
                    int irgb11[] = {qRed(rgb11), qGreen(rgb11), qBlue(rgb11), qAlpha(rgb11)};
                    int irgb[] = {0, 0, 0, 0};
                    for(int i=0;i<4;i++) {
                        int result = (int)((irgb00[i] + irgb10[i] + irgb01[i] + irgb11[i]) / 4.0);
                        irgb[i] = result;
                    }
                    QRgb rgb = qRgba(irgb[0], irgb[1], irgb[2], irgb[3]);
                    new_image.setPixel(x, y, rgb);
                }
            }
            QString save_filename = dst_dir.filePath(file);
            qDebug("    save as %s", save_filename.toStdString().c_str());
            if( !new_image.save(save_filename) ) {
                throw "failed to save";
            }
        }
        else {
            qDebug("    failed to load");
        }
    }
    qDebug("all done!");

    //return a.exec();
    return 0;
}
