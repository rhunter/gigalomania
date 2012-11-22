#pragma once

/** Handles sound samples and music.
*/

using std::string;

#include "resources.h"

#include <QtGlobal> // need this to get Q_OS_ANDROID #define, which we need before we include anything else!

#ifndef Q_OS_ANDROID
// Phonon not supported on Qt Android
#include <phonon/MediaObject>
#else
#include "androidaudio/androidaudio.h"
#endif

bool initSound();
void updateSound();
void freeSound();

class Sample : public TrackedObject {
#ifndef Q_OS_ANDROID
    Phonon::MediaObject *sound;
#else
    //void *sound;
    AndroidSoundEffect *sound;
#endif
    string text;
public:
    Sample() : sound(NULL) {
        // create dummy sample
    }

    virtual ~Sample() {
        if( sound != NULL ) {
            delete sound;
        }
    }

	virtual const char *getClass() const { return "CLASS_SAMPLE"; }
    void play(int ch);
    void setVolume(float volume) {
    }
	void setText(const char *text) {
		this->text = text;
	}

    static Sample *loadSample(const char *filename, bool iff = false);
    static Sample *loadSample(string filename, bool iff = false);
    static Sample *loadSamplesChained(const char *filename1,const char *filename2,const char *filename3);
    static Sample *loadMusic(const char *filename);

};

const int SOUND_CHANNEL_SAMPLES = 0;
const int SOUND_CHANNEL_MUSIC   = 1;
const int SOUND_CHANNEL_FX      = 2;

inline void playSample(Sample *sample, int channel = SOUND_CHANNEL_SAMPLES) {
	sample->play(channel);
}

bool isPlaying(int ch);
bool errorSound();
void resetErrorSound();
