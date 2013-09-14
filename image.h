#pragma once

/** Class for loading and handling images.
*/

#include "resources.h"

using std::string;

struct SDL_Surface;

namespace Gigalomania {
	class Image : public TrackedObject {
		unsigned char *data;
		bool need_to_free_data;
		SDL_Surface *surface;
		static SDL_Surface *dest_surf;
		float scale_x, scale_y;

		Image();

		void free();

	public:
		virtual ~Image();
		virtual const char *getClass() const { return "CLASS_IMAGE"; }

		void draw(int x, int y,bool mask) const;
		void drawWithAlpha(int x, int y, unsigned char alpha) const;
		int getWidth() const;
		int getHeight() const;
		int getScaledWidth() const {
			return (int)(this->getWidth() / scale_x);
		}
		int getScaledHeight() const {
			return (int)(this->getHeight() / scale_y);
		}
		void convertToDisplayFormat();
		bool copyPalette(const Image *image);
		float getScaleX() const {
			return scale_x;
		}
		float getScaleY() const {
			return scale_y;
		}
		void setScale(float scale_x,float scale_y);
		bool scaleTo(int n_w);
		void scale(float sx,float sy);
		void remap(unsigned char sr,unsigned char sg,unsigned char sb,unsigned char rr,unsigned char rg,unsigned char rb);
		void reshadeRGB(int from, bool to_r, bool to_g, bool to_b);
		void brighten(float sr, float sg, float sb);
		void fillRect(int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b);
		//void flipX();
		Image *copy(int x,int y,int w,int h) const;
		Image *copy() const {
			// need to used scaled width/height, to counteract scaling in copy()
			return this->copy(0, 0, getScaledWidth(), getScaledHeight());
		}
		//void print(const char *inStr) const;
		bool isPaletted() const;
		int getNColors() const;
		unsigned char getPixelIndex(int x,int y) const;
		bool setPixelIndex(int x,int y,unsigned char c);
		bool setColor(int index,unsigned char r,unsigned char g,unsigned char b);
		bool createAlphaForColor(bool mask, unsigned char mr, unsigned char mg, unsigned char mb, unsigned char ar, unsigned char ag, unsigned char ab, unsigned char alpha);
		void scaleAlpha(float scale);
		bool convertToHiColor(bool alpha);
		void smooth();

		static Image * loadImage(const char *filename);
		static Image * loadImage(string filename) {
			return loadImage(filename.c_str());
		}
		static Image * createBlankImage(int width,int height, int bpp);
		enum NOISEMODE_t {
			NOISEMODE_PERLIN = 0,
			NOISEMODE_SMOKE = 1,
			NOISEMODE_PATCHY = 2,
			NOISEMODE_MARBLE = 3,
			NOISEMODE_WOOD = 4,
			NOISEMODE_CLOUDS = 5
		};
		static Image * createNoise(int w,int h,float scale_u,float scale_v,const unsigned char filter_max[3],const unsigned char filter_min[3],NOISEMODE_t noisemode,int n_iterations);
		static Image * createRadial(int w,int h,float alpha_scale);

		enum Justify {
			JUSTIFY_LEFT = 0,
			JUSTIFY_CENTRE = 1,
			JUSTIFY_RIGHT = 2,
		};
		static void writeNumbers(int x,int y,Image *images[10],int number,Justify justify,bool mask);
		static void write(int x,int y,Image *images[26],const char *text,Justify justify,bool mask);
		static void writeMixedCase(int x,int y,Image *large[26],Image *little[26],Image *numbers[10],const char *text,Justify justify,bool mask);

		// SDL specific
		static void setGraphicsOutput(SDL_Surface *dest_surf);
	};
}
