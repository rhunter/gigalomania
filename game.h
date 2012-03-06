#pragma once

/** Main game file, mainly contains various setup functions.
*/

class Image;
class Invention;
class Weapon;
class Element;
class Sector;
class PanelPage;
class Screen;
class GameState;
class PlayingGameState;
class Player;
class Sample;
class Application;
class TextEffect;

#include "common.h"

using std::string;

/*struct FSOUND_SAMPLE;
typedef FSOUND_SAMPLE Sample;*/

extern const int majorVersion;
extern const int minorVersion;

extern bool onemousebutton;
extern bool mobile_ui;

extern Application *application;

extern char *maps_dirname;
#ifdef __linux
extern char *alt_maps_dirname;
#endif

extern int offset_flag_x_c;
extern int offset_flag_y_c;

//extern int lastmouseclick_time;

enum GameStateID {
	GAMESTATEID_UNDEFINED = -1,
	GAMESTATEID_CHOOSEGAMETYPE = 0,
	GAMESTATEID_CHOOSEDIFFICULTY,
	GAMESTATEID_CHOOSEPLAYER,
	GAMESTATEID_PLACEMEN,
	GAMESTATEID_PLAYING,
	GAMESTATEID_ENDISLAND,
	GAMESTATEID_GAMECOMPLETE
};

extern GameStateID gameStateID;
extern bool state_changed;
extern bool paused;

enum GameResult {
	GAMERESULT_UNDEFINED = 0,
	GAMERESULT_WON,
	GAMERESULT_QUIT,
	GAMERESULT_LOST
};

extern GameResult gameResult;

const int default_width_c = 320;
//const int default_height_c = 256;
const int default_height_c = 240;
//extern int default_width_c;
//extern int default_height_c;
extern float scale_width;
extern float scale_height;
//extern int screen_width;
//extern int screen_height;
/*const int screen_width = scale_width * default_width_c;
//const int screen_height = scale_height * default_height_c;
const int screen_height = scale_height * 256;
//const int screen_height = scale_height * 240;*/
extern bool original_background;

const int infinity_c = 31;
//const int end_epoch_c = 9; // use this to have the last epoch game
const int end_epoch_c = -1; // use this to have the last epoch as being 2100AD
const int nuclear_epoch_c = 8;
const int laser_epoch_c = 9;
const int n_shields_c = 4;
const int n_playershields_c = 16;
const int n_flag_frames_c = 4;
const int n_defender_frames_c = 8;
const int n_attacker_frames_c = 16;
const int n_trees_c = 4;
const int n_clutter_c = 3;
const int n_tree_frames_c = 4;
const int n_nuke_frames_c = 2;
const int n_saucer_frames_c = 4;
const int n_death_flashes_c = 3;
const int n_blue_flashes_c = 7;
const int n_coast_c = 15;
const int n_map_sq_c = 16;

enum MapColour {
	MAP_UNDEFINED_COL = -1,
	MAP_ORANGE = 0,
	MAP_GREEN,
	MAP_BROWN,
	MAP_WHITE,
	MAP_DBROWN,
	MAP_DGREEN,
	MAP_GREY,
	MAP_N_COLOURS
};

extern Image *player_select;
extern Image *background;
extern Image *land[];
extern Image *fortress[];
extern Image *mine[];
extern Image *factory[];
extern Image *lab[];
extern Image *men[];
extern Image *unarmed_man;
extern Image *flags[n_players_c][n_flag_frames_c];
extern Image *panel_design;
//extern Image *panel_design_dark;
extern Image *panel_lab;
extern Image *panel_factory;
extern Image *panel_shield;
extern Image *panel_defence;
extern Image *panel_attack;
extern Image *panel_bloody_attack;
extern Image *panel_knowndesigns;
extern Image *panel_twoattack;
extern Image *panel_build[];
extern Image *panel_building[];
extern Image *panel_bigdesign;
extern Image *panel_biglab;
extern Image *panel_bigfactory;
extern Image *panel_bigshield;
extern Image *panel_bigdefence;
extern Image *panel_bigattack;
extern Image *panel_bigbuild;
extern Image *panel_bigknowndesigns;
extern Image *numbers_blue[];
extern Image *numbers_grey[];
extern Image *numbers_white[];
extern Image *numbers_orange[];
extern Image *numbers_yellow[];
extern Image *numbers_largegrey[];
extern Image *numbers_largeshiny[];
extern Image *numbers_small[n_players_c][10];
extern Image *numbers_half;
extern Image *letters_large[];
extern Image *letters_small[];
extern Image *mouse_pointers[];
extern Image *playershields[];
extern Image *building_health;
extern Image *dash_grey;
extern Image *icon_shield;
extern Image *icon_defence;
extern Image *icon_weapon;
extern Image *icon_shields[];
extern Image *icon_defences[];
extern Image *icon_weapons[];
extern Image *numbered_defences[];
extern Image *numbered_weapons[];
extern Image *icon_elements[];
extern Image *icon_clocks[];
extern Image *icon_infinity;
extern Image *icon_bc;
extern Image *icon_ad;
extern Image *icon_ad_shiny;
extern Image *icon_towers[];
extern Image *icon_armies[];
extern Image *icon_nuke_hole;
extern Image *mine_gatherable_small;
extern Image *mine_gatherable_large;
extern Image *icon_ergo;
extern Image *icon_trash;
extern Image *coast_icons[n_coast_c];
extern Image *map_sq[MAP_N_COLOURS][n_map_sq_c];
extern Image *defenders[n_players_c][n_epochs_c][n_defender_frames_c];
extern Image *nuke_defences[]; // epoch 8
extern Image *attackers_walking[n_players_c][n_epochs_c+1][n_attacker_frames_c]; // epochs 6-9 are special case!
extern Image *planes[n_players_c][n_epochs_c]; // epochs 6,7 only
extern Image *nukes[n_players_c][n_nuke_frames_c]; // epoch 8
extern Image *saucers[n_players_c][n_saucer_frames_c]; // epoch 9
extern Image *attackers_ammo[n_epochs_c][N_ATTACKER_AMMO_DIRS];
extern Image *icon_openpitmine;
extern Image *icon_trees[n_trees_c][n_tree_frames_c];
extern Image *icon_clutter[n_clutter_c];
extern Image *flashingmapsquare;
extern Image *mapsquare;
extern Image *arrow_left;
extern Image *arrow_right;
extern Image *death_flashes[];
extern Image *blue_flashes[];
extern Image *icon_mice[];
extern Image *icon_speeds[];
extern Image *smoke_image;

extern Image *background_islands;

extern Sample *s_design_is_ready;
extern Sample *s_ergo;
extern Sample *s_advanced_tech;
extern Sample *s_fcompleted;
extern Sample *s_on_hold;
extern Sample *s_running_out_of_elements;
extern Sample *s_tower_critical;
extern Sample *s_sector_destroyed;
extern Sample *s_mine_destroyed;
extern Sample *s_factory_destroyed;
extern Sample *s_lab_destroyed;
extern Sample *s_itis_all_over;
extern Sample *s_conquered;
extern Sample *s_won;
extern Sample *s_weve_nuked_them;
extern Sample *s_weve_been_nuked;
extern Sample *s_alliance_yes[n_players_c];
extern Sample *s_alliance_no[n_players_c];
extern Sample *s_alliance_ask[n_players_c];
extern Sample *s_cant_nuke_ally;

extern Sample *s_explosion;
extern Sample *s_scream;
extern Sample *s_buildingdestroyed;
extern Sample *s_guiclick;

extern const int epoch_dates[];
extern const char *epoch_names[];

extern Invention *invention_shields[];
extern Invention *invention_defences[];
extern Weapon *invention_weapons[];

extern Element *elements[];

extern Player *players[];

enum GameType {
	GAMETYPE_SINGLEISLAND = 0,
	GAMETYPE_ALLISLANDS = 1
};

enum DifficultyLevel {
	DIFFICULTY_EASY = 0,
	DIFFICULTY_MEDIUM,
	DIFFICULTY_HARD,
	DIFFICULTY_N_LEVELS
};
extern DifficultyLevel difficulty_level;
extern GameType gameType;

int getMenAvailable();
int getNSuspended();

//extern int n_men_store;
//extern int n_men_for_this_island;
extern int start_epoch;
extern int n_sub_epochs;
enum PlayerMode {
	PLAYER_DEMO = -2,
	PLAYER_NONE = -1
};
extern int human_player;
//extern int enemy_player;
extern int end_game_time;
extern bool play_music;

//const int MAP_MAX_NAME = 256;

//#include "sector.h"

class Map {
	string name;
	string filename;
	MapColour colour;
	int n_opponents;
	Sector *sectors[map_width_c][map_height_c];
	bool sector_at[map_width_c][map_height_c];
	//bool temp[map_width_c][map_height_c];

	//void clearTemp();
public:

	Map(MapColour colour,int n_opponents,const char *name);
	~Map();

	MapColour getColour() const {
		return this->colour;
	}
	int getNOpponents() const {
		return this->n_opponents;
	}
	const Sector *getSector(int x, int y) const;
	Sector *getSector(int x, int y);
	bool isSectorAt(int x, int y) const;

	void newSquareAt(int x,int y);
	void createSectors(PlayingGameState *gamestate, int epoch);
#if 0
	void checkSectors() const;
#endif
	void freeSectors();
	const char *getName() const {
		return name.c_str();
	}
	const char *getFilename() const {
		return filename.c_str();
	}
	void setFilename(const char *filename) {
		this->filename = filename;
	}
	int getNSquares() const;
	void draw(int offset_x, int offset_y) const;
	void findRandomSector(int *rx,int *ry) const;
	void canMoveTo(bool temp[map_width_c][map_height_c], int sx,int sy,int player) const;
	void calculateStats() const;
};

extern Screen *screen;
//extern GameState *gamestate;
extern bool debugwindow;

const int max_islands_per_epoch_c = 3;
extern Map *maps[n_epochs_c][max_islands_per_epoch_c];
extern Map *map;

void keypressEscape();
void keypressP();
void keypressQ();
void mouseClick(int m_x, int m_y, bool m_left, bool m_middle, bool m_right, bool click);
void updateGame();
void drawGame();

void playGame(int n_args, char *args[]);
void placeTower();
bool playerAlive(int player);
void quitGame();
void newGame();
void nextEpoch();
void nextIsland();
void returnToChooseIsland();
void startNewGame();
void setGameStateID(GameStateID state);
void endIsland();
void setupPlayers();

bool loadGameInfo(DifficultyLevel *difficulty, int *player, int *n_men, int suspended[n_players_c], int *epoch, bool completed[max_islands_per_epoch_c], int slot);
bool loadGame(int slot);
void saveGame(int slot);
bool validPlayer(int player);
void addTextEffect(TextEffect *effect);

static bool validDifficulty(DifficultyLevel difficulty) {
	return difficulty >= 0 && difficulty < DIFFICULTY_N_LEVELS;
}

// game constants

const int SHORT_DELAY = 4000;
const int nuke_delay_c = 250;

//const int gameticks_per_hour_c = 1000;
const int gameticks_per_hour_c = 200;
const int hours_per_day_c = 12;

const int mine_epoch_c = 3;
const int factory_epoch_c = 4;
const int lab_epoch_c = 5;
//const int air_epoch_c = 6;

/* These values have been checked to be correct (for Epoch 1, at least -
* presumably this doesn't affect things?)
*/
const int DESIGNTIME_M3 = 20;
const int DESIGNTIME_M2 = 30;
const int DESIGNTIME_M1 = 40;
const int DESIGNTIME_0 = 50;
const int DESIGNTIME_1 = 100;
const int DESIGNTIME_2 = 200;
const int DESIGNTIME_3 = 400;

/* These values have been checked to be correct (for Epoch 1, at least -
* presumably this doesn't affect things?)
*/
const int MANUFACTURETIME_0 = 15;
const int MANUFACTURETIME_1 = 30;
const int MANUFACTURETIME_2 = 45;
const int MANUFACTURETIME_3 = 60; // unknown

/* These values have been checked to be correct (for Epoch 1, at least -
* presumably this doesn't affect things?)
*/
const int BUILDTIME_TOWER = 80;
const int BUILDTIME_MINE = 40;
const int BUILDTIME_FACTORY = 40;
const int BUILDTIME_LAB = 40;
