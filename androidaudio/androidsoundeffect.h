#ifndef ANDROIDSOUNDEFFECT_H
#define ANDROIDSOUNDEFFECT_H

// modified from https://groups.google.com/forum/?fromgroups=#!topic/android-qt/rpPa_W6PF1Y , by Adam Pigg

#include <QtGlobal> // need this to get Q_OS_ANDROID #define, which we need before we include anything else!

// n.b., need to faff around with #includes here, so it compiles on both Android and other platforms, even though we only need this file for Android

#include <QObject>

#if defined(Q_OS_ANDROID)

#include <QDebug>
#include <QString>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#endif

class AndroidSoundEffect : public QObject
{
    Q_OBJECT
#if defined(Q_OS_ANDROID)
public:
    explicit AndroidSoundEffect(const QString& pPath, QObject *parent = 0);
    ~AndroidSoundEffect();

    const char* getPath();
    bool load();
    bool unload();

signals:

public slots:

private:
    char* mBuffer;
    int mLength;

    QString mPath;

    friend class AndroidAudio;
#endif
};

#endif // ANDROIDSOUNDEFFECT_H
