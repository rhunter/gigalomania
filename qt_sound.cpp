#include "stdafx.h"

#include "qt_sound.h"
#include "gamestate.h"
#include "game.h"

#include <QApplication>

#ifndef Q_OS_ANDROID
#include <phonon/AudioOutput>
#endif

#ifdef Q_OS_ANDROID
AndroidAudio androidAudio;
#endif

void Sample::play(int ch) {
    if( this->sound != NULL && play_music ) {
#ifndef Q_OS_ANDROID
        this->sound->play();
#else
        androidAudio.playSound(sound);
#endif
    }
    if( this->text.length() > 0 ) {
        //const int ypos = 216;
		const int ypos = 224;
        TextEffect *effect = new TextEffect(this->text, 160, ypos, 2000);
        //gamestate->addEffect(effect);
        //gamestate->addTextEffect(effect);
        addTextEffect(effect);
    }
}

bool initSound() {
    return true;
}

void updateSound() {

}

void freeSound() {
}

Sample *Sample::loadSample(const char *filename, bool iff) {
    Sample *sample = new Sample();
    string qt_filename = ":/" + string(filename);
#ifndef Q_OS_ANDROID
    //qDebug("load: %s\n", qt_filename.c_str());
    //sample->sound = new QSound(qt_filename.c_str());
    //sample->sound = Phonon::createPlayer(Phonon::GameCategory, Phonon::MediaSource(qt_filename.c_str()));

    sample->sound = new Phonon::MediaObject(qApp);
    sample->sound->setCurrentSource(Phonon::MediaSource(qt_filename.c_str()));
    Phonon::AudioOutput *audioOutput = new Phonon::AudioOutput(Phonon::GameCategory, qApp);
    Phonon::Path path = Phonon::createPath(sample->sound, audioOutput);
    //qDebug("Sound has volume: %f\n", audioOutput->volume());
    //audioOutput->setVolume(0.5); // doesn't work on Nokia 5800!?
    //qDebug("Sound now has volume: %f\n", audioOutput->volume());
#else
    sample->sound = androidAudio.loadSound(QString(qt_filename.c_str()));
#endif
    return sample;
}

Sample *Sample::loadSample(string filename, bool iff) {
    return loadSample(filename.c_str(), iff);
}

Sample *Sample::loadSamplesChained(const char *filename1,const char *filename2,const char *filename3) {
    return NULL;
}

Sample *Sample::loadMusic(const char *filename) {
    return NULL;
}

bool isPlaying(int ch) {
    return false;
}

bool errorSound() {
    return false;
}

void resetErrorSound() {
}
