#pragma once

/** Classes to manage the various gamestates.
*/

using std::vector;
using std::string;

//#include "game.h"
#include "common.h"

namespace Gigalomania {
	class Image;
	class ImageButton;
	class Button;
	class PanelPage;
}

using namespace Gigalomania;

class ChooseGameTypePanel;
class ChooseDifficultyPanel;
class ChooseMenPanel;
class GamePanel;
class Sector;
class Army;
class Soldier;
class Building;
class Map;
class Design;
class Invention;

const int offset_map_x_c = 8;
const int offset_map_y_c = 16;

const int offset_land_x_c = 96;
const int offset_land_y_c = 50;
//const int offset_land_y_c = 34;

// mustn't be too low, otherwise difficult to touch on Android (Galaxy Nexus)
//const int quit_button_offset_c = 16;
const int quit_button_offset_c = 24;

const int offset_openpitmine_x_c = 32;
const int offset_openpitmine_y_c = 32;

/*const int land_width_c =  300 - offset_land_x_c;
const int land_height_c = 210 - offset_land_y_c;*/
const int land_width_c =  204;
const int land_height_c = 160;

//const int max_soldiers_in_sector_c = 1000;

class Feature {
protected:
	Image **image;
	int n_frames;
	int xpos, ypos;
	bool at_front;
public:
	Feature(Image *image[],int n_frames,int xpos,int ypos) {
		this->image = image;
		this->n_frames = n_frames;
		this->xpos = xpos;
		this->ypos = ypos;
		this->at_front = false;
	}

	const Image *getImage(int counter) const {
		return this->image[counter % n_frames];
	}
	void setImage(Image *image[], int n_frames) {
		this->image = image;
		this->n_frames = n_frames;
	}
	void draw() const;
	int getX() const {
		return this->xpos;
	}
	int getY() const {
		return this->ypos;
	}
	void setAtFront(bool at_front) {
		this->at_front = at_front;
	}
	bool isAtFront() const {
		return this->at_front;
	}
};

class TimedEffect {
protected:
	int timeset;
	void (*func_finish) ();
public:
	TimedEffect();
	TimedEffect(int delay, void (*func_finish)());
	virtual ~TimedEffect() {
		if( func_finish != NULL ) {
			// we need to make a copy, incase this class is
			// destroyed from within the called function
			void (*temp_func_finish) ();
			temp_func_finish = func_finish;
			func_finish = NULL;
			temp_func_finish();
		}
	}
	virtual bool render() const {
		return false;
	}
};

class AmmoEffect : public TimedEffect {
	int gametimeset;
	int epoch;
	AmmoDirection dir;
	int xpos, ypos;
public:
	AmmoEffect(int epoch, AmmoDirection dir, int xpos, int ypos);
	virtual bool render() const;
};

class FadeEffect : public TimedEffect {
	Image *image;
	bool white;
	bool out;
public:
	FadeEffect(bool white,bool out,int delay, void (*func_finish)());
	virtual ~FadeEffect();
	virtual bool render() const;
};

class FlashingSquare : public TimedEffect {
	int xpos, ypos;
public:
	FlashingSquare(int xpos, int ypos) : TimedEffect() {
		this->xpos = xpos;
		this->ypos = ypos;
	}

	virtual bool render() const;
};

class AnimationEffect : public TimedEffect {
	Image **images;
	int n_images;
	int xpos, ypos;
	int time_per_frame;
	bool dir;
public:
	AnimationEffect(int xpos, int ypos,Image **images, int n_images,int time_per_frame,bool dir) : TimedEffect() {
		this->images = images;
		this->n_images = n_images;
		this->xpos = xpos;
		this->ypos = ypos;
		this->time_per_frame = time_per_frame;
		this->dir = dir;
	}
	virtual bool render() const;
};

class TextEffect : public TimedEffect {
	int xpos, ypos;
	string text;
	int duration;
public:
	TextEffect(string text, int xpos, int ypos, int duration) : TimedEffect(), text(text), xpos(xpos), ypos(ypos), duration(duration) {
	}
	virtual bool render() const;
};

class GameState {
protected:
	int client_player;
	FadeEffect *fade;
	FadeEffect *whitefade;
	PanelPage *screen_page;
	bool mobile_ui_display_mouse; // if mobile_ui is true, should we display the mouse icon?
	Image *mouse_image;
	int mouse_off_x, mouse_off_y;
	enum ConfirmType {
		CONFIRMTYPE_UNKNOWN = -1,
		CONFIRMTYPE_NEWGAME = 0,
		CONFIRMTYPE_QUITGAME = 1
	};
	ConfirmType confirm_type;
    PanelPage *confirm_window;
    Button *confirm_yes_button;
    Button *confirm_no_button;

	void setDefaultMouseImage();
    void createQuitWindow();

public:
	GameState(int client_player);
	virtual ~GameState();

	virtual void reset();
	virtual void draw();
	virtual void update() {};
	virtual void mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
    virtual void requestQuit();

	void fadeScreen(bool out, int delay, void (*func_finish)());
	void whiteFlash();
	/*void addEffect(TimedEffect *effect) {
		this->effects->push_back(effect);
	}*/
	virtual void addTextEffect(TextEffect *effect) {
		// only implemented for PlayingGameState
	}
    void closeConfirmWindow();
    bool hasConfirmWindow() const {
        return confirm_window != NULL;
    }
};

class ChooseGameTypeGameState : public GameState {
	ChooseGameTypePanel *choosegametypePanel;

public:
	ChooseGameTypeGameState(int client_player);
	virtual ~ChooseGameTypeGameState();

	virtual void reset();
	virtual void draw();
	virtual void mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);

	ChooseGameTypePanel *getChooseGameTypePanel();
};

class ChooseDifficultyGameState : public GameState {
	ChooseDifficultyPanel *choosedifficultyPanel;

public:
	ChooseDifficultyGameState(int client_player);
	virtual ~ChooseDifficultyGameState();

	virtual void reset();
	virtual void draw();
	virtual void mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);

	ChooseDifficultyPanel *getChooseDifficultyPanel();
};

class ChoosePlayerGameState : public GameState {
	Button *button_red;
	Button *button_yellow;
	Button *button_green;
	Button *button_blue;
public:
	ChoosePlayerGameState(int client_player);
	virtual ~ChoosePlayerGameState();

	virtual void reset();
	virtual void draw();
	virtual void mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
};

class PlaceMenGameState : public GameState {
	ChooseMenPanel *choosemenPanel;
	int off_x;
	PanelPage *map_panels[map_width_c][map_height_c];
	int start_map_x, start_map_y;

public:
	PlaceMenGameState(int client_player);
	virtual ~PlaceMenGameState();

	virtual void reset();
	virtual void draw();
	virtual void mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
    virtual void requestQuit() {
		this->createQuitWindow();
	}

	ChooseMenPanel *getChooseMenPanel();
    /*int getOffX() const {
		return this->off_x;
    }*/
	const PanelPage *getMapPanel(int x, int y) const;
	PanelPage *getMapPanel(int x, int y);
	int getStartMapX() const {
		return this->start_map_x;
	}
	int getStartMapY() const {
		return this->start_map_y;
	}
	void requestNewGame();
};

class PlayingGameState : public GameState {
	const Sector *current_sector;
	GamePanel *gamePanel;
	ImageButton *speed_button;
	ImageButton *shield_buttons[n_players_c];
	ImageButton *shield_blank_button;
	PanelPage *shield_number_panels[n_players_c];
	PanelPage *land_panel;
	Button *pause_button;
	Button *quit_button;
    /*PanelPage *confirm_window;
	Button *confirm_yes_button;
    Button *confirm_no_button;*/
	int flag_frame_step;
	int defenders_last_frame_update;
	int soldiers_last_time_moved_x;
	int soldiers_last_time_moved_y;
	int soldiers_last_time_turned;
	int air_last_time_moved;
	const Army *selected_army;
	//int n_soldiers[n_players_c];
	//Vector *soldiers[n_players_c];
	vector<Soldier *> *soldiers[n_players_c];
	vector<TimedEffect *> *effects;
	//Vector *ammo_effects;
	vector<TimedEffect *> *ammo_effects;
	TextEffect *text_effect;
	/*SmokeParticleSystem *smokeParticleSystem;
	SmokeParticleSystem *smokeParticleSystem_busy;*/
	enum MapDisplay {
		MAPDISPLAY_MAP = 0,
		MAPDISPLAY_UNITS = 1
	};
	MapDisplay map_display;
	int player_asking_alliance;
	PanelPage *map_panels[map_width_c][map_height_c];
	Button *alliance_yes;
	Button *alliance_no;
	int n_deaths[n_players_c][n_epochs_c+1];

	bool openPitMine();
	bool validSoldierLocation(int epoch,int xpos,int ypos);
	bool buildingMouseClick(int s_m_x,int s_m_y,bool m_left,bool m_right,Building *building);
	void moveTo(int map_x,int map_y);
	void blueEffect(int xpos,int ypos,bool dir);
	void refreshShieldNumberPanels();
	void setupMapGUI();
	bool readSectorsProcessLine(Map *map, char *line, bool *done_header, int *sec_x, int *sec_y);
	bool readSectors(Map *map);

	//static void buttonSpeedClick(void *data, int arg, bool m_left, bool m_middle, bool m_right);
public:

	PlayingGameState(int client_player);
	virtual ~PlayingGameState();

	void createSectors( int x, int y, int n_men);

	virtual void reset();
	virtual void draw();
	virtual void update();
	virtual void mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
    virtual void requestQuit() {
		this->createQuitWindow();
	}

	GamePanel *getGamePanel();
	//Sector *getCurrentSector();
	const Sector *getCurrentSector() const;
	bool viewingActiveClientSector() const;
	bool viewingAnyClientSector() const; // includes shutdown sectors
	void resetShieldButtons();
	void setFlashingSquare(int xpos,int ypos);
	void addBuilding(Building *building);
	void refreshSoldiers(bool flash);
	void deathEffect(int xpos,int ypos);
	void addEffect(TimedEffect *effect) {
		this->effects->push_back(effect);
	}
	virtual void addTextEffect(TextEffect *effect) {
		if( text_effect != NULL ) {
			delete text_effect;
		}
		text_effect = effect;
	}
	void refreshButtons();
	const Army *getSelectedArmy() const {
		return this->selected_army;
	}
	void clearSelectedArmy() {
		this->selected_army = NULL;
	}
	bool canRequestAlliance(int player,int i) const;
	void requestAlliance(int player,int i,bool human);
	void makeAlliance(int player,int i);
	const int getPlayerAskingAlliance() const {
		return this->player_asking_alliance;
	}
	void cancelPlayerAskingAlliance();
	void registerDeath(int player, int epoch) {
		n_deaths[player][epoch]++;
	}
	void refreshTimeRate();

	// functions for requesting a modification to the game world based on client user input
	// for now, these functions make the modification themselves directly - later on, we can go via a server class
	void setNDesigners(int sector_x, int sector_y, int n_designers);
	void setNWorkers(int sector_x, int sector_y, int n_workers);
	void setFAmount(int sector_x, int sector_y, int n_famount);
	void setNMiners(int sector_x, int sector_y, Id element, int n_miners);
	void setNBuilders(int sector_x, int sector_y, Type type, int n_builders);

	void setCurrentDesign(int sector_x, int sector_y, Design *design);
	void setCurrentManufacture(int sector_x, int sector_y, Design *design);

	void assembledArmyEmpty(int sector_x, int sector_y);
	void assembleArmyUnarmed(int sector_x, int sector_y, int n);
	bool assembleArmy(int sector_x, int sector_y, int epoch, int n);
	void returnAssembledArmy(int sector_x, int sector_y);
	void returnArmy(int sector_x, int sector_y, int src_x, int src_y);
	bool moveArmyTo(int src_x, int src_y, int target_x, int target_y);
	bool moveAssembledArmyTo(int src_x, int src_y, int target_x, int target_y);
	void nukeSector(int src_x, int src_y, int target_x, int target_y);

	void deployDefender(int sector_x, int sector_y, Type type, int turret, int epoch);
	void returnDefender(int sector_x, int sector_y, Type type, int turret);
	void useShield(int sector_x, int sector_y, Type type, int shield);

	void trashDesign(int sector_x, int sector_y, Invention *invention);

	void shutdown(int sector_x, int sector_y);
	//current_sector->shutdown();
};

class EndIslandGameState : public GameState {
public:
	EndIslandGameState(int client_player) : GameState(client_player) {
	}
	virtual ~EndIslandGameState() {
	}

	virtual void reset();
	virtual void draw();
	virtual void mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
    virtual void requestQuit();
};

class GameCompleteGameState : public GameState {
public:
	GameCompleteGameState(int client_player) : GameState(client_player) {
	}
	virtual ~GameCompleteGameState() {
	}

	virtual void reset();
	virtual void draw();
	virtual void mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
    virtual void requestQuit();
};
