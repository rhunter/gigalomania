//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>
#include <ctime>
#include <cerrno> // n.b., needed on Linux at least

#include <sstream>
using std::stringstream;

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef USING_QT

#ifndef _WIN32
#include <dirent.h>
#include <string.h>
#endif

#ifdef AROS
#include <proto/dos.h>
#endif

#endif

#include "game.h"
#include "utils.h"
#include "sector.h"
#include "gamestate.h"
#include "gui.h"
#include "player.h"

#ifdef USING_QT
#include "qt_screen.h"
#include "qt_image.h"
#include "qt_sound.h"
#else
#include "screen.h"
#include "image.h"
#include "sound.h"
#endif

//---------------------------------------------------------------------------

// onemousebutton means UI can be used with one mouse button only
#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR) || defined(Q_WS_MAEMO_5) || defined(Q_OS_ANDROID) || defined(__ANDROID__)
bool onemousebutton = true;
#else
bool onemousebutton = false;
#endif

bool oneMouseButtonMode() {
	return onemousebutton || application->isBlankMouse();
}

// mobile_ui means no mouse pointer
#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR) || defined(Q_WS_MAEMO_5) || defined(Q_OS_ANDROID) || defined(__ANDROID__)
bool mobile_ui = true;
#else
bool mobile_ui = false;
#endif

bool using_old_gfx = false;

Application *application = NULL;

char *maps_dirname = "islands";
#ifndef USING_QT
    // if using Qt, we use resources even on Linux
#if !defined(__ANDROID__) && defined(__linux)
char *alt_maps_dirname = "/usr/share/gigalomania/islands";
#endif
#endif

//int lastmouseclick_time = 0;

float scale_factor_w = 1.0f; // how much the input graphics are scaled
float scale_factor_h = 1.0f;
float scale_width = 0.0f; // the scale of the logical resolution or graphics size wrt the default 320x240 coordinate system
float scale_height = 0.0f;

int offset_flag_x_c = 22;
int offset_flag_y_c = 6;

bool use_amigadata = true;

GameMode gameMode = GAMEMODE_SINGLEPLAYER;
//GameMode gameMode = GAMEMODE_MULTIPLAYER_CLIENT;

GameType gameType = GAMETYPE_SINGLEISLAND;
//GameType gameType = GAMETYPE_ALLISLANDS;

DifficultyLevel difficulty_level = DIFFICULTY_EASY;

//Image *player_select = NULL;
Image *background = NULL;
Image *land[MAP_N_COLOURS];
Image *fortress[n_epochs_c];
Image *mine[n_epochs_c];
Image *factory[n_epochs_c];
Image *lab[n_epochs_c];
Image *men[n_epochs_c];
Image *unarmed_man = NULL;
Image *flags[n_players_c][n_flag_frames_c];
Image *panel_design = NULL;
//Image *panel_design_dark = NULL;
Image *panel_lab = NULL;
Image *panel_factory = NULL;
Image *panel_shield = NULL;
Image *panel_defence = NULL;
Image *panel_attack = NULL;
Image *panel_bloody_attack = NULL;
Image *panel_twoattack = NULL;
Image *panel_build[N_BUILDINGS];
Image *panel_building[N_BUILDINGS];
Image *panel_knowndesigns = NULL;
Image *panel_bigdesign = NULL;
Image *panel_biglab = NULL;
Image *panel_bigfactory = NULL;
Image *panel_bigshield = NULL;
Image *panel_bigdefence = NULL;
Image *panel_bigattack = NULL;
Image *panel_bigbuild = NULL;
Image *panel_bigknowndesigns = NULL;
Image *numbers_blue[10];
Image *numbers_grey[10];
Image *numbers_white[10];
Image *numbers_orange[10];
Image *numbers_yellow[10];
Image *numbers_largegrey[10];
Image *numbers_largeshiny[10];
Image *numbers_small[n_players_c][10];
Image *numbers_half = NULL;
Image *letters_large[26];
Image *letters_small[26];
Image *mouse_pointers[n_players_c];
Image *playershields[n_playershields_c];
Image *building_health = NULL;
Image *dash_grey = NULL;
Image *icon_shield = NULL;
Image *icon_defence = NULL;
Image *icon_weapon = NULL;
Image *icon_shields[n_shields_c];
Image *icon_defences[n_epochs_c];
Image *icon_weapons[n_epochs_c];
Image *numbered_defences[n_epochs_c];
Image *numbered_weapons[n_epochs_c];
Image *icon_elements[N_ID];
Image *icon_clocks[13];
Image *icon_infinity = NULL;
Image *icon_bc = NULL;
Image *icon_ad = NULL;
Image *icon_ad_shiny = NULL;
Image *icon_towers[n_players_c];
Image *icon_armies[n_players_c];
Image *icon_nuke_hole = NULL;
Image *mine_gatherable_small = NULL;
Image *mine_gatherable_large = NULL;
Image *icon_ergo = NULL;
Image *icon_trash = NULL;
Image *coast_icons[n_coast_c];
int map_sq_offset = 0, map_sq_coast_offset = 0;
Image *map_sq[MAP_N_COLOURS][n_map_sq_c];
Image *defenders[n_players_c][n_epochs_c][n_defender_frames_c];
Image *nuke_defences[n_players_c];
//Image *attackers_walking[n_players_c][n_epochs_c+1][n_attacker_frames_c];
int n_attacker_frames[n_epochs_c+1][n_attacker_directions_c];
Image *attackers_walking[n_players_c][n_epochs_c+1][n_attacker_directions_c][max_attacker_frames_c]; // epochs 6-9 are special case!
Image *planes[n_players_c][n_epochs_c];
Image *nukes[n_players_c][n_nuke_frames_c];
Image *saucers[n_players_c][n_saucer_frames_c];
Image *attackers_ammo[n_epochs_c][N_ATTACKER_AMMO_DIRS];
Image *icon_openpitmine = NULL;
Image *icon_trees[n_trees_c][n_tree_frames_c];
Image *icon_clutter[n_clutter_c];
Image *flashingmapsquare = NULL;
Image *mapsquare = NULL;
Image *arrow_left = NULL;
Image *arrow_right = NULL;
Image *death_flashes[n_death_flashes_c];
Image *blue_flashes[n_blue_flashes_c];
//Image *icon_mice[3];
Image *icon_mice[2];
Image *icon_speeds[3];
Image *smoke_image = NULL;

Image *background_islands = NULL;

// speech
Sample *s_design_is_ready = NULL;
Sample *s_ergo = NULL;
Sample *s_advanced_tech = NULL;
Sample *s_fcompleted = NULL;
Sample *s_on_hold = NULL;
Sample *s_running_out_of_elements = NULL;
Sample *s_tower_critical = NULL;
Sample *s_sector_destroyed = NULL;
Sample *s_mine_destroyed = NULL;
Sample *s_factory_destroyed = NULL;
Sample *s_lab_destroyed = NULL;
Sample *s_itis_all_over = NULL;
Sample *s_conquered = NULL;
Sample *s_won = NULL;
Sample *s_weve_nuked_them = NULL;
Sample *s_weve_been_nuked = NULL;
Sample *s_alliance_yes[n_players_c] = {NULL, NULL, NULL, NULL};
Sample *s_alliance_no[n_players_c] = {NULL, NULL, NULL, NULL};
Sample *s_alliance_ask[n_players_c] = {NULL, NULL, NULL, NULL};
Sample *s_quit[n_players_c] = {NULL, NULL, NULL, NULL};
Sample *s_cant_nuke_ally = NULL;

// effects
Sample *s_explosion = NULL;
Sample *s_scream = NULL;
Sample *s_buildingdestroyed = NULL;
Sample *s_guiclick = NULL;

const unsigned char shadow_alpha_c = (unsigned char)160;

const int epoch_dates[n_epochs_c] = {-10000, -2000, 1, 900, 1400, 1850, 1914, 1944, 1980, 2100};
const char *epoch_names[n_epochs_c] = { "FIRST", "SECOND", "THIRD", "FOURTH", "FIFTH", "SIXTH", "SEVENTH", "EIGHTH", "NINTH", "TENTH" };

Invention *invention_shields[n_epochs_c];
Invention *invention_defences[n_epochs_c];
Weapon *invention_weapons[n_epochs_c];

Element *elements[N_ID];

Player *players[n_players_c];

//bool player_won = false;
GameResult gameResult = GAMERESULT_UNDEFINED;

GameStateID gameStateID = GAMESTATEID_UNDEFINED;
int human_player = 0;
bool isDemo() {
	return human_player == PLAYER_DEMO;
}
//int enemy_player = 1;
//GameWon gameWon = GAME_PLAYING;
bool state_changed = false;
bool paused = false;

//const int n_men_per_epoch_c = 33;
int n_men_store = 0;
int n_suspended[n_players_c];
//int n_men_for_this_island = 0;

int getMenPerEpoch() {
	ASSERT( gameType == GAMETYPE_ALLISLANDS );
	if( difficulty_level == DIFFICULTY_EASY )
		return 150;
	else if( difficulty_level == DIFFICULTY_MEDIUM )
		return 100;
	else if( difficulty_level == DIFFICULTY_HARD )
		return 75;
	ASSERT(false);
	return 0;
}

int getMenAvailable() {
	if( start_epoch == end_epoch_c && gameType == GAMETYPE_ALLISLANDS )
		return n_suspended[human_player];
	return n_men_store;
}

int getNSuspended() {
	return n_suspended[human_player];
}

int start_epoch = 0;
int n_sub_epochs = 4;
int selected_island = 0;
bool completed_island[max_islands_per_epoch_c];
//const int n_islands_c = 10;
Map *maps[n_epochs_c][max_islands_per_epoch_c];
Map *map = NULL;

const Map *getMap() {
	return map;
}

Screen *screen = NULL;
GameState *gamestate = NULL;
//Sector *current_sector = NULL;

const bool default_play_music_c = true;
bool play_music = default_play_music_c; // also affects any sound effects now
/*FMUSIC_MODULE *module = NULL;
FSOUND_STREAM *str_music = NULL;*/
Sample *music = NULL;

#ifdef USING_QT
QSettings *qt_settings = NULL; // n.b., creating on stack means application fails to launch on Nokia 5800?!
const QString play_music_key_c = "play_music";
#endif

Map::Map(MapColour colour,int n_opponents,const char *name) {
	//*this->filename = '\0';
	this->colour = colour;
	this->n_opponents = n_opponents;
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			sector_at[x][y] = false;
			sectors[x][y] = NULL;
			//panels[x][y] = NULL;
		}
	}
	/*for(int i=0;i<N_ID;i++) {
	this->elements[i] = 0;
	}*/
	//clearTemp();
	/*strncpy(this->name,name,MAP_MAX_NAME);
	this->name[MAP_MAX_NAME] = '\0';*/
	this->name = name;
}

Map::~Map() {
	freeSectors();
}

/*void Map::clearTemp() {
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			temp[x][y] = false;
		}
	}
}*/

const Sector *Map::getSector(int x, int y) const {
	ASSERT(x >= 0 && x < map_width_c && y >= 0 && y < map_height_c);
	Sector *sector = this->sectors[x][y];
	return sector;
}

Sector *Map::getSector(int x, int y) {
	ASSERT(x >= 0 && x < map_width_c && y >= 0 && y < map_height_c);
	Sector *sector = this->sectors[x][y];
	return sector;
}

bool Map::isSectorAt(int x, int y) const {
	ASSERT(x >= 0 && x < map_width_c && y >= 0 && y < map_height_c);
	return this->sector_at[x][y];
}

void Map::newSquareAt(int x,int y) {
	ASSERT(x >= 0 && x < map_width_c && y >= 0 && y < map_height_c);
	this->sector_at[x][y] = true;
}

void Map::createSectors(PlayingGameState *gamestate, int epoch) {
	ASSERT_EPOCH(epoch);
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			if( sector_at[x][y] ) {
				this->sectors[x][y] = new Sector(gamestate, epoch, x, y, map->getColour());
				/*for(int i=0;i<N_ID;i++) {
				this->sectors[x][y]->setElements(i, this->elements[i]);
				}*/
			}
		}
	}
}

#if 0
void Map::checkSectors() const {
	//LOG("Map::checkSectors()\n");
#ifdef _WIN32
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			if( sectors[x][y] != NULL ) {
				//LOG("check sector at %d, %d : %d\n", x, y, sectors[x][y]);
				if( !_CrtIsValidHeapPointer( sectors[x][y] ) ) {
					LOG("invalid sector at %d, %d\n", x, y);
					LOG("    ptr %d\n", sectors[x][y]);
					ASSERT( false );
				}
			}
		}
	}
#endif
	//LOG("Map::checkSectors exit\n");
}
#endif

void Map::freeSectors() {
    //LOG("Map::freeSectors()\n");
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			if( sectors[x][y] != NULL ) {
                //LOG("free sector at %d, %d\n", x, y);
                //LOG("    ptr %d\n", sectors[x][y]);
				delete sectors[x][y];
				sectors[x][y] = NULL;
			}
		}
	}
	//current_sector = NULL;
    //LOG("Map::freeSectors exit\n");
}

/*void Map::setElements(int id,int n_elements) {
ASSERT_ELEMENT_ID(id);
this->elements[id] = n_elements;
}*/

void Map::findRandomSector(int *rx,int *ry) const {
	while(true) {
		int x = rand() % map_width_c;
		int y = rand() % map_height_c;
		if( sector_at[x][y] ) {
			*rx = x;
			*ry = y;
			break;
		}
	}
}

void Map::canMoveTo(bool temp[map_width_c][map_height_c], int sx,int sy,int player) const {
	ASSERT(player != -1 );
	ASSERT(this->sector_at[sx][sy]);
	//clearTemp();
	//bool temp[map_width_c][map_height_c];

	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			temp[x][y] = false;
		}
	}

	temp[sx][sy] = true;
	bool changed = false;
	do {
		changed = false;

		for(int x=0;x<map_width_c;x++) {
			for(int y=0;y<map_height_c;y++) {
				if( temp[x][y] ) {
					// can we move through this square?
					if( ( x == sx && y == sy ) ||
						sectors[x][y]->getPlayer() == player ||
						( sectors[x][y]->getPlayer() == -1 &&
						!sectors[x][y]->enemiesPresent(player) ) ) {
							for(int c=0;c<4;c++) {
								int cx = x, cy = y;
								if( c == 0 )
									cy--;
								else if( c == 1 )
									cx++;
								else if( c == 2 )
									cy++;
								else if( c == 3 )
									cx--;
								if( cx >= 0 && cy >= 0 && cx < map_width_c && cy < map_height_c
									&& sector_at[cx][cy]
								&& !sectors[x][y]->isNuked()
									&& !temp[cx][cy] ) {
										temp[cx][cy] = true;
										changed = true;
								}
							}
					}
				}
			}
		}
	} while( changed );
}

void Map::calculateStats() const {
	// deaths = start + births - remaining
	for(int i=0;i<n_players_c;i++) {
		if( players[i] != NULL ) {
			players[i]->setNDeaths( players[i]->getNMenForThisIsland() + players[i]->getNBirths() );
			players[i]->setNSuspended(0);
		}
	}
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			if( this->sector_at[x][y] ) {
				Sector *sector = this->sectors[x][y];
				if( sector->getPlayer() != -1 && players[sector->getPlayer()] != NULL ) {
					// check for players[sector->getPlayer()] not being NULL should be redundant, but just to be safe
					players[sector->getPlayer()]->addNDeaths( - sector->getPopulation() );
					if( sector->isShutdown() && gameResult == GAMERESULT_WON ) {
						//players[sector->getPlayer()]->n_suspended += sector->getPopulation();
						players[sector->getPlayer()]->addNSuspended(sector->getPopulation());
					}
				}
				for(int i=0;i<n_players_c;i++) {
					if( players[i] != NULL ) {
						players[i]->addNDeaths( - sector->getArmy(i)->getTotalMen() );
					}
				}
			}
		}
	}
	/*for(int i=0;i<n_players_c;i++) {
	if( players[i] != NULL )
	ASSERT( players[i]->n_deaths >= 0 );
	}*/
}

/*bool Map::mapIs(char *that_name) {
//return strcmp( this->name, that_name ) == 0;
return this->name == that_name;
}*/

int Map::getNSquares() const {
	int n_squares = 0;
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			if( map->sector_at[x][y] ) {
				n_squares++;
			}
		}
	}
	return n_squares;
}

void Map::draw(int offset_x, int offset_y) const {
    for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			if( map->sector_at[x][y] ) {
				int icon = 0;
				if( y > 0 && map->sector_at[x][y-1] )
					icon += 1;
				if( x < map_width_c-1 && map->sector_at[x+1][y] )
					icon += 2;
				if( y < map_height_c-1 && map->sector_at[x][y+1] )
					icon += 4;
				if( x > 0 && map->sector_at[x-1][y] )
					icon += 8;
				ASSERT( icon >= 0 && icon < 16 );
				if( map_sq[colour][icon] == NULL ) {
					LOG("map name: %s\n", map->getName());
					LOG("ERROR map icon not available [%d,%d]: %d, %d\n", x, y, colour, icon);
					ASSERT( map_sq[colour][icon] != NULL );
				}
				int map_x = offset_x - map_sq_offset + 16 * x;
				int map_y = offset_y - map_sq_offset + 16 * y;
				int coast_map_x = offset_x - map_sq_coast_offset + 16 * x;
				int coast_map_y = offset_y - map_sq_coast_offset + 16 * y;
                //LOG("draw at: %d, %d : %d, %d\n", x, y, map_sq[colour][icon]->getWidth(), map_sq[colour][icon]->getHeight());
                map_sq[colour][icon]->draw(map_x, map_y);
				/*int coast = 15 - icon;
				if( coast > 0 && coast_icons[coast-1] != NULL ) {
				coast_icons[coast-1]->draw(map_x, map_y, true);
				}*/
				//LOG(">>> %d %d %d\n", icon, icon & 1, 4 & 1);
				if( (icon & 1) == 0 && coast_icons[0] != NULL )
					coast_icons[0]->draw(coast_map_x, coast_map_y);
				if( (icon & 2) == 0 && coast_icons[1] != NULL )
					coast_icons[1]->draw(coast_map_x, coast_map_y);
				if( (icon & 4) == 0 && coast_icons[3] != NULL )
					coast_icons[3]->draw(coast_map_x, coast_map_y);
				if( (icon & 8) == 0 && coast_icons[7] != NULL )
					coast_icons[7]->draw(coast_map_x, coast_map_y);
			}
		}
	}
}

void setEpoch(int epoch) {
	LOG("set epoch %d\n", epoch);
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );
	ASSERT( epoch >= 0 && epoch < n_epochs_c );
	start_epoch = epoch;

	n_sub_epochs = 4;
	//if( start_epoch == n_epochs_c-1 )
	if( start_epoch == end_epoch_c )
		n_sub_epochs = 0;
	else if( start_epoch + n_sub_epochs > n_epochs_c ) {
		n_sub_epochs = n_epochs_c - start_epoch;
	}

	selected_island = 0;

	if( gameType == GAMETYPE_ALLISLANDS ) {
		// skip islands we've completed
		while( completed_island[selected_island] ) {
			selected_island++;
			if( selected_island == max_islands_per_epoch_c || maps[start_epoch][selected_island] == NULL ) {
				LOG("error, should be at least one island on this epoch that isn't completed\n");
				ASSERT( false );
			}
		}
	}

	map = maps[start_epoch][selected_island];
    ASSERT( map != NULL );
    gamestate->reset();
}

void drawProgress(int percentage) {
	const int width = (int)(screen->getWidth() * 0.25f);
	const int height = (int)(screen->getHeight()/15.0f);
	const int xpos = (int)(screen->getWidth()*0.5f - width*0.5f);
	const int ypos = (int)(screen->getHeight()*0.5f - height*0.5f);

    screen->clear(); // n.b., needed for Qt/Symbian, where background defaults to white
	screen->fillRect(xpos, ypos, width+1, height+1, 255, 255, 255);
	int progress_width = (int)(((width-1) * percentage) / 100.0f);
	screen->fillRect(xpos+1, ypos+1, progress_width, height-1, 127, 0, 0);
	
	screen->refresh();
#ifdef USING_QT
    application->processEvents(); // needed to update the screen in Qt! (due to being outside of qApp.exec())
#endif
}

void newGame() {
	gamestate->fadeScreen(false, 0, NULL);
	LOG("newGame()\n");
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );
	if( gameType == GAMETYPE_SINGLEISLAND )
		n_men_store = 1000;
	else
		n_men_store = getMenPerEpoch();

	for(int i=0;i<max_islands_per_epoch_c;i++)
		completed_island[i] = false;
	//completed_island[0] = true;

	for(int i=0;i<n_players_c;i++)
		n_suspended[i] = 0;

	setEpoch(0);

	//n_men_store = 1000;
	//setEpoch(9);
}

void setClientPlayer(int set_client_player) {
	::human_player = set_client_player;
	if( gamestate != NULL ) {
		gamestate->setClientPlayer(set_client_player);
	}
}

void nextEpoch() {
	LOG("nextEpoch()\n");
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );
	start_epoch++;
	if( start_epoch == n_epochs_c ) {
		ASSERT( gameType == GAMETYPE_SINGLEISLAND );
		start_epoch = 0;
	}
	for(int i=0;i<max_islands_per_epoch_c;i++)
		completed_island[i] = false;
	setEpoch(start_epoch);
}

void nextIsland() {
	if( gameType == GAMETYPE_SINGLEISLAND ) {
		selected_island++;
		if( selected_island == max_islands_per_epoch_c || maps[start_epoch][selected_island] == NULL )
			selected_island = 0;
	}
	else {
		// skip islands we've completed
		do {
			selected_island++;
			if( selected_island == max_islands_per_epoch_c || maps[start_epoch][selected_island] == NULL )
				selected_island = 0;
		} while( completed_island[selected_island] );
	}
	LOG("next island: %d\n", selected_island);

	map = maps[start_epoch][selected_island];
	LOG("map name: %s\n", map==NULL ? "NULL?!" : map->getName());
	gamestate->reset();
    LOG("done reset\n");
}

char *getFilename(int slot) {
	char name[300] = "";
	sprintf(name, "game_%d.SAV", slot);
	char *filename = getApplicationFilename(name);
    //LOG("filename: %s\n", filename);
	return filename;
}

bool loadGameInfo(DifficultyLevel *difficulty, int *player, int *n_men, int suspended[n_players_c], int *epoch, bool completed[max_islands_per_epoch_c], int slot) {
    //LOG("loadGameInfo(%d)\n",slot);
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );
	ASSERT( slot >= 0 && slot < n_slots_c );

	char *filename = getFilename(slot);
    //LOG("loading: %s\n", filename);
	FILE *file = fopen(filename, "rb+");
	delete [] filename;
	if( file == NULL ) {
        //LOG("FAILED to open file\n");
		return false;
	}

	const int bufsize = 1024;
	char buffer[bufsize+1] = "";
	fgets(buffer, bufsize, file); // header line

	// data line
	/*for(int i=0;i<8;i++) {
	buffer[i] = fgetc(file);
	}*/
	for(int i=0;;) {
		ASSERT( i <= bufsize );
		//buffer[i] = fgetc(file);
		int c = fgetc(file);
		if( c == EOF ) {
			buffer[i] = '\0';
			break;
		}
		buffer[i] = (char)c;
		i++;
	}

	char *ptr = buffer;

	*difficulty = (DifficultyLevel)*((int *)ptr);
	ptr += sizeof(int);
	if( !validDifficulty( *difficulty ) )
		return false;

	*player = *((int *)ptr);
	ptr += sizeof(int);
	if( !validPlayer( *player ) )
		return false;

	*n_men = *((int *)ptr);
	ptr += sizeof(int);
	if( *n_men < 0 )
		return false;

	for(int i=0;i<n_players_c;i++) {
		suspended[i] = *((int *)ptr);
		ptr += sizeof(int);
		if( suspended[i] < 0 )
			return false;
	}

	*epoch = *((int *)ptr);
	ptr += sizeof(int);
	if( *epoch < 0 || *epoch >= n_epochs_c )
		return false;

	for(int i=0;i<max_islands_per_epoch_c;i++) {
		int val = *((int *)ptr);
		if( val != 0 && val != 1 )
			return false;
		completed[i] = (val==1);
		ptr += sizeof(int);
	}

	fclose(file);
	return true;
}

bool loadGame(int slot) {
	LOG("loadGame(%d)\n",slot);
	ASSERT( gameType == GAMETYPE_ALLISLANDS );
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );
	ASSERT( slot >= 0 && slot < n_slots_c );

	DifficultyLevel temp_difficulty = DIFFICULTY_EASY;
	int temp_player = 0;
	int temp_n_men_store = 0;
	int temp_start_epoch = 0;
	int temp_suspended[n_players_c];
	bool temp_completed[max_islands_per_epoch_c];
	if( loadGameInfo(&temp_difficulty, &temp_player, &temp_n_men_store, temp_suspended, &temp_start_epoch, temp_completed, slot) ) {
		difficulty_level = temp_difficulty;
		setClientPlayer(temp_player);
		n_men_store = temp_n_men_store;
		for(int i=0;i<n_players_c;i++)
			n_suspended[i] = temp_suspended[i];
		for(int i=0;i<max_islands_per_epoch_c;i++)
			completed_island[i] = temp_completed[i];
		setEpoch(temp_start_epoch);
		return true;
	}
	return false;
}

void saveGame(int slot) {
	LOG("saveGame(%d)\n",slot);
	ASSERT( gameType == GAMETYPE_ALLISLANDS );
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );
	ASSERT( slot >= 0 && slot < n_slots_c );

	char *filename = getFilename(slot);
	FILE *file = fopen(filename, "wb+");
	delete [] filename;
	if( file == NULL ) {
		LOG("FAILED to open file\n");
		return;
	}

	const int savVersion = 1;
	fprintf(file, "GLMSAV%d.%d.%d\n", majorVersion, minorVersion, savVersion);

	char buffer[256] = "";
	char *ptr = buffer;

	*((int *)ptr) = difficulty_level;
	ptr += sizeof(int);

	*((int *)ptr) = human_player;
	ptr += sizeof(int);

	*((int *)ptr) = n_men_store;
	ptr += sizeof(int);

	for(int i=0;i<n_players_c;i++) {
		*((int *)ptr) = n_suspended[i];
		ptr += sizeof(int);
	}

	*((int *)ptr) = start_epoch;
	ptr += sizeof(int);

	for(int i=0;i<max_islands_per_epoch_c;i++) {
		int val = completed_island[i] ? 1 : 0;
		*((int *)ptr) = val;
		ptr += sizeof(int);
	}

	ptrdiff_t n_bytes = ptr - buffer;
	int sum = 0;
	for(int i=0;i<n_bytes;i++) {
		sum += buffer[i];
	}

	LOG("checksum %d\n", sum);

	*((int *)ptr) = sum;
	ptr += sizeof(int);

	*ptr = '\0';

	//fprintf(file, "%s\n", buffer);
	//fwrite(buffer, (int)ptr - (int)buffer, 1, file);
	ptrdiff_t diff = ptr - buffer;
	fwrite(buffer, diff, 1, file);

	fclose(file);
}

bool validPlayer(int player) {
	bool valid = player >= 0 && player < n_players_c;
	if( !valid ) {
		LOG("ERROR invalid player %d\n", player);
	}
	return valid;
}

void addTextEffect(TextEffect *effect) {
	gamestate->addTextEffect(effect);
}

void stopMusic() {
	/*if( module != NULL ) {
		FMUSIC_FreeSong(module);
		module = NULL;
	}
	if( str_music != NULL ) {
		FSOUND_Stream_Close(str_music);
		str_music = NULL;
	}*/
	if( music != NULL ) {
		delete music;
		music = NULL;
	}
}

void playMusic() {
	stopMusic();
	/*if( !play_music )
	return;*/

	if( gameStateID == GAMESTATEID_CHOOSEPLAYER ) {
		/*music = loadMusic("data/mod.warintro");
		if( music == NULL ) {
			LOG("Failed to load music\n");
		}
		else {
			music->play(SOUND_CHANNEL_MUSIC);
		}*/
	}
	else if( gameStateID == GAMESTATEID_PLACEMEN ) {
		/*music = loadMusic("data/mod.music");
		if( music == NULL ) {
			LOG("Failed to load music\n");
		}
		else {
			music->play(SOUND_CHANNEL_MUSIC);
		}*/
	}
	else if( play_music && gameStateID == GAMESTATEID_PLAYING ) {
		music = Sample::loadMusic("gamemusic.ogg");
		if( music == NULL ) {
			LOG("Failed to load music\n");
		}
		else {
			music->play(SOUND_CHANNEL_MUSIC);
		}
	}
}

bool loadSamples() {
	/*if( use_amigadata )
	{
		s_design_is_ready = loadSample("data/desready", true);
		s_design_is_ready->setText("the design is completed");
		s_ergo = loadSample("data/ergo", true);
		s_ergo->setText("ergonomically terrific");
		s_fcompleted = loadSamplesChained("data/fthe", "data/-prodrun2", NULL);
		s_fcompleted->setText("the production run is completed");
		s_advanced_tech = loadSample("data/teklev2", true);
		s_advanced_tech->setText("we have advanced a tech level");
		s_on_hold = loadSample("data/hold", true);
		//s_on_hold->setText("putting you on hold");
		s_running_out_of_elements = loadSample("data/nelem", true);
		s_running_out_of_elements->setText("running out of elements");
		s_tower_critical = loadSample("data/da-tower", true);
		s_tower_critical->setText("the tower is almost destroyed");
		s_sector_destroyed = loadSample("data/arsect", true);
		s_sector_destroyed->setText("the sector has been destroyed");
		s_mine_destroyed = loadSample("data/ndest", true);
		s_mine_destroyed->setText("the mine is destroyed");
		s_factory_destroyed = loadSamplesChained("data/fthe", "data/ffact", "data/fdest");
		s_factory_destroyed->setText("the factory is destroyed");
		s_lab_destroyed = loadSample("data/labdest", true);
		s_lab_destroyed->setText("the lab is destroyed");
		s_itis_all_over = loadSample("data/da-allover", true);
		s_itis_all_over->setText("game over");
		s_conquered = loadSamplesChained("data/arweve", "data/arconq", NULL);
		s_conquered->setText("we have conquered the sector");
		s_won = loadSamplesChained("data/arweve", "data/arwon", NULL);
		s_won->setText("we have won");
		s_weve_nuked_them = loadSamplesChained("data/arweve", "data/arnuke", NULL);
		s_weve_nuked_them->setText("we nuked them");
		s_weve_been_nuked = loadSample("data/da-nuked", true);
		s_weve_been_nuked->setText("we have been nuked");
		s_alliance_yes[0] = loadSamplesChained("data/dwhy", "data/dall", NULL);
		s_alliance_yes[0]->setText("red team says okay");
		s_alliance_yes[1] = loadSample("data/asisi", true);
		s_alliance_yes[1]->setText("green team says okay");
		s_alliance_yes[2] = loadSample("data/PVERYWELL", true);
		s_alliance_yes[2]->setText("yellow team says okay");
		s_alliance_yes[3] = loadSample("data/yougotit", true);
		s_alliance_yes[3]->setText("blue team says okay");
		s_alliance_no[0] = loadSamplesChained("data/dha", "data/NO", NULL);
		s_alliance_no[0]->setText("red team says no");
		s_alliance_no[1] = loadSample("data/anowork", true);
		s_alliance_no[1]->setText("green team says no");
		s_alliance_no[2] = loadSample("data/pno", true);
		s_alliance_no[2]->setText("yellow team says no");
		s_alliance_no[3] = loadSample("data/noway", true);
		s_alliance_no[3]->setText("blue team says no");
		s_alliance_ask[0] = loadSample("data/djoin", true);
		s_alliance_ask[0]->setText("red team requests alliance");
		s_alliance_ask[1] = loadSample("data/awanna", true);
		s_alliance_ask[1]->setText("green team requests alliance");
		s_alliance_ask[2] = loadSample("data/pmyteam", true);
		s_alliance_ask[2]->setText("yellow team requests alliance");
		s_alliance_ask[3] = loadSample("data/myside2", true);
		s_alliance_ask[3]->setText("blue team requests alliance");
		s_quit[0] = loadSamplesChained("data/dha", "data/dpath", NULL);
		s_quit[1] = loadSample("data/alaff", true);
		s_quit[2] = loadSample("data/pmmm2", true);
		s_quit[3] = loadSample("data/laff", true);
	}*/

	// no longer supporting speech samples
	s_design_is_ready = new Sample();
	s_design_is_ready->setText("the design is completed");
	s_ergo = new Sample();
	s_ergo->setText("ergonomically terrific");
	s_fcompleted = new Sample();
	s_fcompleted->setText("the production run is finished");
	s_advanced_tech = new Sample();
	s_advanced_tech->setText("we have advanced to the next tech level");
	s_running_out_of_elements = new Sample();
	s_running_out_of_elements->setText("run out of elements");
	s_tower_critical = new Sample();
	s_tower_critical->setText("the tower is almost destroyed");
	s_sector_destroyed = new Sample();
	s_sector_destroyed->setText("the sector has been destroyed");
	s_mine_destroyed = new Sample();
	s_mine_destroyed->setText("the mine is destroyed");
	s_factory_destroyed = new Sample();
	s_factory_destroyed->setText("the factory is destroyed");
	s_lab_destroyed = new Sample();
	s_lab_destroyed->setText("the lab is destroyed");
	s_itis_all_over = new Sample();
	s_itis_all_over->setText("game over");
	s_conquered = new Sample();
	s_conquered->setText("we have conquered the sector");
	s_won = new Sample();
	s_won->setText("we have won");
	s_weve_nuked_them = new Sample();
	s_weve_nuked_them->setText("we have nuked them");
	s_weve_been_nuked = new Sample();
	s_weve_been_nuked->setText("we have been nuked");
	s_alliance_yes[0] = new Sample();
	s_alliance_yes[0]->setText("red team says okay");
	s_alliance_yes[1] = new Sample();
	s_alliance_yes[1]->setText("green team says okay");
	s_alliance_yes[2] = new Sample();
	s_alliance_yes[2]->setText("yellow team says okay");
	s_alliance_yes[3] = new Sample();
	s_alliance_yes[3]->setText("blue team says okay");
	s_alliance_no[0] = new Sample();
	s_alliance_no[0]->setText("red team says no");
	s_alliance_no[1] = new Sample();
	s_alliance_no[1]->setText("green team says no");
	s_alliance_no[2] = new Sample();
	s_alliance_no[2]->setText("yellow team says no");
	s_alliance_no[3] = new Sample();
	s_alliance_no[3]->setText("blue team says no");
	s_alliance_ask[0] = new Sample();
	s_alliance_ask[0]->setText("red team requests alliance");
	s_alliance_ask[1] = new Sample();
	s_alliance_ask[1]->setText("green team requests alliance");
	s_alliance_ask[2] = new Sample();
	s_alliance_ask[2]->setText("yellow team requests alliance");
	s_alliance_ask[3] = new Sample();
	s_alliance_ask[3]->setText("blue team requests alliance");

	// text messages without corresponding samples
	//s_cant_nuke_ally = new Sample(NULL);
	s_cant_nuke_ally = new Sample();
	s_cant_nuke_ally->setText("cannot nuke our ally");

	// no samples or text messages, but keep supported in case we re-add samples later
	s_on_hold = new Sample();
	s_quit[0] = new Sample();
	s_quit[1] = new Sample();
	s_quit[2] = new Sample();
	s_quit[3] = new Sample();

	string sound_dir = "sound/";
	// sound effects
	s_explosion = Sample::loadSample(sound_dir + "bomb.wav");
#ifndef USING_QT
    // if using Qt, we use resources even on Linux
#if !defined(__ANDROID__) && defined(__linux)
        if( s_explosion == NULL || errorSound() ) {
            if( s_explosion != NULL ) {
                delete s_explosion;
                s_explosion = NULL;
            }
		resetErrorSound();
		sound_dir = "/usr/share/gigalomania/" + sound_dir;
		LOG("look in %s for sound\n", sound_dir.c_str());
        s_explosion = Sample::loadSample(sound_dir + "bomb.wav");
	}
#endif
#endif

#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR)
    s_scream = new Sample(); // playing this sample causes strange pauses on Symbian?? (Nokia 5800) Also probably best not to have this sound sample until we fix problem of volume control not having an effect...
#else
    s_scream = Sample::loadSample(sound_dir + "pain1.wav");
#endif
	s_buildingdestroyed = Sample::loadSample(sound_dir + "woodbrk.wav");
#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR)
    s_guiclick = new Sample(); // on Symbian, we use vibration feedback
#else
    s_guiclick = Sample::loadSample(sound_dir + "misc_menu_3.wav");
#endif
    bool ok = !errorSound();
	return ok;
}

bool remapLand(Image *image,MapColour colour) {
	for(int y=0;y<image->getHeight();y++) {
		for(int x=0;x<image->getWidth();x++) {
			unsigned char c = image->getPixelIndex(x,y);
			if( c == 0 ) {
				// leave as 0
			}
			else if( c == 1 ) {
				// light
				int c2 = 0;
				if( colour == MAP_ORANGE )
					c2 = 15;
				else if( colour == MAP_GREEN )
					c2 = 1;
				else if( colour == MAP_BROWN )
					c2 = 14;
				else if( colour == MAP_WHITE )
					c2 = 2;
				else if( colour == MAP_DBROWN )
					c2 = 6;
				else if( colour == MAP_DGREEN )
					c2 = 12;
				else if( colour == MAP_GREY )
					c2 = 1;
				else {
					ASSERT(false);
				}
				image->setPixelIndex(x, y, c2);
			}
			else if( c == 2 ) {
				// medium
				int c2 = 0;
				if( colour == MAP_ORANGE )
					c2 = 14;
				else if( colour == MAP_GREEN )
					c2 = 12;
				else if( colour == MAP_BROWN )
					c2 = 6;
				else if( colour == MAP_WHITE )
					c2 = 1;
				else if( colour == MAP_DBROWN )
					c2 = 8;
				else if( colour == MAP_DGREEN )
					c2 = 7;
				else if( colour == MAP_GREY )
					c2 = 5;
				else {
					ASSERT(false);
				}
				image->setPixelIndex(x, y, c2);
			}
			else if( c == 3 ) {
				// dark
				int c2 = 0;
				if( colour == MAP_ORANGE )
					c2 = 6;
				else if( colour == MAP_GREEN )
					c2 = 7;
				else if( colour == MAP_BROWN )
					c2 = 8;
				else if( colour == MAP_WHITE )
					c2 = 5;
				else if( colour == MAP_DBROWN )
					c2 = 3;
				else if( colour == MAP_DGREEN )
					c2 = 8;
				else if( colour == MAP_GREY )
					c2 = 4;
				else {
					ASSERT(false);
				}
				image->setPixelIndex(x, y, c2);
			}
			else {
				//LOG("land slabs should only have colour indices 0-3\n");
				//return false;
			}
		}
	}
	return true;
}

void convertToHiColor(Image *image) {
	if( using_old_gfx ) {
		// create alpha from color keys
		image->createAlphaForColor(true, 255, 0, 255, 127, 0, 127, shadow_alpha_c);
	}
	image->convertToHiColor(false);
}

void processImage(Image *image, bool old_smooth = true) {
    //LOG("    convert to hi color\n");
	convertToHiColor(image);
    //LOG("    scale\n");
    image->scale(scale_factor_w, scale_factor_h);
    //LOG("    set scale\n");
    image->setScale(scale_width, scale_height);
#if SDL_MAJOR_VERSION == 1
	// with SDL 2, we let SDL do smoothing when scaling the graphics on the GPU
	if( using_old_gfx && old_smooth ) {
        image->smooth();
	}
#endif
    //LOG("    done\n");
}

bool loadAttackersWalkingImages(const string &gfx_dir, int epoch) {
	char filename[300] = "";
	sprintf(filename, "attacker_walking_%d.png", epoch);
	Image *gfx_image = Image::loadImage(gfx_dir + filename);
	// if NULL, look for direction specific graphics
	if( gfx_image != NULL ) {
	    gfx_image->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
	}
	for(int dir=0;dir<n_attacker_directions_c;dir++) {
		bool direction_specific = false;
		if( gfx_image == NULL ) {
			// load direction specific image
			//LOG("try loading direction specific images for epoch %d dir %d\n", epoch, dir);
			direction_specific = true;
			sprintf(filename, "attacker_walking_%d_%d.png", epoch, dir);
			gfx_image = Image::loadImage(gfx_dir + filename);
			if( gfx_image == NULL ) {
				LOG("failed to load attacker walking image for epoch %d dir %d\n", epoch, dir);
				return false;
			}
		    gfx_image->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
		}
		int height_per_frame = gfx_image->getScaledHeight();
		int width_per_frame = height_per_frame;
		n_attacker_frames[epoch][dir] = gfx_image->getScaledWidth()/width_per_frame;
		//LOG("epoch %d, direction %d has %d frames\n", epoch, dir, n_attacker_frames[epoch][dir]);
		// need to update max_attacker_frames_c if we ever want to allow more frames!
		ASSERT( n_attacker_frames[epoch][dir] <= max_attacker_frames_c );
		for(int player=0;player<n_players_c;player++) {
			int n_frames = n_attacker_frames[epoch][dir];
			for(int frame=0;frame<n_frames;frame++) {
				attackers_walking[player][epoch][dir][frame] = gfx_image->copy(width_per_frame*frame, 0, width_per_frame, height_per_frame);
				int r = 0, g = 0, b = 0;
				PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)player);
				attackers_walking[player][epoch][dir][frame]->remap(240, 0, 0, r, g, b);
				processImage(attackers_walking[player][epoch][dir][frame]);
			}
		}
		if( direction_specific ) {
			delete gfx_image;
			gfx_image = NULL;
		}
	}
	if( gfx_image != NULL ) {
		delete gfx_image;
	}
	return true;
}

void calculateScale(const Image *image) {
#if SDL_MAJOR_VERSION == 1
	scale_factor_w = ((float)(scale_width*default_width_c))/(float)image->getWidth();
	scale_factor_h = ((float)(scale_height*default_height_c))/(float)image->getHeight();
	LOG("scale factor for images = %f X %f\n", scale_factor_w, scale_factor_h);
#else
	// with SDL 2, we don't scale the graphics, and instead set the logical size to match the graphics
	scale_factor_w = 1.0f;
	scale_factor_h = 1.0f;
	scale_width = ((float)(image->getWidth()))/(float)default_width_c;
	scale_height = ((float)(image->getHeight()))/(float)default_height_c;
	LOG("scale width/height of logical resolution = %f X %f\n", scale_width, scale_height);
	screen->setLogicalSize((int)(scale_width*default_width_c), (int)(scale_height*default_height_c), true);
#endif
}

bool loadOldImages() {
	// progress should go from 0 to 80%
	LOG("try using old graphics\n");
	using_old_gfx = true;

	background = Image::loadImage("data/mlm_sunrise");

	if( background == NULL )
		return false;
	drawProgress(20);
	// do equivalent for calculateScale(), but for a 320x240 image (n.b., not 320x256)
#if SDL_MAJOR_VERSION == 1
	scale_factor_w = scale_width;
	scale_factor_h = scale_height;
	LOG("scale factor for images = %f X %f\n", scale_factor_w, scale_factor_h);
#else
	// with SDL 2, we don't scale the graphics, and instead set the logical size to match the graphics
	scale_factor_w = 1.0f;
	scale_factor_h = 1.0f;
	scale_width = 1.0f;
	scale_height = 1.0f;
	LOG("scale width/height of logical resolution = %f X %f\n", scale_width, scale_height);
	screen->setLogicalSize((int)(scale_width*default_width_c), (int)(scale_height*default_height_c), false); // don't smooth, as doesn't look too good with old graphics
#endif

	// nb, still scale if scale_factor==1, as this is a way of converting to 8bit
	processImage(background);

	Image *image_slabs = Image::loadImage("data/mlm_slabs");
	if( image_slabs == NULL )
		return false;
	drawProgress(30);
	for(int i=0;i<MAP_N_COLOURS;i++) {
		land[i] = image_slabs->copy();
		//land[i]->scale(scale_factor, scale_factor); // also forces to 8bit (since should be paletted)
		land[i]->scale(scale_factor_w, scale_factor_h); // also forces to 8bit (since should be paletted)
		land[i]->setScale(scale_width, scale_height);
		if( !land[i]->isPaletted() ) {
			LOG("land slabs should be paletted\n");
			return false;
		}
		else if( land[i]->getNColors() < 16 ) { // should be 256 by now anyway, due to scaling
			LOG("land slabs should have at least 16 colours\n");
			return false;
		}
		else {
			// 1, 2, 3, 4, 5, 6, 7, 8, 12, 14, 15
			land[i]->setColor(0, 255, 0, 255);
			land[i]->setColor(1, 176, 176, 176);
			land[i]->setColor(2, 240, 240, 240);
			land[i]->setColor(3, 0, 0, 0);
			land[i]->setColor(4, 64, 64, 64);
			land[i]->setColor(5, 112, 112, 112);
			land[i]->setColor(6, 160, 64, 0);
			land[i]->setColor(7, 0, 96, 0);
			land[i]->setColor(8, 112, 36, 16);
			land[i]->setColor(12, 0, 192, 0);
			land[i]->setColor(14, 240, 112, 16);
			land[i]->setColor(15, 240, 240, 0);
		}
		if( !remapLand(land[i], (MapColour)i) ) {
			LOG("failed to remap land\n");
			return false;
		}
		convertToHiColor(land[i]);
#if SDL_MAJOR_VERSION == 1
		// with SDL 2, we let SDL do smoothing when scaling the graphics on the GPU
		land[i]->smooth();
#endif
	}
	delete image_slabs;
	image_slabs = NULL;

	for(int i=0;i<n_epochs_c;i++) {
		fortress[i] = NULL;
		mine[i] = NULL;
		factory[i] = NULL;
	}

	Image *buildings = Image::loadImage("data/mlm_buildings");
	if( buildings == NULL )
		return false;
	buildings->setColor(0, 255, 0, 255);
	processImage(buildings);

	for(int i=0;i<n_epochs_c;i++) {
		fortress[i] = buildings->copy( 0, 60*i, 58, 60);
		mine[i] = buildings->copy( 64, 60*i, 58, 60);
		factory[i] = buildings->copy( 192, 60*i, 58, 60);
		lab[i] = buildings->copy( 128, 60*i, 58, 60);
	}
	delete buildings;
	buildings = NULL;
	drawProgress(40);

	Image *icons = Image::loadImage("data/mlm_icons");
	if( icons == NULL )
		return false;
	if( !icons->scaleTo((int)(scale_width*default_width_c)) ) // must use this method, as still using alongside the new gfx images
		return false;
	//icons->scale(scale_factor, scale_factor);
	icons->setScale(scale_width, scale_height);
	icons->setColor(0, 255, 0, 255);
	convertToHiColor(icons);
#if SDL_MAJOR_VERSION == 1
	// with SDL 2, we let SDL do smoothing when scaling the graphics on the GPU
	icons->smooth();
#endif

	for(int i=0;i<n_epochs_c;i++)
		men[i] = icons->copy(16*i, 0, 16, 16);

	unarmed_man = icons->copy(80, 32, 16, 16);

	panel_design = icons->copy(304, 0, 16, 16);
	//panel_design_dark = panel_design->copy();
	panel_lab = icons->copy(16, 33, 16, 16);
	panel_shield = icons->copy(240, 0, 16, 16);
	panel_defence = icons->copy(256, 0, 16, 16);
	panel_attack = icons->copy(272, 0, 16, 16);
	panel_knowndesigns = icons->copy(240, 16, 16, 16);
	panel_factory = icons->copy(48, 33, 16, 16);
	panel_bloody_attack = icons->copy(240, 32, 16, 16);

	mine_gatherable_small = icons->copy(160, 0, 16, 16);

	//panel_build[BUILDING_TOWER] = icons->copy(224, 63, 19, 16); // not yet used
	panel_build[BUILDING_MINE] = icons->copy(256, 63, 19, 16);
	panel_build[BUILDING_FACTORY] = icons->copy(288, 63, 19, 16);
	panel_build[BUILDING_LAB] = icons->copy(192, 63, 19, 16);

	panel_building[BUILDING_TOWER] = icons->copy(0, 33, 16, 14);
	panel_building[BUILDING_MINE] = icons->copy(32, 33, 16, 14);
	//panel_building[BUILDING_FACTORY] = icons->copy(48, 33, 16, 14);
	//panel_building[BUILDING_LAB] = icons->copy(16, 33, 16, 14);
	panel_building[BUILDING_FACTORY] = panel_factory;
	panel_building[BUILDING_LAB] = panel_lab;

	mine_gatherable_large = icons->copy(160, 64, 32, 16);

	panel_bigdesign = icons->copy(256, 48, 32, 15);
	panel_biglab = icons->copy(96, 48, 32, 16);
	panel_bigfactory = icons->copy(128, 48, 32, 16);
	panel_bigshield = icons->copy(160, 48, 32, 15);
	panel_bigdefence = icons->copy(192, 48, 32, 15);
	panel_bigattack = icons->copy(224, 48, 32, 15);
	panel_bigbuild = icons->copy(32, 49, 32, 15);
	panel_bigknowndesigns = icons->copy(288, 48, 32, 15);
	panel_twoattack = icons->copy(64, 33, 15, 15);

	for(int i=0;i<10;i++)
		numbers_blue[i] = icons->copy(16*i, 64, 6, 8);
	for(int i=0;i<10;i++)
		numbers_grey[i] = icons->copy(16*i, 72, 6, 8);
	for(int i=0;i<10;i++)
		numbers_white[i] = icons->copy(16*i, 80, 6, 8);
	for(int i=0;i<10;i++)
		numbers_orange[i] = icons->copy(16*i, 88, 6, 8);
	for(int i=0;i<10;i++)
		numbers_yellow[i] = icons->copy(16*i, 119, 6, 8);
	for(int i=0;i<3;i++)
		numbers_largeshiny[i] = icons->copy(16*i, 269, 6, 15);
	for(int i=3;i<10;i++)
		numbers_largeshiny[i] = NULL; // not used
	for(int i=0;i<10;i++)
		numbers_largegrey[i] = icons->copy(64 + 16*i, 269, 6, 15);
	for(int i=0;i<10;i++)
		numbers_small[0][i] = icons->copy(16*i, 126, 5, 7);
	for(int i=0;i<10;i++)
		numbers_small[1][i] = icons->copy(16*i, 134, 5, 7);
	for(int i=0;i<10;i++)
		numbers_small[2][i] = icons->copy(160 + 16*i, 126, 5, 7);
	for(int i=0;i<10;i++)
		numbers_small[3][i] = icons->copy(160 + 16*i, 134, 5, 7);
	numbers_half = icons->copy(160, 119, 6, 8);

	for(int i=0;i<4;i++)
		letters_large[i] = icons->copy(256 + 16*i, 269, 6, 15);
	for(int i=0;i<20;i++)
		letters_large[4+i] = icons->copy(16*i, 284, 6, 15);
	for(int i=0;i<2;i++)
		letters_large[24+i] = icons->copy(16*i, 299, 6, 15);

	for(int i=0;i<15;i++)
		letters_small[i] = icons->copy(80 + 16*i, 299, 6, 8);
	for(int i=0;i<11;i++)
		letters_small[15+i] = icons->copy(80 + 16*i, 307, 6, 8);

	for(int i=0;i<n_players_c;i++)
		mouse_pointers[i] = icons->copy(176 + 16*i, 0, 16, 16);

	for(int i=0;i<13;i++)
		icon_clocks[i] = icons->copy(16*i, 16, 16, 16);

	/*for(int i=0;i<3;i++)
		icon_mice[i] = icons->copy(256 + 16*i, 16, 16, 19);*/
	for(int i=0;i<2;i++)
		icon_mice[i] = icons->copy(256 + 16*i, 16, 16, 19);

	for(int i=0;i<n_death_flashes_c;i++)
		death_flashes[i] = icons->copy(272 + 16*i, 346, 16, 19);

	for(int i=0;i<5;i++)
		blue_flashes[i] = icons->copy(225 + 16*i, 173, 16, 16);
	blue_flashes[5] = icons->copy(305, 165, 15, 19);
	blue_flashes[6] = icons->copy(305, 184, 15, 17);

	for(int i=0;i<n_players_c;i++) {
		icon_towers[i] = icons->copy(160 + 16*i, 81, 6, 6);
		icon_armies[i] = icons->copy(160 + 16*i, 87, 4, 4);
	}

	for(int i=0;i<N_ID;i++)
		icon_elements[i] = icons->copy(16*i, 141, 16, 16);

	for(int i=0;i<n_players_c;i++)
		flags[i][0] = icons->copy(160 + 16*i, 157, 16, 15);
	for(int i=0;i<n_players_c;i++)
		flags[i][1] = icons->copy(224 + 16*i, 157, 16, 15);
	for(int i=0;i<n_players_c;i++)
		flags[i][2] = icons->copy(160 + 16*i, 172, 16, 15);
	for(int i=0;i<n_players_c;i++)
		flags[i][3] = flags[i][1];

	for(int i=0;i<n_playershields_c;i++)
		playershields[i] = icons->copy(16*i, 189, 16, 14);

	for(int i=0;i<n_shields_c;i++)
		icon_shields[i] = icons->copy(112 + 16*i, 32, 16, 16);
	for(int i=0;i<n_epochs_c;i++)
		icon_defences[i] = icons->copy(16*i, 253, 16, 16);
	for(int i=0;i<n_epochs_c;i++)
		icon_weapons[i] = icons->copy(16*i, 237, 16, 16);
	for(int i=0;i<n_epochs_c;i++)
		numbered_defences[i] = icons->copy(16*i, 173, 16, 16);
	for(int i=0;i<n_epochs_c;i++)
		numbered_weapons[i] = icons->copy(16*i, 157, 16, 16);

	for(int i=0;i<3;i++)
		icon_speeds[i] = icons->copy(272 + 16*i, 204, 16, 18);

	building_health = icons->copy(0, 203, 41, 5);
	dash_grey = icons->copy(144, 114, 5, 2);
	icon_shield = icons->copy(272, 315, 16, 16);
	icon_defence = icons->copy(288, 315, 16, 16);
	icon_weapon = icons->copy(304, 315, 16, 16);

	icon_infinity = icons->copy(112, 104, 12, 8);

	icon_bc = icons->copy(240, 276, 12, 8);
	icon_ad = icons->copy(224, 276, 12, 8);
	icon_ad_shiny = icons->copy(48, 276, 12, 8);

	icon_nuke_hole = icons->copy(288, 99, 9, 9);

	icon_ergo = icons->copy(176, 111, 16, 16);
	icon_trash = icons->copy(192, 111, 16, 16);

	mapsquare = icons->copy(288, 81, 17, 17);
	flashingmapsquare = icons->copy(192, 92, 17, 17);

	delete icons;
	icons = NULL;
	drawProgress(50);

	for(int i=0;i<n_coast_c;i++)
		coast_icons[i] = NULL;

	Image *smallmap = Image::loadImage("data/mlm_smallmap");
	if( smallmap == NULL )
		return false;
	smallmap->setColor(0, 255, 0, 255);
	processImage(smallmap);
	map_sq_offset = 3;
	map_sq_coast_offset = 3;

	for(int i=0;i<MAP_N_COLOURS;i++) {
		for(int j=0;j<n_map_sq_c;j++)
			map_sq[i][j] = NULL;
	}

	map_sq[MAP_ORANGE][14] = smallmap->copy(0, 0, 22, 22);
	map_sq[MAP_ORANGE][7] = smallmap->copy(32, 0, 22, 22);
	map_sq[MAP_ORANGE][11] = smallmap->copy(64, 0, 22, 22);
	map_sq[MAP_ORANGE][4] = smallmap->copy(96, 0, 22, 22);
	map_sq[MAP_ORANGE][5] = smallmap->copy(128, 0, 22, 22);
	map_sq[MAP_ORANGE][1] = smallmap->copy(160, 0, 22, 22);
	map_sq[MAP_ORANGE][2] = smallmap->copy(192, 0, 22, 22);
	map_sq[MAP_ORANGE][8] = smallmap->copy(224, 0, 22, 22);

	map_sq[MAP_GREEN][6] = smallmap->copy(256, 0, 22, 22);
	map_sq[MAP_GREEN][12] = smallmap->copy(288, 0, 22, 22);
	map_sq[MAP_GREEN][3] = smallmap->copy(0, 22, 22, 22);
	map_sq[MAP_GREEN][9] = smallmap->copy(32, 22, 22, 22);
	map_sq[MAP_GREEN][5] = smallmap->copy(64, 22, 22, 22);
	map_sq[MAP_GREEN][10] = smallmap->copy(96, 22, 22, 22);

	map_sq[MAP_BROWN][6] = smallmap->copy(128, 22, 22, 22);
	map_sq[MAP_BROWN][4] = smallmap->copy(160, 22, 22, 22);
	map_sq[MAP_BROWN][1] = smallmap->copy(192, 22, 22, 22);
	map_sq[MAP_BROWN][8] = smallmap->copy(224, 22, 22, 22);
	map_sq[MAP_BROWN][0] = smallmap->copy(256, 22, 22, 22);

	map_sq[MAP_DBROWN][6] = smallmap->copy(32, 66, 22, 22);
	map_sq[MAP_DBROWN][12] = smallmap->copy(64, 66, 22, 22);
	map_sq[MAP_DBROWN][7] = smallmap->copy(96, 66, 22, 22);
	map_sq[MAP_DBROWN][13] = smallmap->copy(128, 66, 22, 22);
	map_sq[MAP_DBROWN][3] = smallmap->copy(160, 66, 22, 22);
	map_sq[MAP_DBROWN][11] = smallmap->copy(192, 66, 22, 22);
	map_sq[MAP_DBROWN][9] = smallmap->copy(224, 66, 22, 22);
	map_sq[MAP_DBROWN][4] = smallmap->copy(256, 66, 22, 22);
	map_sq[MAP_DBROWN][5] = smallmap->copy(288, 66, 22, 22);
	map_sq[MAP_DBROWN][1] = smallmap->copy(0, 88, 22, 22);
	map_sq[MAP_DBROWN][2] = smallmap->copy(32, 88, 22, 22);
	map_sq[MAP_DBROWN][10] = smallmap->copy(64, 88, 22, 22);

	map_sq[MAP_WHITE][1] = smallmap->copy(224, 44, 22, 22);
	map_sq[MAP_WHITE][2] = smallmap->copy(256, 44, 22, 22);
	map_sq[MAP_WHITE][3] = smallmap->copy(96, 44, 22, 22);
	map_sq[MAP_WHITE][4] = smallmap->copy(192, 44, 22, 22);
	map_sq[MAP_WHITE][6] = smallmap->copy(288, 22, 22, 22);
	map_sq[MAP_WHITE][8] = smallmap->copy(0, 66, 22, 22);
	map_sq[MAP_WHITE][9] = smallmap->copy(160, 44, 22, 22);
	map_sq[MAP_WHITE][10] = smallmap->copy(288, 44, 22, 22);
	map_sq[MAP_WHITE][11] = smallmap->copy(128, 44, 22, 22);
	map_sq[MAP_WHITE][12] = smallmap->copy(32, 44, 22, 22);
	map_sq[MAP_WHITE][14] = smallmap->copy(0, 44, 22, 22);
	map_sq[MAP_WHITE][15] = smallmap->copy(64, 44, 22, 22);

	map_sq[MAP_DGREEN][6] = smallmap->copy(96, 88, 22, 22);
	map_sq[MAP_DGREEN][14] = smallmap->copy(128, 88, 22, 22);
	map_sq[MAP_DGREEN][12] = smallmap->copy(160, 88, 22, 22);
	map_sq[MAP_DGREEN][7] = smallmap->copy(192, 88, 22, 22);
	map_sq[MAP_DGREEN][15] = smallmap->copy(224, 88, 22, 22);
	map_sq[MAP_DGREEN][13] = smallmap->copy(256, 88, 22, 22);
	map_sq[MAP_DGREEN][3] = smallmap->copy(288, 88, 22, 22);
	map_sq[MAP_DGREEN][11] = smallmap->copy(0, 110, 22, 22);
	map_sq[MAP_DGREEN][9] = smallmap->copy(32, 110, 22, 22);

	map_sq[MAP_GREY][0] = smallmap->copy(0, 132, 22, 22);
	map_sq[MAP_GREY][1] = smallmap->copy(192, 110, 22, 22);
	map_sq[MAP_GREY][2] = smallmap->copy(224, 110, 22, 22);
	map_sq[MAP_GREY][4] = smallmap->copy(128, 110, 22, 22);
	map_sq[MAP_GREY][5] = smallmap->copy(160, 110, 22, 22);
	map_sq[MAP_GREY][8] = smallmap->copy(288, 110, 22, 22);
	map_sq[MAP_GREY][10] = smallmap->copy(256, 110, 22, 22);
	map_sq[MAP_GREY][12] = smallmap->copy(64, 110, 22, 22);
	map_sq[MAP_GREY][15] = smallmap->copy(96, 110, 22, 22);

	delete smallmap;
	smallmap = NULL;
	drawProgress(60);

	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<=n_epochs_c;j++) {
			for(int k=0;k<n_attacker_directions_c;k++) {
				for(int l=0;l<max_attacker_frames_c;l++) {
					attackers_walking[k][i][j][l] = NULL;
				}
			}
		}
	}
	for(int j=0;j<=n_epochs_c;j++) {
		for(int k=0;k<n_attacker_directions_c;k++) {
			n_attacker_frames[j][k] = 3;
		}
	}
	for(int i=0;i<n_epochs_c;i++)
		for(int j=0;j<N_ATTACKER_AMMO_DIRS;j++)
			attackers_ammo[i][j] = NULL;

	Image *armies = Image::loadImage("data/mlm_armies");
	if( armies == NULL ) {
		return false;
	}
	armies->setColor(0, 255, 0, 255);
	processImage(armies, false); // don't smooth, as it messes up the colour remapping!

	for(int i=0;i<=5;i++) {
		for(int j=0;j<n_defender_frames_c;j++) {
			for(int k=0;k<n_players_c;k++)
				defenders[k][i][j] = armies->copy(16*j, 16 + 32*i, 16, 16);
		}
	}
	for(int j=0;j<n_defender_frames_c;j++) {
		for(int k=0;k<n_players_c;k++)
			defenders[k][6][j] = armies->copy(128 + 16*j, 192, 16, 16);
	}
	for(int j=0;j<2;j++) {
		for(int k=0;k<n_players_c;k++)
			defenders[k][7][j] = armies->copy(224 + 16*j, 240, 16, 16);
	}
	for(int j=2;j<4;j++) {
		for(int k=0;k<n_players_c;k++)
			defenders[k][7][j] = armies->copy(288 + 16*(j-2), 224, 16, 16);
	}
	for(int j=4;j<6;j++) {
		for(int k=0;k<n_players_c;k++)
			defenders[k][7][j] = armies->copy(256 + 16*(j-4), 240, 16, 16);
	}
	for(int j=6;j<8;j++) {
		for(int k=0;k<n_players_c;k++)
			defenders[k][7][j] = armies->copy(288 + 16*(j-6), 240, 16, 16);
	}
	for(int j=0;j<n_defender_frames_c;j++) {
		defenders[0][8][j] = armies->copy(192, 256, 16, 16);
		defenders[1][8][j] = armies->copy(192, 272, 16, 16);
		defenders[2][8][j] = armies->copy(208, 256, 16, 16);
		defenders[3][8][j] = armies->copy(208, 272, 16, 16);
	}
	for(int k=0;k<n_players_c;k++) {
		int kx = k / 2;
		int ky = k % 2;
		for(int j=0;j<n_defender_frames_c;j++) {
			int j2 = j % 4;
			if( j2 == 0 )
				j2 = 1;
			else if( j2 == 1 )
				j2 = 0;
			defenders[k][9][j] = armies->copy(192 + kx * 64 + j2 * 16, 288 + ky * 13, 16, 13);
		}
	}

	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<=5;j++) {
			for(int k=0;k<n_attacker_directions_c;k++) {
				int n_frames = n_attacker_frames[j][k];
				for(int l=0;l<n_frames;l++) {
					attackers_walking[i][j][k][l] = armies->copy(16*l + 64*k, 32*j, 16, 16);
					int r = 0, g = 0, b = 0;
					PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
					attackers_walking[i][j][k][l]->remap(240, 0, 0, r, g, b);
				}
			}
		}
	}

	for(int i=0;i<n_players_c;i++) {
		for(int k=0;k<n_attacker_directions_c;k++) {
			int n_frames = n_attacker_frames[10][k];
			for(int l=0;l<n_frames;l++) {
					attackers_walking[i][10][k][l] = armies->copy(16*l + 64*k, 320, 16, 16);
					int r = 0, g = 0, b = 0;
					PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
					attackers_walking[i][10][k][l]->remap(240, 0, 0, r, g, b);
			}
		}
	}

	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<n_epochs_c;j++)
			planes[i][j] = NULL;
		planes[i][6] = armies->copy(32*i, 192, 32, 32);
		planes[i][7] = armies->copy(32*i, 232, 32, 24);
	}

	for(int i=0;i<n_players_c;i++) {
		nukes[i][0] = armies->copy(48*i, 256, 16, 32);
		nukes[i][1] = armies->copy(48*i+16, 256, 32, 32);
	}
	for(int i=0;i<n_players_c;i++) {
		for(int j=0;j<n_saucer_frames_c;j++) {
			saucers[i][j] = armies->copy(32*j, 288, 32, 21);
		}
	}

	// ammo:
	// rock
	attackers_ammo[0][ATTACKER_AMMO_RIGHT] = armies->copy(272, 24, 16, 8);
	attackers_ammo[0][ATTACKER_AMMO_LEFT] = armies->copy(288, 24, 16, 8);
	attackers_ammo[0][ATTACKER_AMMO_UP] = armies->copy(288, 16, 16, 8);
	attackers_ammo[0][ATTACKER_AMMO_DOWN] = armies->copy(272, 16, 16, 8);
	// catapult/sword
	attackers_ammo[1][ATTACKER_AMMO_RIGHT] = armies->copy(272, 24, 16, 8);
	attackers_ammo[1][ATTACKER_AMMO_LEFT] = armies->copy(288, 24, 16, 8);
	attackers_ammo[1][ATTACKER_AMMO_UP] = armies->copy(288, 16, 16, 8);
	attackers_ammo[1][ATTACKER_AMMO_DOWN] = armies->copy(272, 16, 16, 8);
	/* spear
	attackers_ammo[1][ATTACKER_AMMO_RIGHT] = armies->copy(272, 32, 16, 8);
	attackers_ammo[1][ATTACKER_AMMO_LEFT] = armies->copy(272, 40, 16, 8);
	attackers_ammo[1][ATTACKER_AMMO_UP] = armies->copy(288, 32, 16, 16);
	attackers_ammo[1][ATTACKER_AMMO_DOWN] = armies->copy(304, 32, 16, 16);*/
	// pike
	attackers_ammo[2][ATTACKER_AMMO_RIGHT] = armies->copy(272, 64, 16, 8);
	attackers_ammo[2][ATTACKER_AMMO_LEFT] = armies->copy(272, 72, 16, 8);
	attackers_ammo[2][ATTACKER_AMMO_UP] = armies->copy(288, 64, 16, 16);
	attackers_ammo[2][ATTACKER_AMMO_DOWN] = armies->copy(304, 64, 16, 16);
	/* bow and arrow
	attackers_ammo[2][ATTACKER_AMMO_RIGHT] = armies->copy(272, 80, 16, 8);
	attackers_ammo[2][ATTACKER_AMMO_LEFT] = armies->copy(272, 88, 16, 8);
	attackers_ammo[2][ATTACKER_AMMO_UP] = armies->copy(288, 80, 16, 16);
	attackers_ammo[2][ATTACKER_AMMO_DOWN] = armies->copy(304, 80, 16, 16);*/
	// longbow
	attackers_ammo[3][ATTACKER_AMMO_RIGHT] = armies->copy(272, 96, 16, 8);
	attackers_ammo[3][ATTACKER_AMMO_LEFT] = armies->copy(272, 104, 16, 8);
	attackers_ammo[3][ATTACKER_AMMO_UP] = armies->copy(288, 96, 16, 16);
	attackers_ammo[3][ATTACKER_AMMO_DOWN] = armies->copy(304, 96, 16, 16);
	// trebuchet
	attackers_ammo[4][ATTACKER_AMMO_RIGHT] = armies->copy(256, 144, 16, 8);
	attackers_ammo[4][ATTACKER_AMMO_LEFT] = armies->copy(256, 144, 16, 8);
	attackers_ammo[4][ATTACKER_AMMO_UP] = armies->copy(256, 144, 16, 16);
	attackers_ammo[4][ATTACKER_AMMO_DOWN] = armies->copy(256, 144, 16, 16);
	// cannon
	attackers_ammo[5][ATTACKER_AMMO_RIGHT] = armies->copy(272, 160, 10, 9);
	attackers_ammo[5][ATTACKER_AMMO_LEFT] = armies->copy(272, 160, 10, 9);
	attackers_ammo[5][ATTACKER_AMMO_UP] = armies->copy(272, 160, 10, 9);
	attackers_ammo[5][ATTACKER_AMMO_DOWN] = armies->copy(272, 160, 10, 9);
	// bombs
	//attackers_ammo[6][ATTACKER_AMMO_BOMB] = armies->copy(304, 208, 16, 16);
	attackers_ammo[6][ATTACKER_AMMO_BOMB] = armies->copy(288, 206, 12, 12);

	delete armies;
	armies = NULL;

	for(int k=0;k<n_players_c;k++) {
		nuke_defences[k] = defenders[k][nuclear_epoch_c][0];
	}

	attackers_ammo[7][ATTACKER_AMMO_BOMB] = attackers_ammo[6][ATTACKER_AMMO_BOMB];
	attackers_ammo[9][ATTACKER_AMMO_BOMB] = attackers_ammo[6][ATTACKER_AMMO_BOMB];

    for(int i=0;i<n_saucer_frames_c;i++) {
        for(int k=0;k<4;k++) {
            int r = 0, g = 0, b = 0;
            PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
            saucers[k][i]->remap(240, 0, 0, r, g, b);
        }
    }
    for(int i=0;i<n_epochs_c;i++) {
        if( defenders[0][i][0] == NULL )
            continue;
        for(int j=0;j<n_defender_frames_c;j++) {
            for(int k=0;k<4;k++) {
                int r = 0, g = 0, b = 0;
                PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
                defenders[k][i][j]->remap(240, 0, 0, r, g, b);
            }
        }
    }
	drawProgress(65);

	Image *features = Image::loadImage("data/mlm_features");
	if( features == NULL )
		return false;
	features->setColor(0, 255, 0, 255);
	processImage(features);

	icon_openpitmine = features->copy(256, 118, 47, 24);

	for(int i=0;i<4;i++) {
		icon_trees[i][0] = features->copy(96 + 32*i, 114, 24, 28);
		for(int j=1;j<n_tree_frames_c;j++) {
			icon_trees[i][j] = icon_trees[i][0]; // no animation for old data available
		}
	}

	delete features;
	features = NULL;

	for(int i=0;i<n_clutter_c;i++) {
		icon_clutter[i] = NULL;
	}

	drawProgress(70);

		background_islands = Image::loadImage("data/mlm_sunrise");
	if( background_islands == NULL )
		return false;
	processImage(background_islands);

	drawProgress(80);

	return true;
}

bool loadImages() {
    //int time_s = clock();
	// progress should go from 0 to 80%
	string gfx_dir = "gfx/";

#if defined(Q_OS_ANDROID)
	background = Image::loadImage(gfx_dir + "starfield.png");
#else
	background = Image::loadImage(gfx_dir + "starfield.jpg");
#endif
#ifndef USING_QT
    // if using Qt, we use resources even on Linux
#if !defined(__ANDROID__) && defined(__linux)
	if( background == NULL ) {
		gfx_dir = "/usr/share/gigalomania/" + gfx_dir;
		LOG("look in %s for gfx\n", gfx_dir.c_str());
		background = Image::loadImage(gfx_dir + "starfield.jpg");
	}
#endif
#endif
	if( background == NULL ) {
		//return false;
		return loadOldImages();
	}
	drawProgress(20);
	//scale_factor = ((float)(scale_width*default_width_c))/(float)player_select->getWidth();
	//LOG("scale factor for images = %f\n", scale_factor);

	calculateScale(background);
	// nb, still scale if scale_factor==1, as this is a way of converting to 8bit
	processImage(background);
	drawProgress(25);

	Image *image_slabs = NULL;
	image_slabs = Image::loadImage(gfx_dir + "slabs.png");
	if( image_slabs == NULL )
		return false;
	drawProgress(30);
	for(int i=0;i<MAP_N_COLOURS;i++) {
		land[i] = image_slabs->copy();
		processImage(land[i]);
		//land[i]->setMaskColor(255, 0, 255); // need to set the mask colour now, to stop it being multiplied!
	}
	drawProgress(32);
	land[MAP_ORANGE]->brighten(187.5f/255.0f, 96.0f/255.0f, 42.0f/255.0f);
	land[MAP_GREEN]->brighten(52.0f/255.0f, 163.5f/255.0f, 52.0f/255.0f);
	land[MAP_BROWN]->brighten(116.0f/255.0f, 72.0f/255.0f, 36.0f/255.0f);
	land[MAP_WHITE]->brighten(163.5f/255.0f, 163.5f/255.0f, 163.5f/255.0f);
	land[MAP_DBROWN]->brighten(120.0f/255.0f, 76.0f/255.0f, 58.0f/255.0f);
	land[MAP_DGREEN]->brighten(26/255.0f, 120.0f/255.0f, 26.0f/255.0f);
	land[MAP_GREY]->brighten(94.0f/255.0f, 94.0f/255.0f, 94.0f/255.0f);
	delete image_slabs;
	image_slabs = NULL;

	/*Image *buildings = Image::loadImage(gfx_dir + "buildings.png");
	if( buildings != NULL ) {
		buildings_shadow = false; // done using alpha channel
		buildings->scale(scale_factor, scale_factor);
		buildings->setScale(scale_width, scale_height);
		for(int i=0;i<n_epochs_c;i++) {
			fortress[i] = buildings->copy( 0, 60*i, 58, 60);
			mine[i] = buildings->copy( 58, 60*i, 58, 60);
			factory[i] = buildings->copy( 174, 60*i, 58, 60);
			lab[i] = buildings->copy( 116, 60*i, 58, 60);
		}
	}*/
	for(int i=0;i<n_epochs_c;i++) {
		fortress[i] = NULL;
		mine[i] = NULL;
		factory[i] = NULL;
	}
	for(int i=0;i<n_epochs_c;i++) {
		stringstream filename;
		filename << gfx_dir << "building_tower_" << i << ".png";
		Image *temp = Image::loadImage(filename.str().c_str());
		if( temp == NULL ) {
			return false;
		}
		processImage(temp);
		//delete fortress[i];
		//fortress[i] = temp->copy(29, 9, 62, 51);
		fortress[i] = temp->copy(27, 9, 64, 51);
		delete temp;
	}
	drawProgress(34);
	for(int i=mine_epoch_c;i<n_epochs_c-1;i++) {
		stringstream filename;
		filename << gfx_dir << "building_mine_" << i << ".png";
		Image *temp = Image::loadImage(filename.str().c_str());
		if( temp == NULL ) {
			return false;
		}
		processImage(temp);
		mine[i] = temp->copy(28, 12, 66, 51);
		delete temp;
	}
	drawProgress(36);
	for(int i=factory_epoch_c;i<n_epochs_c-1;i++) {
		stringstream filename;
		filename << gfx_dir << "building_factory_" << i << ".png";
		Image *temp = Image::loadImage(filename.str().c_str());
		if( temp == NULL ) {
			return false;
		}
		processImage(temp);
		//factory[i] = temp->copy(24, 1, 70, 62);
		factory[i] = temp->copy(25, 1, 68, 62);
		delete temp;
	}
	drawProgress(38);
	for(int i=lab_epoch_c;i<n_epochs_c-1;i++) {
		stringstream filename;
		filename << gfx_dir << "building_lab_" << i << ".png";
		Image *temp = Image::loadImage(filename.str().c_str());
		if( temp == NULL ) {
			return false;
		}
		processImage(temp);
		//lab[i] = temp->copy(28, 12, 66, 51);
		lab[i] = temp->copy(31, 12, 52, 51);
		delete temp;
	}

	offset_flag_y_c = 3; // correct placement of flag on tower for new graphics
	drawProgress(40);

	Image *icons = Image::loadImage(gfx_dir + "icons.png");
	if( icons == NULL )
		return false;
	/*if( !icons->scaleTo(scale_width*default_width_c) )
	return false;*/

    // need to do flags beforehand, due to colour remapping
    icons->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
    for(int i=0;i<n_players_c;i++) { // different locations
        /*for(int j=0;j<3;j++)
            flags[i][j] = icons->copy(160 + 16*j, 144, 16, 16);
        flags[i][3] = icons->copy(160 + 16*1, 144, 16, 16);*/
        for(int j=0;j<4;j++)
            flags[i][j] = icons->copy(144 + 16*j, 144, 16, 16);
        for(int j=0;j<n_flag_frames_c;j++) {
            int r = 0, g = 0, b = 0;
            PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
            flags[i][j]->remap(240, 0, 0, r, g, b);
            processImage(flags[i][j]);
        }
    }

    processImage(icons);

	for(int i=0;i<n_epochs_c;i++)
		men[i] = icons->copy(16*i, 0, 16, 16);

	unarmed_man = icons->copy(80, 32, 16, 16);

	panel_design = icons->copy(304, 0, 16, 16);
	//panel_design_dark = panel_design->copy();
	panel_lab = icons->copy(16, 33, 16, 16);
	panel_shield = icons->copy(240, 0, 16, 16);
	panel_defence = icons->copy(256, 0, 16, 16);
	panel_attack = icons->copy(272, 0, 16, 16);
	panel_knowndesigns = icons->copy(240, 16, 16, 16);
	panel_factory = icons->copy(48, 33, 16, 16);
	panel_bloody_attack = icons->copy(240, 32, 16, 16);

	mine_gatherable_small = icons->copy(160, 0, 16, 16);

	//panel_build[BUILDING_TOWER] = icons->copy(224, 63, 16, 16); // different size // not yet used
	//panel_build[BUILDING_MINE] = icons->copy(256, 63, 16, 16); // different size
	panel_build[BUILDING_MINE] = mine_gatherable_small;
	panel_build[BUILDING_FACTORY] = icons->copy(288, 63, 16, 16); // different size
	panel_build[BUILDING_LAB] = icons->copy(192, 63, 16, 16); // different size
	//panel_build[BUILDING_LAB] = panel_lab;

	panel_building[BUILDING_TOWER] = icons->copy(0, 33, 16, 16); // different size
	//panel_building[BUILDING_MINE] = icons->copy(32, 33, 16, 16); // different size
	panel_building[BUILDING_MINE] = mine_gatherable_small;
	panel_building[BUILDING_FACTORY] = panel_factory;
	panel_building[BUILDING_LAB] = panel_lab;

	mine_gatherable_large = mine_gatherable_small;
	panel_bigdesign = panel_design;
	panel_biglab = panel_lab;
	panel_bigfactory = panel_factory;
	panel_bigshield = panel_shield;
	panel_bigdefence = panel_defence;
	panel_bigattack = panel_attack;
	panel_bigbuild = icons->copy(48, 48, 16, 16);
	panel_bigknowndesigns = panel_knowndesigns;
	panel_twoattack = panel_attack;

	for(int i=0;i<n_players_c;i++)
		mouse_pointers[i] = icons->copy(176 + 16*i, 0, 16, 16);

	for(int i=0;i<13;i++)
		icon_clocks[i] = icons->copy(16*i, 16, 16, 16);

	/*for(int i=0;i<3;i++)
		icon_mice[i] = icons->copy(256 + 16*i, 16, 16, 16); // smaller size*/
	for(int i=0;i<2;i++)
		icon_mice[i] = icons->copy(256 + 16*i, 16, 16, 16); // smaller size

	for(int i=0;i<n_death_flashes_c;i++)
		death_flashes[i] = icons->copy(256 + 16*i, 32, 16, 16); // different location and smaller size

	for(int i=0;i<7;i++)
		blue_flashes[i] = icons->copy(208 + 16*i, 144, 16, 16); // different location (and size for i = 5, 6)

	for(int i=0;i<n_players_c;i++) {
		icon_towers[i] = icons->copy(160 + 16*i, 81, 6, 6);
		icon_armies[i] = icons->copy(160 + 16*i, 87, 4, 4);
	}

	for(int i=0;i<N_ID;i++)
		icon_elements[i] = icons->copy(16*i, 128, 16, 16); // different location

    /*for(int i=0;i<n_players_c;i++) { // different locations
		for(int j=0;j<3;j++)
			flags[i][j] = icons->copy(160 + 16*j, 144, 16, 16);
		flags[i][3] = flags[i][1];
		for(int j=0;j<n_flag_frames_c;j++) {
			int r = 0, g = 0, b = 0;
			PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
			flags[i][j]->remap(240, 0, 0, r, g, b);
		}
    }*/

	for(int i=0;i<n_playershields_c;i++) {
		//playershields[i] = icons->copy(16*i, 176, 16, 14); // different location // original version for my gfx
		playershields[i] = icons->copy(16*i, 176, 16, 16); // different location
		//playershields[i] = icons->copy(16*i, 177, 16, 14); // different location
	}

	for(int i=0;i<n_shields_c;i++)
		icon_shields[i] = icons->copy(112 + 16*i, 32, 16, 16);
	for(int i=0;i<n_epochs_c;i++)
		icon_defences[i] = icons->copy(16*i, 240, 16, 16); // different location
	for(int i=0;i<n_epochs_c;i++)
		icon_weapons[i] = icons->copy(16*i, 224, 16, 16); // different location
	for(int i=0;i<n_epochs_c;i++)
		//numbered_defences[i] = icons->copy(16*i, 173, 16, 16);
		numbered_defences[i] = icon_defences[i]; // todo:
	for(int i=0;i<n_epochs_c;i++)
		//numbered_weapons[i] = icons->copy(16*i, 157, 16, 16);
		numbered_weapons[i] = icon_weapons[i]; // todo:

	for(int i=0;i<3;i++)
		icon_speeds[i] = icons->copy(272 + 16*i, 192, 16, 16); // different location and size

	building_health = icons->copy(0, 192, 38, 16);
	dash_grey = icons->copy(144, 112, 16, 16);

	icon_shield = icons->copy(0, 64, 16, 16);
	icon_defence = icons->copy(16, 64, 16, 16);
	icon_weapon = icons->copy(32, 64, 16, 16);

	/*icon_infinity = icons->copy(128, 112, 12, 8); // original versions for my gfx
	icon_bc = icons->copy(224, 15, 12, 8);
	icon_ad = icons->copy(208, 16, 12, 8);
	icon_ad_shiny = icons->copy(208, 16, 12, 8);*/
	icon_infinity = icons->copy(128, 112, 16, 16);
	icon_bc = icons->copy(224, 15, 16, 16);
	icon_ad = icons->copy(208, 16, 16, 16);
	icon_ad_shiny = icons->copy(208, 16, 16, 16);

	icon_nuke_hole = icons->copy(288, 96, 16, 16);

	icon_ergo = icons->copy(176, 112, 16, 16);
	icon_trash = icons->copy(192, 112, 16, 16);

	icons = Image::loadImage(gfx_dir + "icons64.png");
	if( icons == NULL )
		return false;
	drawProgress(45);
	// replace with new large icons
	/*if( !icons->scaleTo(scale_width*128) ) // may need to update width as more icons added!
	return false;*/
	processImage(icons);

	mapsquare = icons->copy(0, 0, 17, 17);
	flashingmapsquare = icons->copy(32, 0, 17, 17);
	arrow_left = icons->copy(64, 0, 32, 32);
	arrow_left->scaleAlpha(0.625f);
	arrow_right = icons->copy(96, 0, 32, 32);
	arrow_right->scaleAlpha(0.625f);

	icons = Image::loadImage(gfx_dir + "font.png");
	if( icons == NULL )
		return false;
	drawProgress(48);
	processImage(icons);
    //const int number_h_c = 8;
    const int number_h_c = 10;
    for(int i=0;i<10;i++) {
        numbers_blue[i] = icons->copy(8*i, 0, 6, number_h_c);
        numbers_grey[i] = icons->copy(8*i, 0, 6, number_h_c);
        numbers_white[i] = icons->copy(8*i, 0, 6, number_h_c);
        numbers_orange[i] = icons->copy(8*i, 0, 6, number_h_c);
        numbers_yellow[i] = icons->copy(8*i, 0, 6, number_h_c);
        numbers_largegrey[i] = icons->copy(8*i, 0, 6, number_h_c); // should be 6x15
        numbers_largeshiny[i] = icons->copy(8*i, 0, 6, number_h_c); // should be 6x15
		for(int j=0;j<4;j++) {
            numbers_small[j][i] = icons->copy(8*i, 32, 6, number_h_c);
			// todo: remap to dark player colours too
			/*int r=0, g=0, b=0;
			PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)j);
			numbers_small[j][i]->remap(240, 0, 0, r, g, b);
			numbers_small[j][i]->remap(112, 32, 16, r, g, b);*/
		}
		numbers_small[1][i]->reshadeRGB(0, false, true, false);
		numbers_small[2][i]->reshadeRGB(0, true, true, false);
		numbers_small[3][i]->reshadeRGB(0, false, false, true);
	}
	for(int i=0;i<26;i++) {
        letters_small[i] = icons->copy(80 + 8*i, 16, 6, number_h_c);
        letters_large[i] = icons->copy(80 + 8*i, 16, 6, number_h_c); // should be 6x15
	}
    numbers_half = icons->copy(288, 16, 6, number_h_c);

	delete icons;
	drawProgress(50);

    smoke_image = Image::createRadial((int)(scale_width * 16), (int)(scale_height * 16), 0.5f);
	processImage(smoke_image);

	for(int i=0;i<n_coast_c;i++)
		coast_icons[i] = NULL;
	map_sq_offset = 0;
	map_sq_coast_offset = 3;
    int map_width = (int)(scale_width * 16);
    int map_height = (int)(scale_height * 16);
    {
		unsigned char filter_max[3] = {255, 192, 84};
		unsigned char filter_min[3] = {120, 0, 0};
        map_sq[MAP_ORANGE][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {104, 255, 104};
		unsigned char filter_min[3] = {0, 72, 0};
        map_sq[MAP_GREEN][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {216, 144, 72};
		unsigned char filter_min[3] = {16, 0, 0};
        map_sq[MAP_BROWN][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {255, 255, 255};
		unsigned char filter_min[3] = {72, 72, 72};
        map_sq[MAP_WHITE][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {224, 152, 116};
		unsigned char filter_min[3] = {16, 0, 0};
        map_sq[MAP_DBROWN][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {52, 232, 52};
		unsigned char filter_min[3] = {0, 8, 0};
        map_sq[MAP_DGREEN][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	{
		unsigned char filter_max[3] = {188, 188, 188};
		unsigned char filter_min[3] = {0, 0, 0};
        map_sq[MAP_GREY][0] = Image::createNoise(map_width, map_height, 4.0f, 4.0f, filter_max, filter_min, Image::NOISEMODE_PERLIN, 4);
	}
	for(int i=0;i<MAP_N_COLOURS;i++) {
        convertToHiColor(map_sq[i][0]);
        map_sq[i][0]->setScale(scale_width, scale_height);
        for(int j=1;j<n_map_sq_c;j++) {
			map_sq[i][j] = map_sq[i][0]->copy(0, 0, 16, 16);
		}
    }
	drawProgress(53);

	Image *smallmap_coast = Image::loadImage(gfx_dir + "smallmap_coast.png");
	if( smallmap_coast != NULL ) {
		/*if( !smallmap_coast->scaleTo(scale_width*616) )
		return false;*/
		processImage(smallmap_coast);
		for(int i=0;i<n_coast_c;i++)
			coast_icons[i] = smallmap_coast->copy(22*i, 0, 22, 22);
	}
	delete smallmap_coast;
	drawProgress(55);

	{
		// initialise
		for(int i=0;i<n_players_c;i++) {
			for(int j=0;j<=n_epochs_c;j++) {
				for(int k=0;k<n_attacker_directions_c;k++) {
					for(int l=0;l<max_attacker_frames_c;l++) {
						attackers_walking[k][i][j][l] = NULL;
					}
				}
			}
		}
		for(int j=0;j<=n_epochs_c;j++) {
			for(int k=0;k<n_attacker_directions_c;k++) {
				n_attacker_frames[j][k] = 0;
			}
		}
		for(int i=0;i<n_epochs_c;i++)
			for(int j=0;j<N_ATTACKER_AMMO_DIRS;j++)
				attackers_ammo[i][j] = NULL;

		Image *gfx_def_image = Image::loadImage(gfx_dir + "defenders.png");
		if( gfx_def_image == NULL )
			return false;
		drawProgress(58);
		/*if( !gfx_def_image->scaleTo(scale_width*default_width_c) )
		return false;*/
        //processImage(gfx_def_image);
        gfx_def_image->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
        for(int k=0;k<n_players_c;k++) {
            int r = 0, g = 0, b = 0;
            PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
            for(int i=0;i<n_epochs_c;i++) {
				for(int j=0;j<n_defender_frames_c;j++) {
					defenders[k][i][j] = gfx_def_image->copy(16*i, 0, 16, 16);
                    defenders[k][i][j]->remap(240, 0, 0, r, g, b);
                    processImage(defenders[k][i][j]);
                }
			}
		}

		for(int i=0;i<=5;i++) {
			if( !loadAttackersWalkingImages(gfx_dir, i) ) {
				return false;
			}
		}

		if( !loadAttackersWalkingImages(gfx_dir, n_epochs_c) ) {
			return false;
		}
		drawProgress(60);

		Image *gfx_planes = Image::loadImage(gfx_dir + "attacker_flying.png");
		if( gfx_planes == NULL )
			return false;
		drawProgress(62);
		/*if( !gfx_planes->scaleTo(scale_width*default_width_c) )
		return false;*/
        gfx_planes->setScale(scale_width/scale_factor_w, scale_height/scale_factor_h); // so the copying will work at the right scale for the input image
        // do remapping before scaling
        for(int i=0;i<n_players_c;i++) {
            int r = 0, g = 0, b = 0;
            PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
            for(int j=0;j<n_saucer_frames_c;j++) {
                saucers[i][j] = gfx_planes->copy(32*j, 64, 32, 32);
                saucers[i][j]->remap(240, 0, 0, r, g, b);
                processImage(saucers[i][j]);
            }
        }
        processImage(gfx_planes);
		for(int i=0;i<n_players_c;i++) {
			for(int j=0;j<n_epochs_c;j++)
				planes[i][j] = NULL;
			planes[i][6] = gfx_planes->copy(32*i, 0, 32, 32);
			planes[i][7] = gfx_planes->copy(128+32*i, 0, 32, 32);
		}

		for(int i=0;i<n_players_c;i++) {
			nukes[i][0] = gfx_planes->copy(64*i, 32, 32, 32);
			nukes[i][1] = gfx_planes->copy(64*i+32, 32, 32, 32);
		}

		Image *gfx_ammo = Image::loadImage(gfx_dir + "attacker_ammo.png");
		if( gfx_ammo == NULL )
			return false;
		drawProgress(65);
		/*if( !gfx_ammo->scaleTo(scale_width*default_width_c) )
		return false;*/
		processImage(gfx_ammo);
		for(int i=0;i<6;i++) {
			attackers_ammo[i][ATTACKER_AMMO_RIGHT] = gfx_ammo->copy(0, 16*i, 16, 16);
			attackers_ammo[i][ATTACKER_AMMO_LEFT] = gfx_ammo->copy(16, 16*i, 16, 16);
			attackers_ammo[i][ATTACKER_AMMO_UP] = gfx_ammo->copy(32, 16*i, 16, 16);
			attackers_ammo[i][ATTACKER_AMMO_DOWN] = gfx_ammo->copy(48, 16*i, 16, 16);
		}
		// bombs
		//attackers_ammo[6][ATTACKER_AMMO_BOMB] = armies->copy(304, 208, 16, 16);
		attackers_ammo[6][ATTACKER_AMMO_BOMB] = gfx_ammo->copy(0, 96, 16, 16); // different size

		delete gfx_def_image;
		delete gfx_planes;
		delete gfx_ammo;

		/*nuke_defences[0] = armies->copy(192, 256, 16, 16);
		nuke_defences[1] = armies->copy(208, 256, 16, 16);
		nuke_defences[2] = armies->copy(192, 272, 16, 16);
		nuke_defences[3] = armies->copy(208, 272, 16, 16);*/
		for(int k=0;k<n_players_c;k++) {
			nuke_defences[k] = defenders[k][nuclear_epoch_c][0];
		}

		attackers_ammo[7][ATTACKER_AMMO_BOMB] = attackers_ammo[6][ATTACKER_AMMO_BOMB];
		attackers_ammo[9][ATTACKER_AMMO_BOMB] = attackers_ammo[6][ATTACKER_AMMO_BOMB];

        for(int i=0;i<=n_epochs_c;i++) {
            if( i == 6 || i == 7 || i == 8 || i == 9 )
                continue;
			for(int player=0;player<n_players_c;player++) {
				for(int dir=0;dir<n_attacker_directions_c;dir++) {
					int n_frames = n_attacker_frames[i][dir];
					for(int frame=0;frame<n_frames;frame++) {
						int r = 0, g = 0, b = 0;
						PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)player);
						ASSERT(attackers_walking[player][i][dir][frame] != NULL);
						attackers_walking[player][i][dir][frame]->remap(240, 0, 0, r, g, b);
					}
				}
			}
        }
        for(int i=0;i<n_saucer_frames_c;i++) {
            for(int k=0;k<4;k++) {
                int r = 0, g = 0, b = 0;
                PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
                saucers[k][i]->remap(240, 0, 0, r, g, b);
            }
        }
        for(int i=0;i<n_epochs_c;i++) {
            if( defenders[0][i][0] == NULL )
                continue;
            for(int j=0;j<n_defender_frames_c;j++) {
                for(int k=0;k<4;k++) {
                    int r = 0, g = 0, b = 0;
                    PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)k);
                    defenders[k][i][j]->remap(240, 0, 0, r, g, b);
                }
            }
        }
    }

	// features
	Image *gfx_features = Image::loadImage(gfx_dir + "features.png");
	if( gfx_features == NULL )
		return false;
	/*if( !gfx_features->scaleTo(scale_width*default_width_c) )
	return false;*/
	processImage(gfx_features);
	icon_openpitmine = gfx_features->copy(0, 0, 47, 24);

	icon_trees[0][0] = Image::loadImage(gfx_dir + "tree2_00.png");
	icon_trees[0][1] = Image::loadImage(gfx_dir + "tree2_01.png");
	icon_trees[0][2] = Image::loadImage(gfx_dir + "tree2_02.png");
	icon_trees[0][3] = Image::loadImage(gfx_dir + "tree2_03.png");

	icon_trees[1][0] = Image::loadImage(gfx_dir + "tree3_00.png");
	icon_trees[1][1] = Image::loadImage(gfx_dir + "tree3_01.png");
	icon_trees[1][2] = Image::loadImage(gfx_dir + "tree3_02.png");
	icon_trees[1][3] = Image::loadImage(gfx_dir + "tree3_03.png");

	// [2][] is the nuked tree image
	icon_trees[2][0] = Image::loadImage(gfx_dir + "deadtree1_00.png");
	for(int j=1;j<n_tree_frames_c;j++) {
		icon_trees[2][j] = icon_trees[2][0]->copy(); // no animation for nuked tree
	}

	icon_trees[3][0] = Image::loadImage(gfx_dir + "tree5_00.png");
	icon_trees[3][1] = Image::loadImage(gfx_dir + "tree5_01.png");
	icon_trees[3][2] = Image::loadImage(gfx_dir + "tree5_02.png");
	icon_trees[3][3] = Image::loadImage(gfx_dir + "tree5_03.png");

	for(int i=0;i<n_trees_c;i++) {
		for(int j=0;j<n_tree_frames_c;j++) {
			if( icon_trees[i][j] == NULL )
				return false;
			processImage(icon_trees[i][j]);
		}
	}

	icon_clutter[0] = Image::loadImage(gfx_dir + "rock0.png");
	icon_clutter[1] = Image::loadImage(gfx_dir + "rock1.png");
	icon_clutter[2] = Image::loadImage(gfx_dir + "log0.png");
	for(int i=0;i<n_clutter_c;i++) {
		if( icon_clutter[i] == NULL )
			return false;
		processImage(icon_clutter[i]);
	}
	drawProgress(70);

#if defined(Q_OS_ANDROID)
        background_islands = Image::loadImage(gfx_dir + "sunrise.png");
#else
        background_islands = Image::loadImage(gfx_dir + "sunrise.jpg");
#endif
	if( background_islands == NULL )
		return false;
	processImage(background_islands);

	// finished loading/extracting images

	drawProgress(80);
    /*int time_taken = clock() - time_s;
    LOG("time taken to load images: %d (%d=1sec)\n", time_taken, CLOCKS_PER_SEC);*/
	return true;
}

bool setupInventions() {
	/*for(int i=0;i<n_shields_c;i++)
	invention_shields[i] = new Invention("SHIELD", Invention::SHIELD, start_epoch + i);*/
	for(int i=0;i<n_epochs_c;i++)
		invention_shields[i] = new Invention("SHIELD", Invention::SHIELD, i);

	invention_defences[0] = new Invention("STICK", Invention::DEFENCE, 0);
	invention_defences[1] = new Invention("SPEAR", Invention::DEFENCE, 1);
	invention_defences[2] = new Invention("SHORTBOW", Invention::DEFENCE, 2);
	invention_defences[3] = new Invention("CAULDRON OF OIL", Invention::DEFENCE, 3);
	invention_defences[4] = new Invention("CROSSBOW", Invention::DEFENCE, 4);
	invention_defences[5] = new Invention("RIFLE", Invention::DEFENCE, 5);
	invention_defences[6] = new Invention("MACHINE GUN", Invention::DEFENCE, 6);
	invention_defences[7] = new Invention("ROCKET", Invention::DEFENCE, 7);
	invention_defences[8] = new Invention("NUCLEAR DEFENCE", Invention::DEFENCE, 8);
	invention_defences[9] = new Invention("SDI LASER", Invention::DEFENCE, 9);

	invention_weapons[0] = new Weapon("ROCK WEAPON", 0, 1);
	//invention_weapons[1] = new Weapon("CATAPULT", 1, 1);
	invention_weapons[1] = new Weapon("SWORD", 1, 1);
	invention_weapons[2] = new Weapon("PIKE", 2, 1);
	invention_weapons[3] = new Weapon("LONGBOW", 3, 1);
	invention_weapons[4] = new Weapon("TREBUCHET", 4, 2);
	invention_weapons[5] = new Weapon("CANNON", 5, 3);
	invention_weapons[6] = new Weapon("BIPLANE", 6, 2);
	invention_weapons[7] = new Weapon("BOMBER", 7, 3);
	invention_weapons[8] = new Weapon("NUCLEAR MISSILE", 8, 0);
	invention_weapons[9] = new Weapon("SPACESHIP", 9, 10);

	return true;
}

bool setupElements() {
	elements[WOOD] = new Element("WOOD", WOOD, Element::GATHERABLE);
	elements[ROCK] = new Element("ROCK", ROCK, Element::GATHERABLE);
	elements[BONE] = new Element("BONE", BONE, Element::GATHERABLE);
	elements[SLATE] = new Element("SLATE", SLATE, Element::GATHERABLE);
	elements[MOONLITE] = new Element("MOONLITE", MOONLITE, Element::OPENPITMINE);
	elements[PLANETARIUM] = new Element("PLANETARIUM", PLANETARIUM, Element::OPENPITMINE);
	elements[BETHLIUM] = new Element("BETHLIUM", BETHLIUM, Element::OPENPITMINE);
	elements[SOLARIUM] = new Element("SOLARIUM", SOLARIUM, Element::OPENPITMINE);
	elements[ARULDITE] = new Element("ARULDITE", ARULDITE, Element::DEEPMINE);
	elements[HERBIRITE] = new Element("HERBIRITE", HERBIRITE, Element::DEEPMINE);
	elements[YERIDIUM] = new Element("YERIDIUM", YERIDIUM, Element::DEEPMINE);
	elements[VALIUM] = new Element("VALIUM", VALIUM, Element::DEEPMINE);
	elements[PARASITE] = new Element("PARASITE", PARASITE, Element::DEEPMINE);
	elements[AQUARIUM] = new Element("AQUARIUM", AQUARIUM, Element::DEEPMINE);
	elements[PALADIUM] = new Element("PALADIUM", PALADIUM, Element::DEEPMINE);
	elements[ONION] = new Element("ONION", ONION, Element::DEEPMINE);
	elements[TEDIUM] = new Element("TEDIUM", TEDIUM, Element::DEEPMINE);
	elements[MORON] = new Element("MORON", MORON, Element::DEEPMINE);
	elements[MARMITE] = new Element("MAAMITE", MARMITE, Element::DEEPMINE);
	elements[ALIEN] = new Element("ALIEN", ALIEN, Element::DEEPMINE);
	return true;
}

void cleanupPlayers() {
	for(int i=0;i<n_players_c;i++) {
		if( players[i] )
			delete players[i];
		players[i] = NULL;
	}

	// need to reset alliance flags!
	Player::resetAllAlliances();
}

void setupPlayers() {
	cleanupPlayers();

	int seed = clock();
	//seed = 72638; // test
	LOG("setupPlayers(): set random seed to %d\n", seed);
	srand( seed );

	//   0 - Red team
	//   1 - Green team
	//   2 - Yellow team
	//   3 - Blue team

	int n_opponents = maps[start_epoch][selected_island]->getNOpponents();
	ASSERT( n_opponents+1 <= maps[start_epoch][selected_island]->getNSquares() );
	int n_free = 4;
	if( human_player != PLAYER_DEMO ) {
		players[human_player] = new Player(true, human_player);
		n_free--;
	}
	else {
		n_opponents++;
	}

	if( gameMode == GAMEMODE_SINGLEPLAYER ) {
		for(int i=0;i<n_opponents && n_free > 0;i++) {
			int indx = rand() % n_free;
			for(int j=0;j<4;j++) {
				if( players[j] == NULL ) {
					if( indx == 0 ) {
						players[j] = new Player(false, j);
						n_free--;
						break;
					}
					indx--;
				}
			}
		}
	}

}

bool openScreen(bool fullscreen) {
	if( !fullscreen ) {
		int user_width = 0, user_height = 0;
#ifdef USING_QT
		user_width = QApplication::desktop()->width();
		user_height = QApplication::desktop()->height();
		qDebug("window is %d x %d", user_width, user_height);
		LOG("window is %d x %d\n", user_width, user_height);
		if( mobile_ui && user_width < user_height ) {
			// we'll be switching to landscape mode
			qDebug("swap to landscape mode");
			LOG("swap to landscape mode\n");
			int dummy = user_width;
			user_width = user_height;
			user_height = dummy;
		}
		// test
		/*user_width = 640;
		user_height = 360;*/
		/*user_width = 854;
		user_height = 480;*/
#elif defined(_WIN32)
		//#if 0
		// we do it using system calls instead of SDL, to ignore the start bar (if showing)
		RECT rect;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		user_width = rect.right - rect.left;
		user_height = rect.bottom - rect.top;
		LOG("desktop is %d x %d\n", user_width, user_height);
		user_height -= GetSystemMetrics(SM_CYCAPTION); // also ignore the window height
		LOG("available height is %d\n", user_height);
		//user_width = 320;
		//user_height = 240;
		//user_width = 640;
		//user_height = 480;
#elif AROS
		// AROS doesn't have latest SDL version with SDL_GetVideoInfo, so use native code!
		getAROSScreenSize(&user_width, &user_height);
#elif defined(__MORPHOS__)
		// MorphOS doesn't have latest SDL version with SDL_GetVideoInfo, so use native code!
		getAROSScreenSize(&user_width, &user_height);
#else

#if SDL_MAJOR_VERSION == 1
		const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
		user_width = videoInfo->current_w;
		user_height = videoInfo->current_h;
		LOG("desktop is %d x %d\n", user_width, user_height);
#else
		SDL_DisplayMode displayMode;
		if( SDL_GetDesktopDisplayMode(0, &displayMode) != 0 ) {
			LOG("SDL_GetDesktopDisplayMode failed!");
			user_width = 640;
			user_height = 480;
		}
		else {
			user_width = displayMode.w;
			user_height = displayMode.h;
			LOG("desktop is %d x %d\n", user_width, user_height);
		}
#endif

#endif

#if SDL_MAJOR_VERSION == 1
		// Ideally only multiples of 0.5 allowed, otherwise we get problems of fractional widths/heights/positioning
		// (still works, though uneven spacings).
		// We make an exception for height of 4/3, as a fairly common low end Android resolution is 480x320, and
		// restricting to height 240 means a significant portion of wasted screen space!
		//user_width = 1184;
		//user_height = 720;

		if( user_width >= 4*default_width_c ) {
			scale_width = 4.0f;
			LOG("scale width 4x\n");
		}
		else if( user_width >= 3.5f*default_width_c ) {
			scale_width = 3.5f;
			LOG("scale width 3.5x\n");
		}
		else if( user_width >= 3*default_width_c ) {
			scale_width = 3.0f;
			LOG("scale width 3x\n");
		}
		else if( user_width >= 2.5f*default_width_c ) {
			scale_width = 2.5f;
			LOG("scale width 2.5x\n");
		}
		else if( user_width >= 2*default_width_c ) {
			scale_width = 2.0f;
			LOG("scale width 2x\n");
		}
		else if( user_width >= 1.5*default_width_c ) {
			scale_width = 1.5f;
			LOG("scale width 1.5x\n");
		}
		else if( user_width >= default_width_c ) {
			scale_width = 1.0f;
			LOG("scale width 1x\n");
		}
		else {
			LOG("desktop resolution too small even for 1x scale width\n");
			return false;
		}

		if( user_height >= 4*default_height_c ) {
			scale_height = 4.0f;
			LOG("scale height 4x\n");
		}
		else if( user_height >= 3.5f*default_height_c ) {
			scale_height = 3.5f;
			LOG("scale height 3.5x\n");
		}
		else if( user_height >= 3*default_height_c ) {
			scale_height = 3.0f;
			LOG("scale height 3x\n");
		}
		else if( user_height >= 2.5f*default_height_c ) {
			scale_height = 2.5f;
			LOG("scale height 2.5x\n");
		}
		else if( user_height >= 2*default_height_c ) {
			scale_height = 2.0f;
			LOG("scale height 2x\n");
		}
		else if( user_height >= 1.5*default_height_c ) {
			scale_height = 1.5f;
			LOG("scale height 1.5x\n");
		}
		else if( user_height >= (4.0f/3.0f)*default_height_c ) {
			// see comment above
			scale_height = (4.0f/3.0f);
			LOG("scale height 4/3x\n");
		}
		else if( user_height >= default_height_c ) {
			scale_height = 1.0f;
			LOG("scale height 1x\n");
		}
		else {
			LOG("desktop resolution too small even for 1x scale height\n");
			return false;
		}

		//scale_width = 2.0f; scale_height = 1.5f;
		//scale_width = scale_height = 1.0f; // test
		//scale_width = scale_height = 2.0f; // test
		//scale_width = 2.5f;
		//scale_height = 2.0f;
		
		int screen_width = (int)(scale_width * default_width_c);
		int screen_height = (int)(scale_height * default_height_c);
#else
		// with SDL2, we let SDL do the scaling via SDL_RenderSetLogicalSize, so we don't have to do the scaling ourselves, and can set the screen width/height to whatever we like
		// for windowed mode, we pick a suitable size based on the available desktop space
		int screen_width = 1280;
		int screen_height = 960;

		screen_width = default_width_c;
		screen_height = default_height_c;
		while( 2*screen_width <= user_width && 2*screen_height <= user_height ) {
			screen_width *= 2;
			screen_height *= 2;
		}
#endif

		screen = new Screen();
		if( !screen->open(screen_width, screen_height, fullscreen) )
			return false;

	}
	else {
		// fullscreen
		screen = new Screen();

#if SDL_MAJOR_VERSION == 1
		if( screen->open(4*default_width_c, 4*default_height_c, fullscreen) ) {
			scale_width = scale_height = 4.0f;
			LOG("scale 4x\n");
		}
		else if( screen->open(3*default_width_c, 3*default_height_c, fullscreen) ) {
			scale_width = scale_height = 3.0f;
			LOG("scale 3x\n");
		}
		else if( screen->open(3*default_width_c, 2.5f*default_height_c, fullscreen) ) {
			scale_width = 3.0f;
			scale_height = 2.5f;
			LOG("scale width 3x\n");
			LOG("scale height 2.5x\n");
		}
		else if( screen->open(2.5f*default_width_c, 2.5f*default_height_c, fullscreen) ) {
			scale_width = scale_height = 2.5f;
			LOG("scale 2.5x\n");
		}
		else if( screen->open(2.5f*default_width_c, 2*default_height_c, fullscreen) ) {
			scale_width = 2.5f;
			scale_height = 2.0f;
			LOG("scale width 2.5x\n");
			LOG("scale height 2x\n");
		}
		else if( screen->open(2*default_width_c, 2*default_height_c, fullscreen) ) {
			scale_width = scale_height = 2.0f;
			LOG("scale 2x\n");
		}
		else if( screen->open(default_width_c, default_height_c, fullscreen) ) {
			scale_width = scale_height = 1.0f;
			LOG("scale 1x\n");
		}
		else {
			LOG("can't even open screen at 1x scale\n");
			return false;
		}
#else
		// with SDL2, we let SDL do the scaling via SDL_RenderSetLogicalSize, so we don't have to do the scaling ourselves
		// for fullscreen, the supplied width/height is ignored, as we always run at the native resolution
		if( !screen->open(0, 0, fullscreen) ) {
			LOG("can't even open screen at 1x scale\n");
			return false;
		}
#endif

	}

	char buffer[256] = "";
	sprintf(buffer, "Gigalomania, version %d.%d - Loading...", majorVersion, minorVersion);
	screen->setTitle(buffer);
	return true;
}

bool readMapProcessLine(int *epoch, int *index, Map **l_map, char *line, const int MAX_LINE, const char *filename) {
	bool ok = true;
	line[ strlen(line) - 1 ] = '\0'; // trim new line
	line[ strlen(line) - 1 ] = '\0'; // trim carriage return
	//LOG("line: %s\n", line);
	if( *l_map == NULL ) {
		if( line[0] != '#' ) {
			LOG("expected first character to be '#'\n");
			ok = false;
			return ok;
		}

		//char name[MAX_LINE+1] = "";
		int n_opponents = -1;
		//char colname[MAX_LINE+1] = "";

		char *ptr = strtok(&line[1], " ");
		if( ptr == NULL ) {
			LOG("can't find map name\n");
			ok = false;
			return ok;
		}
		//strcpy(name, ptr);
		string name = ptr;

		ptr = strtok(NULL, " ");
		if( ptr == NULL ) {
			LOG("can't find epoch\n");
			ok = false;
			return ok;
		}
		*epoch = atoi(ptr);

		ptr = strtok(NULL, " ");
		if( ptr == NULL ) {
			LOG("can't find n_opponents\n");
			ok = false;
			return ok;
		}
		n_opponents = atoi(ptr);

		ptr = strtok(NULL, " ");
		if( ptr == NULL ) {
			LOG("can't find colour name\n");
			ok = false;
			return ok;
		}
		//strcpy(colname, ptr);
		string colname = ptr;

		MapColour map_colour = MAP_UNDEFINED_COL;
		if( strcmp(colname.c_str(), "ORANGE") == 0 ) {
			map_colour = MAP_ORANGE;
		}
		else if( strcmp(colname.c_str(), "GREEN") == 0 ) {
			map_colour = MAP_GREEN;
		}
		else if( strcmp(colname.c_str(), "BROWN") == 0 ) {
			map_colour = MAP_BROWN;
		}
		else if( strcmp(colname.c_str(), "WHITE") == 0 ) {
			map_colour = MAP_WHITE;
		}
		else if( strcmp(colname.c_str(), "DBROWN") == 0 ) {
			map_colour = MAP_DBROWN;
		}
		else if( strcmp(colname.c_str(), "DGREEN") == 0 ) {
			map_colour = MAP_DGREEN;
		}
		else if( strcmp(colname.c_str(), "GREY") == 0 ) {
			map_colour = MAP_GREY;
		}
		else {
			LOG("unknown map colour: %s\n", colname.c_str());
			ok = false;
			return ok;
		}

		*index = 0;
		while( *index < max_islands_per_epoch_c && maps[*epoch][*index] != NULL )
			*index = *index + 1;
		if( *index == max_islands_per_epoch_c ) {
			LOG("too many islands for this epoch\n");
			ok = false;
			return ok;
		}
		*l_map = maps[*epoch][*index] = new Map(map_colour, n_opponents, name.c_str());
		(*l_map)->setFilename(filename);
	}
	else {
		char *line_ptr = line;
		while( *line_ptr == ' ' || *line_ptr == '\t' )
			line_ptr++;
		char *ptr = strtok(line_ptr, " ");
		if( ptr == NULL ) {
			LOG("can't find first word\n");
			ok = false;
			return ok;
		}
		else if( strcmp(ptr, "SECTOR") == 0 ) {
			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find sec_x\n");
				ok = false;
				return ok;
			}
			int sec_x = atoi(ptr);
			if( sec_x < 0 || sec_x >= map_width_c ) {
				LOG("invalid map x %d\n", sec_x);
				ok = false;
				return ok;
			}

			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find sec_y\n");
				ok = false;
				return ok;
			}
			int sec_y = atoi(ptr);
			if( sec_y < 0 || sec_y >= map_height_c ) {
				LOG("invalid map y %d\n", sec_y);
				ok = false;
				return ok;
			}
			(*l_map)->newSquareAt(sec_x, sec_y);
		}
		else if( strcmp(ptr, "ELEMENT") == 0 ) {
			// ignore for now
		}
		else {
			LOG("unknown word: %s\n", ptr);
			ok = false;
			return ok;
		}
	}
	return ok;
}

bool readLineFromRWOps(bool &ok, SDL_RWops *file, char *buffer, char *line, int MAX_LINE, int &buffer_offset, int &newline_index, bool &reached_end) {
	if( newline_index > 1 ) {
		// not safe to use strcpy on overlapping strings (undefined behaviour)
		int len = strlen(&buffer[newline_index-1]);
		memmove(buffer, &buffer[newline_index-1], len);
		buffer[len] = '\0';
		if( reached_end && buffer[0] == '\0' ) {
			return true;
		}
		buffer_offset = MAX_LINE - (newline_index-1);
	}
	if( !reached_end ) {
		// fill up buffer
		for(;;) {
			int n_read = file->read(file, &buffer[buffer_offset], 1, MAX_LINE-buffer_offset);
			//LOG("buffer offset %d , read %d\n", buffer_offset, n_read);
			if( n_read == 0 ) {
				// error or eof - don't quit yet, still need to finish reading buffer
				//LOG("read all of file\n");
				reached_end = true;
				break;
			}
			else {
				buffer[buffer_offset+n_read] = '\0';
				if( n_read < MAX_LINE-buffer_offset ) {
					// we didn't read all of the available buffer, and haven't reached end of file yet
					buffer_offset += n_read;
				}
				else {
					break;
				}
			}
		}
	}
	//LOG("buffer: %s\n", buffer);
	newline_index = 0;
	while( buffer[newline_index] != '\n' && buffer[newline_index] != '\0' ) {
		line[newline_index] = buffer[newline_index];
		newline_index++;
	}
	if( buffer[newline_index] == '\0' ) {
		LOG("file has too long line\n");
		ok = false;
		return true;
	}
	line[newline_index++] = '\n';
	line[newline_index++] = '\0';
	return false;
}

bool readMap(const char *filename) {
	//LOG("readMap: %s\n", filename); // disabled logging to improve performance on startup
	bool ok = true;
	const int MAX_LINE = 4096;
	//const int MAX_LINE = 64;
	char line[MAX_LINE+1] = "";
	Map *l_map = NULL;
	int epoch = -1;
	int index = -1;

#if defined(USING_QT)
    char fullname[4096] = "";
    sprintf(fullname, "%s%s/%s", DEPLOYMENT_PATH, maps_dirname, filename);
    //LOG("open: %s\n", fullname);
    QFile file(fullname);
    if( !file.open(QIODevice::ReadOnly) ) {
        LOG("failed to open file: %s\n", fullname);
        return false;
    }
    while( ok ) {
        qint64 amount_read = file.readLine(line, MAX_LINE);
        //LOG("returned: %d\n", amount_read);
        if( amount_read <= 0 ) {
            /*if( amount_read == -1 ) {
                LOG("error reading line\n");
                ok = false;
            }*/
            break;
        }
        else {
            ok = readMapProcessLine(&epoch, &index, &l_map, line, MAX_LINE, filename);
        }
    }
    file.close();
#else
    char fullname[4096] = "";
	sprintf(fullname, "%s/%s", maps_dirname, filename);
	// open in binary mode, so that we parse files in an OS-independent manner
	// (otherwise, Windows will parse "\r\n" as being "\n", but Linux will still read it as "\n")
	//FILE *file = fopen(fullname, "rb");
	SDL_RWops *file = SDL_RWFromFile(fullname, "rb");
#if !defined(__ANDROID__) && defined(__linux)
	if( file == NULL ) {
		LOG("searching in /usr/share/gigalomania/ for islands folder\n");
		sprintf(fullname, "%s/%s", alt_maps_dirname, filename);
		file = SDL_RWFromFile(fullname, "rb");
	}
#endif
    if( file == NULL ) {
		LOG("failed to open file: %s\n", fullname);
		return false;
	}
	char buffer[MAX_LINE+1] = "";
	int buffer_offset = 0;
	bool reached_end = false;
	int newline_index = 0;
	while( ok ) {
		bool done = readLineFromRWOps(ok, file, buffer, line, MAX_LINE, buffer_offset, newline_index, reached_end);
		if( !ok )  {
			LOG("failed to read line for: %s\n", filename);
		}
		else if( done ) {
			break;
		}
		else {
			ok = readMapProcessLine(&epoch, &index, &l_map, line, MAX_LINE, filename);
		}
	}
	file->close(file);
#endif
    if( !ok && l_map != NULL ) {
		LOG("delete map that was created\n");
		delete l_map;
		ASSERT(epoch != -1); // should have been set, if l_map!=NULL
		ASSERT(index != -1); // should have been set, if l_map!=NULL
		l_map = maps[epoch][index] = NULL;
	}
	return ok;
}

int sortMapsFunc(const void *a, const void *b) {
	Map *map_a = *(Map **)a;
	Map *map_b = *(Map **)b;
	// handling for 'Ohm'...
	if( map_a->getName()[0] == '0' )
		return 1;
	else if( map_b->getName()[0] == '0' )
		return -1;
	return strcmp(map_a->getName(), map_b->getName());
}

bool createMaps() {
	LOG("createMaps()...\n");
#if defined(USING_QT)
    /*vector<string> maps;
    maps.push_back("")*/
    QDir dir(QString(DEPLOYMENT_PATH) + "islands/");
    QStringList list = dir.entryList();
    foreach(const QString file, list) {
        if( !readMap(file.toLatin1().data()) ) {
            LOG("failed reading map: %s\n", file.toLatin1().data());
            // don't fail altogether, just ignore
        }
    }
#elif defined(_WIN32)
    WIN32_FIND_DATAA findFileData;
	char maps_dirname_w[256];
	sprintf(maps_dirname_w, "%s\\*", maps_dirname);
	HANDLE handle = FindFirstFileA(maps_dirname_w, &findFileData);
	if( handle == INVALID_HANDLE_VALUE ) {
		LOG("Invalid File Handle. GetLastError reports %d\n", GetLastError());
		return false;
	}
	for(;;) {
		if( !readMap(findFileData.cFileName) ) {
			LOG("failed reading map: %s\n", findFileData.cFileName);
			// don't fail altogether, just ignore
		}
		if( FindNextFileA(handle, &findFileData) == 0 ) {
			FindClose(handle);
			DWORD error = GetLastError();
			if( error != ERROR_NO_MORE_FILES ) {
				LOG("error reading directory: %d\n", error);
				return false;
			}
			break;
		}
	}
#elif defined(__ANDROID__)
	// unclear how to read contents of a folder in assets, so we just hardcode the islands (not like the user can easily drop new files there)
	readMap("0mega.map");
	readMap("alpha.map");
	readMap("binary.map");
	readMap("castle.map");
	readMap("devil.map");
	readMap("eep.map");
	readMap("final.map");
	readMap("font.map");
	readMap("globe.map");
	readMap("home.map");
	readMap("infinity.map");
	readMap("just.map");
	readMap("koala.map");
	readMap("loop.map");
	readMap("moon.map");
	readMap("ninth.map");
	readMap("oxygen.map");
	readMap("quart.map");
	readMap("rare.map");
	readMap("semi.map");
	readMap("toxic.map");
	readMap("universal.map");
	readMap("vine.map");
	readMap("wreath.map");
	readMap("x.map");
	readMap("yen.map");
	readMap("zinc.map");
#else

	DIR *dir = opendir(maps_dirname);

#if !defined(__ANDROID__) && defined(__linux)
	if( dir == NULL ) {
		LOG("searching in /usr/share/gigalomania/ for islands folder\n");
		dir = opendir(alt_maps_dirname);
	}
#endif
	if( dir == NULL ) {
		LOG("failed to open directory: %s\n", maps_dirname);
		LOG("error: %d\n", errno);
		return false;
	}
	for(;;) {
		errno = 0;
		dirent *ent = readdir(dir);
		if( ent == NULL ) {
			closedir(dir);
			if( errno ) {
				LOG("error reading directory: %d\n", errno);
				return false;
			}
			break;
		}
		if( !readMap(ent->d_name) ) {
			LOG("failed reading map: %s\n", ent->d_name);
			// don't fail altogether, just ignore
		}
	}
#endif
	LOG("done reading directory\n");

	for(int i=0;i<n_epochs_c;i++) {
		int n_islands = 0;
		while(n_islands < max_islands_per_epoch_c && maps[i][n_islands] != NULL)
			n_islands++;
		if( n_islands == 0 ) {
			LOG("can't find any islands for epoch %d\n", i);
			return false;
		}
		qsort((void *)&maps[i], n_islands, sizeof(maps[i][0]), sortMapsFunc);
	}
	return true;
}

GameState *dispose_gamestate = NULL;

void disposeGameState() {
	ASSERT( dispose_gamestate == NULL );
	dispose_gamestate = gamestate;
	gamestate = NULL;
}

void setGameStateID(GameStateID state) {
	gameStateID = state;
	playMusic();

	GameState *old_gamestate = gamestate;
	if( gamestate != NULL ) {
		disposeGameState();
	}

	if( gameStateID == GAMESTATEID_CHOOSEGAMETYPE )
		gamestate = new ChooseGameTypeGameState(human_player);
	else if( gameStateID == GAMESTATEID_CHOOSEDIFFICULTY )
		gamestate = new ChooseDifficultyGameState(human_player);
	else if( gameStateID == GAMESTATEID_CHOOSEPLAYER )
		gamestate = new ChoosePlayerGameState(human_player);
	else if( gameStateID == GAMESTATEID_PLACEMEN )
		gamestate = new PlaceMenGameState(human_player);
	else if( gameStateID == GAMESTATEID_PLAYING ) {
		gamestate = new PlayingGameState(human_player);
		int map_x = static_cast<PlaceMenGameState *>(old_gamestate)->getStartMapX();
		int map_y = static_cast<PlaceMenGameState *>(old_gamestate)->getStartMapY();
		int n_men = human_player == PLAYER_DEMO ? 0 : players[human_player]->getNMenForThisIsland();
		//createSectors(static_cast<PlayingGameState *>(gamestate), map_x, map_y, n_men);
		static_cast<PlayingGameState *>(gamestate)->createSectors(map_x, map_y, n_men);
	}
	else if( gameStateID == GAMESTATEID_ENDISLAND )
		gamestate = new EndIslandGameState(human_player);
	else if( gameStateID == GAMESTATEID_GAMECOMPLETE )
		gamestate = new GameCompleteGameState(human_player);
	else {
		ASSERT(false);
	}

	gamestate->reset();
	state_changed = false;
	//gameWon = GAME_PLAYING;
	/*if( state == GAMESTATE_PLACEMEN ) {
	placeMenInfo.init();
	}*/
}

void startIsland() {
	ASSERT(gameStateID == GAMESTATEID_PLACEMEN);
	/*int map_x = static_cast<PlaceMenGameState *>(gamestate)->getStartMapX();
	int map_y = static_cast<PlaceMenGameState *>(gamestate)->getStartMapY();*/

	//setupPlayers();
	time_rate = human_player == PLAYER_DEMO ? 5 : 1;
	LOG("time_rate = %d\n", time_rate);
	setGameStateID(GAMESTATEID_PLAYING);
	/*int n_men = human_player == PLAYER_DEMO ? players[human_player]->getNMenForThisIsland() : 0;
	createSectors(static_cast<PlayingGameState *>(gamestate), map_x, map_y, n_men);*/
	gamestate->fadeScreen(false, 0, NULL);
	gameResult = GAMERESULT_UNDEFINED;
}

void endIsland() {
	ASSERT(gameStateID == GAMESTATEID_PLAYING);
	map->calculateStats();
	map->freeSectors();
	for(int i=0;i<n_players_c;i++) {
		if( players[i] != NULL )
			n_suspended[i] += players[i]->getNSuspended();
	}
	//cleanupPlayers();
	setGameStateID(GAMESTATEID_ENDISLAND);
	gamestate->fadeScreen(false, 0, NULL);

	if( gameResult == GAMERESULT_QUIT ) {
		// pick a random non-human player
		int n_cpu = 0;
		for(int i=0;i<n_players_c;i++) {
			if( players[i] != NULL && !players[i]->isDead() && i != human_player ) {
				n_cpu++;
			}
		}
		if( n_cpu > 0 ) {
			int index = rand() % n_cpu;
			n_cpu = 0;
			for(int i=0;i<n_players_c;i++) {
				if( players[i] != NULL && !players[i]->isDead() && i != human_player ) {
					if( n_cpu == index ) {
						playSample( s_quit[i] );
						break;
					}
					n_cpu++;
				}
			}
		}
	}
}

void returnToChooseIsland() {
	ASSERT(gameStateID == GAMESTATEID_ENDISLAND);
	if( gameResult == GAMERESULT_WON && gameType == GAMETYPE_ALLISLANDS ) {
		//n_men_store -= n_men_for_this_island;
		n_men_store -= players[human_player]->getNMenForThisIsland();
		if( start_epoch == n_epochs_c-1 ) {
			setGameStateID(GAMESTATEID_GAMECOMPLETE);
		}
		else {
			setGameStateID(GAMESTATEID_PLACEMEN);

			ASSERT( !completed_island[selected_island] );
			completed_island[selected_island] = true;
			// check advance to next epoch
			bool completed_epoch = true;
			for(int i=0;i<max_islands_per_epoch_c && maps[start_epoch][i] != NULL && completed_epoch;i++) {
				if( !completed_island[i] )
					completed_epoch = false;
			}
			LOG("completed epoch? %d\n", completed_epoch);
			if( completed_epoch ) {
				n_men_store += getMenPerEpoch();
				nextEpoch();
			}
			else {
				nextIsland();
			}
		}
	}
	else {
		setGameStateID(GAMESTATEID_PLACEMEN);
		// test
		//setGameStateID(GAMESTATEID_GAMECOMPLETE);
	}

	gamestate->fadeScreen(false, 0, NULL);

	//n_men_for_this_island = 0;
	if( human_player != PLAYER_DEMO )
		players[human_player]->setNMenForThisIsland(0);
}

void startNewGame() {
	ASSERT(gameStateID == GAMESTATEID_GAMECOMPLETE);
	setGameStateID(GAMESTATEID_PLACEMEN);
	//gamestate->fadeScreen(false, 0, NULL);
	newGame();
}

void placeTower() {
	ASSERT( gameStateID == GAMESTATEID_PLACEMEN );
	//play_music = static_cast<PlaceMenGameState *>(gamestate)->getChooseMenPanel()->musicOn();
	if( !state_changed ) {
		//createSectors(x, y, n_men); // now done in startIsland()
		state_changed = true;
		gamestate->fadeScreen(true, 0, startIsland);
		//startIsland();
	}
}


void cleanup() {
	LOG("cleanup()\n");
	if( gamestate != NULL ) {
		LOG("delete gamestate %d\n", gamestate);
		delete gamestate;
		gamestate = NULL;
	}
	LOG("delete maps\n");
	for(int i=0;i<n_epochs_c;i++) {
		for(int j=0;j<max_islands_per_epoch_c;j++) {
			if( maps[i][j] != NULL ) {
				delete maps[i][j];
				maps[i][j] = NULL;
			}
		}
	}
	map = NULL;
	if( screen != NULL ) {
		LOG("delete screen %d\n", screen);
		delete screen;
		screen = NULL;
	}
	LOG("clean up tracked objects\n");
	TrackedObject::cleanup();
	// no longer need to stop music, as it's deleted as a TrackedObject
	//stopMusic();
	LOG("free sound\n");
	freeSound();
#ifdef USING_QT
        if( qt_settings != NULL ) {
            delete qt_settings;
            qt_settings = NULL;
        }
#endif
	LOG("delete application %d\n", application);
	delete application;
	application = NULL;
	LOG("cleanup done\n");
}

//bool quit = false;
bool debugwindow = false;

void keypressEscape() {
    /*if( gameStateID == GAMESTATEID_PLACEMEN || gameStateID == GAMESTATEID_PLAYING ) {
        gamestate->requestQuit();
    }
	else {
		application->setQuit();
	}*/
    if( !state_changed ) {
	    gamestate->requestQuit();
	}
}

void keypressP() {
    if( gameStateID == GAMESTATEID_PLAYING ) {
        paused = !paused;
        if( paused ) {
            playSample(s_on_hold);

            //playSample(s_conquered); // uncomment to test chained samples
            //playSample(s_explosion, SOUND_CHANNEL_FX);
            //playSample(s_explosion);
        }
    }
}

/*void keypressQ() {
	if( gameStateID == GAMESTATEID_PLAYING && !state_changed ) {
		static_cast<PlayingGameState *>(gamestate)->requestQuit();
	}
}*/

void mouseClick(int m_x, int m_y, bool m_left, bool m_middle, bool m_right, bool click) {
	const int mousepress_delay = 100;
	static int lastmousepress_time = 0;
	T_ASSERT( m_left || m_middle || m_right );
	if( click ) {
		//LOG("mouse click\n");
        gamestate->mouseClick(m_x, m_y, m_left, m_middle, m_right, click);
	}
	else {
		// use getTicks() not getRealTime(), as this function may be called at any occasion (due to mouse click events), so getRealTime() might not yet be updated!
		int ticks = application->getTicks();
		if( ticks - lastmousepress_time >= mousepress_delay ) {
			//LOG("mouse press: %d, %d\n", lastmousepress_time, ticks);
			lastmousepress_time = ticks;
			gamestate->mouseClick(m_x, m_y, m_left, m_middle, m_right, click);
		}
		else {
			//LOG("ignore mouse press\n");
		}
	}
}

void updateGame() {
	if( !paused ) {
		int m_x = 0, m_y = 0;
		bool m_left = false, m_middle = false, m_right = false;
		bool m_res = screen->getMouseState(&m_x, &m_y, &m_left, &m_middle, &m_right);
		/*screen->getMouseCoords(&m_x, &m_y);
		application->getMousePressed(&m_left, &m_middle, &m_right);*/
		if( m_res ) {
			mouseClick(m_x, m_y, m_left, m_middle, m_right, false);
		}
		else {
			resetMouseClick();
		}
	}

	// update
	if( !paused ) {
		if( gameStateID == GAMESTATEID_PLAYING ) {
			for(int i=0;i<n_players_c;i++) {
				if( i != human_player && players[i] != NULL )
					players[i]->doAIUpdate(human_player, static_cast<PlayingGameState *>(gamestate));
			}
			//players[ enemy_player ]->doAIUpdate();
			gamestate->update();
			for(int y=0;y<map_height_c;y++) {
				for(int x=0;x<map_width_c;x++) {
					/*if( map->sectors[x][y] != NULL )
					map->sectors[x][y]->update();*/
					Sector *sector = map->getSector(x, y);
					if( sector != NULL ) {
						sector->update(human_player);
					}
				}
			}
		}
	}

	if( gameStateID == GAMESTATEID_PLAYING && !state_changed && gameMode != GAMEMODE_MULTIPLAYER_CLIENT ) {
		if( human_player != PLAYER_DEMO && !playerAlive(human_player) ) {
			playSample(s_itis_all_over);
			state_changed = true;
			gameResult = GAMERESULT_LOST;
			gamestate->fadeScreen(true, SHORT_DELAY, endIsland);
		}
		else {
			bool all_dead = true;
			/*for(int i=0;i<n_players_c && all_dead;i++) {
			if( i != human_player && players[i] != NULL && playerAlive(i) )
			all_dead = false;
			}*/
			for(int i=0;i<n_players_c;i++) {
				if( i != human_player && players[i] != NULL && !players[i]->isDead() ) {
					if( playerAlive(i) )
						all_dead = false;
					else {
						players[i]->kill(static_cast<PlayingGameState *>(gamestate));
					}
				}
			}
			//if( !playerAlive(enemy_player) ) {
			if( all_dead ) {
				playSample(s_won);
				state_changed = true;
				gameResult = GAMERESULT_WON;
				gamestate->fadeScreen(true, SHORT_DELAY, endIsland);
			}
		}
	}

	if( dispose_gamestate != NULL ) {
		LOG("delete dispose_gamestate %d (current gamestate is %d)\n", dispose_gamestate, gamestate);
		delete dispose_gamestate;
		LOG("done delete\n");
		dispose_gamestate = NULL;
	}
}

void drawGame() {
	// we now redraw even when paused, to display paused message
	gamestate->draw();
}

const char prefs_filename[] = "prefs";
const char onemousebutton_key[] = "onemousebutton";
const char play_music_key[] = "play_music";

void loadPrefs() {
	char *prefs_fullfilename = getApplicationFilename(prefs_filename);
	SDL_RWops *prefs_file = SDL_RWFromFile(prefs_fullfilename, "rb");
	if( prefs_file != NULL ) {
		// reset
		play_music = false;
		onemousebutton = false;

		const int MAX_LINE = 4096;
		char line[MAX_LINE+1] = "";
		char buffer[MAX_LINE+1] = "";
		int buffer_offset = 0;
		bool reached_end = false;
		int newline_index = 0;
		bool ok = true;
		while( ok ) {
			bool done = readLineFromRWOps(ok, prefs_file, buffer, line, MAX_LINE, buffer_offset, newline_index, reached_end);
			if( !ok )  {
				LOG("failed to read line for: %s\n", prefs_fullfilename);
			}
			else if( done ) {
				break;
			}
			else {
				LOG("read prefs line: %s", line);
				if( strncmp(line, onemousebutton_key, strlen(onemousebutton_key)) == 0 ) {
					LOG("enable onemousebutton from prefs\n");
					onemousebutton = true;
				}
				else if( strncmp(line, play_music_key, strlen(play_music_key)) == 0 ) {
					LOG("enable play_music from prefs\n");
					play_music = true;
				}
			}
		}
		prefs_file->close(prefs_file);

#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR) || defined(Q_WS_MAEMO_5) || defined(Q_OS_ANDROID) || defined(__ANDROID__)
		// force onemousebutton mode, just to be safe
		onemousebutton = true;
#endif
	}
	delete [] prefs_fullfilename;
}

void savePrefs() {
	char *prefs_fullfilename = getApplicationFilename(prefs_filename);
	SDL_RWops *prefs_file = SDL_RWFromFile(prefs_fullfilename, "wb+");
	if( prefs_file == NULL ) {
		LOG("failed to open: %s\n", prefs_fullfilename);
	}
	else {
		if( onemousebutton ) {
			prefs_file->write(prefs_file, onemousebutton_key, sizeof(char), strlen(onemousebutton_key));
			prefs_file->write(prefs_file, "\n", sizeof(char), 1);
		}
		if( play_music ) {
			prefs_file->write(prefs_file, play_music_key, sizeof(char), strlen(play_music_key));
			prefs_file->write(prefs_file, "\n", sizeof(char), 1);
		}
		prefs_file->close(prefs_file);
	}
	delete [] prefs_fullfilename;
}

void playGame(int n_args, char *args[]) {
    LOG("playGame()\n");
	bool fullscreen = false;
#ifdef _DEBUG
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF );  // TEST!
	debugwindow = true;
#endif
	//debugwindow = true;
	//fullscreen = true;

#if defined(__ANDROID__)
	fullscreen = true; // always fullscreen on Android
#endif

#if !defined(Q_OS_ANDROID) && !defined(__ANDROID__)
        // n.b., crashes when run on Galaxy Nexus (even though fine in the emulator)
	for(int i=0;i<n_args;i++) {
		if( strcmp(args[i], "fullscreen") == 0 )
			fullscreen = true;
		else if( strcmp(args[i], "debugwindow") == 0 )
			debugwindow = true;
		else if( strcmp(args[i], "onemousebutton") == 0 )
			onemousebutton = true;
		else if( strcmp(args[i], "mobile_ui") == 0 )
			mobile_ui = true;
		else if( strcmp(args[i], "server") == 0 )
			gameMode = GAMEMODE_MULTIPLAYER_SERVER;
		else if( strcmp(args[i], "client") == 0 )
			gameMode = GAMEMODE_MULTIPLAYER_CLIENT;
	}
#endif

#ifdef _WIN32
	if( debugwindow ) {
		AllocConsole();
        /*FILE *dummy = NULL;
        freopen_s(&dummy, "con", "w", stdout);*/
        FILE *dummy = freopen("con", "w", stdout);
		SetConsoleTitleA("DEBUG:");
	}
#endif

	initLogFile();

        /*if( access("data", 0)==0 ) {
	use_amigadata = true;
	}
	else if( access("Mega Lo Mania", 0)==0 ) {
	use_amigadata = false;
	}
	else {
	LOG("can't find data folder\n");
	return;
	}*/
	LOG("onemousebutton?: %d\n", onemousebutton);
	LOG("mobile_ui?: %d\n", mobile_ui);

#ifdef USING_QT
        qt_settings = new QSettings("Mark Harman", "Gigalomania");
        bool qt_ok = true;
        int play_music_i = qt_settings->value(play_music_key_c, default_play_music_c).toInt(&qt_ok);
        if( !qt_ok ) {
            LOG("qt_settings: play_music not ok, set to default\n");
            play_music = default_play_music_c;
        }
        else {
            play_music = play_music_i != 0;
            LOG("qt_settings: set play_music to %d\n", play_music);
        }
#else
	loadPrefs();
#endif

    for(int i=0;i<n_epochs_c;i++)
		for(int j=0;j<max_islands_per_epoch_c;j++)
			maps[i][j] = NULL;

	for(int i=0;i<max_islands_per_epoch_c;i++)
		completed_island[i] = false;

	for(int i=0;i<n_players_c;i++)
		n_suspended[i] = 0;

	// init application
	application = new Application();
	if( !application->init() ) {
		LOG("failed to init application\n");
	}
	// init sound
	else if( !initSound() ) {
		// don't fail, just warn
		LOG("Failed to initialise sound system\n");
	}

	LOG("successfully opened libraries\n");

	bool ok = true;
	if( !openScreen(fullscreen) ) {
		LOG("failed to open screen\n");
		ok = false;
#ifdef _WIN32
		MessageBoxA(NULL, "Failed to open screen", "Error", MB_OK|MB_ICONEXCLAMATION);
#endif
	}

    int time_s = clock();
	drawProgress(0);

	if( ok && !loadImages() ) {
		LOG("failed to load images\n");
		ok = false;
#ifdef _WIN32
		MessageBoxA(NULL, "Failed to load images", "Error", MB_OK|MB_ICONEXCLAMATION);
#endif
	}
	LOG("time taken to load images: %d\n", clock() - time_s);
	// loadImages takes progress up to 80%
	if( !ok ) {
		cleanup();
		return;
	}

	// n.b., still need to load samples even if sound failed to initialise, as we want the Sample objects for the textual display
	if( !loadSamples() ) {
		// don't fail, just warn
		LOG("warning - failed to load samples\n");
		// no longer show message - no longer an error, as the default install won't have any samples!
/*#ifdef _WIN32
		MessageBoxA(NULL, "Failed to load all samples", "Warning", MB_OK|MB_ICONEXCLAMATION);
#endif*/
	}
	drawProgress(85);

	setupInventions();
	drawProgress(87);
	setupElements();
	drawProgress(89);
	Design::setupDesigns();
	drawProgress(90);
	if( !createMaps() ) {
		LOG("failed to create maps\n");
		cleanup();
#ifdef _WIN32
		MessageBoxA(NULL, "Failed to create maps", "Error", MB_OK|MB_ICONEXCLAMATION);
#endif
		return;
	}
	drawProgress(95);

	map = maps[start_epoch][selected_island];

	for(size_t i=0;i<TrackedObject::getNumTags();i++) {
		TrackedObject *to = TrackedObject::getTag(i);
		if( to != NULL && strcmp( to->getClass(), "CLASS_IMAGE" ) == 0 ) {
			Image *image = (Image *)to;
			if( !image->convertToDisplayFormat() ) {
				LOG("failed to convertToDisplayFormat\n");
				cleanup();
#ifdef _WIN32
				MessageBoxA(NULL, "Failed to create texture images", "Error", MB_OK|MB_ICONEXCLAMATION);
#endif
				return;
			}
		}
	}

	drawProgress(100);
    int time_taken = clock() - time_s;
	LOG("time taken to load data: %d\n", time_taken);

	char buffer[256] = "";
	sprintf(buffer, "Gigalomania, version %d.%d", majorVersion, minorVersion);
	screen->setTitle(buffer);

	setGameStateID(GAMESTATEID_CHOOSEGAMETYPE);
	//setGameStateID(GAMESTATEID_CHOOSEPLAYER);

    LOG("all done!\n");
	application->runMainLoop();

	cleanup();
	LOG("exiting..\n");
}

void quitGame() {
	//quit = true;
	application->setQuit();
}

bool playerAlive(int player) {
	/*int n_player_sectors = 0;
	int n_army = 0;
	for(int x=0;x<map_width_c;x++) {
	for(int y=0;y<map_height_c;y++) {
	Sector *sector = map->sectors[x][y];
	if( sector != NULL ) {
	if( player == sector->getActivePlayer() )
	n_player_sectors++;
	n_army += sector->getArmy(player)->getTotal();
	}
	}
	}
	return ( n_player_sectors > 0 || n_army > 0 );*/
	for(int x=0;x<map_width_c;x++) {
		for(int y=0;y<map_height_c;y++) {
			//Sector *sector = map->sectors[x][y];
			Sector *sector = map->getSector(x, y);
			if( sector != NULL ) {
				if( player == sector->getActivePlayer() )
					return true;
				else if( sector->getArmy(player)->any(true) )
					return true;
			}
		}
	}
	return false;
}

#if defined(__ANDROID__)

// JNI for Android

#include <jni.h>
#include <android/log.h>

// see http://wiki.libsdl.org/SDL_AndroidGetActivity

void launchUrl(string url) {
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: launch url: %s", url.c_str());
    // retrieve the JNI environment.
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    if (!env)
    {
        __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: can't find env");
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: obtained env");

    // retrieve the Java instance of the SDLActivity
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (!activity)
    {
        __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: can't find activity");
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: obtained activity");

    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    jclass clazz( env->GetObjectClass(activity) );
    if (!clazz)
    {
        __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: can't find class");
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: obtained class");

    // find the identifier of the method to call
    jmethodID method_id = env->GetMethodID(clazz, "launchUrl", "(Ljava/lang/String;)V");
    if (!method_id)
    {
        __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: can't find launchUrl method");
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: obtained method");

    // effectively call the Java method
	jstring str = env->NewStringUTF(url.c_str());
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: about to call static method");
    env->CallVoidMethod( activity, method_id, str );
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: called method");
    
    // clean up the local references.
    env->DeleteLocalRef(str);
    env->DeleteLocalRef(activity);
    __android_log_print(ANDROID_LOG_INFO, "Gigalomania", "JNI: done");
}
#endif
