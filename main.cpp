#include "stdafx.h"

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include "game.h"

int main(int argc, char *argv[])
{
#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "started main");
#endif

	playGame(argc, argv);
#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "about to exit main");
#endif
    return 0;
}
