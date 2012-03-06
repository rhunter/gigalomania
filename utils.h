#pragma once

/** Debugging stuff and other miscellaneous utilities.
 */

/*#ifndef DEBUGprintf
#define DEBUGprintf(x) if( !DEBUG || x > DEBUGLEVEL ) ((void)0); else log
#endif*/

const bool LOGGING = true; // enable logging even for release builds, for now

#ifndef LOG
#define LOG if( !LOGGING ) ((void)0); else log
#endif

//extern const bool DEBUG;
//extern const int DEBUGLEVEL;

char *getApplicationFilename(const char *name);
void initLogFile();
bool log(const char *text,...);

#ifndef ASSERT
#define ASSERT(test) {                                 \
        bool v = test;                                 \
        if( v )                                        \
                ((void)0);                             \
        else {                                         \
                LOG("ASSERTION FAILED:\n");            \
                LOG("%s\n", #test);                    \
				LOG("File: %s\n", __FILE__);           \
				LOG("Line: %d\n", __LINE__);           \
                assert(test);                          \
        }                                              \
}
#endif

#ifdef _DEBUG
#define T_ASSERT( x ) ASSERT( x )
#else
#define T_ASSERT( x )
#endif

/*
                int *assert_crash = NULL;              \
                *assert_crash = 0;                     \
*/

#ifndef ASSERT_ANY_EPOCH
#define ASSERT_ANY_EPOCH(epoch) ASSERT( epoch >= 0 && epoch < n_epochs_c );
#endif

#ifndef ASSERT_EPOCH
#define ASSERT_EPOCH(epoch) ASSERT( epoch >= 0 && epoch < n_epochs_c );
#endif

#ifndef ASSERT_S_EPOCH
#define ASSERT_S_EPOCH(epoch) ASSERT( epoch >= start_epoch && epoch <= n_epochs_c );
#endif

#ifndef ASSERT_R_EPOCH
#define ASSERT_R_EPOCH(r_epoch) ASSERT( r_epoch >= 0 && r_epoch < 4 );
#endif

#ifndef ASSERT_SHIELD
#define ASSERT_SHIELD(shield) ASSERT( shield >= 0 && shield < n_shields_c );
#endif

#ifndef ASSERT_ELEMENT_ID
#define ASSERT_ELEMENT_ID(id) ASSERT( id >= 0 && id < N_ID );
#endif

#ifndef ASSERT_PLAYER
//#define ASSERT_PLAYER(player) ASSERT( player >= 0 && player < n_players_c && players[player] != NULL );
#define ASSERT_PLAYER(player) ASSERT( validPlayer(player) );
#endif

#ifndef ASSERT_DIFFICULTY
#define ASSERT_DIFFICULTY(difficulty) ASSERT( validDifficulty(difficulty) );
#endif

class Rect2D {
public:
	int x, y, w, h;
	Rect2D() {
		set(0,0,0,0);
	}
	Rect2D(int x,int y,int w,int h) {
		set(x,y,w,h);
	}
	void set(int x,int y,int w,int h) {
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	}
	int getRight() const {
		return x + w;
	}
	int getBottom() const {
		return y + h;
	}
};

extern int frame_counter;
extern const int ticks_per_frame_c;
extern const float time_ratio_c;
extern int time_rate;

int getRealTime();
int getRealLoopTime();
int getGameTime();
int getLoopTime();
void updateTime(int time);
//int timeLeft();

void resetMouseClick();
int getNClicks();

int poisson(int mean_ticks_per_event,int time_interval);

int n_digits(int number);

void textLines(int *n_lines,int *max_wid,const char *text);

float perlin_noise2(float vec[2]);

#include <vector>
using std::vector;

template<class T>
bool remove_vec(vector<T> *vec,const T& value) {
	for(unsigned int i=0;i<vec->size();i++) {
		if( vec->at(i) == value ) {
			vec->erase(vec->begin() + i);
			return true;
		}
	}
	return false;
}

/*template<class T>
bool contains_vec(vector<T> *vec,const T& value) {
        if( find(vec->begin(), vec->end(), value) == vec->end() ) {
                return false;
        }
        return true;
}

template<class T>
bool addIfAbsent_vec(vector<T> *vec,const T& value) {
        if( find(vec->begin(), vec->end(), value) == vec->end() ) {
                vec->push_back(value);
                return true;
        }
        return false;
}*/
