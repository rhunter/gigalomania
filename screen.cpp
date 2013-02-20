//---------------------------------------------------------------------------
#include "stdafx.h"

#if defined(__linux) || defined(__MORPHOS__)
#include <SDL/SDL.h>
#else
#include <sdl.h>
#endif

#include <cassert>
#include <ctime>

#include "screen.h"
#include "sound.h"
#include "game.h"
#include "utils.h"
#include "gamestate.h"
#include "image.h"
#include "sector.h"
#include "panel.h"
#include "gui.h"
#include "player.h"

//---------------------------------------------------------------------------

Screen::Screen() {
}

Screen::~Screen() {
}

bool Screen::canOpenFullscreen(int width, int height) {
	if( SDL_VideoModeOK(width, height, 32, SDL_HWSURFACE|SDL_HWPALETTE|SDL_FULLSCREEN) != 0 )
		return true;
	return false;
}

bool Screen::open(int screen_width, int screen_height, bool fullscreen) {
	LOG("Screen::open(%d, %d, %s)\n", screen_width, screen_height, fullscreen?"fullscreen":"windowed");
	if( fullscreen )
		surface = SDL_SetVideoMode(screen_width, screen_height, 32, SDL_HWSURFACE|SDL_HWPALETTE|SDL_FULLSCREEN);
	else
		surface = SDL_SetVideoMode(screen_width, screen_height, 32, SDL_HWSURFACE|SDL_HWPALETTE);
	if( surface == NULL ) {
		LOG("failed to open screen at this resolution\n");
		return false;
	}
	LOG("screen opened ok\n");

	//SDL_Palette *palette = background->getSDLSurface()->format->palette;
	//SDL_SetColors(surface, palette->colors, 0, palette->ncolors);

	if( mobile_ui ) {
		SDL_ShowCursor(false); // comment out to test with system cursor, for testing purposes when ran on non-touchscreen devices
	}
	else {
		//SDL_ShowCursor(false);
		// Set up a blank cursor
		// We use this hacky way due to a bug in SDL_ShowCursor(false), where when alt-tabbing out of full screen mode in Windows, the mouse pointer is
		// trapped in the top left corner. See 
		unsigned char data[1] = "";
		unsigned char mask[1] = "";
		SDL_Cursor *cursor = SDL_CreateCursor(data, mask, 8, 1, 0, 0);
		SDL_Cursor *old_cursor = SDL_GetCursor();
		SDL_SetCursor(cursor);
		SDL_FreeCursor(old_cursor);
	}

	Image::setGraphicsOutput(surface);

	//this->reset();
	return true;
}

void Screen::setTitle(const char *title) {
	SDL_WM_SetCaption(title, "");
}

void Screen::clear() {
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = getWidth();
	rect.h = getHeight();
	SDL_FillRect(surface, &rect, 0);
}

void Screen::refresh() {
	SDL_Flip(this->surface);
}

int Screen::getWidth() const {
	return this->surface->w;
}

int Screen::getHeight() const {
	return this->surface->h;
}

void Screen::fillRect(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b) {
	Uint32 col = SDL_MapRGB(surface->format, r, g, b);
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	SDL_FillRect(surface, &rect, col);
}

void Screen::fillRectWithAlpha(short x, short y, short w, short h, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha) {
	// not supported with SDL!
	this->fillRect(x, y, w, h, r, g, b);
}

void Screen::getMouseCoords(int *m_x, int *m_y) {
	SDL_GetMouseState(m_x, m_y);
}

bool Screen::getMouseState(int *m_x, int *m_y, bool *m_left, bool *m_middle, bool *m_right) {
	int m_b = SDL_GetMouseState(m_x, m_y);
	*m_left = ( m_b & SDL_BUTTON(1) ) != 0;
	*m_middle = ( m_b & SDL_BUTTON(2) ) != 0;
	*m_right = ( m_b & SDL_BUTTON(3) ) != 0;
	return ( *m_left || *m_middle || *m_right );
}

Application::Application() : quit(false)/*, m_left_pressed(false), m_middle_pressed(false), m_right_pressed(false)*/ {
}

Application::~Application() {
	SDL_Quit();
}

bool Application::init() {
#ifndef __MORPHOS__
	putenv("SDL_VIDEO_CENTERED=0,0");
#endif
	if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) == -1 ) {
		LOG("SDL_Init failed\n");
		return false;
	}
	return true;
}

int Application::getTicks() const {
	return SDL_GetTicks();
}

void Application::delay(int time) {
	SDL_Delay(time);
}

const int TICK_INTERVAL = 25; // 40 fps max
static int next_time = 0;

void wait() {
	//int now = SDL_GetTicks();
	int now = application->getTicks();
	int res = 0;
	if(next_time > now)
		res = next_time - now;
	//SDL_Delay(res);
	application->delay(res);
	next_time = now + res + TICK_INTERVAL;
}

void Application::runMainLoop() {
	//Uint32 elapsed_time = SDL_GetTicks();
	int elapsed_time = application->getTicks();

	//lastmouseclick_time = 0;

	SDL_Event event;
	quit = false;
	const bool print_fps = false;
	int last_fps_time = clock();
	const int fps_frames_c = 50;
	//const int fps_frames_c = 1;
	int frames = 0;
	while(!quit) {
		if( print_fps && frames == fps_frames_c ) {
			int new_fps_time = clock();
			float t = ((float)(new_fps_time - last_fps_time)) / (float)CLOCKS_PER_SEC;
			float fps = fps_frames_c / t;
			LOG("FPS: %f\n", fps);
			frames = 0;
			last_fps_time = new_fps_time;
		}
		frames++;

		updateSound();

		// draw screen
		/*if( !paused )
			gamestate->draw();*/
		drawGame();

		/* wait() to avoid 100% CPU - it's debatable whether we should do this,
		 * due to risk of SDL_Delay waiting too long, but since Gigalomania
		 * doesn't really need high frame rate, might as well avoid using full
		 * CPU.
		 */
		wait();

		//Uint32 new_time = SDL_GetTicks();
		int new_time = application->getTicks();
		if( !paused ) {
			updateTime(new_time - elapsed_time);
		}
		else
			updateTime(0);
		elapsed_time = new_time;

		// user input
		//bool handled_mouseclick_this_frame = false;
		//if( SDL_PollEvent(&event) == 1 ) {
		while( SDL_PollEvent(&event) == 1 ) {
			//bool playing_game_quit = false;
			switch (event.type) {
			case SDL_QUIT:
				/*if( gameStateID == GAMESTATEID_PLAYING ) {
					static_cast<PlayingGameState *>(gamestate)->requestQuit();
				}
				else
					quit = true;*/
				// same as pressing escape
				keypressEscape();
				break;
			case SDL_KEYDOWN:
				{
					SDL_keysym key = event.key.keysym;
					if( key.sym == SDLK_ESCAPE ) {
						keypressEscape();
					}
					else if( key.sym == SDLK_p ) {
						keypressP();
					}
					else if( key.sym == SDLK_q ) {
						keypressQ();
					}
					break;
				}
			case SDL_MOUSEBUTTONDOWN:
				{
					bool m_left = false, m_middle = false, m_right = false;
					Uint8 button = event.button.button;
					if( button == SDL_BUTTON_LEFT ) {
						m_left = true;
					}
					else if( button == SDL_BUTTON_MIDDLE ) {
						m_middle = true;
					}
					else if( button == SDL_BUTTON_RIGHT ) {
						m_right = true;
					}

					if( paused ) {
						// click automatically unpaused (needed to work without keyboard!)
						paused = false;
					}
					else if( m_left || m_middle || m_right ) {
						int m_x = 0, m_y = 0;
						screen->getMouseCoords(&m_x, &m_y);
						/*
						// also check state, to allow for multiple buttons held down
						// UPDATE: not done, as this doesn't work well on Linux - best to avoid two-mouse-button actions altogether!
						Uint8 m_b = SDL_GetMouseState(&m_x,&m_y);
						if( ( m_b & SDL_BUTTON(1) ) != 0 )
							m_left = true;
						if( ( m_b & SDL_BUTTON(2) ) != 0 )
							m_middle = true;
						if( ( m_b & SDL_BUTTON(3) ) != 0 )
							m_right = true;
						*/
						mouseClick(m_x, m_y, m_left, m_middle, m_right, true);
					}

					/*if( m_left )
						m_left_pressed = true;
					if( m_middle )
						m_middle_pressed = true;
					if( m_right )
						m_right_pressed = true;*/
					break;
				}
			/*case SDL_MOUSEBUTTONUP:
				{
					bool m_left = false, m_middle = false, m_right = false;
					Uint8 button = event.button.button;
					if( button == SDL_BUTTON_LEFT ) {
						m_left = true;
					}
					else if( button == SDL_BUTTON_MIDDLE ) {
						m_middle = true;
					}
					else if( button == SDL_BUTTON_RIGHT ) {
						m_right = true;
					}
					if( m_left )
						m_left_pressed = false;
					if( m_middle )
						m_middle_pressed = false;
					if( m_right )
						m_right_pressed = false;
					break;
				}*/
			case SDL_ACTIVEEVENT:
				if( (event.active.state & SDL_APPINPUTFOCUS) != 0 || (event.active.state & SDL_APPACTIVE) != 0 ) {
					if( event.active.gain == 1 ) {
						// activate
					}
					else if( event.active.gain == 0 ) {
						// inactive
						keypressP(); // automatically pause when application goes inactive
					}
				}
				break;
			}
		}
		SDL_PumpEvents();

		updateGame();
	}
}
