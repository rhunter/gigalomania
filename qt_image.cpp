#include "stdafx.h"

#include <algorithm>
using std::min;
using std::max;

#include "qt_image.h"
#include "qt_screen.h"
#include "game.h"
#include "utils.h"

Image::Image() : scale_x(1.0f), scale_y(1.0f) {
}

Image::~Image() {
}

void Image::draw(int x, int y,bool mask) const {
    x = (int)(x * scale_x);
    y = (int)(y * scale_y);
    ASSERT( !this->pixmap.isNull() );

    /*QPixmap newPixmap = QPixmap(64, 64);
    newPixmap.fill(Qt::white);

    QPixmap newPixmap2 = QPixmap(640, 640);
    newPixmap2.fill(Qt::black);

    //QPainter painter(screen->getMainWindow());
    QPainter painter(&newPixmap2);
    //painter.fillRect(0, 0, 640, 360, Qt::black);
    painter.drawPixmap(x, y, this->pixmap);
    //painter.drawPixmap(x, y, newPixmap);*/
    QPainter painter(&screen->getPixmap());
    //painter.fillRect(x, y, this->pixmap.width(), this->pixmap.height(), Qt::red);
    //painter.fillRect(x, y, this->image.width(), this->image.height(), Qt::red);
    //qDebug("%d, %d, %d, %d", x, y, this->pixmap.width(), this->pixmap.height());
    //painter.fillRect(32, 32, 64, 64, Qt::red);
    //painter.fillRect(x, y, 64, 64, Qt::red);
    painter.drawPixmap(x, y, this->pixmap);
    //painter.drawImage(x, y, this->image);
}

void Image::drawWithAlpha(int x, int y, unsigned char alpha) const {
    x = (int)(x * scale_x);
    y = (int)(y * scale_y);
    QPainter painter(&screen->getPixmap());
    painter.setOpacity((qreal)(alpha/255.0));
    painter.drawPixmap(x, y, this->pixmap);
}

void Image::convertToDisplayFormat() {
    //LOG("convertToDisplayFormat: %d\n", this);
    ASSERT( !this->image.isNull() );
    pixmap = QPixmap::fromImage(image, Qt::OrderedAlphaDither);
    ASSERT( !this->pixmap.isNull() );
}

bool Image::copyPalette(const Image *image) {
    LOG("Image::copyPalette not supported\n");
    ASSERT(false);
    return true;
}

void Image::setScale(float scale_x,float scale_y) {
    this->scale_x = scale_x;
    this->scale_y = scale_y;
}

bool Image::scaleTo(int n_w) {
    this->image = this->image.scaledToWidth(n_w);
    return true;
}

void Image::scale(float sx,float sy) {
    //qDebug("scale, from %d, %d", this->getWidth(), this->getHeight());
    // smooth rescaling still looks better, even on phone!
    //this->image = this->image.scaled( sx*this->getWidth(), sy*this->getHeight() );
    this->image = this->image.scaled( sx*this->getWidth(), sy*this->getHeight(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    //qDebug("    to %d, %d", this->getWidth(), this->getHeight());
}

void Image::remap(unsigned char sr,unsigned char sg,unsigned char sb,unsigned char rr,unsigned char rg,unsigned char rb) {
    // only supported for cases where pixmap not yet created
    ASSERT( !this->image.isNull() );
    ASSERT( this->pixmap.isNull() );

    for(int cx=0;cx<image.width();cx++) {
        for(int cy=0;cy<image.height();cy++) {
            QRgb rgb = this->image.pixel(cx, cy);
            int r = qRed(rgb);
            int g = qGreen(rgb);
            int b = qBlue(rgb);
            int a = qAlpha(rgb);
            if( r == sr && g == sg && b == sb ) {
                this->image.setPixel(cx, cy, qRgba(rr, rg, rb, a));
            }
        }
    }
}

void Image::reshadeRGB(int from, bool to_r, bool to_g, bool to_b) {
    // only supported for cases where pixmap not yet created
    ASSERT( !this->image.isNull() );
    ASSERT( this->pixmap.isNull() );

    ASSERT(from >= 0 && from < 3);
    bool to[3] = {to_r, to_g, to_b};
    for(int cx=0;cx<image.width();cx++) {
        for(int cy=0;cy<image.height();cy++) {
            QRgb rgb = this->image.pixel(cx, cy);
            int r = qRed(rgb);
            int g = qGreen(rgb);
            int b = qBlue(rgb);
            int a = qAlpha(rgb);
            int rgba[] = {r, g, b, a};
            int val = rgba[from];
            int t_diff = 0;
            int n = 0;
            for(int j=0;j<3;j++) {
                if( to[j] && j != from ) {
                    int val2 = rgba[j];
                    int diff = val2 - val;
                    t_diff += diff;
                    n++;
                    rgba[j] = val;
                }
            }
            if( n > 0 && !to[from] ) {
                t_diff /= n;
                val += t_diff;
                ASSERT(val >=0 && val < 256);
                rgba[from] = val;
            }
            this->image.setPixel(cx, cy, qRgba(rgba[0], rgba[1], rgba[2], rgba[3]));
        }
    }
}

void Image::brighten(float sr, float sg, float sb) {
    // only supported for cases where pixmap not yet created
    ASSERT( !this->image.isNull() );
    ASSERT( this->pixmap.isNull() );

    float scale[3] = {sr, sg, sb};
    for(int cx=0;cx<image.width();cx++) {
        for(int cy=0;cy<image.height();cy++) {
            QRgb rgb = this->image.pixel(cx, cy);
            int r = qRed(rgb);
            int g = qGreen(rgb);
            int b = qBlue(rgb);
            int a = qAlpha(rgb);
            int rgba[] = {r, g, b, a};
            for(int j=0;j<3;j++) {
                float col = (float)rgba[j];
                col *= scale[j];
                if( col < 0 )
                    col = 0;
                else if( col > 255 )
                    col = 255;
                rgba[j] = (unsigned char)col;
            }
            this->image.setPixel(cx, cy, qRgba(rgba[0], rgba[1], rgba[2], rgba[3]));
        }
    }
}

void Image::fillRect(int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    //LOG("fillRect\n");
    // should only be called when pixmap already created
    ASSERT( !this->pixmap.isNull() );
    QPainter painter(&this->pixmap);
    QColor color(r, g, b);
    QBrush brush(color);
    painter.setBrush(brush);
    painter.fillRect(x, y, w, h, brush);
}

Image *Image::copy(int x,int y,int w,int h) const {
    //qDebug("copy from %d, %d, %d, %d", x, y, w, h);
    x = (int)(x * scale_x);
    y = (int)(y * scale_y);
    w = (int)(w * scale_x);
    h = (int)(h * scale_y);
    //qDebug("-> %d, %d, %d, %d", x, y, w, h);
    Image *new_image = new Image();
    new_image->image = this->image.copy(x, y, w, h);
    new_image->scale_x = this->scale_x;
    new_image->scale_y = this->scale_y;
    ASSERT( !new_image->image.isNull() );
    return new_image;
    //return NULL;
}

bool Image::isPaletted() const {
    LOG("Image::isPaletted not supported\n");
    ASSERT(false);
    return false;
}

int Image::getNColors() const {
    LOG("Image::getNColors not supported\n");
    ASSERT(false);
    return 0;
}

unsigned char Image::getPixelIndex(int x,int y) const {
    LOG("Image::getPixelIndex not supported\n");
    ASSERT(false);
    return 0;
}

bool Image::setPixelIndex(int x,int y,unsigned char c) {
    LOG("Image::setPixelIndex not supported\n");
    ASSERT(false);
    return true;
}

bool Image::setColor(int index,unsigned char r,unsigned char g,unsigned char b) {
    LOG("Image::setColor not supported\n");
    ASSERT(false);
    return true;
}

bool Image::createAlphaForColor(bool mask, unsigned char mr, unsigned char mg, unsigned char mb, unsigned char ar, unsigned char ag, unsigned char ab, unsigned char alpha) {
    // only supported for cases where pixmap not yet created
    ASSERT( !this->image.isNull() );
    ASSERT( this->pixmap.isNull() );
    if( this->image.format() != QImage::Format_ARGB32 ) {
        //LOG("convert image to ARGB32\n");
        this->image = this->image.convertToFormat(QImage::Format_ARGB32);
    }
    //return true;
    for(int cx=0;cx<image.width();cx++) {
        for(int cy=0;cy<image.height();cy++) {
            QRgb rgb = this->image.pixel(cx, cy);
            int r = qRed(rgb);
            int g = qGreen(rgb);
            int b = qBlue(rgb);
            int a = 255;
            if( r == ar && g == ag && b == ab ) {
                r = 0;
                g = 0;
                b = 0;
                a = alpha;
                this->image.setPixel(cx, cy, qRgba(r, g, b, a));
            }
            //else if( mask && r == this->mask_r && g == this->mask_g && b == mask_b ) {
            else if( mask && r == mr && g == mg && b == mb ) {
                r = 0;
                g = 0;
                b = 0;
                a = 0;
                this->image.setPixel(cx, cy, qRgba(r, g, b, a));
            }
        }
    }

    return true;
}

void Image::scaleAlpha(float scale) {
    // only supported for cases where pixmap not yet created
    ASSERT( !this->image.isNull() );
    ASSERT( this->pixmap.isNull() );
    /*if( this->image.format() != QImage::Format_ARGB32 && this->image.format() != Format_ARGB32_Premultiplied) {
        //qDebug("not okay: %d", this->image.format());
        return;
    }*/
    if( this->image.format() != QImage::Format_ARGB32 ) {
        qDebug("convert image to ARGB32");
        this->image = this->image.convertToFormat(QImage::Format_ARGB32);
    }
    for(int cx=0;cx<image.width();cx++) {
        for(int cy=0;cy<image.height();cy++) {
            QRgb rgb = this->image.pixel(cx, cy);
            int r = qRed(rgb);
            int g = qGreen(rgb);
            int b = qBlue(rgb);
            int a = qAlpha(rgb);
            a *= scale;
            this->image.setPixel(cx, cy, qRgba(r, g, b, a));
        }
    }
}

bool Image::convertToHiColor(bool alpha) {
    // no need to do this with Qt
    return true;
}

Image * Image::loadImage(const char *filename) {
    string qt_filename = ":/" + string(filename);
    LOG("loading image: %s\n", qt_filename.c_str());
    Image *image = new Image();
    if( !image->image.load(qt_filename.c_str()) ) {
        LOG("failed to load image: %s\n", filename);
        delete image;
        image = NULL;
    }
    if( image != NULL ) {
        //LOG("created image: %d, depth %d\n", image, image->image.depth());
        ASSERT( !image->image.isNull() );
    }
    return image;
}

Image * Image::createBlankImage(int width,int height, int bpp) {
    Image *new_image = new Image();
    new_image->image = QImage(width, height, QImage::Format_ARGB32);
    new_image->image.fill(0); // needed to initialise image!
    //LOG("Image::createBlankImage %dx%d: %d\n", width, height, new_image);
    ASSERT( !new_image->image.isNull() );
    return new_image;
}

Image * Image::createNoise(int w,int h,float scale_u,float scale_v,const unsigned char filter_max[3],const unsigned char filter_min[3],NOISEMODE_t noisemode,int n_iterations) {
    Image *new_image = Image::createBlankImage(w, h, 32);
    //LOG("Image::createNoise: %d\n", new_image);
    ASSERT( !new_image->image.isNull() );
    //return new_image;

    float fvec[2] = {0.0f, 0.0f};
    for(int y=0;y<h;y++) {
        fvec[0] = scale_v * ((float)y) / ((float)h - 1.0f);
        for(int x=0;x<w;x++) {
            fvec[1] = scale_u * ((float)x) / ((float)w - 1.0f);
            float h = 0.0f;
            float max_val = 0.0f;
            float mult = 1.0f;
            for(int j=0;j<n_iterations;j++,mult*=2.0f) {
                float this_fvec[2];
                this_fvec[0] = fvec[0] * mult;
                this_fvec[1] = fvec[1] * mult;
                float this_h = perlin_noise2(this_fvec) / mult;
                if( noisemode == NOISEMODE_PATCHY || noisemode == NOISEMODE_MARBLE )
                    this_h = abs(this_h);
                h += this_h;
                max_val += 1.0f / mult;
            }
            if( noisemode == NOISEMODE_PATCHY ) {
                h /= max_val;
            }
            else if( noisemode == NOISEMODE_MARBLE ) {
                h = sin(scale_u * ((float)x) / ((float)w - 1.0f) + h);
                h = 0.5f + 0.5f * h;
            }
            else {
                h /= max_val;
                h = 0.5f + 0.5f * h;
            }

            if( noisemode == NOISEMODE_CLOUDS ) {
                //const float offset = 0.4f;
                //const float offset = 0.3f;
                const float offset = 0.2f;
                h = offset - h * h;
                h = max(h, 0.0f);
                h /= offset;
            }
            // h is now in range [0, 1]
            if( h < 0.0 || h > 1.0 ) {
                LOG("h value is out of bounds\n");
                ASSERT(false);
            }
            if( noisemode == NOISEMODE_WOOD ) {
                h = 20 * h;
                h = h - floor(h);
            }
            int r = (int)((filter_max[0] - filter_min[0]) * h + filter_min[0]);
            int g = (int)((filter_max[1] - filter_min[1]) * h + filter_min[1]);
            int b = (int)((filter_max[2] - filter_min[2]) * h + filter_min[2]);
            int a = 255;
            new_image->image.setPixel(x, y, qRgba(r, g, b, a));
        }
    }
    return new_image;
}

Image * Image::createRadial(int w,int h,float alpha_scale) {
    Image *new_image = Image::createBlankImage(w, h, 32);
    //LOG("Image::createRadial: %d\n", new_image);
    ASSERT( !new_image->image.isNull() );

    int radius = min(w/2, h/2);
    for(int y=0;y<h;y++) {
        int dy = abs(y - h/2);
        for(int x=0;x<w;x++) {
            int dx = abs(x - w/2);
            float dist = sqrt((float)(dx*dx + dy*dy));
            dist /= (float)radius;
            if( dist >= 1.0f )
                dist = 1.0f;
            dist = 1.0f - dist;
            dist *= alpha_scale;
            int r = 255;
            int g = 255;
            int b = 255;
            int a = (int)(255.0f * dist);
            new_image->image.setPixel(x, y, qRgba(r, g, b, a));
        }
    }
    return new_image;
}

void Image::writeNumbers(int x,int y,Image *images[10],int number,Justify justify,bool mask) {
   /*QString str = QString::number(number);
   writeMixedCase(x, y, images, images, NULL, str.toLatin1(), justify, mask);*/
    char buffer[16] = "";
    sprintf(buffer,"%d",number);
    int len = strlen(buffer);
    int w = images[0]->getScaledWidth();
    int sx = 0;
    if( justify == JUSTIFY_LEFT )
        sx = x;
    else if( justify == JUSTIFY_CENTRE )
        sx = x - ( w * len ) / 2;
    else if( justify == JUSTIFY_RIGHT )
        sx = x - w * len;

    for(int i=0;i<len;i++) {
        images[ buffer[i] - '0' ]->draw(sx, y, mask);
        sx += w;
    }
}

void Image::write(int x,int y,Image *images[26],const char *text,Justify justify,bool mask) {
    writeMixedCase(x, y, images, images, NULL, text, justify, mask);
    /*QPainter painter(&screen->getPixmap());
    // TODO: justify
    painter.drawText(x, y, text);*/
}

void Image::writeMixedCase(int x,int y,Image *large[26],Image *little[26],Image *numbers[10],const char *text,Justify justify,bool mask) {
    //LOG("### %f, %f\n", large[0]->scale_x, large[0]->scale_y);
    /*x = (int)(x * large[0]->scale_x);
    y = (int)(y * large[0]->scale_y);
    QPainter painter(&screen->getPixmap());
    // TODO: justify
    QFont font;
    QFontMetrics qm(font);
    int font_height = qm.height();
    QColor color(255, 255, 255);
    painter.setPen(color);
    painter.drawText(x, y + font_height, text);*/
    int len = strlen(text);
    int n_lines = 0;
    int max_wid = 0;
    textLines(&n_lines, &max_wid, text);
    ASSERT(n_lines == 1 || justify == JUSTIFY_LEFT);
    int w = little[0]->getScaledWidth();
    int n_h = 0;
    if( numbers != NULL )
        n_h = numbers[0]->getScaledHeight();
    int s_h = little[0]->getScaledHeight();
    int l_h = large[0]->getScaledHeight();
    int sx = 0;
    if( justify == JUSTIFY_LEFT )
        sx = x;
    else if( justify == JUSTIFY_CENTRE )
        sx = x - ( w * max_wid ) / 2;
    else if( justify == JUSTIFY_RIGHT )
        sx = x - w * max_wid;
    int cx = sx;

    for(int i=0;i<len;i++) {
        char ch = text[i];
        if( numbers == NULL && ch == '0' ) {
            ch = 'O'; // hack for 0hm (we don't spell it Ohm, due to alphabetical ordering)
        }
        if( ch == '\n' ) {
            // newline
            cx = sx;
            y += l_h + 2;
            continue; // don't increase sx
        }
        else if( isspace( ch ) )
            ; // do nothing
        else if( ch >= '0' && ch <= '9' ) {
            ASSERT( numbers != NULL );
            int indx = ch - '0';
            numbers[indx]->draw(cx, y + l_h - n_h, mask);
        }
        else if( isupper( ch ) ) {
            int indx = ch - 'A';
            large[indx]->draw(cx, y, mask);
        }
        else {
            little[ ch - 'a' ]->draw(cx, y + l_h - s_h, mask);
        }
        cx += w;
    }
}
