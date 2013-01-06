#pragma once

/** Class for managing the display. There shouldn't be anything specific to a
*   particlar game in this class.
*/

struct SDL_Surface;

namespace Gigalomania {
	class Screen {
		SDL_Surface *surface;

	public:
		Screen();
		~Screen();

		static bool canOpenFullscreen(int width, int height);
		bool open(int screen_width, int screen_height, bool fullscreen);
		void setTitle(const char *title);
		void clear();
		void refresh();
		/*const SDL_Surface *getSurface() const {
			return this->surface;
		}
		SDL_Surface *getSurface() {
			return this->surface;
		}*/
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
	/*void getMousePressed(bool *m_left, bool *m_middle, bool *m_right) const {
		*m_left = m_left_pressed;
		*m_middle = m_middle_pressed;
		*m_right = m_right_pressed;
	}*/
};
