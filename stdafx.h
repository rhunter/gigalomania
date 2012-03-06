#pragma once

// pre-compiled header file

#ifdef _WIN32
// get rid of "unsafe" warnings for file commands (fopen vs fopen_s etc)
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <vector>
#include <string>
#include <cassert>

/*#ifdef __linux
#include <SDL/SDL.h>
#else
#include <sdl.h>
#endif*/
