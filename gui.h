#pragma once

/** Implements the GUI.
*/

namespace Gigalomania {
	class PanelPage;
	class Button;
	class ImageButton;
	class CycleButton;
}

using namespace Gigalomania;

class PlaceMenGameState;
class PlayingGameState;
class Invention;

#include "common.h"
#include "panel.h"

typedef void (ClickFunc) (void *data, int arg, bool m_left, bool m_middle, bool m_right);

class OneMouseButtonPanel : public PanelPage {
	ClickFunc *clickFunc;
	void *data;
	int arg;
    ImageButton *button_right;
    ImageButton *button_left;
public:
	OneMouseButtonPanel(ClickFunc *clickFunc, void *data, int arg, PanelPage *caller_button);

	virtual void input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
};

void processClick(ClickFunc *clickFunc, PanelPage *panel, void *data, int arg, PanelPage *caller_button, bool m_left, bool m_middle, bool m_right, bool click);

class ChooseGameTypePanel : public MultiPanel {
	Button *button_singleisland;
	Button *button_allislands;

public:
	ChooseGameTypePanel();

	virtual void input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
};

class ChooseDifficultyPanel : public MultiPanel {
	Button *button_easy;
	Button *button_medium;
	Button *button_hard;

public:
	ChooseDifficultyPanel();

	virtual void input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
};

class ChooseMenPanel : public MultiPanel {
	PlaceMenGameState *gamestate;
	Button *button_continue;
	CycleButton *button_sound;
	CycleButton *button_music;
	CycleButton *button_onemousebutton;
	Button *button_nextisland;
	Button *button_nextepoch;
	Button *button_options;
	Button *button_play;
#if defined(_WIN32) || defined(__ANDROID__)
    Button *button_help;
#endif
	Button *button_quit;
	Button *button_new;
	Button *button_load;
	Button *button_save;
	Button *button_load_load[n_slots_c];
	Button *button_load_cancel;
	Button *button_save_save[n_slots_c];
	Button *button_save_cancel;
	PanelPage *button_nmen;
	Button *button_cancel;
	int n_men;

	void setInfoText();
	void refreshLoadSaveButtons();

	static void buttonNMenClick(void *data, int arg, bool m_left, bool m_middle, bool m_right);
public:
	enum State {
		STATE_CHOOSEISLAND = 0,
		STATE_CHOOSEMEN,
		STATE_OPTIONS,
		STATE_LOADGAME,
		STATE_SAVEGAME,
		N_STATES
	};

	ChooseMenPanel(PlaceMenGameState *placeMenGameState);

	virtual void draw();
	virtual void input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);

	int getNMen() const {
		return n_men;
	}
	void setNMen(int n_men) {
		this->n_men = n_men;
	}
};

class GamePanel : public MultiPanel {
public:
	enum State {
		STATE_SECTORCONTROL = 0,
		STATE_DESIGN = 1,
		STATE_DEFENCE = 2,
		STATE_ATTACK = 3,
		STATE_ELEMENTSTOCKS = 4,
		STATE_BUILD = 5,
		STATE_SHIELD = 6,
		STATE_KNOWNDESIGNS = 7,
		STATE_DESIGNINFO = 8,
		STATE_FACTORY,
		N_STATES
	};
	enum MouseState {
		MOUSESTATE_NORMAL = 0,
		MOUSESTATE_DEPLOY_DEFENCE = 1,
		MOUSESTATE_DEPLOY_WEAPON = 2,
		MOUSESTATE_DEPLOY_SHIELD = 3,
		MOUSESTATE_SHUTDOWN = 4,
		N_MOUSESTATES
	};
private:
	PlayingGameState *gamestate;
	int client_player;
	//State state;
	MouseState mousestate;
	int deploy_shield;
	int deploy_defence;
	int deploy_weapon;
	Invention *designinfo;

	ImageButton *button_design;
	PanelPage *button_ndesigners;
	ImageButton *button_shield;
	ImageButton *button_defence;
	ImageButton *button_attack;
	//ImageButton *button_elementstocks;
	int element_index[4];
	PanelPage *button_elements[4];
	PanelPage *button_nminers[4];
	ImageButton *button_build[N_BUILDINGS];
	PanelPage *button_nbuilders[N_BUILDINGS];
	ImageButton *button_knowndesigns;
	ImageButton *button_factory;
	PanelPage *button_nworkers;

	ImageButton *button_bigdesign;
	ImageButton *button_designers;
	ImageButton *button_shields[4];
	ImageButton *button_defences[4];
	ImageButton *button_weapons[4];

	ImageButton *button_bigshield;
	ImageButton *button_deploy_shields[4];
	ImageButton *button_shutdown;

	ImageButton *button_bigdefence;
	ImageButton *button_deploy_defences[4];

	ImageButton *button_bigattack;
	ImageButton *button_deploy_unarmedmen;
	ImageButton *button_deploy_attackers[4];
	ImageButton *button_return_attackers;

	ImageButton *button_bigelementstocks;
	PanelPage *button_elements2[4];
	PanelPage *button_nminers2[4];

	ImageButton *button_bigbuild;
	ImageButton *button_nbuilders2[N_BUILDINGS];

	ImageButton *button_bigknowndesigns;
	ImageButton *button_knownshields[4];
	ImageButton *button_knowndefences[4];
	ImageButton *button_knownweapons[4];

	ImageButton *button_bigdesigninfo;
	ImageButton *button_trashdesign;

	ImageButton *button_bigfactory;
	ImageButton *button_workers;
	ImageButton *button_famount;
	ImageButton *button_fshields[4];
	ImageButton *button_fdefences[4];
	ImageButton *button_fweapons[4];

	void changeMiners(Id element,bool decrease,int n);

	static void buttonNDesignersClick(void *data, int arg, bool m_left, bool m_middle, bool m_right);
	static void buttonNManufacturersClick(void *data, int arg, bool m_left, bool m_middle, bool m_right);
	static void buttonFAmountClick(void *data, int arg, bool m_left, bool m_middle, bool m_right);
	static void buttonNMinersClick(void *data, int arg, bool m_left, bool m_middle, bool m_right);
	static void buttonNBuildersClick(void *data, int arg, bool m_left, bool m_middle, bool m_right);
public:
	GamePanel(PlayingGameState *gamestate, int client_player);
	virtual ~GamePanel();

	void setup();
	void refreshCanDesign();
	void refreshDesignInventions();
	void refreshManufactureInventions();
	void refreshDeployInventions();
	void refreshShutdown();
	void refresh();
	//void setState(State state);
	virtual void setPage(int page);
	MouseState getMouseState() const;
	void setMouseState(MouseState mousestate);
	int getDeployShield() const {
		return deploy_shield;
	}
	int getDeployDefence() const {
		return deploy_defence;
	}
	int getDeployWeapon() const {
		return deploy_weapon;
	}
	virtual void draw();
	virtual void input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click);
};
