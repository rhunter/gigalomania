#pragma once

/** Class for managing the display. There shouldn't be anything specific to a
*   particlar game in this class.
*/

#if defined(__linux) || defined(__MORPHOS__)
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
		int width, height;
#endif

	public:
		Screen();
		~Screen();

		static bool canOpenFullscreen(int width, int height);
		bool open(int screen_width, int screen_height, bool fullscreen);
		void setTitle(const char *title);
		void clear();
		void refresh();
		int getWidth() const;
		int getHeight() const;
		void fillRect(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b);
		void fillRectWithAlpha(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);

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
