#include <QtGlobal> // need this to get Q_OS_ANDROID #define, which we need before we include anything else!

#if defined(Q_OS_ANDROID)

#include "androidaudio.h"
#include "androidsoundeffect.h"

#include <QDebug>

AndroidAudio::AndroidAudio(QObject *parent) :
    QObject(parent), sound_ok(false), mEngineObject(NULL), mEngineEngine(NULL), mOutputMixObject(NULL), mPlayerObject(NULL)
{
    if( createEngine() ) {
        if( startSoundPlayer() ) {
            sound_ok = true;
        }
    }
}

AndroidAudio::~AndroidAudio()
{
    destroyEngine();

}

// create the engine and output mix objects
bool AndroidAudio::createEngine()
{
    SLresult result;

    // create engine
    result = slCreateEngine(&mEngineObject, 0, NULL, 0, NULL, NULL);
    Q_ASSERT(SL_RESULT_SUCCESS == result);
    if( result != SL_RESULT_SUCCESS ) {
        qDebug("slCreateEngine failed");
        return false;
    }

    // realize the engine
    result = (*mEngineObject)->Realize(mEngineObject, SL_BOOLEAN_FALSE);
    Q_ASSERT(SL_RESULT_SUCCESS == result);
    if( result != SL_RESULT_SUCCESS ) {
        qDebug("engine Realize failed");
        return false;
    }

    // get the engine interface, which is needed in order to create other objects
    result = (*mEngineObject)->GetInterface(mEngineObject, SL_IID_ENGINE, &mEngineEngine);
    Q_ASSERT(SL_RESULT_SUCCESS == result);
    if( result != SL_RESULT_SUCCESS ) {
        qDebug("failed to get engine interface");
        return false;
    }

    // create output mix
    const SLInterfaceID ids[] = {};
    const SLboolean req[] = {};
    result = (*mEngineEngine)->CreateOutputMix(mEngineEngine, &mOutputMixObject, 0, ids, req);
    Q_ASSERT(SL_RESULT_SUCCESS == result);
    if( result != SL_RESULT_SUCCESS ) {
        qDebug("failed to create output mix");
        return false;
    }

    // realize the output mix
    result = (*mOutputMixObject)->Realize(mOutputMixObject, SL_BOOLEAN_FALSE);
    Q_ASSERT(SL_RESULT_SUCCESS == result);
    if( result != SL_RESULT_SUCCESS ) {
        qDebug("failed to realize output mix");
        return false;
    }

    qDebug() << "Created Android Audio Engine";
    return true;
}

void AndroidAudio::destroyEngine()
{
    if (mOutputMixObject != NULL) {
        (*mOutputMixObject)->Destroy(mOutputMixObject);
    }

    if (mEngineObject != NULL) {
        (*mEngineObject)->Destroy(mEngineObject);
    }

    if (mPlayerObject != NULL) {
        (*mPlayerObject)->Destroy(mPlayerObject);
    }

    qDebug() << "Destroyed Android Audio Engine";
}

AndroidSoundEffect *AndroidAudio::loadSound(const QString &filename) {
    if( !sound_ok ) {
        qDebug("sound engine not available");
        return NULL;
    }
    AndroidSoundEffect *sound = new AndroidSoundEffect(filename, this);
    if( !sound->load() ) {
        qDebug() << "failed to load sound";
        delete sound;
        sound = NULL;
    }
    return sound;
}

bool AndroidAudio::startSoundPlayer()
{
    qDebug() << "Starting Sound Player";

    SLresult lRes;

    //Configure the sound player input/output
    SLDataLocator_AndroidSimpleBufferQueue lDataLocatorIn;
    lDataLocatorIn.locatorType = SL_DATALOCATOR_BUFFERQUEUE;
    lDataLocatorIn.numBuffers = 1;

    //Set the data format
    SLDataFormat_PCM lDataFormat;
    lDataFormat.formatType = SL_DATAFORMAT_PCM;
    lDataFormat.numChannels = 2;
    //lDataFormat.samplesPerSec = SL_SAMPLINGRATE_44_1;
    lDataFormat.samplesPerSec = SL_SAMPLINGRATE_22_05; // for some reason, samples play too high pitched when using SL_SAMPLINGRATE_44_1?!
    lDataFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    lDataFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    lDataFormat.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    lDataFormat.endianness = SL_BYTEORDER_LITTLEENDIAN;

    SLDataSource lDataSource;
    lDataSource.pLocator = &lDataLocatorIn;
    lDataSource.pFormat = &lDataFormat;

    SLDataLocator_OutputMix lDataLocatorOut;
    lDataLocatorOut.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    lDataLocatorOut.outputMix = mOutputMixObject;

    SLDataSink lDataSink;
    lDataSink.pLocator = &lDataLocatorOut;
    lDataSink.pFormat = NULL;

    //Create the sound player
    const SLuint32 lSoundPlayerIIDCount = 2;
    const SLInterfaceID lSoundPlayerIIDs[] = { SL_IID_PLAY, SL_IID_BUFFERQUEUE };
    const SLboolean lSoundPlayerReqs[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

    qDebug() << "Configured Sound Player";

    lRes = (*mEngineEngine)->CreateAudioPlayer(mEngineEngine, &mPlayerObject, &lDataSource, &lDataSink, lSoundPlayerIIDCount, lSoundPlayerIIDs, lSoundPlayerReqs);
    Q_ASSERT(SL_RESULT_SUCCESS == lRes);
    if( lRes != SL_RESULT_SUCCESS ) {
        qDebug("failed to create audio player");
        return false;
    }

    qDebug() << "Created Sound Player";

    lRes = (*mPlayerObject)->Realize(mPlayerObject, SL_BOOLEAN_FALSE);
    Q_ASSERT(SL_RESULT_SUCCESS == lRes);
    if( lRes != SL_RESULT_SUCCESS ) {
        qDebug("failed to realize sound player");
        return false;
    }

    qDebug() << "Realised Sound Player";
    lRes = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_PLAY, &mPlayerPlay);
    Q_ASSERT(SL_RESULT_SUCCESS == lRes);
    if( lRes != SL_RESULT_SUCCESS ) {
        qDebug("failed to get player interface");
        return false;
    }

    lRes = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_BUFFERQUEUE, &mPlayerQueue);
    Q_ASSERT(SL_RESULT_SUCCESS == lRes);
    if( lRes != SL_RESULT_SUCCESS ) {
        qDebug("failed to get player queue interface");
        return false;
    }

    lRes = (*mPlayerPlay)->SetPlayState(mPlayerPlay, SL_PLAYSTATE_PLAYING);
    Q_ASSERT(SL_RESULT_SUCCESS == lRes);
    if( lRes != SL_RESULT_SUCCESS ) {
        qDebug("failed to set player state");
        return false;
    }

    qDebug() << "Created Buffer Player";
    return true;
}

void AndroidAudio::playSound(const AndroidSoundEffect *sound) {
    SLresult lRes;
    SLuint32 lPlayerState;

    if( !sound_ok ) {
        qDebug("sound engine not available");
        return;
    }

    if (!sound) {
        return;
    }
    //Get the current state of the player
    (*mPlayerObject)->GetState(mPlayerObject, &lPlayerState);

    //If the player is realised
    if (lPlayerState == SL_OBJECT_STATE_REALIZED) {
        //Get the buffer and length of the effect
        int16_t* lBuffer = (int16_t *)sound->mBuffer;
        off_t lLength = sound->mLength;

        //Remove any sound from the queue
        lRes = (*mPlayerQueue)->Clear(mPlayerQueue);
        Q_ASSERT(SL_RESULT_SUCCESS == lRes);

        //Play the new sound
        (*mPlayerQueue)->Enqueue(mPlayerQueue, lBuffer, lLength);
        Q_ASSERT(SL_RESULT_SUCCESS == lRes);

    }
}

#endif
