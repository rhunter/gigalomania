//---------------------------------------------------------------------------
#include "stdafx.h"

#ifdef __linux
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#else
#include <sdl.h>
#include <sdl_image.h>
#endif

#include <cassert>
#include <cmath> // n.b., needed on Linux at least

//#define TIMING

#ifdef TIMING
#include <ctime> // for performance testing
#endif

#include <algorithm>
using std::min;
using std::max;

#include "image.h"
#include "utils.h"
//---------------------------------------------------------------------------

inline void CreateMask( Uint32& rmask, Uint32& gmask, Uint32& bmask, Uint32& amask) {
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#elif defined(__APPLE__) && defined(__MACH__)
	rmask = 0x00ff0000;
	gmask = 0x0000ff00;
	bmask = 0x000000ff;
	amask = 0xff000000;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
// TEST:
	/*rmask = 0x00ff0000;
	gmask = 0x0000ff00;
	bmask = 0x000000ff;
	amask = 0xff000000;*/
#endif
}

extern const bool DEBUG;
extern const int DEBUGLEVEL;

using namespace Gigalomania;

SDL_Surface *Image::dest_surf = NULL;

Image::Image() {
	//this->bitmap = NULL;
	this->data = NULL;
	this->need_to_free_data = NULL;
	this->surface = NULL;
	//this->userData = NULL;
	this->scale_x = 1;
	this->scale_y = 1;
	/*this->scale_x = 2;
	this->scale_y = 2;*/
	/*this->mask_r = 0;
	this->mask_g = 0;
	this->mask_b = 0;*/
}

Image::~Image() {
	free();
}

void Image::free() {
	if( this->surface != NULL ) {
		SDL_FreeSurface(this->surface);
		this->surface = NULL;
	}
	/*if( this->bitmap != NULL ) {
	FreeImage_Free(this->bitmap);
	this->bitmap = NULL;
	}*/
	if( need_to_free_data && this->data != NULL ) {
		delete [] this->data;
		this->data = NULL;
	}
}

void Image::draw(int x, int y,bool mask) const {
	x = (int)(x * scale_x);
	y = (int)(y * scale_y);
	/*if( mask ) {
		int index = SDL_MapRGB(this->surface->format, mask_r, mask_g, mask_b);
		SDL_SetColorKey(surface, SDL_SRCCOLORKEY, index);
	}*/
	SDL_Rect srcrect;
	srcrect.x = 0;
	srcrect.y = 0;
	srcrect.w = (short)this->getWidth();
	srcrect.h = (short)this->getHeight();
	SDL_Rect dstrect;
	dstrect.x = (short)x;
	dstrect.y = (short)y;
	dstrect.w = 0;
	dstrect.h = 0;
	SDL_BlitSurface(surface, &srcrect, dest_surf, &dstrect);
	/*if( mask ) {
		SDL_SetColorKey(surface, 0, 0);
	}*/
}

void Image::drawWithAlpha(int x, int y, unsigned char alpha) const {
	// n.b., only works if the image doesn't have per-pixel alpha channel
	SDL_SetAlpha(this->surface, SDL_SRCALPHA|SDL_RLEACCEL, alpha);
	this->draw(x, y, false);
}

/*void Image::drawAlpha(int x, int y,float alpha) {
x *= scale_x;
y *= scale_y;
//scale_w = scale_h = 1;
int w = this->getWidth();
int h = this->getHeight();
int d_w = dest_surf->w;
int d_h = dest_surf->h;
SDL_LockSurface(surface);
SDL_LockSurface(dest_surf);
int s_bpp = surface->format->BitsPerPixel;
int d_bpp = dest_surf->format->BitsPerPixel;
int s_bytespp = surface->format->BytesPerPixel;
int d_bytespp = dest_surf->format->BytesPerPixel;
for(int cy=0;cy<h;cy++) {
for(int cx=0;cx<w;cx++) {
int d_x = ( x + cx );
int d_y = ( y + cy );
if( d_x >= 0 && d_x < d_w && d_y >= 0 && d_y < d_h ) {
unsigned char *ptr = &((unsigned char *)surface->pixels)[ surface->pitch * cy + s_bytespp * cx ];
unsigned char *d_ptr = &((unsigned char *)dest_surf->pixels)[ dest_surf->pitch * d_y + d_bytespp * d_x ];
for(int i=0;i<s_bytespp;i++) {
int s_col = ptr[i];
int d_col = d_ptr[i];
//int n_col = alpha * s_col + ( 1.0 - alpha ) * d_col;
int n_col = d_col;
//d_ptr[i] = n_col;
}
}
}
}
SDL_UnlockSurface(surface);
SDL_UnlockSurface(dest_surf);
}*/

/*void Image::draw(int x, int y,bool mask,int scale_w, int scale_h) {
//if( false && scale_w == 1 && scale_h == 1 ) {
if( true ) {
this->draw(x,y,mask);
}
else {
x *= scale_x;
y *= scale_y;
scale_w = scale_h = 1;
int w = this->getWidth();
int h = this->getHeight();
int d_w = dest_surf->w;
int d_h = dest_surf->h;
SDL_LockSurface(surface);
SDL_LockSurface(dest_surf);
for(int cy=0;cy<h;cy++) {
for(int cx=0;cx<w;cx++) {
unsigned char c = ((unsigned char *)surface->pixels)[ surface->pitch * cy + cx ];
if( mask && c == 0 )
continue;
for(int j=0;j<scale_h;j++) {
for(int i=0;i<scale_w;i++) {
int d_x = ( x + cx ) * scale_w + i;
int d_y = ( y + cy ) * scale_h + j;
//int d_x = x * scale_w + cx + i;
//int d_y = y * scale_h + cy + j;
if( d_x >= 0 && d_x < d_w && d_y >= 0 && d_y < d_h )
((unsigned char *)dest_surf->pixels)[ dest_surf->pitch * d_y + d_x ] = c;
}
}
}
}
SDL_UnlockSurface(surface);
SDL_UnlockSurface(dest_surf);
}
}*/

/*void Image::getColor(int *r,int *g,int *b,int index) {
*r = this->palette[index].rgbRed;
*g = this->palette[index].rgbGreen;
*b = this->palette[index].rgbBlue;
}

void Image::getSDLColor(SDL_Color *color,int index) {
color->r = this->palette[index].rgbRed;
color->g = this->palette[index].rgbGreen;
color->b = this->palette[index].rgbBlue;
}*/

int Image::getWidth() const {
	return this->surface->w;
}

int Image::getHeight() const {
	return this->surface->h;
}

void Image::setScale(float scale_x,float scale_y) {
	this->scale_x = scale_x;
	this->scale_y = scale_y;
}

/*const SDL_Surface *Image::getSDLSurface() const {
	return this->surface;
}

SDL_Surface *Image::getSDLSurface() {
	return this->surface;
}*/

bool Image::isPaletted() const {
	return this->surface->format->palette != NULL;
}

int Image::getNColors() const {
	return this->surface->format->palette->ncolors;
	//return 0;
}

unsigned char Image::getPixelIndex(int x,int y) const {
	if( !isPaletted() )
		return 0;

	SDL_LockSurface(this->surface);
	unsigned char c = ((unsigned char *)this->surface->pixels)[ y * this->surface->pitch + x ];
	SDL_UnlockSurface(this->surface);
	return c;
}

bool Image::setPixelIndex(int x,int y,unsigned char c) {
	if( !isPaletted() )
		return false;


	SDL_LockSurface(this->surface);
	((unsigned char *)this->surface->pixels)[ y * this->surface->pitch + x ] = c;
	SDL_UnlockSurface(this->surface);
	return true;
}

bool Image::setColor(int index,unsigned char r,unsigned char g,unsigned char b) {
	if( !isPaletted() )
		return false;

	SDL_LockSurface(this->surface);
	SDL_Color *col = &this->surface->format->palette->colors[index];
	col->r = r;
	col->g = g;
	col->b = b;
	SDL_UnlockSurface(this->surface);
	return true;
}

/*bool Image::setMaskColor(unsigned char r,unsigned char g,unsigned char b) {
	this->mask_r = r;
	this->mask_g = g;
	this->mask_b = b;
	return true;
}*/

/*
* Return the pixel value at (x, y)
* NOTE: The surface must be locked before calling this!
*/
Uint32 getpixel(SDL_Surface *surface, int x, int y) {
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to retrieve */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
	case 1:
		return *p;

	case 2:
		return *(Uint16 *)p;

	case 3:
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;

	case 4:
		return *(Uint32 *)p;

	default:
		return 0;       /* shouldn't happen, but avoids warnings */
	}
}

/*
* Set the pixel at (x, y) to the given value
* NOTE: The surface must be locked before calling this!
*/
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
	case 1:
		*p = pixel;
		break;

	case 2:
		*(Uint16 *)p = pixel;
		break;

	case 3:
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			p[0] = (pixel >> 16) & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = pixel & 0xff;
		} else {
			p[0] = pixel & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = (pixel >> 16) & 0xff;
		}
		break;

	case 4:
		*(Uint32 *)p = pixel;
		break;
	}
}

// Creates an alpha from the mask; also adds in shadow effect based on supplied ar/ag/ab colour
bool Image::createAlphaForColor(bool mask, unsigned char mr, unsigned char mg, unsigned char mb, unsigned char ar, unsigned char ag, unsigned char ab, unsigned char alpha) {
	int w = this->getWidth();
	int h = this->getHeight();

	this->convertToHiColor(true);

#ifdef TIMING
	int time_s = clock();
#endif

	SDL_LockSurface(this->surface);
	// faster to read in x direction! (caching?)
	for(int cy=0;cy<h;cy++) {
		for(int cx=0;cx<w;cx++) {
			Uint32 pixel = getpixel(this->surface, cx, cy);
			Uint8 r = 0, g = 0, b = 0, a = 0;
			SDL_GetRGB(pixel, this->surface->format, &r, &g, &b);
			if( r == ar && g == ag && b == ab ) {
				r = 0;
				g = 0;
				b = 0;
				a = alpha;
				pixel = SDL_MapRGBA(this->surface->format, r, g, b, a);
				putpixel(this->surface, cx, cy, pixel);
			}
			else if( mask && r == mr && g == mg && b == mb ) {
				r = 0;
				g = 0;
				b = 0;
				a = 0;
				pixel = SDL_MapRGBA(this->surface->format, r, g, b, a);
				putpixel(this->surface, cx, cy, pixel);
			}
		}
	}

	SDL_UnlockSurface(this->surface);
#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image createAlphaForColor time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image createAlphaForColor total %d\n", total);
#endif
	return true;
}

void Image::scaleAlpha(float scale) {
	int bpp = this->surface->format->BitsPerPixel;
	if( bpp != 32 )
		return;

#ifdef TIMING
	int time_s = clock();
#endif

	int w = this->getWidth();
	int h = this->getHeight();
	SDL_LockSurface(this->surface);
	// faster to read in x direction! (caching?)
	for(int cy=0;cy<h;cy++) {
		for(int cx=0;cx<w;cx++) {
			Uint32 pixel = getpixel(this->surface, cx, cy);
			Uint8 r = 0, g = 0, b = 0, a = 0;
			SDL_GetRGBA(pixel, this->surface->format, &r, &g, &b, &a);
			//a *= scale;
			a = (Uint8)(a*scale);
			pixel = SDL_MapRGBA(this->surface->format, r, g, b, a);
			putpixel(this->surface, cx, cy, pixel);
		}
	}
	SDL_UnlockSurface(this->surface);
#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image scaleAlpha time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image scaleAlpha total %d\n", total);
#endif
}

bool Image::convertToHiColor(bool alpha) {
#ifdef TIMING
	int time_s = clock();
#endif
	// todo: repeated conversions don't seem to work? seems to be due to repeated blitting not working
	int bpp = this->surface->format->BitsPerPixel;
	if( bpp == 32 )
		return false;
	/*SDL_PixelFormat new_format = *this->surface->format;
	new_format.palette = NULL;
	new_format.BitsPerPixel = alpha ? 24 : 16;
	new_format.BytesPerPixel = alpha ? 4 : 3;*/
	Uint32 rmask, gmask, bmask, amask;
	CreateMask(rmask, gmask, bmask, amask);

	/*unsigned char bytes[4];
	int n_bytes = alpha ? 4 : 3;
	SDL_Surface *dummy = SDL_CreateRGBSurfaceFrom(bytes, 1, 1, n_bytes * 8, 1 * n_bytes, rmask, gmask, bmask, amask);

	SDL_Surface *new_surf = SDL_ConvertSurface(this->surface, dummy->format, NULL);
	free();
	this->surface = new_surf;
	this->need_to_free_data = false;*/
	int depth = alpha ? 32 : 24;
	SDL_Surface *new_surf = SDL_CreateRGBSurface(NULL, this->getWidth(), this->getHeight(), depth, rmask, gmask, bmask, amask);
	SDL_BlitSurface(this->surface, NULL, new_surf, NULL);
	free();
	this->surface = new_surf;
	this->need_to_free_data = false;

#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image convert time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image convert total %d\n", total);
#endif
	return true;
}

/*void Image::convertToFormat(Image *image) {
	SDL_Surface *new_surf = SDL_ConvertSurface(this->surface, image->surface->format, NULL);
	free();
	this->surface = new_surf;
}*/

/*bool Image::convertToHiColor(bool alpha) {
int w = this->getWidth();
int h = this->getHeight();
int bpp = this->surface->format->BitsPerPixel;
if( bpp == 32 )
return false;
int bytesperpixel = this->surface->format->BytesPerPixel;
if( bytesperpixel == 4 )
return false;
SDL_LockSurface(this->surface);
int n_bytes = alpha ? 4 : 3;
unsigned char *new_data = new unsigned char[w * h * n_bytes];
for(int cx=0;cx<w;cx++) {
for(int cy=0;cy<h;cy++) {
Uint32 pixel = getpixel(this->surface, cx, cy);
Uint8 r, g, b;
SDL_GetRGB(pixel, this->surface->format, &r, &g, &b);
if( pixel != 0 ) {
printf("");
}
unsigned char *d_ptr = &new_data[ cy * w * n_bytes + cx * n_bytes ];
d_ptr[0] = r;
d_ptr[1] = g;
d_ptr[2] = b;
if( alpha )
d_ptr[3] = 255;
}
}
SDL_UnlockSurface(this->surface);
free();

Uint32 rmask, gmask, bmask, amask;
CreateMask(rmask, gmask, bmask, amask);

this->surface = SDL_CreateRGBSurfaceFrom(new_data, w, h, n_bytes * 8, w * n_bytes, rmask, gmask, bmask, amask);
this->data = new_data;
//this->need_to_free_data = true;

return true;
}*/

void Image::convertToDisplayFormat() {
	SDL_Surface *new_surf = NULL;
	int bpp = this->surface->format->BitsPerPixel;
	if( bpp == 32 && this->surface->format->Amask != 0 )
		new_surf = SDL_DisplayFormatAlpha(this->surface);
	else
		new_surf = SDL_DisplayFormat(this->surface);
	SDL_FreeSurface(this->surface);
	this->surface = new_surf;
}

//Image *Image::createFromBitmap(FIBITMAP *bitmap, Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask) {
//Image *image = new Image();
////image->bitmap = bitmap;
//int w = FreeImage_GetWidth(bitmap);
//int h = FreeImage_GetHeight(bitmap);
//int bpp = FreeImage_GetBPP(bitmap);
//unsigned char *bm_data = (unsigned char *)FreeImage_GetBits(bitmap);
////image->n_colours = FreeImage_GetColorsUsed(image->bitmap);
//int pitch = FreeImage_GetPitch(bitmap);
//image->data = new unsigned char[pitch * h];
//image->need_to_free_data = true;
//for(int i=0;i<h * pitch;i++)
//image->data[i] = bm_data[i];
//
///*Uint32 rmask, gmask, bmask, amask;
//#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
//rmask = 0xff000000;
//gmask = 0x00ff0000;
//bmask = 0x0000ff00;
//amask = 0x000000ff;
//#else
//rmask = 0x000000ff;
//gmask = 0x0000ff00;
//bmask = 0x00ff0000;
//amask = 0xff000000;
//#endif*/
//image->surface = SDL_CreateRGBSurfaceFrom(image->data, w, h, bpp, pitch, rmask, gmask, bmask, amask);
///*RGBQUAD *palette = FreeImage_GetPalette(image->bitmap);
//for(int i=0;i<256;i++) {
//SDL_Color color = { palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue };
//SDL_SetColors(image->surface, &color, i, 1);
//}*/
////FreeImage_Free(bitmap);
//return image;
//}

bool Image::copyPalette(const Image *image) {
	if( this->surface->format->palette == NULL || image->surface->format->palette == NULL )
		return false;

	if( this->surface->format->palette->ncolors != image->surface->format->palette->ncolors )
		return false;

	SDL_SetColors(this->surface, image->surface->format->palette->colors, 0, this->surface->format->palette->ncolors);
	return true;
}

// side-effect: also converts images with < 256 colours to have 256 colours, unless scaling is 1.0
void Image::scale(float sx,float sy) {
	if( sx == 1.0f && sy == 1.0f ) {
		return;
	}
	// only supported for either reducing or englarging the size - this is all we need, and is easier to optimise for performance
	bool enlarging = false;
	if( sx > 1.0f || sy > 1.0f ) {
		// making larger
		ASSERT( sx > 1.0f );
		ASSERT( sy > 1.0f );
		enlarging = true;
	}
#ifdef TIMING
	int time_s = clock();
#endif
	int w = this->getWidth();
	int h = this->getHeight();
	SDL_LockSurface(this->surface);
	unsigned char *src_data = (unsigned char *)this->surface->pixels;
	int bytesperpixel = this->surface->format->BytesPerPixel;
	int new_width = (int)(w * sx);
	int new_height = (int)(h * sy);
	int new_size = (int)(new_width * new_height * bytesperpixel);
	bool is_paletted = this->isPaletted();
	unsigned char *new_data = NULL;
	int *new_data_nonpaletted = NULL;
	int *count = NULL;
	if( is_paletted || enlarging ) {
		new_data = new unsigned char[new_size];
	}
	else {
		new_data_nonpaletted = new int[new_size];
		count = new int[new_size];
		for(int i=0;i<new_size;i++) {
			new_data_nonpaletted[i] = 0;
			count[i] = 0;
		}
	}
	// faster to read in x direction! (caching?)
	if( enlarging ) {
		for(int cy=0;cy<h;cy++) {
			int src_indx = cy * this->surface->pitch;
			for(int cx=0;cx<w;cx++) {
				for(int i=0;i<bytesperpixel;i++, src_indx++) {
					T_ASSERT(src_indx >= 0 && src_indx < this->surface->pitch*h );
					unsigned char pt = src_data[ src_indx ];
					for(int y=0;y<sy;y++) {
						for(int x=0;x<sx;x++) {
							int dx = (int)(cx * sx + x);
							int dy = (int)(cy * sy + y);
							T_ASSERT( dx >= 0 && dx < new_width );
							T_ASSERT( dy >= 0 && dy < new_height );
							int dst_indx = (int)(dy * new_width * bytesperpixel + dx * bytesperpixel + i);
							T_ASSERT(dst_indx >= 0 && dst_indx < new_width*new_height*bytesperpixel );
							new_data[ dst_indx ] = pt;
						}
					}
				}
			}
		}
	}
	else {
		for(int cy=0;cy<h;cy++) {
			int src_indx = cy * this->surface->pitch;
			for(int cx=0;cx<w;cx++) {
				for(int i=0;i<bytesperpixel;i++, src_indx++) {
					T_ASSERT(src_indx >= 0 && src_indx < this->surface->pitch*h );
					unsigned char pt = src_data[ src_indx ];

					int dx = (int)(cx * sx);
					int dy = (int)(cy * sy);
					if( dx >= new_width || dy >= new_height ) {
						// ignore leftover part
						continue;
					}
					T_ASSERT( dx >= 0 && dx < new_width );
					T_ASSERT( dy >= 0 && dy < new_height );
					int dst_indx = (int)(dy * new_width * bytesperpixel + dx * bytesperpixel + i);
					T_ASSERT(dst_indx >= 0 && dst_indx < new_width*new_height*bytesperpixel );
					if( is_paletted )
						new_data[ dst_indx ] = pt;
					else {
						new_data_nonpaletted[ dst_indx ] += pt;
						count[ dst_indx ]++;
					}
				}
			}
		}
	}
	SDL_UnlockSurface(this->surface);

	{
		/*w *= sx;
		h *= sy;*/
		w = (int)(w * sx);
		h = (int)(h * sy);
		int bpp = this->surface->format->BitsPerPixel;
		int pitch = this->surface->format->BytesPerPixel * w;

		/*Uint32 rmask, gmask, bmask;
		#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0x00ff0000;
		gmask = 0x0000ff00;
		bmask = 0x000000ff;
		#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		#endif*/
		Uint32 rmask = this->surface->format->Rmask;
		Uint32 gmask = this->surface->format->Gmask;
		Uint32 bmask = this->surface->format->Bmask;
		Uint32 amask = this->surface->format->Amask;
		if( !(is_paletted || enlarging) ) {
			new_data = new unsigned char[new_size];
			for(int i=0;i<new_size;i++) {
				new_data[i] = (unsigned char)(new_data_nonpaletted[i] / count[i]);
			}
			delete [] new_data_nonpaletted;
			new_data_nonpaletted = NULL;
			delete [] count;
			count = NULL;
		}
		SDL_Surface *new_surf = SDL_CreateRGBSurfaceFrom(new_data, w, h, bpp, pitch, rmask, gmask, bmask, amask);
		if( this->surface->format->palette != NULL ) {
			SDL_SetColors(new_surf, this->surface->format->palette->colors, 0, this->surface->format->palette->ncolors);

			/*SDL_Surface *new_surf2 = SDL_ConvertSurface(new_surf, this->surface->format, NULL);
			SDL_FreeSurface(new_surf);
			new_surf = new_surf2;*/
		}
		free();
		this->surface = new_surf;
		//this->bitmap = new_bitmap;
		this->data = new_data;
		this->need_to_free_data = true;
	}

#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image scale time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image scale total %d\n", total);
#endif
}

bool Image::scaleTo(int n_w) {
	/*if( n_w == this->getWidth() ) {
		this->scale(1, 1); // hacky way of forcing to 8bit
		return true;
	}
	else if( n_w == this->getWidth()*2 ) {
		this->scale(2, 2);
		//this->setScale(2, 2);
		return true;
	}
	else if( n_w == this->getWidth()*4 ) {
		this->scale(4, 4);
		//this->setScale(4, 4);
		return true;
	}
	LOG("unexpected scale size: from %d , %d to width %d\n", this->getWidth(), this->getHeight(), n_w);
	return false;*/
	float scale_factor = ((float)n_w) / (float)this->getWidth();
	this->scale(scale_factor, scale_factor); // nb, still scale if scale_factor==1, as this is a way of converting to 8bit
	return true;
}

/*bool Image::scaleTo(int n_w,int n_h) {
	if( n_w == this->getWidth() && n_h == this->getHeight() ) {
		return true;
	}
	else if( n_w == this->getWidth()*2 && n_h == this->getHeight()*2 ) {
		this->scale(2, 2);
		this->setScale(2, 2);
		return true;
	}
	else if( n_w == this->getWidth()*4 && n_h == this->getHeight()*4 ) {
		this->scale(4, 4);
		this->setScale(4, 4);
		return true;
	}
	LOG("unexpected scale size: from %d , %d to %d , %d\n", this->getWidth(), this->getHeight(), n_w, n_h);
	return false;
}*/

void Image::remap(unsigned char sr,unsigned char sg,unsigned char sb,unsigned char rr,unsigned char rg,unsigned char rb) {
	if( this->surface->format->BitsPerPixel != 24 && this->surface->format->BitsPerPixel != 32 ) {
		return;
	}
	SDL_LockSurface(this->surface);
	int w = getWidth();
	int h = getHeight();
	int bytesperpixel = this->surface->format->BytesPerPixel;
	int pitch = this->surface->pitch;
	// faster to read in x direction! (caching?)
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			/*unsigned char *src_data = (unsigned char *)this->surface->pixels;
			unsigned char *ptr = &src_data[ y * pitch + x * bytesperpixel ];
			if( ptr[0] == sr && ptr[1] == sg && ptr[2] == sb ) {
				ptr[0] = rr;
				ptr[1] = rg;
				ptr[2] = rb;
			}*/
			Uint32 pixel = getpixel(this->surface, x, y);
			Uint8 r = 0, g = 0, b = 0, a = 0;
			SDL_GetRGBA(pixel, this->surface->format, &r, &g, &b, &a);
			if( r == sr && g == sg && b == sb ) {
				pixel = SDL_MapRGBA(surface->format, rr, rg, rb, a);
				putpixel(this->surface, x, y, pixel);
			}
		}
	}

	SDL_UnlockSurface(this->surface);
}

//Image *Image::copy(int x,int y,int w,int h) {
//LOG("Image::copy(%d,%d,%d,%d)\n",x,y,w,h);
//x *= scale_x;
//y *= scale_y;
//w *= scale_x;
//h *= scale_y;
//FIBITMAP *new_bitmap = FreeImage_Allocate(w, h, this->surface->format->BitsPerPixel);
//SDL_LockSurface(this->surface);
//unsigned char *src_data = (unsigned char *)this->surface->pixels;
//unsigned char *data = (unsigned char *)FreeImage_GetBits(new_bitmap);
//int bytesperpixel = this->surface->format->BytesPerPixel;
//for(int cx=0;cx<w;cx++) {
//for(int cy=0;cy<h;cy++) {
//for(int i=0;i<bytesperpixel;i++) {
//data[ cy * FreeImage_GetPitch(new_bitmap) + cx * bytesperpixel + i ] = src_data[ ( y + cy ) * this->surface->pitch + ( x + cx ) * bytesperpixel + i ];
//}
//}
//}
//SDL_UnlockSurface(this->surface);
//Image *copy_image = createFromBitmap(new_bitmap, this->surface->format->Rmask, this->surface->format->Gmask, this->surface->format->Bmask, this->surface->format->Amask);
//copy_image->copyPalette(this);
//copy_image->scale_x = scale_x;
//copy_image->scale_y = scale_y;
//return copy_image;
//}

void Image::reshadeRGB(int from, bool to_r, bool to_g, bool to_b) {
	ASSERT(from >= 0 && from < 3);
	if( this->surface->format->BitsPerPixel != 24 && this->surface->format->BitsPerPixel != 32 ) {
		return;
	}
	bool to[3] = {to_r, to_g, to_b};
	SDL_LockSurface(this->surface);
	int w = getWidth();
	int h = getHeight();
	int bytesperpixel = this->surface->format->BytesPerPixel;
	int pitch = this->surface->pitch;
	// faster to read in x direction! (caching?)
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			Uint32 pixel = getpixel(this->surface, x, y);
			Uint8 rgba[] = {0, 0, 0, 0};
			SDL_GetRGBA(pixel, this->surface->format, &rgba[0], &rgba[1], &rgba[2], &rgba[3]);
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
			pixel = SDL_MapRGBA(surface->format, rgba[0], rgba[1], rgba[2], rgba[3]);
			putpixel(this->surface, x, y, pixel);
		}
	}

	SDL_UnlockSurface(this->surface);
}

void Image::brighten(float sr, float sg, float sb) {
#ifdef TIMING
	int time_s = clock();
#endif
	if( this->surface->format->BitsPerPixel != 24 && this->surface->format->BitsPerPixel != 32 ) {
		return;
	}
	float scale[3] = {sr, sg, sb};
	SDL_LockSurface(this->surface);
	int w = getWidth();
	int h = getHeight();
	int bytesperpixel = this->surface->format->BytesPerPixel;
	int pitch = this->surface->pitch;
	// faster to read in x direction! (caching?)
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			Uint32 pixel = getpixel(this->surface, x, y);
			Uint8 rgba[] = {0, 0, 0, 0};
			SDL_GetRGBA(pixel, this->surface->format, &rgba[0], &rgba[1], &rgba[2], &rgba[3]);
			/*if( mask && rgba[0] == mask_r && rgba[1] == mask_g && rgba[2] == mask_b ) {
				continue;
			}*/
			for(int j=0;j<3;j++) {
				float col = (float)rgba[j];
				col *= scale[j];
				if( col < 0 )
					col = 0;
				else if( col > 255 )
					col = 255;
				rgba[j] = (unsigned char)col;
			}
			pixel = SDL_MapRGBA(surface->format, rgba[0], rgba[1], rgba[2], rgba[3]);
			putpixel(this->surface, x, y, pixel);
		}
	}
	SDL_UnlockSurface(this->surface);
#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image brighten time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image brighten total %d\n", total);
#endif
}

void Image::fillRect(int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
	int col = SDL_MapRGB(this->surface->format, r, g, b);
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	SDL_FillRect(this->surface, &rect, col);
}

/*void Image::print(const char *inStr) const {
	printf("\n--------------------\nImage %s\n",inStr);
	printf("scale_x %d, scale_y %d\n",this->scale_x, this->scale_y);
	//printf("mask_r %d, mask_g %d, mask_b %d\n",this->mask_r,this->mask_g, this->mask_b);
	printf("format %d\n",this->surface->format);
	int w = this->surface->w;
	int h = this->surface->h;
	SDL_LockSurface(this->surface);
	unsigned char *src_data = (unsigned char *)this->surface->pixels;
	int bytesperpixel = this->surface->format->BytesPerPixel;
	for(int cx=0;cx<w;cx++) {
		for(int cy=0;cy<h;cy++) {
			for(int i=0;i<bytesperpixel;i++) {
				printf("%02x ", src_data[ ( cy ) * this->surface->pitch + ( cx ) * bytesperpixel + i ]);
			}
		}
		printf("\n");
	}
	SDL_UnlockSurface(this->surface);
}*/

/*void Image::flipX() {
	LOG("Image::flipX()\n");

	int w = this->getWidth();
	int h = this->getHeight();
	unsigned char *data = (unsigned char *)this->surface->pixels;
	SDL_LockSurface(this->surface);
	int bytesperpixel = this->surface->format->BytesPerPixel;
	for(int cx=0;cx<w/2;cx++) {
		int f_cx = w - 1 - cx;
		for(int cy=0;cy<h;cy++) {
			for(int i=0;i<bytesperpixel;i++) {
				unsigned char dummy = data[ cy * this->surface->pitch + cx * bytesperpixel + i ];
				data[ cy * this->surface->pitch + cx * bytesperpixel + i ] = data[ cy * this->surface->pitch + f_cx * bytesperpixel + i ];
				data[ cy * this->surface->pitch + f_cx * bytesperpixel + i ] = dummy;
				//dst_data[ cy * copy_image->surface->pitch + cx * bytesperpixel + i ] = src_data[ ( y + cy ) * this->surface->pitch + ( x + cx ) * bytesperpixel + i ];
			}
		}
	}
	SDL_UnlockSurface(this->surface);
}*/

Image *Image::copy(int x, int y, int w, int h) const {
	//LOG("Image::copy(%d,%d,%d,%d)\n",x,y,w,h);
#ifdef TIMING
	int time_s = clock();
#endif

	/*x *= scale_x;
	y *= scale_y;
	w *= scale_x;
	h *= scale_y;*/
	x = (int)(x * scale_x);
	y = (int)(y * scale_y);
	w = (int)(w * scale_x);
	h = (int)(h * scale_y);

	Image *copy_image = NULL;
	{
		SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, this->surface->format->BitsPerPixel, this->surface->format->Rmask, this->surface->format->Gmask, this->surface->format->Bmask, this->surface->format->Amask);
		copy_image = new Image();
		copy_image->surface = surface;
		copy_image->data = (unsigned char *)copy_image->surface->pixels;
		copy_image->need_to_free_data = false;
	}

	unsigned char *src_data = (unsigned char *)this->surface->pixels;
	unsigned char *dst_data = (unsigned char *)copy_image->surface->pixels;
	SDL_LockSurface(this->surface);
	int bytesperpixel = this->surface->format->BytesPerPixel;
	// faster to read in x direction! (caching?)
	for(int cy=0;cy<h;cy++) {
		for(int cx=0;cx<w;cx++) {
			for(int i=0;i<bytesperpixel;i++) {
				int src_indx = ( y + cy ) * this->surface->pitch + ( x + cx ) * bytesperpixel + i;
				int dst_indx = cy * copy_image->surface->pitch + cx * bytesperpixel + i;
				ASSERT( src_indx >= 0 && src_indx < this->surface->pitch * this->surface->h * bytesperpixel );
				ASSERT( dst_indx >= 0 && dst_indx < copy_image->surface->pitch * copy_image->surface->h * copy_image->surface->format->BytesPerPixel );
				dst_data[ dst_indx ] = src_data[ src_indx ];
			}
		}
	}
	SDL_UnlockSurface(this->surface);

	copy_image->copyPalette(this);
	copy_image->scale_x = scale_x;
	copy_image->scale_y = scale_y;

#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image copy time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image copy total %d\n", total);
#endif

	return copy_image;
}

Image *Image::loadImage(const char *filename) {
#ifdef TIMING
	int time_s = clock();
#endif
	LOG("Image::loadImage(\"%s\")\n",filename);
	//image->surface=IMG_LoadLBM_RW(SDL_RWFromFile(filename, "rb"));
	SDL_RWops *src = SDL_RWFromFile(filename, "rb");
	if( src == NULL ) {
		LOG("SDL_RWFromFile failed: %s\n", SDL_GetError());
		return NULL;
	}
	Image *image = new Image();
	image->surface = IMG_Load_RW(src, 1);
	if( image->surface == NULL ) {
		LOG("IMG_Load_RW failed: %s\n", IMG_GetError());
		delete image;
		image = NULL;
	}
#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image load time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image load total %d\n", total);
#endif

	// Workaround: every image is loaded as 32 bit, but on <32 bit images, the alpha mask is not set
	// so we have to set it manually for transparency to work:
#if defined(__APPLE__) && defined(__MACH__)
	if (image->surface->format->BitsPerPixel == 32 && image->surface->format->Amask == 0)
	{
		Uint32 rmask, gmask, bmask, amask;
		CreateMask(rmask, gmask, bmask, amask);
		image->surface->format->Amask = amask;
	}
#endif

	return image;
}


//Image *Image::loadImage(char *filename) {
//LOG("Image::loadImage(\"%s\")\n",filename);
//Image *image = NULL;
////FREE_IMAGE_FORMAT type = FreeImage_GetFileType(filename, 0);
////FREE_IMAGE_FORMAT type = FreeImage_GetFileTypeFromExt("iff",0);
////LOG("Got fileformat %d, should be %d.\n", type, FIF_IFF);
//FREE_IMAGE_FORMAT type = FIF_GIF;
//FIBITMAP *bitmap = FreeImage_Load(type, filename,0);
//LOG("Got result %d.\n", bitmap);
//RGBQUAD *palette = FreeImage_GetPalette(bitmap);
///*palette[0].rgbRed = 255;
//palette[0].rgbGreen = 0;
//palette[0].rgbBlue = 255;*/
///*if( bitmap != NULL && FreeImage_GetBPP(bitmap) != 24 ) {
//bitmap = FreeImage_ConvertTo24Bits(bitmap);
//}*/
//
///*if( image->bitmap != NULL ) {
//image->bitmap = FreeImage_ConvertTo8Bits(image->bitmap);
//}*/
//
//if( bitmap != NULL ) {
//LOG("In if.\n");
//image = new Image();
////image->bitmap = bitmap;
//int w = FreeImage_GetWidth(bitmap);
//int h = FreeImage_GetHeight(bitmap);
//int bpp = FreeImage_GetBPP(bitmap);
//palette = FreeImage_GetPalette(bitmap);
//unsigned char *fi_data = (unsigned char *)FreeImage_GetBits(bitmap);
///*unsigned char *data = (unsigned char *)FreeImage_GetBits(image->bitmap);
//image->n_colours = FreeImage_GetColorsUsed(image->bitmap);*/
//int pitch = FreeImage_GetPitch(bitmap);
//int bytesperpixel = bpp / 8;
//image->data = new unsigned char[h * pitch];
//for(int i=0;i<h * pitch;i++)
//image->data[i] = fi_data[i];
//
//// flip
//for(int x=0;x<w;x++) {
//for(int y=0;y<h/2;y++) {
//int y2 = h - 1 - y;
//for(int i=0;i<bytesperpixel;i++) {
//unsigned char dummy = image->data[ y * pitch + x * bytesperpixel + i ];
//image->data[ y * pitch + x * bytesperpixel + i ] = image->data[ y2 * pitch + x * bytesperpixel + i ];
//image->data[ y2 * pitch + x * bytesperpixel + i ] = dummy;
//}
//}
//}
//
//Uint32 rmask, gmask, bmask, amask;
//#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
//rmask = 0xff000000;
//gmask = 0x00ff0000;
//bmask = 0x0000ff00;
//amask = 0x000000ff;
//#else
//rmask = 0x000000ff;
//gmask = 0x0000ff00;
//bmask = 0x00ff0000;
//amask = 0xff000000;
//#endif
//if( palette == NULL ) {
//rmask = FreeImage_GetRedMask(bitmap);
//gmask = FreeImage_GetGreenMask(bitmap);
//bmask = FreeImage_GetBlueMask(bitmap);
//}
//amask = 0;
//image->surface = SDL_CreateRGBSurfaceFrom(image->data, w, h, bpp, pitch, rmask, gmask, bmask, amask);
//image->need_to_free_data = true;
//if( palette != NULL ) {
//for(int i=0;i<256;i++) {
//SDL_Color color = { palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue };
//SDL_SetColors(image->surface, &color, i, 1);
//}
//}
//}
//else {
//LOG("Nope.\n");
//delete image;
//image = NULL;
//}
//return image;
//}

Image *Image::createBlankImage(int width,int height, int bpp) {
	Uint32 rmask, gmask, bmask, amask;
	CreateMask(rmask, gmask, bmask, amask);

	SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bpp, rmask, gmask, bmask, amask);

	Image *image = new Image();
	image->surface = surface;
	image->data = (unsigned char *)image->surface->pixels;
	image->need_to_free_data = false;

	return image;
}

/*Image *Image::createNoise(int w,int h,float scale_u,float scale_v,const unsigned char filter_max[3],const unsigned char filter_min[3],NOISEMODE_t noisemode,int n_iterations) {
	unsigned char *data = new unsigned char[w*h*4];
	float fvec[2];
	for(int y=0,i=0;y<h;y++) {
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
			data[i++] = (unsigned char)((filter_max[0] - filter_min[0]) * h + filter_min[0]);
			data[i++] = (unsigned char)((filter_max[1] - filter_min[1]) * h + filter_min[1]);
			data[i++] = (unsigned char)((filter_max[2] - filter_min[2]) * h + filter_min[2]);
			data[i++] = 255;
		}
	}

	Uint32 rmask, gmask, bmask, amask;
	CreateMask(rmask, gmask, bmask, amask);

	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(data, w, h, 32, w*4, rmask, gmask, bmask, amask);

	Image *image = new Image();
	image->surface = surface;
	image->data = data;
	image->need_to_free_data = true;
	return image;
}*/

Image *Image::createNoise(int w,int h,float scale_u,float scale_v,const unsigned char filter_max[3],const unsigned char filter_min[3],NOISEMODE_t noisemode,int n_iterations) {
	Image *image = Image::createBlankImage(w, h, 32);
	SDL_LockSurface(image->surface);
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
			Uint8 r = (Uint8)((filter_max[0] - filter_min[0]) * h + filter_min[0]);
			Uint8 g = (Uint8)((filter_max[1] - filter_min[1]) * h + filter_min[1]);
			Uint8 b = (Uint8)((filter_max[2] - filter_min[2]) * h + filter_min[2]);
			Uint8 a = 255;
			Uint32 pixel = SDL_MapRGBA(image->surface->format, r, g, b, a);
			putpixel(image->surface, x, y, pixel);
		}
	}
	SDL_UnlockSurface(image->surface);

	return image;
}

/*Image * Image::createRadial(int w,int h,float alpha_scale) {
	unsigned char *data = new unsigned char[w*h*4];
	int radius = min(w/2, h/2);
	for(int y=0,i=0;y<h;y++) {
		int dy = abs(y - h/2);
		for(int x=0;x<w;x++) {
			int dx = abs(x - w/2);
			float dist = sqrt((float)(dx*dx + dy*dy));
			dist /= (float)radius;
			if( dist >= 1.0f )
				dist = 1.0f;
			dist = 1.0f - dist;
			dist *= alpha_scale;
			unsigned char v = (int)(255.0f * dist);
			data[i++] = 255;
			data[i++] = 255;
			data[i++] = 255;
			data[i++] = v;
		}
	}

	Uint32 rmask, gmask, bmask, amask;
	CreateMask(rmask, gmask, bmask, amask);

	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(data, w, h, 32, w*4, rmask, gmask, bmask, amask);

	Image *image = new Image();
	image->surface = surface;
	image->data = data;
	image->need_to_free_data = true;
	return image;
}*/

Image * Image::createRadial(int w,int h,float alpha_scale) {
	Image *image = Image::createBlankImage(w, h, 32);
	SDL_LockSurface(image->surface);
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
			unsigned char v = (int)(255.0f * dist);
			Uint8 r = 255;
			Uint8 g = 255;
			Uint8 b = 255;
			Uint8 a = v;
			Uint32 pixel = SDL_MapRGBA(image->surface->format, r, g, b, a);
			putpixel(image->surface, x, y, pixel);
		}
	}

	SDL_UnlockSurface(image->surface);
	return image;
}

void Image::setGraphicsOutput(SDL_Surface *dest_surf) {
	Image::dest_surf = dest_surf;
}

void Image::writeNumbers(int x,int y,Image *images[10],int number,Justify justify,bool mask) {
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
}

void Image::writeMixedCase(int x,int y,Image *large[26],Image *little[26],Image *numbers[10],const char *text,Justify justify,bool mask) {
	int len = strlen(text);
	int n_lines = 0;
	int max_wid = 0;
	textLines(&n_lines, &max_wid, text);
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
			ch = 'O'; // hack for 0 (we don't spell it O, due to alphabetical ordering)
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

void Image::smooth() {
	if( this->surface->format->BitsPerPixel != 24 && this->surface->format->BitsPerPixel != 32 ) {
		return;
	}
	int w = getWidth();
	int h = getHeight();
	unsigned char *src_data = (unsigned char *)this->surface->pixels;
	int bytesperpixel = this->surface->format->BytesPerPixel;
	int pitch = this->surface->pitch;
	unsigned char *new_data = new unsigned char[w * h * bytesperpixel];

	SDL_LockSurface(this->surface);
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			for(int i=0;i<bytesperpixel;i++) {
				Uint32 col = 0;
				if(	x > 0 && x < w-1 && y > 0 && y < h-1 ) {
						Uint32 sq[9];
						int indx = 0;
						for(int sx=x-1;sx<=x+1;sx++) {
							for(int sy=y-1;sy<=y+1;sy++) {
								sq[indx++] = src_data[ sy * pitch + sx * bytesperpixel + i ];
							}
						}
						col = ( sq[0] + 2 * sq[1] + sq[2] + 2 * sq[3] + 4 * sq[4] + 2 * sq[5] + sq[6] + 2 * sq[7] + sq[8] ) / 16;
						//col = ( sq[1] + sq[3] + sq[5] + sq[7] + 12 * sq[4] ) / 16;
				}
				else
					col = src_data[ y * pitch + x * bytesperpixel + i ];
				new_data[ y * w * bytesperpixel + x * bytesperpixel + i] = (unsigned char)col;
			}
		}
	}
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			for(int i=0;i<bytesperpixel;i++) {
				src_data[ y * pitch + x * bytesperpixel + i ] = new_data[ y * w * bytesperpixel + x * bytesperpixel + i];
			}
		}
	}
	delete [] new_data;
	SDL_UnlockSurface(this->surface);
}
