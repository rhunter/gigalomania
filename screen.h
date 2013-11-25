#pragma once

/** Class for managing the display. There shouldn't be anything specific to a
*   particlar game in this class.
*/

#if defined(__ANDROID__)
#include <sdl.h>
#elif defined(__linux) || defined(__MORPHOS__)
#include <SDL/SDL.h>
#else
#include <sdl.h>
#endif

namespace Gigalomania {
	class Screen {
#if SDL_MAJOR_VERSION == 1
		SDL_Surface *surface;
#else
		SDL_Window *sdlWindow;
		SDL_Renderer *sdlRenderer;
		int width, height; // this stores the logical size rather than the window size

		void convertWindowToLogical(int *m_x, int *m_y);
#endif

	public:
		Screen();
		~Screen();

		static bool canOpenFullscreen(int width, int height);
		bool open(int screen_width, int screen_height, bool fullscreen);
#if SDL_MAJOR_VERSION == 1
#else
		void setLogicalSize(int width, int height, bool smooth);
#endif
		void setTitle(const char *title);
		void clear();
		void refresh();
		// in SDL2, these return the logical size rather than the window size
		int getWidth() const;
		int getHeight() const;
		void fillRect(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b);
#if SDL_MAJOR_VERSION == 1
		// not supported with SDL 1.2 (as SDL_FillRect can't do blending)!
#else
		void fillRectWithAlpha(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);
#endif
		void getMouseCoords(int *m_x, int *m_y);
		bool getMouseState(int *m_x, int *m_y, bool *m_left, bool *m_middle, bool *m_right);
	};
}

class Application {
	bool quit;

public:
	Application();
	~Application();

	bool init();

	int getTicks() const;
	void delay(int time);
	void runMainLoop();
	void setQuit() {
		quit = true;
	}
};
