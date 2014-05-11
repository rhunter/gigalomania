#pragma once

/** Class for managing the display. There shouldn't be anything specific to a
*   particlar game in this class.
*/

namespace Gigalomania {
	class Screen {
#if SDL_MAJOR_VERSION == 1
		SDL_Surface *surface;
#else
		SDL_Window *sdlWindow;
		SDL_Renderer *sdlRenderer;
		int width, height; // this stores the logical size rather than the window size
#endif
		int m_pos_x;
		int m_pos_y;
		bool m_down_left;
		bool m_down_middle;
		bool m_down_right;

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
		// not supported with SDL 1.2
#else
		void fillRectWithAlpha(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);
		void convertWindowToLogical(int *m_x, int *m_y);
		void getWindowSize(int *window_width, int *window_height);
#endif
		void setMousePos(int x, int y) {
			this->m_pos_x = x;
			this->m_pos_y = y;
		}
		void setMouseLeft(bool down) {
			this->m_down_left = down;
		}
		void setMouseMiddle(bool down) {
			this->m_down_middle = down;
		}
		void setMouseRight(bool down) {
			this->m_down_right = down;
		}
		void getMouseCoords(int *m_x, int *m_y);
		bool getMouseState(int *m_x, int *m_y, bool *m_left, bool *m_middle, bool *m_right);
	};
}

class Application {
	bool quit;
	bool blank_mouse;

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
	bool isBlankMouse() const {
		return this->blank_mouse;
	}
};
