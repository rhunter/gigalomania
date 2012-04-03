//---------------------------------------------------------------------------
#include "stdafx.h"

#ifdef __linux
#include <SDL/SDL.h>
#else
#include <sdl.h>
#endif

#include "sound.h"
#include "utils.h"
#include "game.h"
#include "gamestate.h"
//---------------------------------------------------------------------------

//const int header_c = 0x64;

//FMOD::System *fmod_system = NULL;
bool error_occurred = false;
bool have_sound = false;

bool errorSound() {
	bool rtn = error_occurred;
	//error_occurred = false;
	return rtn;
}

void resetErrorSound() {
	error_occurred = false;
}

Sample::~Sample() {
	/*if( this->fsound_sample != NULL ) {
		FSOUND_Sample_Free(this->fsound_sample);
	}*/
	/*if( this->sound != NULL ) {
		this->sound->release();
	}*/
	if( music != NULL )
		Mix_FreeMusic(music);
	if( chunk != NULL )
		Mix_FreeChunk(chunk);
}

bool initSound() {
	/*FMOD_RESULT result = FMOD::System_Create(&fmod_system);
	if( result != FMOD_OK ) {
		LOG("FMOD error creating system! (%d) %s\n", result, FMOD_ErrorString(result));
		return false;
	}
	else if( fmod_system == NULL ) {
		LOG("FMOD system is NULL\n");
		return false;
	}

	result = fmod_system->init(32, FMOD_INIT_NORMAL, 0);
	if( result != FMOD_OK ) {
		LOG("FMOD error initialising system! (%d) %s\n", result, FMOD_ErrorString(result));
		fmod_system->release();
		fmod_system = NULL;
		return false;
	}*/

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
	/*if( fmod_system != NULL ) {
		fmod_system->update();
	}*/
}

void freeSound() {
	/*if( fmod_system != NULL ) {
		fmod_system->release();
		fmod_system = NULL;
	}*/
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
	LOG("loadSample %s\n", filename);
	/*FMOD::Sound *sound = NULL;
	if( fmod_system != NULL ) {
		FMOD_RESULT result = FMOD_ERR_MUSIC_UNINITIALIZED;
		if( iff ) {
			FMOD_CREATESOUNDEXINFO info;
			memset(&info, NULL, sizeof(info));
			info.cbsize = sizeof(info);
			info.defaultfrequency = 14500;
			info.numchannels = 1;
			info.format = FMOD_SOUND_FORMAT_PCM8;
			info.fileoffset = header_c;
			result = fmod_system->createSound(filename, FMOD_OPENRAW, &info, &sound);
		}
		else {
			result = fmod_system->createSound(filename, FMOD_DEFAULT, 0, &sound);
		}
		if( result != FMOD_OK ) {
			LOG("failed to load %s\n", filename);
			LOG("FMOD error %d: %s\n", result, FMOD_ErrorString(result));
			error_occurred = true;
			sound = NULL; // just to be sure
		}
		else if( sound == NULL ) {
			LOG("null sound when loading %s\n", filename);
			error_occurred = true;
		}
	}*/
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
	/*
	int n_buffers = 1;
	if( filename2 != NULL )
		n_buffers++;
	if( filename3 != NULL )
		n_buffers++;

	FMOD::Sound *sound = NULL;
	if( fmod_system != NULL ) {
		FILE *file[3];
		file[0] = fopen(filename1, "rb");
		file[1] = filename2 == NULL ? NULL : fopen(filename2, "rb");
		file[2] = filename3 == NULL ? NULL : fopen(filename3, "rb");
		bool ok = true;
		for(int i=0;i<n_buffers && ok;i++) {
			ok = file[i] != NULL;
		}

		if( ok ) {
			int size = 0;
			int lengths[3];
			for(int i=0;i<n_buffers && ok;i++) {
				long curpos = ftell(file[i]);
				fseek(file[i], 0L, SEEK_END);
				lengths[i] = ftell(file[i]);
				//lengths[i] *= 2;
				//lengths[i] += 0x60;
				lengths[i] -= header_c;
				fseek(file[i], curpos, SEEK_SET);
				size += lengths[i];
			}
			char *buffer = new char[size];
			for(int i=0;i<size;i++) {
				buffer[i] = '\0';
			}
			//char *ptr = buffer;
			int offset = 0;
			for(int i=0;i<n_buffers && ok;i++) {
				fseek(file[i], header_c, SEEK_SET);
				fread(&buffer[offset], sizeof(char), lengths[i], file[i]);
				offset += lengths[i];
			}
			FMOD_CREATESOUNDEXINFO info;
			memset(&info, NULL, sizeof(info));
			info.cbsize = sizeof(info);
			info.defaultfrequency = 14500;
			info.numchannels = 1;
			info.format = FMOD_SOUND_FORMAT_PCM8;
			info.length = size;
			FMOD_RESULT result = fmod_system->createSound(buffer, FMOD_OPENMEMORY | FMOD_OPENRAW, &info, &sound);
			if( result != FMOD_OK ) {
				LOG("failed to load chained samples\n");
				LOG("FMOD error %d: %s\n", result, FMOD_ErrorString(result));
				error_occurred = true;
			}
			else if( sound == NULL ) {
				LOG("null sound when loading chained samples\n");
				error_occurred = true;
			}
		}
		else {
			LOG("failed to open chained samples\n");
			error_occurred = true;
		}

		for(int i=0;i<n_buffers;i++) {
			if( file[i] != NULL )
				fclose(file[i]);
		}
	}
	//Sample *sample = new Sample(fsound_sample); // still create sample struct even if failed to load sample - see note above in loadSample()
	Sample *sample = new Sample(sound); // still create sample struct even if failed to load sample - see note above in loadSample()

	return sample;*/

	// TODO
	error_occurred = true;
	Sample *sample = new Sample(false, NULL, NULL);
	return sample;
}

Sample *Sample::loadMusic(const char *filename) {
	LOG("loadMusic %s\n", filename);
	/*
	FMOD::Sound *sound = NULL;
	if( fmod_system != NULL ) {
		FMOD_RESULT result = fmod_system->createStream(filename, FMOD_DEFAULT | FMOD_LOOP_NORMAL, 0, &sound);
		if( result != FMOD_OK ) {
			LOG("failed to load %s\n", filename);
			LOG("FMOD error %d: %s\n", result, FMOD_ErrorString(result));
			error_occurred = true;
		}
		else if( sound == NULL ) {
			LOG("null sound when loading %s\n", filename);
			error_occurred = true;
		}
	}
	// we still create the structure even if no sample is loaded, as we also use for displaying the associated text
	Sample *sample = new Sample(sound);
	return sample;
	*/
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
	/*if( fmod_system != NULL && this->sound != NULL ) {
		FMOD::Channel *channel = NULL;
		// prefer to reuse channels, so previous sample is cut short to make way for this one
		FMOD_RESULT result = fmod_system->getChannel(ch, &channel);
		if( result != FMOD_OK ) {
			LOG("FMOD error getting channel %d: %s\n", result, FMOD_ErrorString(result));
			channel = NULL;
		}
		else {
			result = fmod_system->playSound(FMOD_CHANNEL_REUSE, sound, false, &channel);
			//FMOD_RESULT result = fmod_system->playSound(FMOD_CHANNEL_FREE, sound, false, &channel);
			if( result != FMOD_OK ) {
				LOG("FMOD error playing sound %d: %s\n", result, FMOD_ErrorString(result));
			}
			else {
				//channel->setFrequency(14500);
				//FMOD_RESULT result = channel->setPosition(header_c, FMOD_TIMEUNIT_PCMBYTES);
			}
		}
	}*/

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
	/*if( fmod_system != NULL ) {
		FMOD::Channel *channel = NULL;
		FMOD_RESULT result = fmod_system->getChannel(ch, &channel);
		if( result != FMOD_OK ) {
			LOG("FMOD error getting channel %d: %s\n", result, FMOD_ErrorString(result));
			channel = NULL;
		}
		else {
			result = channel->isPlaying(&is_playing);
			if( result != FMOD_OK ) {
				LOG("FMOD error getting channel is playing %d: %s\n", result, FMOD_ErrorString(result));
			}
		}
	}*/
	if( have_sound ) {
		if( Mix_Playing(ch) != 0 ) {
			is_playing = true;
		}
	}
	return is_playing;
}
