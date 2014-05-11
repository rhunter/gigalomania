//---------------------------------------------------------------------------
#include "stdafx.h"

#include "sound.h"
#include "utils.h"
#include "game.h"
#include "gamestate.h"
//---------------------------------------------------------------------------

bool error_occurred = false;
bool have_sound = false;

bool errorSound() {
	bool rtn = error_occurred;
	return rtn;
}

void resetErrorSound() {
	error_occurred = false;
}

Sample::~Sample() {
	if( music != NULL )
		Mix_FreeMusic(music);
	if( chunk != NULL )
		Mix_FreeChunk(chunk);
}

bool initSound() {
	if( SDL_InitSubSystem(SDL_INIT_AUDIO) != 0 ) {
		LOG("failed to init SDL audio subsystem");
		return false;
	}
	if( Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1 ) {
		LOG("Mix_OpenAudio failed: %s\n", Mix_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}
#ifdef _WIN32
    // not yet available on Linux!
	Mix_Init(0);
#endif

	have_sound = true;
	return true;
}

void updateSound() {
}

void freeSound() {
	if( have_sound ) {
		Mix_CloseAudio();
#ifdef _WIN32
		// not yet available on Linux!
		Mix_Quit();
#endif
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}
}

Sample *Sample::loadSample(const char *filename, bool iff) {
	//LOG("loadSample %s\n", filename); // disabled logging to improve performance on startup
	Mix_Chunk *chunk = NULL;
	if( have_sound ) {
		chunk = Mix_LoadWAV(filename);
		if( chunk == NULL ) {
			LOG("Mix_LoadWAV failed: %s\n", Mix_GetError());
			error_occurred = true;
		}
	}

	// we still create the structure even if no sample is loaded, as we also use for displaying the associated text
	Sample *sample = new Sample(false, NULL, chunk);
	return sample;
}

Sample *Sample::loadSample(string filename, bool iff) {
	return loadSample(filename.c_str(), iff);
}

Sample *Sample::loadSamplesChained(const char *filename1,const char *filename2,const char *filename3) {
	// TODO:
	error_occurred = true;
	Sample *sample = new Sample(false, NULL, NULL);
	return sample;
}

Sample *Sample::loadMusic(const char *filename) {
	LOG("loadMusic %s\n", filename);
	Mix_Music *music = NULL;
	if( have_sound ) {
		music = Mix_LoadMUS(filename);
		if( music == NULL ) {
			LOG("Mix_LoadMUS failed: %s\n", Mix_GetError());
			error_occurred = true;
		}
	}
	// we still create the structure even if no sample is loaded, as we also use for displaying the associated text
	Sample *sample = new Sample(true, music, NULL);
	return sample;
}

void Sample::play(int ch) {
	if( have_sound && play_music ) {
		if( is_music ) {
			if( Mix_PlayMusic(music, -1) == -1 ) {
			//if( Mix_FadeInMusic(music, -1, 2000) == -1 ) {
				LOG("Mix_PlayMusic failed: %s\n", Mix_GetError());
			}
			Mix_VolumeMusic(MIX_MAX_VOLUME);
		}
		else {
			if( chunk != NULL ) {
				bool done = false;
				if( channel != -1 ) {
					if( Mix_Paused(channel) ) {
						// sound was paused, so let's just resume
						Mix_Resume(channel);
						done = true;
					}
					// otherwise, let's stop the currently playing sound
					Mix_HaltChannel(channel);
				}
				if( !done ) {
					const bool loop = false;
					//channel = Mix_PlayChannel(-1, chunk, loop ? -1 : 0);
					channel = Mix_PlayChannel(ch, chunk, loop ? -1 : 0);
					if( channel == -1 ) {
						LOG("Failed to play sound: %s\n", Mix_GetError());
					}
					else {
						Mix_Volume(channel, MIX_MAX_VOLUME);
					}
				}
			}
		}
	}
	if( this->text.length() > 0 ) {
		//const int ypos = 216;
		const int ypos = 224;
		TextEffect *effect = new TextEffect(this->text, 160, ypos, 2000);
		addTextEffect(effect);
	}
}

void Sample::setVolume(float volume) {
	if( have_sound ) {
		if( is_music ) {
			Mix_VolumeMusic(MIX_MAX_VOLUME*volume);
		}
		else if( channel != -1 ) {
			Mix_Volume(channel, MIX_MAX_VOLUME*volume);
		}
	}
}

bool isPlaying(int ch) {
	bool is_playing = false;
	if( have_sound ) {
		if( Mix_Playing(ch) != 0 ) {
			is_playing = true;
		}
	}
	return is_playing;
}
