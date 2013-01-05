//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cstdio> // n.b., needed on Linux at least
#include <cstring> // n.b., needed on Linux at least
#include <cstdlib> // n.b., needed on Linux at least
#include <cstdarg> // n.b., needed on Linux at least

#include "game.h"

#ifdef USING_QT
#include "qt_screen.h"
#else
#include "screen.h"
#endif

#ifdef _WIN32
#include <shlobj.h>
#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

#include <io.h> // for access
#include <direct.h> // for mkdir
#define access _access
#elif __linux
#include <sys/stat.h> // for mkdir
#endif

#include <cassert>
#include <cmath> // n.b., needed on Linux at least

#include "utils.h"
#include "game.h"
//---------------------------------------------------------------------------

//const bool DEBUG = true;
//const int DEBUGLEVEL = 4;

const int MAX_TIME = 1000;
const int ticks_per_frame_c = 100;
const float time_ratio_c = 0.15f; // game time ticks per time ticks [correct]
int time_rate = 1; // time factor

int frame_counter = 0;

/*int timeLeft() {
	//int now = SDL_GetTicks();
	int now = clock();
	int res = 0;
	if(next_time > now)
		res = next_time - now;
	next_time = now + TICK_INTERVAL;
	return res;
}*/

static int real_time = 0;
static int real_loop_time = 0;
static int game_time = 0;
static int loop_time = 0;
//static int current_time = 0;
//static int new_time = 0;

int getRealTime() {
	return real_time;
}

int getRealLoopTime() {
	return real_loop_time;
}

int getGameTime() {
	return game_time;
}

int getLoopTime() {
	//return new_time - current_time;
	return loop_time;
}

/*void initTime(int time) {
current_time = time;
}*/

void updateTime(int time) {
	if( time > MAX_TIME )
		time = MAX_TIME;
	real_loop_time = time;
	real_time += time;
	loop_time = (int)(time * time_ratio_c * time_rate);
	game_time += loop_time;
	frame_counter = getRealTime() / ticks_per_frame_c;
}

int mouseTime = -1;

void resetMouseClick() {
	mouseTime = -1;
}

int getNClicks() {
	if( mouseTime == -1 )
		mouseTime = getRealTime();
	int time = getRealTime() - mouseTime;
	if( time < 2000 )
		return 1;
	else if( time < 5000 )
		return 2;
	return 3;
}

/* Return probability (as a proportion of RAND_MAX) that at least one poisson event
* occurred within the time_interval, given the mean number of time units per event.
*/
int poisson(int mean_ticks_per_event,int time_interval) {
	if( mean_ticks_per_event == 0 )
		return RAND_MAX;
	ASSERT( mean_ticks_per_event > 0 );
	int prob = (int)(RAND_MAX * ( 1.0 - exp( - ((double)time_interval) / mean_ticks_per_event ) ));
	return prob;
}

int n_digits(int number) {
	int num = 0;
	if( number < 0 ) {
		number = - number;
		num++;
	}
	while( number != 0 ) {
		num++;
		number /= 10;
	}
	return num;
}

void textLines(int *n_lines,int *max_wid,const char *text) {
	*n_lines = 0;
	*max_wid = 0;
	const char *ptr = text;
	const char *n_ptr = ptr;
	for(;;) {
		*n_lines = *n_lines + 1;

		while( *n_ptr != '\n' && *n_ptr != '\0' )
			n_ptr++;

		int wid = (int)(n_ptr - ptr);
		if( wid > *max_wid )
			*max_wid = wid;

		if( *n_ptr == '\0' )
			break;
		n_ptr++;
		ptr = n_ptr;
	}
}

char application_name[] = "Gigalomania";
#if defined(Q_OS_ANDROID)
char application_package_name[] = "net.sourceforge.gigalomania";
#endif

FILE *logfile = NULL;

// Maemo/Meego treated as Linux as far as paths are concerned
#ifdef USING_QT
char *application_path = NULL;
char *logfilename = NULL;
char *oldlogfilename = NULL;
#elif _WIN32
char application_path[MAX_PATH] = "";
char logfilename[MAX_PATH] = "";
char oldlogfilename[MAX_PATH] = "";
#elif __linux
char *application_path = NULL;
char *logfilename = NULL;
char *oldlogfilename = NULL;
#else
char application_path[] = "";
char logfilename[] = "log.txt";
char oldlogfilename[] = "log_old.txt";
#endif

/* Returns a full path for a filename in userspace (i.e., where we'll have read/write access).
 * For Windows, this is in %APPDATA%/application_name/
 * For Linux (including Maemo and Meego), this is in user's home/.config/application_name/ (note the '.', to make it a hidden folder)
 * If the folder can't be accessed (or running on a new operating system), the program folder is used.
 * For Qt platforms, we use QDesktopServices::storageLocation(QDesktopServices::DataLocation).
 */
char *getApplicationFilename(const char *name) {
    // not safe to use LOG here, as logfile may not have been initialised!
    //printf("getApplicationFilename: %s\n", name);
    //printf("application_path: %s\n", application_path);
    // Maemo/Meego treated as Linux as far as paths are concerned
#ifdef USING_QT
    /*int len = strlen(application_path) + 1 + strlen(name);
    char *filename = new char[len+1];
    sprintf(filename, "%s/%s", application_path, name);*/
    QString pathQt = QString(application_path) + QString("/") + QString(name);
    QString nativePath(QDir::toNativeSeparators(pathQt));
    char *filename = new char[nativePath.length()+1];
    strcpy(filename, nativePath.toLatin1().data());
    qDebug("getApplicationFilename returns: %s", filename);
#elif _WIN32
	char *filename = new char[MAX_PATH];
	strcpy(filename, application_path);
	PathAppendA(filename, name);
#elif __linux
	char *filename = NULL;
	int application_path_len = strlen(application_path);
	if( application_path_len == 0 || application_path[application_path_len-1] == '/' ) {
		// shouldn't add path separator
		int len = application_path_len + strlen(name);
		filename = new char[len+1];
		sprintf(filename, "%s%s", application_path, name);
	}
	else {
		// should add path separator
		int len = application_path_len + 1 + strlen(name);
		filename = new char[len+1];
		sprintf(filename, "%s/%s", application_path, name);
	}
#else
	char *filename = new char[strlen(name)+1];
	strcpy(filename, name);
#endif
	//printf("getApplicationFilename returns: %s\n", filename);
    return filename;
}

void initLogFile() {
    // not safe to use LOG here, as logfile not initialised!
    // first need to establish full path, and create folder if necessary
    // Maemo/Meego treated as Linux as far as paths are concerned
#ifdef USING_QT

#if defined(Q_OS_ANDROID)
    // on Android, try for the sdcard, so we can find somewhere more accessible to the user (and that I can read on my Galaxy Nexus!!!)
    QString nativePath = QString("/sdcard/") + QString(application_package_name);
    qDebug("try sd card: %s", nativePath.toStdString().c_str());
    if( !QDir(nativePath).exists() ) {
        qDebug("try creating application folder in sdcard/");
        // folder doesn't seem to exist - try creating it
        QDir().mkdir(nativePath);
        if( !QDir(nativePath).exists() ) {
            qDebug("failed to create application folder in sdcard/");
            QString pathQt(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
            nativePath = QDir::toNativeSeparators(pathQt);
        }
    }
#else
    QString pathQt(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    QString nativePath(QDir::toNativeSeparators(pathQt));
#endif

    application_path = new char[nativePath.length()+1];
    strcpy(application_path, nativePath.toLatin1().data());
    logfilename = getApplicationFilename("log.txt");
    oldlogfilename = getApplicationFilename("log_old.txt");
    qDebug("application_path: %s", application_path);
    qDebug("logfilename: %s", logfilename);
    qDebug("oldlogfilename: %s", oldlogfilename);
#elif _WIN32
	bool ok = true;
    if ( SUCCEEDED( SHGetFolderPathA( NULL, CSIDL_APPDATA,
                                     NULL, 0, logfilename ) ) ) {
        PathAppendA(logfilename, application_name);

		if( access(logfilename, 0) != 0 ) {
			// folder doesn't seem to exist - try creating it
			int res = mkdir(logfilename);
			//int res = 1; // test
			if( res != 0 ) {
				printf("Failed to create folder for application data!\n");
				MessageBoxA(NULL, "Failed to create folder for application data - storing in local folder instead.\n", "Warning", MB_OK|MB_ICONEXCLAMATION);
				ok = false;
			}
		}
    }
	else {
		printf("Failed to obtain path for application folder!\n");
		MessageBoxA(NULL, "Failed to obtain path for application folder - storing in local folder instead.\n", "Warning", MB_OK|MB_ICONEXCLAMATION);
		ok = false;
	}

	if( ok ) {
		strcpy(application_path, logfilename);
		strcpy(oldlogfilename, logfilename);
		PathAppendA(logfilename, "log.txt");
		PathAppendA(oldlogfilename, "log_old.txt");
	}
	else {
		// just save in local directory and hope for the best!
		strcpy(application_path, "");
		strcpy(logfilename, "log.txt");
		strcpy(oldlogfilename, "log_old.txt");
	}
#elif __linux
	char *homedir = getenv("HOME");
	//char *subdir = "/.gigalomania";
	char *subdir = "/.config/gigalomania";
	int len = strlen(homedir) + strlen(subdir);
	application_path = new char[len+1];
	sprintf(application_path, "%s%s", homedir, subdir);

	// create the folder if it doesn't already exist
	bool ok = true;
	if( access(application_path, 0) != 0 ) {
		int res = mkdir(application_path, S_IRWXU | S_IRWXG | S_IRWXO);
		if( res != 0 ) {
			ok = false;
		}
	}

	if( ok ) {
		logfilename = getApplicationFilename("log.txt");
		oldlogfilename = getApplicationFilename("log_old.txt");
	}
	else {
		// just save in local directory and hope for the best!
		strcpy(application_path, "");
		strcpy(logfilename, "log.txt");
		strcpy(oldlogfilename, "log_old.txt");
	}
#else
	// no need to do anything
#endif

	remove(oldlogfilename);
	rename(logfilename, oldlogfilename);
	remove(logfilename);

	LOG("Initialising Log File...\n");
	LOG("Version %d.%d\n", majorVersion, minorVersion);

#ifdef _DEBUG
	LOG("Running in Debug mode\n");
#else
	LOG("Running in Release mode\n");
#endif

#if defined(Q_WS_SIMULATOR)
    LOG("Platform: Qt Smartphone Simulator\n");
#elif defined(_WIN32)
    LOG("Platform: Windows\n");
#elif defined(Q_WS_MAEMO_5)
    // must be before __linux, as Maemo/Meego also defines __linux
    LOG("Platform: Maemo/Meego\n");
#elif defined(Q_OS_ANDROID)
        // must be before __linux, as Android also defines __linux
        LOG("Platform: Android\n");
#elif __linux
	LOG("Platform: Linux\n");
#elif defined(__APPLE__) && defined(__MACH__)
	LOG("Platform: MacOS X\n");
#elif AROS
    LOG("Platform: AROS\n");
#elif defined(Q_OS_SYMBIAN)
    LOG("Platform: Symbian\n");
#else
	LOG("Platform: UNKNOWN\n");
#endif
}

bool log(const char *text,...) {
	//return true;
	logfile = fopen(logfilename,"at+");
	va_list vlist;
	char buffer[65536];
	va_start(vlist, text);
	vsprintf(buffer,text,vlist);
	if( logfile != NULL )
		fprintf(logfile,buffer);
	if( debugwindow )
		printf(buffer);
#ifdef USING_QT
        qDebug(buffer);
#endif
        va_end(vlist);
	if( logfile != NULL )
		fclose(logfile);
	return true;
}

// Perlin noise

#define B 0x100
#define BM 0xff

#define N 0x1000
#define NP 12   /* 2^N */
#define NM 0xfff

static int p[B + B + 2];
static float g3[B + B + 2][3];
static float g2[B + B + 2][2];
static float g1[B + B + 2];
static int start = 1;

static void normalize2(float v[2])
{
	float s;

	s = sqrt(v[0] * v[0] + v[1] * v[1]);
	v[0] = v[0] / s;
	v[1] = v[1] / s;
}

static void normalize3(float v[3])
{
	float s;

	s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] = v[0] / s;
	v[1] = v[1] / s;
	v[2] = v[2] / s;
}

void initPerlin() {
	start = 0;
	int i, j, k;

	for (i = 0 ; i < B ; i++) {
		p[i] = i;

		g1[i] = (float)((rand() % (B + B)) - B) / B;

		for (j = 0 ; j < 2 ; j++)
			g2[i][j] = (float)((rand() % (B + B)) - B) / B;
		normalize2(g2[i]);

		for (j = 0 ; j < 3 ; j++)
			g3[i][j] = (float)((rand() % (B + B)) - B) / B;
		normalize3(g3[i]);
	}

	while (--i) {
		k = p[i];
		p[i] = p[j = rand() % B];
		p[j] = k;
	}

	for (i = 0 ; i < B + 2 ; i++) {
		p[B + i] = p[i];
		g1[B + i] = g1[i];
		for (j = 0 ; j < 2 ; j++)
			g2[B + i][j] = g2[i][j];
		for (j = 0 ; j < 3 ; j++)
			g3[B + i][j] = g3[i][j];
	}
}

#define s_curve(t) ( t * t * (3.0f - 2.0f * t) )

#define lerp(t, a, b) ( a + t * (b - a) )

#define setup(i,b0,b1,r0,r1)\
	t = vec[i] + N;\
	b0 = ((int)t) & BM;\
	b1 = (b0+1) & BM;\
	r0 = t - (int)t;\
	r1 = r0 - 1.0f;

float perlin_noise2(float vec[2]) {
	int bx0, bx1, by0, by1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
	register int i, j;

	if (start) {
		initPerlin();
	}

	setup(0, bx0,bx1, rx0,rx1);
	setup(1, by0,by1, ry0,ry1);

	i = p[ bx0 ];
	j = p[ bx1 ];

	b00 = p[ i + by0 ];
	b10 = p[ j + by0 ];
	b01 = p[ i + by1 ];
	b11 = p[ j + by1 ];

	sx = s_curve(rx0);
	sy = s_curve(ry0);

#define at2(rx,ry) ( rx * q[0] + ry * q[1] )

	q = g2[ b00 ] ; u = at2(rx0,ry0);
	q = g2[ b10 ] ; v = at2(rx1,ry0);
	a = lerp(sx, u, v);

	q = g2[ b01 ] ; u = at2(rx0,ry1);
	q = g2[ b11 ] ; v = at2(rx1,ry1);
	b = lerp(sx, u, v);

	return lerp(sy, a, b);
}
