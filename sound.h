#pragma once

/** Handles sound samples and music.
*/

#include <string>
using std::string;

#include "resources.h"

#if defined(__ANDROID__)
#include <sdl_mixer.h>
#elif defined(__linux)
#include <SDL2/SDL_mixer.h>
#else
#include <SDL_mixer.h>
#endif

bool initSound();
void updateSound();
void freeSound();

namespace Gigalomania {
	class Sample : public TrackedObject {
		bool is_music;
		Mix_Music *music;
		Mix_Chunk *chunk;
		int channel;

		string text;

		Sample(bool is_music, Mix_Music *music, Mix_Chunk *chunk) : is_music(is_music), music(music), chunk(chunk) {
			channel = -1;
		}
	public:
		Sample() : is_music(false), music(NULL), chunk(NULL) {
			// create dummy Sample
		}
		virtual ~Sample();
		virtual const char *getClass() const { return "CLASS_SAMPLE"; }
		void play(int ch, int loops);
		//bool isPlaying() const;
		void fadeOut(int duration_ms);
		void setVolume(float volume);
		void setText(const char *text) {
			this->text = text;
		}

		static void pauseMusic();
		static void unpauseMusic();

		static Sample *loadSample(const char *filename, bool iff = false);
		static Sample *loadSample(string filename, bool iff = false);
		static Sample *loadSamplesChained(const char *filename1,const char *filename2,const char *filename3);
		static Sample *loadMusic(const char *filename);
	};
}

using namespace Gigalomania;

const int SOUND_CHANNEL_SAMPLES = 0;
const int SOUND_CHANNEL_MUSIC   = 1;
const int SOUND_CHANNEL_FX      = 2;
const int SOUND_CHANNEL_PLANES  = 3;

inline void playSample(Sample *sample, int channel = SOUND_CHANNEL_SAMPLES, int loops = 0) {
	sample->play(channel, loops);
}

bool isPlaying(int ch);

bool errorSound();
void resetErrorSound();
