//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>
#include <cmath> // n.b., needed on Linux at least
#include <cstdlib> // n.b., needed on Linux at least (for abs)
#include <cstdio> // n.b., needed on Linux at least

#include <sstream>
using std::stringstream;

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#include "gui.h"
#include "sector.h"
#include "player.h"
#include "game.h"
#include "gamestate.h"
#include "utils.h"

#ifdef USING_QT
#include "qt_screen.h"
#include "qt_image.h"
#else
#include "screen.h"
#include "image.h"
#endif

//---------------------------------------------------------------------------

const int offset_panel_x_c = 0;
const int offset_panel_y_c = 100;
const int offset_attack_x_c = 4;
const int space_attack_x_c = 18;

const int build_step_y_c = 40;

void processClick(ClickFunc *clickFunc, PanelPage *panel, void *data, int arg, PanelPage *caller_button, bool m_left, bool m_middle, bool m_right, bool click) {
	// wrapper function to handle one-mouse-button UI support
	if( oneMouseButtonMode() && click) {
		LOG("open onemousebutton panel\n");
		OneMouseButtonPanel *oneMouseButtonPanel = new OneMouseButtonPanel(clickFunc, data, arg, caller_button);
		//panel->addToPanel(panel->getPage(), oneMouseButtonPanel);
		panel->add(oneMouseButtonPanel);
		oneMouseButtonPanel->setModal();
		if( caller_button->getInfoLMB() != NULL && mobile_ui ) {
			addTextEffect(new TextEffect(caller_button->getInfoLMB(), 160, caller_button->getTop(), 3000));
		}
	}
	else if( !oneMouseButtonMode() && !click ) {
		//buttonNMenClick(panel, m_left, m_middle, m_right);
		//(*clickFunc)(panel, arg, m_left, m_middle, m_right);
		(*clickFunc)(data, arg, m_left, m_middle, m_right);
	}
}

//OneMouseButtonPanel::OneMouseButtonPanel(ClickFunc *clickFunc, void *data, PanelPage *caller_button) : PanelPage(caller_button->getLeft() - 16, caller_button->getTop(), caller_button->getWidth() + 32, 64), clickFunc(clickFunc), data(data) {
OneMouseButtonPanel::OneMouseButtonPanel(ClickFunc *clickFunc, void *data, int arg, PanelPage *caller_button) : PanelPage(caller_button->getOffsetX() - 32, caller_button->getOffsetY(), caller_button->getWidth() + 64, 32), clickFunc(clickFunc), data(data), arg(arg), caller_button(caller_button) {
	this->button_left = new ImageButton(0, 0, arrow_left);
    //this->button_left->setAlpha(true, 160);
	this->add(this->button_left);
	this->button_right = new ImageButton(32 + caller_button->getWidth(), 0, arrow_right);
    //this->button_right->setAlpha(true, 160);
    this->add(this->button_right);
}

void OneMouseButtonPanel::input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	/*if( caller_button == NULL ) {
		LOG("caller_button NULL - delete onemousebuttonpanel\n");
		delete this;
	}
	else*/ if( m_left && !click && this->button_left->mouseOver(m_x, m_y) ) {
		(*clickFunc)(data, arg, true, false, false);
	}
	else if( m_left && !click && this->button_right->mouseOver(m_x, m_y) ) {
		(*clickFunc)(data, arg, false, false, true);
	}
    else if( m_left && !this->mouseOver(m_x, m_y) ) {
        // must do this whether click is true or false, so that the arrows can be removed, and another action taken, with the same click!
		LOG("delete onemousebuttonpanel\n");
		delete this;
	}
}

ChooseGameTypePanel::ChooseGameTypePanel() : MultiPanel(1, 0, 0) {
	this->c_page = 0;
	this->addToPanel(0, new Button(105, 50, "CHOOSE A GAME TYPE", letters_large));
    this->button_singleisland = new Button(120, 90, "SINGLE ISLAND", letters_large);
	this->button_singleisland->setInfoLMB("choose to play on\nany island");
	this->addToPanel(0, button_singleisland);
    this->button_allislands = new Button(125, 130, "ALL ISLANDS", letters_large);
	this->button_allislands->setInfoLMB("start on first age and\nplay full game");
	this->addToPanel(0, button_allislands);
}

void ChooseGameTypePanel::input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	MultiPanel::input(m_x, m_y, m_left, m_middle, m_right, click);
	if( this->hasModal() ) {
		return;
	}
	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);

	if( m_left && click && this->button_singleisland->mouseOver(m_x, m_y) ) {
        registerClick();
        gameType = GAMETYPE_SINGLEISLAND;
        setGameStateID(GAMESTATEID_CHOOSEPLAYER);
	}
	else if( m_left && click && this->button_allislands->mouseOver(m_x, m_y) ) {
        registerClick();
        gameType = GAMETYPE_ALLISLANDS;
		//setGameStateID(GAMESTATEID_CHOOSEPLAYER);
		setGameStateID(GAMESTATEID_CHOOSEDIFFICULTY);
	}
}

ChooseDifficultyPanel::ChooseDifficultyPanel() : MultiPanel(1, 0, 0) {
	this->c_page = 0;
	this->addToPanel(0, new Button(105, 50, "CHOOSE DIFFICULTY", letters_large));
    this->button_easy = new Button(120, 90, "EASY", letters_large);
	this->addToPanel(0, button_easy);
    this->button_medium = new Button(120, 130, "MEDIUM", letters_large);
	this->addToPanel(0, button_medium);
    this->button_hard = new Button(120, 170, "HARD", letters_large);
	this->addToPanel(0, button_hard);
}

void ChooseDifficultyPanel::input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	MultiPanel::input(m_x, m_y, m_left, m_middle, m_right, click);
	if( this->hasModal() ) {
		return;
	}
	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);

	if( m_left && click && this->button_easy->mouseOver(m_x, m_y) ) {
        registerClick();
        difficulty_level = DIFFICULTY_EASY;
		setGameStateID(GAMESTATEID_CHOOSEPLAYER);
	}
	else if( m_left && click && this->button_medium->mouseOver(m_x, m_y) ) {
        registerClick();
        difficulty_level = DIFFICULTY_MEDIUM;
		setGameStateID(GAMESTATEID_CHOOSEPLAYER);
	}
	else if( m_left && click && this->button_hard->mouseOver(m_x, m_y) ) {
        registerClick();
        difficulty_level = DIFFICULTY_HARD;
		setGameStateID(GAMESTATEID_CHOOSEPLAYER);
	}
}

ChooseMenPanel::ChooseMenPanel(PlaceMenGameState *gamestate) : MultiPanel(N_STATES, 0, 0), gamestate(gamestate) {
	this->c_page = (int)STATE_CHOOSEISLAND;
	this->n_men = 0;

	this->button_nextisland = NULL;
    //int mx = gamestate->getOffX();
    int mx = 200;
    const int start_y = 76;
    int cy = start_y;
    int step_y = 28;
	int fw = letters_large[0]->getScaledWidth();
	if( maps[start_epoch][1] != NULL ) {
        this->button_nextisland = new Button((int)(mx - 5.5*fw), cy, "NEXT ISLAND", letters_large);
        cy += step_y;
        this->addToPanel(STATE_CHOOSEISLAND, button_nextisland);
	}
    else {
        cy += step_y;
    }
	this->button_nextepoch = NULL;
	if( gameType == GAMETYPE_SINGLEISLAND ) {
        this->button_nextepoch = new Button((int)(mx - 4.0*fw), cy, "NEXT AGE", letters_large);
        cy += step_y;
        this->addToPanel(STATE_CHOOSEISLAND, button_nextepoch);
	}
    this->button_options = new Button((int)(mx - 3.5*fw), cy, "OPTIONS", letters_large);
    cy += step_y;
    this->addToPanel(STATE_CHOOSEISLAND, button_options);
    this->button_play = new Button((int)(mx - 5.5*fw), cy, "PLAY ISLAND", letters_large);
    cy += step_y;
    this->addToPanel(STATE_CHOOSEISLAND, button_play);
#if defined(USING_QT) || defined(_WIN32) || defined(__ANDROID__)
    this->button_help = new Button((int)(mx - 5.5*fw), cy, "ONLINE HELP", letters_large);
    cy += step_y;
    this->addToPanel(STATE_CHOOSEISLAND, button_help);
#endif
#if defined(Q_OS_ANDROID) || defined(__ANDROID__)
	// Applications don't quit on Android
	this->button_quit = NULL;
#else
    this->button_quit = new Button((int)(mx - 2.0*fw), cy, "QUIT", letters_large);
    cy += step_y;
    this->addToPanel(STATE_CHOOSEISLAND, button_quit);
#endif

    //this->button_nmen = new PanelPage(mx - 12, 154, 24, 12);
    this->button_nmen = new PanelPage(mx - 12, 154, 24, 32);
    this->addToPanel(STATE_CHOOSEMEN, button_nmen);

	this->button_cancel = new Button((int)(mx - 10.5*fw), 210, "CHOOSE ANOTHER ISLAND", letters_large);
    this->addToPanel(STATE_CHOOSEMEN, button_cancel);

    cy = start_y;
    step_y = 26;
    this->button_continue = new Button((int)(mx - 4.0*fw), cy, "CONTINUE", letters_large);
    cy += step_y;
    this->addToPanel(STATE_OPTIONS, button_continue);

    //char *music_texts[] = { "MUSIC ON", "MUSIC OFF" };
    const char *music_texts[] = { "SOUND ON", "SOUND OFF" };
    this->button_music = new CycleButton((int)(mx - 4.5*fw), cy, music_texts, 2, letters_large);
    cy += step_y;
    this->button_music->setActive( play_music ? 0 : 1 );
	this->addToPanel(STATE_OPTIONS, button_music);

#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR) || defined(Q_WS_MAEMO_5) || defined(Q_OS_ANDROID) || defined(__ANDROID__)
    this->button_onemousebutton = NULL;
#else
	const char *onemousebutton_texts[] = { "ONE MOUSE BUTTON UI", "TWO MOUSE BUTTON UI" };
    this->button_onemousebutton = new CycleButton((int)(mx - 9.5*fw), cy, onemousebutton_texts, 2, letters_large);
    cy += step_y;
    this->button_onemousebutton->setActive( onemousebutton ? 0 : 1 );
	this->addToPanel(STATE_OPTIONS, button_onemousebutton);
#endif

	this->button_new = NULL;
	this->button_load = NULL;
	this->button_save = NULL;
	if( gameType == GAMETYPE_ALLISLANDS ) {
        this->button_new = new Button((int)(mx - 4.0*fw), cy, "NEW GAME", letters_large);
        cy += step_y;
        this->addToPanel(STATE_OPTIONS, button_new);
        this->button_load = new Button((int)(mx - 2.0*fw), cy, "LOAD", letters_large);
        cy += step_y;
        this->addToPanel(STATE_OPTIONS, button_load);
        this->button_save = new Button((int)(mx - 2.0*fw), cy, "SAVE", letters_large);
        cy += step_y;
        this->addToPanel(STATE_OPTIONS, button_save);
	}

	this->button_load_cancel = new Button((int)(mx - 3.0*fw), 228, "CANCEL", letters_small);
	this->addToPanel(STATE_LOADGAME, button_load_cancel);

	this->button_save_cancel = new Button((int)(mx - 3.0*fw), 228, "CANCEL", letters_small);
	this->addToPanel(STATE_SAVEGAME, button_save_cancel);

	for(int i=0;i<n_slots_c;i++) {
		this->button_load_load[i] = NULL;
		this->button_save_save[i] = NULL;
	}
	if( gameType == GAMETYPE_ALLISLANDS ) {
		refreshLoadSaveButtons();
	}

	this->setInfoText();
}

void ChooseMenPanel::setInfoText() {
	if( onemousebutton ) {
		this->button_nmen->setInfoLMB("change the number of men\nto play this island");
	}
	else {
		this->button_nmen->setInfoLMB("decrease the number of men\nto play this island");
		this->button_nmen->setInfoRMB("increase the number of men\nto play this island");
	}
}

void ChooseMenPanel::refreshLoadSaveButtons() {
	for(int i=0;i<n_slots_c;i++) {
		if( this->button_load_load[i] != NULL )
			delete this->button_load_load[i];
		this->button_load_load[i] = NULL;
		DifficultyLevel difficulty = DIFFICULTY_EASY;
		int player = 0;
		int n_men = 0;
		int epoch = 0;
		int suspended[n_players_c];
		bool completed[max_islands_per_epoch_c];
		if( loadGameInfo( &difficulty, &player, &n_men, suspended, &epoch, completed, i ) ) {
			char buffer[256] = "";
			char diff_ch = '\0';
			if( difficulty == DIFFICULTY_EASY )
				diff_ch = 'E';
			else if( difficulty == DIFFICULTY_MEDIUM )
				diff_ch = 'M';
			else if( difficulty == DIFFICULTY_HARD )
				diff_ch = 'H';
			else {
				ASSERT(false);
			}
			sprintf(buffer, "%d %c %s Age %d  Men %d  Suspended %d", i, diff_ch, PlayerType::getName((PlayerType::PlayerTypeID)player), epoch+1, n_men, suspended[player]);
			//this->button_load_load[i] = new Button(96, 228 + (i-n_slots_c) * 8, buffer, letters_small);
            //int xpos = 16;
            //int ypos = 228 + (i-n_slots_c) * 8;
            /*int xshift = i/4;
            int yshift = i%4;
            int xpos = 16 + 128 * xshift;
            int ypos = 228 + (yshift-n_slots_c) * 8;*/
            int xpos = 16;
            int ypos = 8 + i*22;
            this->button_load_load[i] = new Button(xpos, ypos, buffer, letters_small);
			this->addToPanel(STATE_LOADGAME, button_load_load[i]);
		}
	}

	for(int i=0;i<n_slots_c;i++) {
		delete this->button_save_save[i];
		DifficultyLevel difficulty = DIFFICULTY_EASY;
		int player = 0;
		int n_men = 0;
		int epoch = 0;
		char buffer[256] = "";
		int suspended[n_players_c];
		bool completed[max_islands_per_epoch_c];
		if( loadGameInfo( &difficulty, &player, &n_men, suspended, &epoch, completed, i ) ) {
			char diff_ch = '\0';
			if( difficulty == DIFFICULTY_EASY )
				diff_ch = 'E';
			else if( difficulty == DIFFICULTY_MEDIUM )
				diff_ch = 'M';
			else if( difficulty == DIFFICULTY_HARD )
				diff_ch = 'H';
			else {
				ASSERT(false);
			}
			//sprintf(buffer, "%d %c %s Age %d Men %d Stored %d", i, diff_ch, PlayerType::getName((PlayerType::PlayerTypeID)player), epoch+1, n_men, suspended[player]);
			sprintf(buffer, "%d %c %s Age %d  Men %d  Saved %d", i, diff_ch, PlayerType::getName((PlayerType::PlayerTypeID)player), epoch+1, n_men, suspended[player]);
		}
		else {
			sprintf(buffer, "UNUSED Slot %d", i);
		}
		//this->button_save_save[i] = new Button(96, 228 + (i-n_slots_c) * 8, buffer, letters_small);
        //int xpos = 16;
        //int ypos = 228 + (i-n_slots_c) * 8;
        /*int xshift = i/4;
        int yshift = i%4;
        int xpos = 16 + 128 * xshift;
        int ypos = 228 + (yshift-n_slots_c) * 8;*/
        int xpos = 16;
        int ypos = 8 + i*22;
        this->button_save_save[i] = new Button(xpos, ypos, buffer, letters_small);
		this->addToPanel(STATE_SAVEGAME, button_save_save[i]);
	}
}

void ChooseMenPanel::draw() {
    if( ( this->getPage() == STATE_LOADGAME || this->getPage() == STATE_SAVEGAME ) && !this->gamestate->hasConfirmWindow() ) {
        // hide background
        // (if request quit, then we'll hide background after drawing the GUI)
#if SDL_MAJOR_VERSION == 1
        screen->fillRect(0, 0, default_width_c*scale_width, default_height_c*scale_height, 0, 0, 0);
#else
        screen->fillRectWithAlpha(0, 0, (short)(default_width_c*scale_width), (short)(default_height_c*scale_height), 0, 0, 0, 255-32);
#endif
    }

    MultiPanel::draw();

	if( this->getPage() == STATE_CHOOSEMEN ) {
		// draw this after the popups, so we can see it...
		men[start_epoch]->draw( this->button_nmen->getLeft() + 4, this->button_nmen->getTop() + 16);
		Image::writeNumbers( this->button_nmen->getXCentre(), this->button_nmen->getTop() + 2, numbers_white, this->n_men,Image::JUSTIFY_CENTRE);
		Image::writeNumbers( this->button_nmen->getXCentre(), this->button_nmen->getTop() + 36, numbers_white, getMenAvailable() - this->n_men,Image::JUSTIFY_CENTRE);
	}

    if( this->gamestate->hasConfirmWindow() ) {
        // hide background (do here, as also want to cover the GUI)
#if SDL_MAJOR_VERSION == 1
        screen->fillRect(0, 0, default_width_c*scale_width, default_height_c*scale_height, 0, 0, 0);
#else
        screen->fillRectWithAlpha(0, 0, (short)(default_width_c*scale_width), (short)(default_height_c*scale_height), 0, 0, 0, 255-32);
#endif
    }

}

void ChooseMenPanel::buttonNMenClick(void *data, int arg, bool m_left, bool m_middle, bool m_right) {
	ChooseMenPanel *chooseMenPanel = static_cast<ChooseMenPanel *>(data);

	int n = getNClicks();
	if( m_left && chooseMenPanel->n_men > 0 ) {
		chooseMenPanel->n_men -= n;
		if( chooseMenPanel->n_men < 0 )
			chooseMenPanel->n_men = 0;
	}
	else if( m_right ) {
		chooseMenPanel->n_men += n;
		if( chooseMenPanel->n_men > getMenAvailable() )
			chooseMenPanel->n_men = getMenAvailable();
	}
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			if( getMap()->isSectorAt(x, y) ) {
				/*ASSERT( ((PlaceMenGameState *)gamestate)->map_panels[x][y] != NULL );
				((PlaceMenGameState *)gamestate)->map_panels[x][y]->setEnabled( n_men > 0 );*/
				/*ASSERT( ((PlaceMenGameState *)gamestate)->getMapPanel(x, y) != NULL );
				((PlaceMenGameState *)gamestate)->getMapPanel(x, y)->setEnabled( n_men > 0 );*/
				ASSERT( chooseMenPanel->gamestate->getMapPanel(x, y) != NULL );
				chooseMenPanel->gamestate->getMapPanel(x, y)->setEnabled( chooseMenPanel->n_men > 0 );
			}
		}
	}
}

void ChooseMenPanel::input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	MultiPanel::input(m_x, m_y, m_left, m_middle, m_right, click);

	// update from options
	/*if( button_music != NULL ) {
		play_music = button_music->getActive() == 0;
#ifdef USING_QT
                // also save
                qt_settings->setValue(play_music_key_c, play_music ? 1 : 0);
#endif
	}
    //onemousebutton = this->onemousebuttonOn();
	if( button_onemousebutton != NULL ) {
		onemousebutton = button_onemousebutton->getActive() == 0;
	}*/

	if( this->hasModal() ) {
		return;
	}
	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);
    bool done = false;

    if( this->c_page == STATE_CHOOSEISLAND ) {
		if( m_left && click && this->button_nextisland != NULL && this->button_nextisland->mouseOver(m_x, m_y) ) {
            done = true;
            registerClick();
            gamestate->closeConfirmWindow();
			nextIsland();
			// this panel is now destroyed, need to exit immediately!
			return;
        }
		else if( m_left && click && this->button_nextepoch != NULL && this->button_nextepoch->mouseOver(m_x, m_y) ) {
            done = true;
            registerClick();
            gamestate->closeConfirmWindow();
			nextEpoch();
			// this panel is now destroyed, need to exit immediately!
			return;
        }
		else if( m_left && click && this->button_options->mouseOver(m_x, m_y) ) {
            done = true;
            registerClick();
            this->c_page = STATE_OPTIONS;
            gamestate->closeConfirmWindow();
        }
        else if( m_left && click && this->button_play->mouseOver(m_x, m_y) ) {
            done = true;
            registerClick();
            this->setInfoText();
            this->c_page = STATE_CHOOSEMEN;
            setupPlayers();
            gamestate->closeConfirmWindow();
        }
#if defined(USING_QT) || defined(_WIN32) || defined(__ANDROID__)
        else if( m_left && click && this->button_help->mouseOver(m_x, m_y) ) {
			LOG("clicked online help\n");
            done = true;
            registerClick();
            gamestate->closeConfirmWindow();
			char help_url[] = "http://homepage.ntlworld.com/mark.harman/comp_gigalomania.html";
#if defined(USING_QT)
            QDesktopServices::openUrl(QUrl(help_url));
#elif defined(_WIN32)
			ShellExecute(0, 0, help_url, 0, 0 , SW_SHOW );
#elif defined(__ANDROID__)
			launchUrl(help_url);
#endif
        }
#endif
        else if( m_left && click && this->button_quit != NULL && this->button_quit->mouseOver(m_x, m_y) ) {
            done = true;
            //quitGame();
            gamestate->requestQuit();
        }
    }
	else if( this->c_page == STATE_CHOOSEMEN ) {
		if( ( m_left || m_right ) && this->button_nmen->mouseOver(m_x, m_y) ) {
            done = true;
			processClick(buttonNMenClick, this->get(this->c_page), this, 0, button_nmen, m_left, m_middle, m_right, click);
		}
		else if( ( m_left || m_right ) && this->button_cancel->mouseOver(m_x, m_y) ) {
            done = true;
			gamestate->requestQuit();
		}
	}
	else if( this->c_page == STATE_OPTIONS ) {
		if( m_left && click && this->button_continue->mouseOver(m_x, m_y) ) {
            done = true;
			if( button_music != NULL ) {
				play_music = button_music->getActive() == 0;
#ifdef USING_QT
				// also save
				qt_settings->setValue(play_music_key_c, play_music ? 1 : 0);
#endif
			}
			if( button_onemousebutton != NULL ) {
				onemousebutton = button_onemousebutton->getActive() == 0;
			}
			savePrefs();

			registerClick();
            this->c_page = STATE_CHOOSEISLAND;
		}
		else if( m_left && click && this->button_new != NULL && this->button_new->mouseOver(m_x, m_y) ) {
            done = true;
            registerClick();
			//newGame();
			// this panel is now destroyed, need to exit immediately!
			//return;
			this->gamestate->requestNewGame();
		}
		else if( m_left && click && this->button_load != NULL && this->button_load->mouseOver(m_x, m_y) ) {
            done = true;
            registerClick();
            this->c_page = STATE_LOADGAME;
		}
		else if( m_left && click && this->button_save != NULL && this->button_save->mouseOver(m_x, m_y) ) {
            done = true;
            registerClick();
            //this->c_page = STATE_CHOOSEISLAND;
			this->c_page = STATE_SAVEGAME;
		}
	}
	else if( this->c_page == STATE_LOADGAME ) {
		if( m_left && click && this->button_load_cancel->mouseOver(m_x, m_y) ) {
            done = true;
            registerClick();
            this->c_page = STATE_CHOOSEISLAND;
		}
		else if( m_left && click ) {
            for(int i=0;i<n_slots_c && !done;i++) {
				if( this->button_load_load[i] != NULL && this->button_load_load[i]->mouseOver(m_x, m_y) ) {
                    done = true;
                    registerClick();
                    loadGame(i);
					return; // hack - need to return, as loading the game destroys this class! (the gamestate will be reset - which will also switch the c_page back to STATE_CHOOSEISLAND)
				}
			}
		}
		/*else if( m_left && click && this->button_load_load->mouseOver(m_x, m_y) ) {
		loadGame(0);
		this->c_page = STATE_CHOOSEISLAND;
		}*/
	}
	else if( this->c_page == STATE_SAVEGAME ) {
		if( m_left && click && this->button_save_cancel->mouseOver(m_x, m_y) ) {
            done = true;
            registerClick();
            this->c_page = STATE_CHOOSEISLAND;
		}
		else if( m_left && click ) {
            for(int i=0;i<n_slots_c && !done;i++) {
				if( this->button_save_save[i]->mouseOver(m_x, m_y) ) {
                    done = true;
                    registerClick();
                    saveGame(i);
					this->c_page = STATE_CHOOSEISLAND;
					this->refreshLoadSaveButtons();
				}
			}
		}
	}
}

/*bool ChooseMenPanel::onemousebuttonOn() const {
    if( button_onemousebutton == NULL )
        return onemousebutton;
    return ( button_onemousebutton->getActive() == 0 );
}*/

GamePanel::GamePanel(PlayingGameState *gamestate, int client_player) : MultiPanel(GamePanel::N_STATES, offset_panel_x_c, offset_panel_y_c), gamestate(gamestate), client_player(client_player) {
	//this->state = STATE_SECTORCONTROL;
	this->c_page = (int)STATE_SECTORCONTROL;
	this->mousestate = MOUSESTATE_NORMAL;

	this->deploy_shield = -1;
	this->deploy_defence = -1;
	this->deploy_weapon = -1;
	this->designinfo = NULL;

	//this->setup();
}

GamePanel::~GamePanel() {
}

void GamePanel::setup() {
	char buffer[256] = "";
	this->free(false);

	// SECTORCONTROL

	const int start_x = 8;
	const int step_x = 20;
	const int step_y = 30;
	int xpos = start_x;
	//const int ypos = 224 - offset_panel_y_c;
	int ypos = 18;
	xpos += step_x;
	//ypos += step_y;

	//
	//this->button_shield = new ImageButton(28, 18, panel_shield, "check on building\ndamage status\nand repair buildings");
	this->button_shield = new ImageButton(xpos, ypos, panel_shield, "check on building\ndamage status\nand repair buildings");
	this->addToPanel(STATE_SECTORCONTROL, button_shield);
	xpos += step_x;

	//
	this->button_defence = new ImageButton(xpos, ypos, panel_defence, "view building weapon\nstocks and deploy\nbuilding weapons");
	this->addToPanel(STATE_SECTORCONTROL, button_defence);
	xpos += step_x;

	//
	this->button_attack = new ImageButton(xpos, ypos, panel_attack, "view army weapon stocks\nand assemble army");
	this->addToPanel(STATE_SECTORCONTROL, button_attack);
	//xpos += step_x;
	xpos = start_x;
	ypos += step_y;

	//this->button_knowndesigns = new ImageButton(8, 18, panel_knowndesigns, "view blueprints\nof completed designs");
	this->button_knowndesigns = new ImageButton(xpos, ypos, panel_knowndesigns, "view blueprints\nof completed designs");
	this->addToPanel(STATE_SECTORCONTROL, button_knowndesigns);
	xpos += step_x;

	//
	//this->button_design = new ImageButton(8, 40, panel_design, "view and alter\ncurrent design");
	this->button_design = new ImageButton(xpos, ypos, panel_design, "view and alter\ncurrent design");
	this->addToPanel(STATE_SECTORCONTROL, button_design);
	this->button_ndesigners = new PanelPage(xpos, ypos+16, 16, 16);
	if( onemousebutton ) {
		this->button_ndesigners->setInfoLMB("change the number of designers");
	}
	else {
		this->button_ndesigners->setInfoLMB("decrease the number of designers");
		this->button_ndesigners->setInfoRMB("increase the number of designers");
	}
	this->addToPanel(STATE_SECTORCONTROL, button_ndesigners);
	xpos += step_x;

	//
	//this->button_elementstocks = new ImageButton(40, 70, mine_gatherable_small, "view element stocks");
	/*this->button_elementstocks = new ImageButton(xpos, ypos, mine_gatherable_small, "view element stocks");
	this->addToPanel(STATE_SECTORCONTROL, button_elementstocks);
	xpos += step_x;*/

	//
	//this->button_factory = new ImageButton(68, 40, panel_factory, "view and alter current\nitem being manufactured");
	this->button_factory = new ImageButton(xpos, ypos, panel_factory, "view and alter current\nitem being manufactured");
	this->addToPanel(STATE_SECTORCONTROL, button_factory);
	this->button_nworkers = new PanelPage(xpos, ypos+16, 16, 16);
	if( onemousebutton ) {
		this->button_nworkers->setInfoLMB("change the number of workers");
	}
	else {
		this->button_nworkers->setInfoLMB("decrease the number of workers");
		this->button_nworkers->setInfoRMB("increase the number of workers");
	}
	this->addToPanel(STATE_SECTORCONTROL, button_nworkers);
	xpos += step_x;

	xpos = start_x;
	ypos += step_y;
	//
	{
		for(int i=0;i<4;i++) {
			this->button_elements[i] = NULL;
			this->element_index[i] = -1;
		}
		int indx = 0;
		for(int i=0;i<N_ID;i++) {
			if( indx == 4 )
				break;
			int n_elements = 0, fraction = 0;
			gamestate->getCurrentSector()->getTotalElements(&n_elements, &fraction, i);
			if( n_elements > 0 || fraction > 0 ) {
				/*this->button_elements[indx] = new Button(8 + 24*indx, 90, icon_elements[i]);
				this->add(STATE_SECTORCONTROL, button_elements[indx]);*/
				this->element_index[indx] = i;
				/*this->button_nminers[indx] = new Button(8 + 24*indx, 98, 16, 24);
				this->add(STATE_SECTORCONTROL, button_nminers[indx]);*/
				indx++;
			}
		}
	}
	for(int i=0;i<4;i++) {
		if( this->element_index[i] == -1 ) {
			this->button_elements[i] = new PanelPage(0, 0, 0, 0);
			this->addToPanel(STATE_SECTORCONTROL, button_elements[i]);
			this->button_nminers[i] = new PanelPage(0, 0, 0, 0);
			this->addToPanel(STATE_SECTORCONTROL, button_nminers[i]);
			xpos += step_x;
			continue;
		}
		ASSERT_ELEMENT_ID( this->element_index[i] );
		//this->button_elements[i] = new ImageButton(8 + 24*i, 90, icon_elements[ this->element_index[i] ]);
		this->button_elements[i] = new ImageButton(xpos, ypos, icon_elements[ this->element_index[i] ], "view element stocks");
		//this->button_elements[i]->setInfoLMB("view element stocks");
		this->addToPanel(STATE_SECTORCONTROL, button_elements[i]);
		this->button_nminers[i] = new PanelPage(xpos, ypos+16, 16, 12);
		if( onemousebutton ) {
			this->button_nminers[i]->setInfoLMB("change the number of miners");
		}
		else {
			this->button_nminers[i]->setInfoLMB("decrease the number of miners");
			this->button_nminers[i]->setInfoRMB("increase the number of miners");
		}
		this->addToPanel(STATE_SECTORCONTROL, button_nminers[i]);
		xpos += step_x;
	}
	xpos = start_x;
	ypos += step_y;

	//
	for(int i=0;i<N_BUILDINGS;i++) {
		this->button_build[i] = NULL;
		this->button_nbuilders[i] = NULL;
	}

	//this->button_build[BUILDING_MINE] = new ImageButton(8, 118, panel_build[BUILDING_MINE]);
	this->button_build[BUILDING_MINE] = new ImageButton(xpos, ypos, panel_build[BUILDING_MINE]);
	this->button_build[BUILDING_MINE]->setInfoLMB("change the number of builders");
	this->addToPanel(STATE_SECTORCONTROL, button_build[BUILDING_MINE]);
	//this->button_nbuilders[BUILDING_MINE] = new PanelPage(8, 134, 16, 12);
	this->button_nbuilders[BUILDING_MINE] = new PanelPage(xpos, ypos+16, 16, 12);
	if( onemousebutton ) {
		this->button_nbuilders[BUILDING_MINE]->setInfoLMB("change the number of builders\nbuilding a mine");
	}
	else {
		this->button_nbuilders[BUILDING_MINE]->setInfoLMB("decrease the number of builders\nbuilding a mine");
		this->button_nbuilders[BUILDING_MINE]->setInfoRMB("increase the number of builders\nbuilding a mine");
	}
	this->addToPanel(STATE_SECTORCONTROL, button_nbuilders[BUILDING_MINE]);
	xpos += step_x;

	//this->button_build[BUILDING_FACTORY] = new ImageButton(28, 118, panel_build[BUILDING_FACTORY]);
	this->button_build[BUILDING_FACTORY] = new ImageButton(xpos, ypos, panel_build[BUILDING_FACTORY]);
	this->button_build[BUILDING_FACTORY]->setInfoLMB("change the number of builders");
	this->addToPanel(STATE_SECTORCONTROL, button_build[BUILDING_FACTORY]);
	//this->button_nbuilders[BUILDING_FACTORY] = new PanelPage(28, 134, 16, 12);
	this->button_nbuilders[BUILDING_FACTORY] = new PanelPage(xpos, ypos+16, 16, 12);
	if( onemousebutton ) {
		this->button_nbuilders[BUILDING_FACTORY]->setInfoLMB("change the number of builders\nbuilding a factory");
	}
	else {
		this->button_nbuilders[BUILDING_FACTORY]->setInfoLMB("decrease the number of builders\nbuilding a factory");
		this->button_nbuilders[BUILDING_FACTORY]->setInfoRMB("increase the number of builders\nbuilding a factory");
	}
	this->addToPanel(STATE_SECTORCONTROL, button_nbuilders[BUILDING_FACTORY]);
	xpos += step_x;

	//this->button_build[BUILDING_LAB] = new ImageButton(48, 118, panel_build[BUILDING_LAB]);
	this->button_build[BUILDING_LAB] = new ImageButton(xpos, ypos, panel_build[BUILDING_LAB]);
	this->button_build[BUILDING_LAB]->setInfoLMB("change the number of builders");
	this->addToPanel(STATE_SECTORCONTROL, button_build[BUILDING_LAB]);
	//this->button_nbuilders[BUILDING_LAB] = new PanelPage(48, 134, 16, 12);
	this->button_nbuilders[BUILDING_LAB] = new PanelPage(xpos, ypos+16, 16, 12);
	if( onemousebutton ) {
		this->button_nbuilders[BUILDING_LAB]->setInfoLMB("change the number of builders\nbuilding a lab");
	}
	else {
		this->button_nbuilders[BUILDING_LAB]->setInfoLMB("decrease the number of builders\nbuilding a lab");
		this->button_nbuilders[BUILDING_LAB]->setInfoRMB("increase the number of builders\nbuilding a lab");
	}
	this->addToPanel(STATE_SECTORCONTROL, button_nbuilders[BUILDING_LAB]);
	xpos += step_x;

	// SHIELD
	this->button_bigshield = new ImageButton(33, 0, 32, 16, panel_bigshield, "return to main screen");
	this->addToPanel(STATE_SHIELD, button_bigshield);
	for(int i=0;i<n_sub_epochs;i++) {
		this->button_deploy_shields[i] = new ImageButton(28 + 22*i, 24, icon_shields[i]);
		this->button_deploy_shields[i]->setInfoLMB("select a shield to use");
		this->addToPanel(STATE_SHIELD, button_deploy_shields[i]);
	}
	this->button_shutdown = new ImageButton(8, 40, men[n_epochs_c-1], "get ready to\nshutdown the sector");
	this->addToPanel(STATE_SHIELD, button_shutdown);

	// DEFENCE
	this->button_bigdefence = new ImageButton(33, 0, 32, 16, panel_bigdefence, "return to main screen");
	this->addToPanel(STATE_DEFENCE, button_bigdefence);
	for(int i=0;i<n_sub_epochs;i++) {
		this->button_deploy_defences[i] = new ImageButton(28 + 22*i, 24, numbered_defences[start_epoch + i]);
		sprintf(buffer, "deploy a %s", Invention::getInvention(Invention::DEFENCE, start_epoch + i)->getName());
		this->button_deploy_defences[i]->setInfoLMB(buffer);
		this->addToPanel(STATE_DEFENCE, button_deploy_defences[i]);
	}

	// ATTACK
	this->button_bigattack = new ImageButton(33, 0, 32, 16, panel_bigattack, "return to main screen");
	this->addToPanel(STATE_ATTACK, button_bigattack);
	this->button_deploy_unarmedmen = new ImageButton(offset_attack_x_c, 24, 16, 28, unarmed_man);
	this->button_deploy_unarmedmen->setInfoLMB("add an unarmed man to the army");
	this->addToPanel(STATE_ATTACK, button_deploy_unarmedmen);
	for(int i=0;i<n_sub_epochs;i++) {
		//this->button_deploy_attackers[i] = new ImageButton(offset_attack_x_c + space_attack_x_c*i, 56, 16, 28, numbered_weapons[start_epoch + i]);
		this->button_deploy_attackers[i] = new ImageButton(offset_attack_x_c + space_attack_x_c*i, 56, 16, 40, numbered_weapons[start_epoch + i]);
		sprintf(buffer, "add a %s to the army", Invention::getInvention(Invention::WEAPON, start_epoch + i)->getName());
		this->button_deploy_attackers[i]->setInfoLMB(buffer);
		this->addToPanel(STATE_ATTACK, button_deploy_attackers[i]);
	}
	this->button_return_attackers = new ImageButton(80, 96, 16, 28, panel_twoattack);
	this->button_return_attackers->setInfoLMB("return the assembled army");
	this->addToPanel(STATE_ATTACK, button_return_attackers);

	// DESIGN
	this->button_bigdesign = new ImageButton(33, 0, 32, 16, panel_bigdesign, "return to main screen");
	this->addToPanel(STATE_DESIGN, button_bigdesign);
	this->button_designers = new ImageButton(40, 16, 16, 40, men[gamestate->getCurrentSector()->getBuildingEpoch()]);
	if( onemousebutton ) {
		this->button_designers->setInfoLMB("change the number of designers");
	}
	else {
		this->button_designers->setInfoLMB("decrease the number of designers");
		this->button_designers->setInfoRMB("increase the number of designers");
	}
	this->addToPanel(STATE_DESIGN, button_designers);
	for(int i=0;i<n_sub_epochs;i++) {
        //int this_y = 60 + i*16;
        int this_y = 60 + i*20;
        this->button_shields[i] = new ImageButton(8, this_y, icon_shields[i]);
		this->button_shields[i]->setInfoLMB("design a shield");
		this->addToPanel(STATE_DESIGN, button_shields[i]);
        this->button_defences[i] = new ImageButton(40, this_y, icon_defences[start_epoch + i]);
		sprintf(buffer, "design a %s", Invention::getInvention(Invention::DEFENCE, start_epoch + i)->getName());
		this->button_defences[i]->setInfoLMB(buffer);
		this->addToPanel(STATE_DESIGN, button_defences[i]);
        this->button_weapons[i] = new ImageButton(72, this_y, icon_weapons[start_epoch + i]);
		sprintf(buffer, "design a %s", Invention::getInvention(Invention::WEAPON, start_epoch + i)->getName());
		this->button_weapons[i]->setInfoLMB(buffer);
		this->addToPanel(STATE_DESIGN, button_weapons[i]);
	}

	// ELEMENTSTOCKS
	this->button_bigelementstocks = new ImageButton(33, 0, 32, 16, mine_gatherable_large, "return to main screen");
	this->addToPanel(STATE_ELEMENTSTOCKS, button_bigelementstocks);
	for(int i=0;i<4;i++) {
		if( this->element_index[i] == -1 ) {
			this->button_elements2[i] = new PanelPage(0, 0, 0, 0);
			this->addToPanel(STATE_SECTORCONTROL, button_elements2[i]);
			this->button_nminers2[i] = new PanelPage(0, 0, 0, 0);
			this->addToPanel(STATE_SECTORCONTROL, button_nminers2[i]);
			continue;
		}
		ASSERT_ELEMENT_ID( this->element_index[i] );
        //int this_y = 20 + 28*i;
        int this_y = 20 + 30*i;
        this->button_elements2[i] = new ImageButton(64, this_y, 16, 24, icon_elements[ this->element_index[i] ]);
		this->addToPanel(STATE_ELEMENTSTOCKS, button_elements2[i]);
        this->button_nminers2[i] = new PanelPage(40, this_y, 16, 16);
		if( onemousebutton ) {
			this->button_nminers2[i]->setInfoLMB("change the number of miners");
		}
		else {
			this->button_nminers2[i]->setInfoLMB("decrease the number of miners");
			this->button_nminers2[i]->setInfoRMB("increase the number of miners");
		}
		this->addToPanel(STATE_ELEMENTSTOCKS, button_nminers2[i]);
	}

	// BUILD
	this->button_bigbuild = new ImageButton(33, 0, 32, 16, panel_bigbuild, "return to main screen");
	this->addToPanel(STATE_BUILD, button_bigbuild);
	for(int i=0;i<N_BUILDINGS;i++)
		this->button_nbuilders2[i] = NULL;
	//this->button_nbuilders2[BUILDING_MINE] = new ImageButton(40, 20, 19, 28, panel_build[BUILDING_MINE]);
	this->button_nbuilders2[BUILDING_MINE] = new ImageButton(40, 20, 19, build_step_y_c, panel_build[BUILDING_MINE]);
	if( onemousebutton ) {
		this->button_nbuilders2[BUILDING_MINE]->setInfoLMB("change the number of builders\nbuilding a mine");
	}
	else {
		this->button_nbuilders2[BUILDING_MINE]->setInfoLMB("decrease the number of builders\nbuilding a mine");
		this->button_nbuilders2[BUILDING_MINE]->setInfoRMB("increase the number of builders\nbuilding a mine");
	}
	this->addToPanel(STATE_BUILD, button_nbuilders2[BUILDING_MINE]);
    //this->button_nbuilders2[BUILDING_FACTORY] = new ImageButton(40, 20 + build_step_y_c, 19, 28, panel_build[BUILDING_FACTORY]);
    this->button_nbuilders2[BUILDING_FACTORY] = new ImageButton(40, 20 + build_step_y_c, 19, build_step_y_c, panel_build[BUILDING_FACTORY]);
    if( onemousebutton ) {
		this->button_nbuilders2[BUILDING_FACTORY]->setInfoLMB("change the number of builders\nbuilding a factory");
	}
	else {
		this->button_nbuilders2[BUILDING_FACTORY]->setInfoLMB("decrease the number of builders\nbuilding a factory");
		this->button_nbuilders2[BUILDING_FACTORY]->setInfoRMB("increase the number of builders\nbuilding a factory");
	}
	this->addToPanel(STATE_BUILD, button_nbuilders2[BUILDING_FACTORY]);
    //this->button_nbuilders2[BUILDING_LAB] = new ImageButton(40, 20 + 2*build_step_y_c, 19, 28, panel_build[BUILDING_LAB]);
    this->button_nbuilders2[BUILDING_LAB] = new ImageButton(40, 20 + 2*build_step_y_c, 19, build_step_y_c, panel_build[BUILDING_LAB]);
    if( onemousebutton ) {
		this->button_nbuilders2[BUILDING_LAB]->setInfoLMB("change the number of builders\nbuilding a lab");
	}
	else {
		this->button_nbuilders2[BUILDING_LAB]->setInfoLMB("decrease the number of builders\nbuilding a lab");
		this->button_nbuilders2[BUILDING_LAB]->setInfoRMB("increase the number of builders\nbuilding a lab");
	}
	this->addToPanel(STATE_BUILD, button_nbuilders2[BUILDING_LAB]);

	// KNOWNDESIGNS
	this->button_bigknowndesigns = new ImageButton(33, 0, 32, 16, panel_bigknowndesigns, "return to main screen");
	this->addToPanel(STATE_KNOWNDESIGNS, button_bigknowndesigns);
	for(int i=0;i<n_sub_epochs;i++) {
        //int this_y = 40 + i*16;
        int this_y = 40 + i*20;
        this->button_knownshields[i] = new ImageButton(8, this_y, icon_shields[i]);
		this->button_knownshields[i]->setInfoLMB("view blueprint for a shield");
		this->addToPanel(STATE_KNOWNDESIGNS, button_knownshields[i]);
        this->button_knowndefences[i] = new ImageButton(40, this_y, icon_defences[start_epoch + i]);
		sprintf(buffer, "view blueprint for a %s", Invention::getInvention(Invention::DEFENCE, start_epoch + i)->getName());
		this->button_knowndefences[i]->setInfoLMB(buffer);
		this->addToPanel(STATE_KNOWNDESIGNS, button_knowndefences[i]);
        this->button_knownweapons[i] = new ImageButton(72, this_y, icon_weapons[start_epoch + i]);
		sprintf(buffer, "view blueprint for a %s", Invention::getInvention(Invention::WEAPON, start_epoch + i)->getName());
		this->button_knownweapons[i]->setInfoLMB(buffer);
		this->addToPanel(STATE_KNOWNDESIGNS, button_knownweapons[i]);
	}

	// DESIGNINFO
	this->button_bigdesigninfo = new ImageButton(33, 0, panel_bigknowndesigns, "view blueprints\nof completed designs");
	this->addToPanel(STATE_DESIGNINFO, button_bigdesigninfo);
	this->button_trashdesign = new ImageButton(64, 100, icon_trash);
	/*if( onemousebutton ) {
		button_trashdesign->setInfoLMB("trash this design");
	}
	else {
		button_trashdesign->setInfoBMB("trash this design");
	}*/
	button_trashdesign->setInfoLMB("trash this design");
	this->addToPanel(STATE_DESIGNINFO, button_trashdesign);

	// FACTORY
	this->button_bigfactory = new ImageButton(33, 0, 32, 16, panel_bigfactory, "return to main screen");
	this->addToPanel(STATE_FACTORY, button_bigfactory);
	this->button_workers = new ImageButton(40, 14, 16, 26, men[gamestate->getCurrentSector()->getBuildingEpoch()]);
	if( onemousebutton ) {
		this->button_workers->setInfoLMB("change the number of workers");
	}
	else {
		this->button_workers->setInfoLMB("decrease the number of workers");
		this->button_workers->setInfoRMB("increase the number of workers");
	}
	this->addToPanel(STATE_FACTORY, button_workers);
	this->button_famount = new ImageButton(40, 40, 14, 28, men[gamestate->getCurrentSector()->getBuildingEpoch()]);
	if( onemousebutton ) {
		this->button_famount->setInfoLMB("change the number to produce");
	}
	else {
		this->button_famount->setInfoLMB("decrease the number to produce");
		this->button_famount->setInfoRMB("increase the number to produce");
	}
	this->addToPanel(STATE_FACTORY, button_famount);
	for(int i=0;i<n_sub_epochs;i++) {
        const int this_y = 82 + i*14;
        this->button_fshields[i] = new ImageButton(8, this_y, 16, 14, icon_shields[i]);
		this->button_fshields[i]->setInfoLMB("manufacture a shield");
		this->addToPanel(STATE_FACTORY, button_fshields[i]);
        this->button_fdefences[i] = new ImageButton(40, this_y, 16, 14, icon_defences[start_epoch + i]);
		sprintf(buffer, "manufacture a %s", Invention::getInvention(Invention::DEFENCE, start_epoch + i)->getName());
		this->button_fdefences[i]->setInfoLMB(buffer);
		this->addToPanel(STATE_FACTORY, button_fdefences[i]);
        this->button_fweapons[i] = new ImageButton(72, this_y, 16, 14, icon_weapons[start_epoch + i]);
		sprintf(buffer, "manufacture a %s", Invention::getInvention(Invention::WEAPON, start_epoch + i)->getName());
		this->button_fweapons[i]->setInfoLMB(buffer);
		this->addToPanel(STATE_FACTORY, button_fweapons[i]);
	}

	//printf(">>> %d\n", this->pages[STATE_SECTORCONTROL]->nChildren());
	this->refresh();
	//printf(">>> %d\n", this->pages[STATE_SECTORCONTROL]->nChildren());
}

void GamePanel::refreshCanDesign() {
	if( !gamestate->viewingActiveClientSector() ) {
		return;
	}
	if( start_epoch == end_epoch_c ) {
		this->button_design->setEnabled(false);
		return;
	}
	bool any_design = false;
	if( gamestate->getCurrentSector()->getCurrentDesign() != NULL ) {
		any_design = true; // even though no new designs available anymore, we can keep designing the selected design
	}
	for(int i=0;i<n_epochs_c && !any_design;i++) {
		if( gamestate->getCurrentSector()->canResearch(Invention::SHIELD, i) )
			any_design = true;
		else if( gamestate->getCurrentSector()->canResearch(Invention::DEFENCE, i) )
			any_design = true;
		else if( gamestate->getCurrentSector()->canResearch(Invention::WEAPON, i) )
			any_design = true;
	}
	this->button_design->setEnabled(any_design);
	if( gamestate->getCurrentSector()->getBuilding(BUILDING_LAB) != NULL ) {
		this->button_design->setImage(panel_lab);
		this->button_bigdesign->setImage(panel_biglab);
	}
	else {
		//this->button_design->image = any_design ? panel_design : panel_design_dark;
		this->button_design->setImage(panel_design);
		this->button_bigdesign->setImage(panel_bigdesign);
	}
	this->button_ndesigners->setEnabled( gamestate->getCurrentSector()->getCurrentDesign() != NULL );
}

void GamePanel::refreshDesignInventions() {
	if( !gamestate->viewingActiveClientSector() ) {
		return;
	}
	for(int i=0;i<n_sub_epochs;i++) {
		this->button_shields[i]->setEnabled(true);
		this->button_defences[i]->setEnabled(true);
		this->button_weapons[i]->setEnabled(true);

		if( !gamestate->getCurrentSector()->canResearch(Invention::SHIELD, start_epoch + i) )
			this->button_shields[i]->setEnabled(false);
		if( !gamestate->getCurrentSector()->canResearch(Invention::DEFENCE, start_epoch + i) )
			this->button_defences[i]->setEnabled(false);
		if( !gamestate->getCurrentSector()->canResearch(Invention::WEAPON, start_epoch + i) )
			this->button_weapons[i]->setEnabled(false);
	}
}

void GamePanel::refreshManufactureInventions() {
	if( !gamestate->viewingActiveClientSector() ) {
		return;
	}
	for(int i=0;i<n_sub_epochs;i++) {
		this->button_fshields[i]->setEnabled(true);
		this->button_fdefences[i]->setEnabled(true);
		this->button_fweapons[i]->setEnabled(true);

		if( start_epoch + i < factory_epoch_c || !gamestate->getCurrentSector()->canBuildDesign(Invention::SHIELD, start_epoch + i) )
			this->button_fshields[i]->setEnabled(false);
		if( start_epoch + i < factory_epoch_c || !gamestate->getCurrentSector()->canBuildDesign(Invention::DEFENCE, start_epoch + i) )
			this->button_fdefences[i]->setEnabled(false);
		if( start_epoch + i < factory_epoch_c || !gamestate->getCurrentSector()->canBuildDesign(Invention::WEAPON, start_epoch + i) )
			this->button_fweapons[i]->setEnabled(false);
	}
}

void GamePanel::refreshDeployInventions() {
	if( !gamestate->viewingActiveClientSector() ) {
		return;
	}
	for(int i=0;i<n_sub_epochs;i++) {
		this->button_deploy_shields[i]->setEnabled(false);
		if( gamestate->getCurrentSector()->inventionKnown(Invention::SHIELD, start_epoch + i) )
			this->button_deploy_shields[i]->setEnabled(true);
	}

	for(int i=0;i<n_sub_epochs;i++) {
		this->button_deploy_defences[i]->setEnabled(false);
		if( gamestate->getCurrentSector()->inventionKnown(Invention::DEFENCE, start_epoch + i) )
			this->button_deploy_defences[i]->setEnabled(true);
	}

	this->button_deploy_unarmedmen->setEnabled(true);
	for(int i=0;i<n_sub_epochs;i++) {
		this->button_deploy_attackers[i]->setEnabled(false);
		if( gamestate->getCurrentSector()->inventionKnown(Invention::WEAPON, start_epoch + i) ||
			gamestate->getCurrentSector()->getStoredArmy()->getSoldiers(start_epoch + i) > 0 )
			this->button_deploy_attackers[i]->setEnabled(true);
	}
}

void GamePanel::refreshShutdown() {
	if( !gamestate->viewingActiveClientSector() ) {
		return;
	}
	this->button_shutdown->setEnabled( gamestate->getCurrentSector()->canShutdown() );
}

void GamePanel::refresh() {
	//LOG("GamePanel::refresh()\n");
	// set all enabled to false
	/*for(i=0;i<nChildren();i++) {
	for(int j=0;j<get(i)->nChildren();j++) {
	PanelPage *panel = get(i)->get(j);
	panel->setEnabled(false);
	}
	}*/
	if( !gamestate->viewingActiveClientSector() ) {
		this->setEnabled(false);
		return;
	}

	this->setEnabled(true);

	this->refreshCanDesign();

	bool any_defences = false;
	for(int i=0;i<n_epochs_c && !any_defences;i++)
		any_defences = ( gamestate->getCurrentSector()->getStoredDefenders(i) > 0 || gamestate->getCurrentSector()->inventionKnown(Invention::DEFENCE, i) );
	bool any_designs = ( gamestate->getCurrentSector()->getNDesigns() > 0 );

	this->button_shield->setEnabled(true);
	this->button_defence->setEnabled(any_defences);
	this->button_attack->setEnabled(true);
	//this->button_elementstocks->setEnabled(start_epoch != end_epoch_c);
	for(int i=0;i<4;i++) {
		this->button_elements[i]->setEnabled(false);
		this->button_nminers[i]->setEnabled(false);
		this->button_elements2[i]->setEnabled(false);
		this->button_nminers2[i]->setEnabled(false);

		if( this->element_index[i] != -1 ) {
			ASSERT_ELEMENT_ID( this->element_index[i] );
			bool can_mine = gamestate->getCurrentSector()->canMine( (Id)this->element_index[i] );
			int n_elements = 0, fraction = 0;
			gamestate->getCurrentSector()->getElementStocks(&n_elements, &fraction, (Id)this->element_index[i]);
			if( n_elements > 0 || fraction > 0 || can_mine ) {
				this->button_elements[i]->setEnabled(true);
				this->button_elements2[i]->setEnabled(true);
			}
			if( can_mine && elements[ this->element_index[i] ]->getType() != Element::GATHERABLE ) {
				this->button_nminers[i]->setEnabled(true);
				this->button_nminers2[i]->setEnabled(true);
			}
		}
	}
	/*if( onemousebutton ) {
		// if one mouse button, only allow changing via the sub-menu (otherwise problems with left/right icons going off screen)
		this->button_nbuilders[BUILDING_MINE]->setEnabled(false);
		this->button_nbuilders[BUILDING_FACTORY]->setEnabled(false);
		this->button_nbuilders[BUILDING_LAB]->setEnabled(false);
	}*/
	if( !gamestate->getCurrentSector()->canBuild( BUILDING_MINE ) ) {
		this->button_build[BUILDING_MINE]->setEnabled(false);
		this->button_nbuilders[BUILDING_MINE]->setEnabled(false);
		this->button_nbuilders2[BUILDING_MINE]->setEnabled(false);
	}
	if( !gamestate->getCurrentSector()->canBuild( BUILDING_FACTORY ) ) {
		this->button_build[BUILDING_FACTORY]->setEnabled(false);
		this->button_nbuilders[BUILDING_FACTORY]->setEnabled(false);
		this->button_nbuilders2[BUILDING_FACTORY]->setEnabled(false);
	}
	if( !gamestate->getCurrentSector()->canBuild( BUILDING_LAB ) ) {
		this->button_build[BUILDING_LAB]->setEnabled(false);
		this->button_nbuilders[BUILDING_LAB]->setEnabled(false);
		this->button_nbuilders2[BUILDING_LAB]->setEnabled(false);
	}
	this->button_bigbuild->setEnabled(true);
	this->button_knowndesigns->setEnabled(any_designs);
	if( gamestate->getCurrentSector()->getBuilding(BUILDING_FACTORY) == NULL ) {
		this->button_factory->setEnabled(false);
	}
	this->button_nworkers->setEnabled( gamestate->getCurrentSector()->getCurrentManufacture() != NULL );

	this->button_bigdesign->setEnabled(true);
	this->button_designers->setEnabled( gamestate->getCurrentSector()->getCurrentDesign() != NULL );
	this->button_designers->enableHelpText( gamestate->getCurrentSector()->getCurrentDesign() != NULL );
	this->refreshDesignInventions();

	this->button_bigshield->setEnabled(true);
	this->refreshShutdown();

	this->button_bigdefence->setEnabled(true);

	this->button_bigattack->setEnabled(true);
	this->button_return_attackers->setEnabled(true);

	this->refreshDeployInventions();

	this->button_bigelementstocks->setEnabled(true);

	this->button_bigknowndesigns->setEnabled(true);
	for(int i=0;i<n_sub_epochs;i++) {
		if( !gamestate->getCurrentSector()->knownDesign(Invention::SHIELD, start_epoch + i) )
			this->button_knownshields[i]->setEnabled(false);
		if( !gamestate->getCurrentSector()->knownDesign(Invention::DEFENCE, start_epoch + i) )
			this->button_knowndefences[i]->setEnabled(false);
		if( !gamestate->getCurrentSector()->knownDesign(Invention::WEAPON, start_epoch + i) )
			this->button_knownweapons[i]->setEnabled(false);
	}

	this->button_bigdesigninfo->setEnabled(true);
	this->button_trashdesign->setEnabled(true);

	this->button_bigfactory->setEnabled(true);
	this->button_workers->setEnabled( gamestate->getCurrentSector()->getCurrentManufacture() != NULL );
	this->button_workers->enableHelpText( gamestate->getCurrentSector()->getCurrentManufacture() != NULL );
	this->button_famount->setEnabled( gamestate->getCurrentSector()->getCurrentManufacture() != NULL );
	this->button_famount->enableHelpText( gamestate->getCurrentSector()->getCurrentManufacture() != NULL );
	this->refreshManufactureInventions();

	//((PlayingGameState *)gamestate)->refreshButtons();
	gamestate->refreshButtons();
}

void GamePanel::setPage(int page) {
	this->c_page = page;
	this->setMouseState(MOUSESTATE_NORMAL);
}

GamePanel::MouseState GamePanel::getMouseState() const {
	return this->mousestate;
}

void GamePanel::setMouseState(MouseState mousestate) {
	this->mousestate = mousestate;
	//((PlayingGameState *)gamestate)->refreshButtons();
	gamestate->refreshButtons();
}

void GamePanel::draw() {
	if( this->c_page == STATE_SECTORCONTROL ) {
		//Image::write(offset_panel_x_c + 8, offset_panel_y_c + 6, letters_small, map->getName(), Image::JUSTIFY_LEFT, true, scale_width, scale_height);
		Image::writeMixedCase(offset_panel_x_c + 8, offset_panel_y_c - 1, letters_large, letters_small, NULL, getMap()->getName(), Image::JUSTIFY_LEFT);
		if( start_epoch != end_epoch_c ) {
			int year = epoch_dates[gamestate->getCurrentSector()->getEpoch()];
			bool shiny = gamestate->getCurrentSector()->getEpoch() == n_epochs_c-1;
			Image::writeNumbers(offset_panel_x_c + 96, offset_panel_y_c, shiny ? numbers_largeshiny : numbers_largegrey, abs(year),Image::JUSTIFY_RIGHT);
			Image *era = ( year < 0 ) ? icon_bc :
				shiny ? icon_ad_shiny : icon_ad;
			era->draw(offset_panel_x_c + 88, offset_panel_y_c + 7);
		}

		if( gamestate->viewingActiveClientSector() ) {
			const int xpos = 8, ypos = 18;
			//const int xpos = 40, ypos = 40;
			men[gamestate->getCurrentSector()->getBuildingEpoch()]->draw(offset_panel_x_c + xpos, offset_panel_y_c + ypos);
			Image::writeNumbers(offset_panel_x_c + xpos + 8, offset_panel_y_c + ypos + 18, numbers_grey, gamestate->getCurrentSector()->getSparePopulation(),Image::JUSTIFY_CENTRE);
			if( gamestate->getCurrentSector()->getCurrentDesign() != NULL ) {
				Image::writeNumbers( this->button_ndesigners->getXCentre(), this->button_ndesigners->getTop() + 2, numbers_white, gamestate->getCurrentSector()->getDesigners(),Image::JUSTIFY_CENTRE);
			}
			for(int i=0;i<4;i++) {
				Id element = (Id)this->element_index[i];
				if( this->button_nminers[i]->isEnabled() ) {
					ASSERT_ELEMENT_ID( this->element_index[i] );
					int n_miners = gamestate->getCurrentSector()->getMiners( element );
					Image::writeNumbers( this->button_nminers[i]->getXCentre(), this->button_nminers[i]->getTop() + 2, numbers_white, n_miners,Image::JUSTIFY_CENTRE);
				}
			}
			if( gamestate->getCurrentSector()->getCurrentManufacture() != NULL ) {
				Image::writeNumbers( this->button_nworkers->getXCentre(), this->button_nworkers->getTop() + 2, numbers_white, gamestate->getCurrentSector()->getWorkers(),Image::JUSTIFY_CENTRE);
			}
			for(int i=0;i<N_BUILDINGS;i++) {
				if( this->button_nbuilders[i] != NULL && this->button_nbuilders[i]->isEnabled() ) {
					Image::writeNumbers( this->button_nbuilders[i]->getXCentre(), this->button_nbuilders[i]->getBottom() - 8, numbers_white, gamestate->getCurrentSector()->getBuilders((Type)i),Image::JUSTIFY_CENTRE);
				}
			}
		}
		else if( gamestate->viewingAnyClientSector() ) {
			ASSERT( gamestate->getCurrentSector()->isShutdown() );
			men[gamestate->getCurrentSector()->getBuildingEpoch()]->draw(offset_panel_x_c + 40, offset_panel_y_c + 40);
			Image::writeNumbers(offset_panel_x_c + 48, offset_panel_y_c + 58, numbers_grey, gamestate->getCurrentSector()->getPopulation(),Image::JUSTIFY_CENTRE);
		}
		else if( gamestate->getCurrentSector()->getPlayer() == -1 ) {
			int n_players_in_sector = 0;
			int player_in_sector = -1;
			for(int i=0;i<n_players_c;i++) {
				if( gamestate->getCurrentSector()->getArmy(i)->getTotal() > 0 ) {
					player_in_sector = i;
					n_players_in_sector++;
				}
			}
			if( n_players_in_sector == 1 && player_in_sector == client_player ) {
				int halfdays = 0, hours = 0;
				gamestate->getCurrentSector()->buildingTowerTimeLeft(player_in_sector, &halfdays, &hours);
				int clock_index = hours + 1;
				icon_clocks[ clock_index ]->draw(offset_panel_x_c + 72, offset_panel_y_c + 16);
				Image::writeNumbers(offset_panel_x_c + 80, offset_panel_y_c + 34, numbers_white, halfdays,Image::JUSTIFY_CENTRE);
			}
		}
	}
	else if( this->c_page == STATE_DESIGN ) {
		this->button_designers->setImage( men[gamestate->getCurrentSector()->getBuildingEpoch()] );
		if( gamestate->getCurrentSector()->getCurrentDesign() == NULL ) {
			dash_grey->draw(offset_panel_x_c + 45, offset_panel_y_c + 36);
		}
		else {
			Invention *current_invention = gamestate->getCurrentSector()->getCurrentDesign()->getInvention();
			current_invention->getImage()->draw(offset_panel_x_c + 8, offset_panel_y_c + 16);
			Image::writeNumbers(offset_panel_x_c + 48, offset_panel_y_c + 34, numbers_white, gamestate->getCurrentSector()->getDesigners(),Image::JUSTIFY_CENTRE);
			if( gamestate->getCurrentSector()->getDesigners() > 0 ) {
				int halfdays = 0, hours = 0;
				gamestate->getCurrentSector()->inventionTimeLeft(&halfdays, &hours);
				int clock_index = hours + 1;
				icon_clocks[ clock_index ]->draw(offset_panel_x_c + 72, offset_panel_y_c + 16);
				Image::writeNumbers(offset_panel_x_c + 80, offset_panel_y_c + 34, numbers_white, halfdays,Image::JUSTIFY_CENTRE);
			}
			else {
				icon_clocks[ 0 ]->draw(offset_panel_x_c + 72, offset_panel_y_c + 16);
				icon_infinity->draw(offset_panel_x_c + 74, offset_panel_y_c + 32);
			}
		}
		icon_shield->draw(offset_panel_x_c + 8, offset_panel_y_c + 44);
		icon_defence->draw(offset_panel_x_c + 40, offset_panel_y_c + 44);
		icon_weapon->draw(offset_panel_x_c + 72, offset_panel_y_c + 44);
	}
	else if( this->c_page == STATE_SHIELD ) {
		for(int i=0;i<N_BUILDINGS;i++) {
			if( gamestate->getCurrentSector()->getBuilding((Type)i) != NULL ) {
				int width = building_health->getScaledWidth();
				int health = gamestate->getCurrentSector()->getBuilding((Type)i)->getHealth();
				int max_health = gamestate->getCurrentSector()->getBuilding((Type)i)->getMaxHealth();
				//health = 50;
				//max_health = 60;
				int offx = offset_panel_x_c + 24;
				/*rect.x = (short)(scale_width * ( offset_panel_x_c + 24 ));
				rect.y = (short)(scale_height * ( offset_panel_y_c + 64 + 16 * i ));
				rect.w = (short)(( scale_width * health ) * 0.6);
				rect.h = (short)(scale_height * 4);*/
				short x = (short)(scale_width * offx + ( scale_width * health * width ) / (float)max_health);
				short y = (short)(scale_height * ( offset_panel_y_c + 64 + 16 * i ));
				short w = (short)ceil(( scale_width * (max_health - health) * width ) / (float)max_health);
				short h = (short)(scale_height * 5);
				building_health->draw(offx, (int)(y/scale_height));
				screen->fillRect(x, y, w, h, 0, 0, 0);
				panel_building[i]->draw(offset_panel_x_c + 8, offset_panel_y_c + 64 - 5 + 16 * i);
			}
		}

		for(int i=0;i<n_sub_epochs;i++) {
			if( this->button_deploy_shields[i]->isEnabled() ) {
				int n_store = gamestate->getCurrentSector()->getStoredShields(i);
				if( gamestate->getCurrentSector()->canBuildDesign(Invention::SHIELD, start_epoch+i) || n_store > 0 ) {
					if( n_store == 0 && start_epoch + i < factory_epoch_c )
						Image::write(offset_panel_x_c + 36 + 22*i, offset_panel_y_c + 42, letters_small, "OK", Image::JUSTIFY_CENTRE);
					else
						Image::writeNumbers(offset_panel_x_c + 36 + 22*i, offset_panel_y_c + 42, numbers_yellow, n_store, Image::JUSTIFY_CENTRE);
				}
				else {
					dash_grey->draw(offset_panel_x_c + 32 + 22*i, offset_panel_y_c + 46);
				}
			}
		}
	}
	else if( this->c_page == STATE_DEFENCE ) {
		//unarmed_man->draw(offset_panel_x_c + 8, offset_panel_y_c + 24, true);
		men[gamestate->getCurrentSector()->getBuildingEpoch()]->draw(offset_panel_x_c + 8, offset_panel_y_c + 24);
		Image::writeNumbers(offset_panel_x_c + 16, offset_panel_y_c + 42, numbers_grey, gamestate->getCurrentSector()->getSparePopulation(), Image::JUSTIFY_CENTRE);
		for(int i=0;i<n_sub_epochs;i++) {
			if( this->button_deploy_defences[i]->isEnabled() ) {
				int n_store = gamestate->getCurrentSector()->getStoredDefenders(start_epoch+i);
				if( gamestate->getCurrentSector()->canBuildDesign(Invention::DEFENCE, start_epoch+i) || n_store > 0 ) {
					if( n_store == 0 && start_epoch + i < factory_epoch_c )
						Image::write(offset_panel_x_c + 36 + 22*i, offset_panel_y_c + 42, letters_small, "OK", Image::JUSTIFY_CENTRE);
					else
						Image::writeNumbers(offset_panel_x_c + 36 + 22*i, offset_panel_y_c + 42, numbers_yellow, n_store, Image::JUSTIFY_CENTRE);
				}
				else {
					dash_grey->draw(offset_panel_x_c + 32 + 22*i, offset_panel_y_c + 46);
				}
			}
		}
	}
	else if( this->c_page == STATE_ATTACK ) {
		Image::writeNumbers(offset_panel_x_c + offset_attack_x_c + 8, offset_panel_y_c + 42, numbers_yellow, gamestate->getCurrentSector()->getSparePopulation(), Image::JUSTIFY_CENTRE);
		for(int i=0;i<n_sub_epochs;i++) {
			if( this->button_deploy_attackers[i]->isEnabled() ) {
				int n_store = gamestate->getCurrentSector()->getStoredArmy()->getSoldiers(start_epoch+i);
				if( gamestate->getCurrentSector()->canBuildDesign(Invention::WEAPON, start_epoch+i) || n_store > 0 ) {
					if( n_store == 0 && start_epoch + i < factory_epoch_c )
						Image::write(offset_panel_x_c + offset_attack_x_c + 8 + space_attack_x_c*i, offset_panel_y_c + 74, letters_small, "OK", Image::JUSTIFY_CENTRE);
					else
						Image::writeNumbers(offset_panel_x_c + offset_attack_x_c + 8 + space_attack_x_c*i, offset_panel_y_c + 74, numbers_yellow, n_store, Image::JUSTIFY_CENTRE);
				}
				else {
					dash_grey->draw(offset_panel_x_c + offset_attack_x_c + 4 + space_attack_x_c*i, offset_panel_y_c + 78);
				}
			}
		}

		int n_army = gamestate->getCurrentSector()->getAssembledArmy()->getTotal();
		if( n_army == 0 )
			dash_grey->draw(offset_panel_x_c + 84, offset_panel_y_c + 116);
		else
			Image::writeNumbers(offset_panel_x_c + 88, offset_panel_y_c + 116, numbers_orange, n_army, Image::JUSTIFY_CENTRE);
	}
	else if( this->c_page == STATE_ELEMENTSTOCKS ) {
		for(int i=0;i<4;i++) {
			Id element = (Id)this->element_index[i];
			if( element == -1 )
				continue;
			ASSERT_ELEMENT_ID( this->element_index[i] );
			int n_elements = 0, fraction = 0;
			gamestate->getCurrentSector()->getElementStocks(&n_elements, &fraction, element);
			int off = 0;
			//int ypos = offset_panel_y_c + 38 + 28 * i;
			int ypos = offset_panel_y_c + 40 + 28 * i;
			if( n_elements > 0 ) {
				Image::writeNumbers(offset_panel_x_c + 72, ypos, numbers_blue, n_elements, Image::JUSTIFY_LEFT);
				off += 8 * n_digits(n_elements);
			}
			if( fraction == 1 ) {
				numbers_half->draw(offset_panel_x_c + 72 + off, ypos);
			}
			if( this->button_nminers2[i]->isEnabled() ) {
				int n_miners = gamestate->getCurrentSector()->getMiners( element );
				Image::writeNumbers( this->button_nminers2[i]->getXCentre(), this->button_nminers2[i]->getTop() + 2, numbers_white, n_miners,Image::JUSTIFY_CENTRE);
			}
		}
	}
	else if( this->c_page == STATE_BUILD ) {
		for(int i=0;i<N_BUILDINGS;i++)
		{
			if( this->button_nbuilders2[i] != NULL && this->button_nbuilders2[i]->isEnabled() ) {
				Image::writeNumbers( this->button_nbuilders2[i]->getXCentre(), this->button_nbuilders2[i]->getTop() + 20, numbers_white, gamestate->getCurrentSector()->getBuilders((Type)i),Image::JUSTIFY_CENTRE);
				if( gamestate->getCurrentSector()->getBuilders((Type)i) > 0 ) {
					int halfdays = 0, hours = 0;
					gamestate->getCurrentSector()->buildingTimeLeft((Type)i, &halfdays, &hours);
					int clock_index = hours + 1;
					icon_clocks[ clock_index ]->draw(offset_panel_x_c + 72, offset_panel_y_c + 16 + build_step_y_c*(i-1));
					Image::writeNumbers(offset_panel_x_c + 80, offset_panel_y_c + 34 + build_step_y_c*(i-1), numbers_white, halfdays,Image::JUSTIFY_CENTRE);
				}
				else {
					icon_clocks[ 0 ]->draw(offset_panel_x_c + 72, offset_panel_y_c + 16 + build_step_y_c*(i-1));
					icon_infinity->draw(offset_panel_x_c + 74, offset_panel_y_c + 32 + build_step_y_c*(i-1));
				}
			}
		}
	}
	else if( this->c_page == STATE_KNOWNDESIGNS ) {
		icon_shield->draw(offset_panel_x_c + 8, offset_panel_y_c + 24);
		icon_defence->draw(offset_panel_x_c + 40, offset_panel_y_c + 24);
		icon_weapon->draw(offset_panel_x_c + 72, offset_panel_y_c + 24);
	}
	else if( this->c_page == STATE_DESIGNINFO ) {
		ASSERT(this->designinfo != NULL);
		Design *design = gamestate->getCurrentSector()->knownDesign( this->designinfo->getType(), this->designinfo->getEpoch() );
		ASSERT(design != NULL);
		int cnt = 0;
		for(int i=0;i<N_ID;i++) {
			int cost = design->getCost((Id)i);
			if( cost > 0 ) {
				int whole = cost / element_multiplier_c;
				int frac = cost % element_multiplier_c;
				icon_elements[i]->draw(offset_panel_x_c + 16, offset_panel_y_c + 32 + 18 * cnt);
				int off = 0;
				if( whole > 0 ) {
					Image::writeNumbers(offset_panel_x_c + 36, offset_panel_y_c + 34 + 18 * cnt, numbers_blue, whole, Image::JUSTIFY_LEFT);
					off += 8 * n_digits(whole);
				}
				if( frac == 1 ) {
					numbers_half->draw(offset_panel_x_c + 36 + off, offset_panel_y_c + 34 + 18 * cnt);
				}
				cnt++;
			}
		}
		if( design->isErgonomicallyTerrific() ) {
			icon_ergo->draw(offset_panel_x_c + 64, offset_panel_y_c + 32);
		}
	}
	else if( this->c_page == STATE_FACTORY ) {
		//const int y0 = 16;
		const int y0 = 14;
		//const int y1 = 44;
		const int y1 = 40;
		//const int y2 = 72;
		const int y2 = 66;
		this->button_workers->setImage( men[gamestate->getCurrentSector()->getBuildingEpoch()] );
		if( gamestate->getCurrentSector()->getCurrentManufacture() == NULL ) {
			dash_grey->draw(offset_panel_x_c + 45, offset_panel_y_c + y0 + 20);
			this->button_famount->setImage(NULL);
		}
		else {
			Invention *current_manufacture = gamestate->getCurrentSector()->getCurrentManufacture()->getInvention();
			this->button_famount->setImage( current_manufacture->getImage() );
			//current_manufacture->getImage()->draw(offset_panel_x_c + 8, offset_panel_y_c + 16, true);
			Image::writeNumbers(offset_panel_x_c + 48, offset_panel_y_c + y0 + 20, numbers_white, gamestate->getCurrentSector()->getWorkers(),Image::JUSTIFY_CENTRE);
			int famount = gamestate->getCurrentSector()->getFAmount();
			int famount_x = offset_panel_x_c + 48;
			int famount_y = offset_panel_y_c + y1 + 18;
			if( famount == infinity_c ) {
				icon_infinity->draw(famount_x - icon_infinity->getScaledWidth()/2, famount_y - 2);
			}
			else {
				Image::writeNumbers(famount_x, famount_y, numbers_white, famount,Image::JUSTIFY_CENTRE);
			}

			if( gamestate->getCurrentSector()->getWorkers() > 0 ) {
				int halfdays = 0, hours = 0;
				int thalfdays = 0, thours = 0;
				gamestate->getCurrentSector()->manufactureTimeLeft(&halfdays, &hours);
				gamestate->getCurrentSector()->manufactureTotalTime(&thalfdays, &thours);
				icon_clocks[ hours+1 ]->draw(offset_panel_x_c + 72, offset_panel_y_c + y0);
				Image::writeNumbers(offset_panel_x_c + 80, offset_panel_y_c + y0 + 18, numbers_white, halfdays,Image::JUSTIFY_CENTRE);
				int amount = gamestate->getCurrentSector()->getFAmount();
				if( amount == infinity_c ) {
					icon_clocks[ 0 ]->draw(offset_panel_x_c + 72, offset_panel_y_c + y1);
					icon_infinity->draw(offset_panel_x_c + 74, offset_panel_y_c + y1 + 16);
				}
				else {
					thours *= (amount-1);
					thalfdays *= (amount-1);
					thours += hours;
					thalfdays += halfdays;
					thalfdays += thours/12;
					thours = thours % 12;
					icon_clocks[ thours+1 ]->draw(offset_panel_x_c + 72, offset_panel_y_c + y1);
					Image::writeNumbers(offset_panel_x_c + 80, offset_panel_y_c + y1 + 18, numbers_white, thalfdays, Image::JUSTIFY_CENTRE);
				}
			}
			else {
				icon_clocks[ 0 ]->draw(offset_panel_x_c + 72, offset_panel_y_c + y0);
				icon_infinity->draw(offset_panel_x_c + 74, offset_panel_y_c + y0 + 16);
				icon_clocks[ 0 ]->draw(offset_panel_x_c + 72, offset_panel_y_c + y1);
				icon_infinity->draw(offset_panel_x_c + 74, offset_panel_y_c + y1 + 16);
			}
		}
		icon_shield->draw(offset_panel_x_c + 8, offset_panel_y_c + y2);
		icon_defence->draw(offset_panel_x_c + 40, offset_panel_y_c + y2);
		icon_weapon->draw(offset_panel_x_c + 72, offset_panel_y_c + y2);
	}

	MultiPanel::draw();
}

void GamePanel::changeMiners(Id element,bool decrease,int n) {
	ASSERT(element >= 0 && element < N_ID);
	ASSERT( ::elements[element]->getType() != Element::GATHERABLE );
	int n_miners = gamestate->getCurrentSector()->getMiners( element );
	int n_spare = gamestate->getCurrentSector()->getAvailablePopulation();
	if( decrease && n_miners > 0 ) {
		n_miners -= n;
		if( n_miners < 0 )
			n_miners = 0;
		//gamestate->getCurrentSector()->setMiners( element, n_miners );
		const Sector *current_sector = this->gamestate->getCurrentSector();
		this->gamestate->setNMiners(current_sector->getXPos(), current_sector->getYPos(), element, n_miners);
	}
	else if( !decrease && n_spare > 0 ) {
		if( n > n_spare )
			n = n_spare;
		n_miners += n;
		//gamestate->getCurrentSector()->setMiners( element, n_miners );
		const Sector *current_sector = this->gamestate->getCurrentSector();
		this->gamestate->setNMiners(current_sector->getXPos(), current_sector->getYPos(), element, n_miners);
	}
}

void GamePanel::buttonNDesignersClick(void *data, int arg, bool m_left, bool m_middle, bool m_right) {
	GamePanel *gamePanel = static_cast<GamePanel *>(data);
	if( gamePanel->gamestate->getCurrentSector()->getCurrentDesign() == NULL ) {
		// no longer relevant
		return;
	}
	int n_designers = gamePanel->gamestate->getCurrentSector()->getDesigners();
	int n_spare = gamePanel->gamestate->getCurrentSector()->getAvailablePopulation();
	int n = getNClicks();
	if( m_left ) {
		n_designers -= n;
		if( n_designers < 0 )
			n_designers = 0;
		//gamePanel->gamestate->getCurrentSector()->setDesigners( n_designers );
		const Sector *current_sector = gamePanel->gamestate->getCurrentSector();
		gamePanel->gamestate->setNDesigners(current_sector->getXPos(), current_sector->getYPos(), n_designers);
	}
	else if( m_right ) {
		if( n > n_spare )
			n = n_spare;
		n_designers += n;
		//gamePanel->gamestate->getCurrentSector()->setDesigners( n_designers );
		const Sector *current_sector = gamePanel->gamestate->getCurrentSector();
		gamePanel->gamestate->setNDesigners(current_sector->getXPos(), current_sector->getYPos(), n_designers);
	}
}

void GamePanel::buttonNManufacturersClick(void *data, int arg, bool m_left, bool m_middle, bool m_right) {
	GamePanel *gamePanel = static_cast<GamePanel *>(data);
	if( gamePanel->gamestate->getCurrentSector()->getCurrentManufacture() == NULL ) {
		// no longer relevant
		return;
	}
	int n_workers = gamePanel->gamestate->getCurrentSector()->getWorkers();
	int n_spare = gamePanel->gamestate->getCurrentSector()->getAvailablePopulation();
	int n = getNClicks();
	if( m_left ) {
		n_workers -= n;
		if( n_workers < 0 )
			n_workers = 0;
		//gamePanel->gamestate->getCurrentSector()->setWorkers( n_workers );
		const Sector *current_sector = gamePanel->gamestate->getCurrentSector();
		gamePanel->gamestate->setNWorkers(current_sector->getXPos(), current_sector->getYPos(), n_workers);
	}
	else if( m_right ) {
		if( n > n_spare )
			n = n_spare;
		n_workers += n;
		//gamePanel->gamestate->getCurrentSector()->setWorkers( n_workers );
		const Sector *current_sector = gamePanel->gamestate->getCurrentSector();
		gamePanel->gamestate->setNWorkers(current_sector->getXPos(), current_sector->getYPos(), n_workers);
	}
}

void GamePanel::buttonFAmountClick(void *data, int arg, bool m_left, bool m_middle, bool m_right) {
	GamePanel *gamePanel = static_cast<GamePanel *>(data);
	if( gamePanel->gamestate->getCurrentSector()->getCurrentManufacture() == NULL ) {
		// no longer relevant
		return;
	}
	int famount = gamePanel->gamestate->getCurrentSector()->getFAmount();
	if( m_left ) {
		if( famount > 1 ) {
			//gamePanel->gamestate->getCurrentSector()->setFAmount( famount - 1 );
			const Sector *current_sector = gamePanel->gamestate->getCurrentSector();
			gamePanel->gamestate->setFAmount(current_sector->getXPos(), current_sector->getYPos(), famount - 1);
		}
	}
	else if( m_right && famount != infinity_c ) {
		//gamePanel->gamestate->getCurrentSector()->setFAmount( famount + 1 );
		const Sector *current_sector = gamePanel->gamestate->getCurrentSector();
		gamePanel->gamestate->setFAmount(current_sector->getXPos(), current_sector->getYPos(), famount + 1);
	}
}

void GamePanel::buttonNMinersClick(void *data, int arg, bool m_left, bool m_middle, bool m_right) {
	GamePanel *gamePanel = static_cast<GamePanel *>(data);
    if( gamePanel->button_nminers[arg] == NULL || !gamePanel->button_nminers[arg]->isEnabled() ) {
		// no longer relevant
		return;
	}
    int n = getNClicks();
	Id element = (Id)gamePanel->element_index[arg];
	gamePanel->changeMiners(element, m_left, n);
}

void GamePanel::buttonNBuildersClick(void *data, int arg, bool m_left, bool m_middle, bool m_right) {
	GamePanel *gamePanel = static_cast<GamePanel *>(data);
	if( gamePanel->button_nbuilders[arg] == NULL || !gamePanel->button_nbuilders[arg]->isEnabled() ) {
		// no longer relevant
		return;
	}
	int n_builders = gamePanel->gamestate->getCurrentSector()->getBuilders((Type)arg);
	int n_spare = gamePanel->gamestate->getCurrentSector()->getAvailablePopulation();
	int n = getNClicks();
	if( m_left && n_builders > 0 ) {
		n_builders -= n;
		if( n_builders < 0 )
			n_builders = 0;
		//gamePanel->gamestate->getCurrentSector()->setBuilders((Type)arg, n_builders);
		const Sector *current_sector = gamePanel->gamestate->getCurrentSector();
		gamePanel->gamestate->setNBuilders(current_sector->getXPos(), current_sector->getYPos(), (Type)arg, n_builders);
	}
	else if( m_right && n_spare > 0 ) {
		if( n > n_spare )
			n = n_spare;
		n_builders += n;
		//gamePanel->gamestate->getCurrentSector()->setBuilders((Type)arg, n_builders);
		const Sector *current_sector = gamePanel->gamestate->getCurrentSector();
		gamePanel->gamestate->setNBuilders(current_sector->getXPos(), current_sector->getYPos(), (Type)arg, n_builders);
	}
}

void GamePanel::input(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	MultiPanel::input(m_x, m_y, m_left, m_middle, m_right, click);
	if( this->hasModal() ) {
		return;
	}
	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);
    bool done = false;

	const int help_delay_c = 1000;
	const int help_x_c = 160;
	const int help_y_c = 120;
	const char help_elementstocks_c[] = "mine new elements";
	const char help_build_c[] = "construct new buildings";

	if( this->c_page == STATE_SECTORCONTROL ) {
		if( m_left && click && this->button_design->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
			if( gamestate->getCurrentSector()->getCurrentDesign() == NULL ) {
				addTextEffect(new TextEffect("click on a new design to invent", help_x_c, help_y_c, help_delay_c));
			}
            this->setPage(STATE_DESIGN);
		}
		else if( m_left && click && this->button_shield->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
			addTextEffect(new TextEffect("displays strength of your buildings", help_x_c, help_y_c, help_delay_c));
            this->setPage(STATE_SHIELD);
		}
		else if( m_left && click && this->button_defence->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
			addTextEffect(new TextEffect("deploy defenders to your buildings", help_x_c, help_y_c, help_delay_c));
            this->setPage(STATE_DEFENCE);
		}
		else if( m_left && click && this->button_attack->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
			addTextEffect(new TextEffect("deploy your armies", help_x_c, help_y_c, help_delay_c));
            this->setPage(STATE_ATTACK);
		}
		/*else if( m_left && click && this->button_elementstocks->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            this->setPage(STATE_ELEMENTSTOCKS);
x		}*/
		else if( ( m_left || m_right ) && this->button_ndesigners->mouseOver(m_x,m_y) && gamestate->getCurrentSector()->getCurrentDesign() != NULL ) {
            done = true;
			processClick(buttonNDesignersClick, this->get(this->c_page), this, 0, button_ndesigners, m_left, m_middle, m_right, click);
		}
		else if( ( m_left || m_right ) && this->button_nworkers->mouseOver(m_x,m_y) && gamestate->getCurrentSector()->getCurrentManufacture() != NULL ) {
            done = true;
			processClick(buttonNManufacturersClick, this->get(this->c_page), this, 0, button_nworkers, m_left, m_middle, m_right, click);
		}
		else if( m_left && click && ( this->button_build[BUILDING_MINE]->mouseOver(m_x,m_y) || this->button_build[BUILDING_FACTORY]->mouseOver(m_x,m_y) || this->button_build[BUILDING_LAB]->mouseOver(m_x,m_y) )  ) {
            done = true;
            registerClick();
			addTextEffect(new TextEffect(help_build_c, help_x_c, help_y_c, help_delay_c));
            this->setPage(STATE_BUILD);
		}
		else if( m_left && click && this->button_knowndesigns->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
			addTextEffect(new TextEffect("displays elements needed for each design", help_x_c, help_y_c, help_delay_c));
            this->setPage(STATE_KNOWNDESIGNS);
		}
		else if( m_left && click && this->button_factory->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
			if( gamestate->getCurrentSector()->getCurrentManufacture() == NULL ) {
				addTextEffect(new TextEffect("use the factory to manufacture advanced weapons", help_x_c, help_y_c, help_delay_c));
			}
            this->setPage(STATE_FACTORY);
		}

        for(int i=0;i<4 && ( m_left || m_right ) && !done;i++) {
			//Id element = (Id)this->element_index[i];
			if( m_left && click && this->button_elements[i]->mouseOver(m_x, m_y) ) {
                done = true;
                registerClick();
				addTextEffect(new TextEffect(help_elementstocks_c, help_x_c, help_y_c, help_delay_c));
                this->setPage(STATE_ELEMENTSTOCKS);
			}
            else if( this->button_nminers[i]->mouseOver(m_x,m_y) ) {
                done = true;
                // if one mouse button, only allow changing via the sub-menu (otherwise problems with left/right icons going off screen)
                if( oneMouseButtonMode() ) {
					if( click ) {
	                    registerClick();
						addTextEffect(new TextEffect(help_elementstocks_c, help_x_c, help_y_c, help_delay_c));
		                this->setPage(STATE_ELEMENTSTOCKS);
					}
                }
                else {
                    ASSERT_ELEMENT_ID( this->element_index[i] );
                    processClick(buttonNMinersClick, this->get(this->c_page), this, i, button_nminers[i], m_left, m_middle, m_right, click);
                }
			}
		}

        for(int i=0;i<N_BUILDINGS && ( m_left || m_right ) && !done;i++) {
            if( button_nbuilders[i] != NULL && this->button_nbuilders[i]->mouseOver(m_x,m_y) ) {
                done = true;
                // if one mouse button, only allow changing via the sub-menu (otherwise problems with left/right icons going off screen)
                if( oneMouseButtonMode() ) {
					if( click ) {
	                    registerClick();
						addTextEffect(new TextEffect(help_build_c, help_x_c, help_y_c, help_delay_c));
		                this->setPage(STATE_BUILD);
					}
                }
                else {
                    processClick(buttonNBuildersClick, this->get(this->c_page), this, i, button_nbuilders[i], m_left, m_middle, m_right, click);
                }
			}
		}
	}
	else if( this->c_page == STATE_DESIGN ) {
		if( m_left && click && this->button_bigdesign->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            this->setPage(STATE_SECTORCONTROL);
		}
        for(int i=0;i<n_sub_epochs && !done && click;i++) {
			if( m_left && this->button_shields[i]->mouseOver(m_x,m_y)
				&& ( gamestate->getCurrentSector()->getCurrentDesign() == NULL
				|| gamestate->getCurrentSector()->getCurrentDesign()->getInvention() != invention_shields[start_epoch + i] )
				) {
                    done = true;
                    Design *design = gamestate->getCurrentSector()->bestDesign(Invention::SHIELD, start_epoch + i);
					//gamestate->getCurrentSector()->setCurrentDesign( design );
					gamestate->setCurrentDesign(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos(), design);
					{
						stringstream str;
						str << "start designing a " << design->getInvention()->getName();
						addTextEffect(new TextEffect(str.str(), help_x_c, help_y_c, help_delay_c));
					}
			}
			else if( m_left && this->button_defences[i]->mouseOver(m_x,m_y)
				&& ( gamestate->getCurrentSector()->getCurrentDesign() == NULL
				|| gamestate->getCurrentSector()->getCurrentDesign()->getInvention() != invention_defences[start_epoch + i] )
				) {
                    done = true;
                    Design *design = gamestate->getCurrentSector()->bestDesign(Invention::DEFENCE, start_epoch + i);
					//gamestate->getCurrentSector()->setCurrentDesign( design );
					gamestate->setCurrentDesign(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos(), design);
					{
						stringstream str;
						str << "start designing a " << design->getInvention()->getName();
						addTextEffect(new TextEffect(str.str(), help_x_c, help_y_c, help_delay_c));
					}
			}
			else if( m_left && this->button_weapons[i]->mouseOver(m_x,m_y)
				&& ( gamestate->getCurrentSector()->getCurrentDesign() == NULL
				|| gamestate->getCurrentSector()->getCurrentDesign()->getInvention() != invention_weapons[start_epoch + i] )
				) {
                    done = true;
                    Design *design = gamestate->getCurrentSector()->bestDesign(Invention::WEAPON, start_epoch + i);
					//gamestate->getCurrentSector()->setCurrentDesign( design );
					gamestate->setCurrentDesign(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos(), design);
					{
						stringstream str;
						str << "start designing a " << design->getInvention()->getName();
						addTextEffect(new TextEffect(str.str(), help_x_c, help_y_c, help_delay_c));
					}
			}
		}
        if( ( m_left || m_right ) && !done && this->button_designers->mouseOver(m_x,m_y) ) {
            done = true;
			processClick(buttonNDesignersClick, this->get(this->c_page), this, 0, button_designers, m_left, m_middle, m_right, click);
		}
	}
	else if( this->c_page == STATE_SHIELD ) {
		if( m_left && click && this->button_bigshield->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            this->setPage(STATE_SECTORCONTROL);
		}
        for(int i=0;i<n_sub_epochs && !done;i++) {
			if( m_left && click && this->button_deploy_shields[i]->mouseOver(m_x,m_y) ) {
                done = true;
                registerClick();
                if( mousestate == MOUSESTATE_DEPLOY_SHIELD && deploy_shield == i )
					setMouseState(MOUSESTATE_NORMAL);
				else {
					int n_store = gamestate->getCurrentSector()->getStoredShields(i);
					if( n_store > 0 || ( start_epoch + i < factory_epoch_c && gamestate->getCurrentSector()->canBuildDesign(Invention::SHIELD, start_epoch+i) ) ) {
						setMouseState(MOUSESTATE_DEPLOY_SHIELD);
						deploy_shield = i;
						ASSERT_EPOCH(deploy_shield);
					}
				}
			}
		}
        if( m_left && click && !done && this->button_shutdown->mouseOver(m_x,m_y) ) {
            done = true;
            if( mousestate == MOUSESTATE_SHUTDOWN )
				setMouseState(MOUSESTATE_NORMAL);
			else
				setMouseState(MOUSESTATE_SHUTDOWN);
		}
	}
	else if( this->c_page == STATE_DEFENCE ) {
		if( m_left && click && this->button_bigdefence->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            this->setPage(STATE_SECTORCONTROL);
		}
        for(int i=0;i<n_sub_epochs && !done;i++) {
			if( m_left && click && this->button_deploy_defences[i]->mouseOver(m_x,m_y) ) {
                done = true;
                registerClick();
                if( mousestate == MOUSESTATE_DEPLOY_DEFENCE && deploy_defence == start_epoch + i )
					setMouseState(MOUSESTATE_NORMAL);
				else {
					int n_store = gamestate->getCurrentSector()->getStoredDefenders(start_epoch + i);
					if( n_store > 0 || ( start_epoch + i < factory_epoch_c && gamestate->getCurrentSector()->canBuildDesign(Invention::DEFENCE, start_epoch+i) ) ) {
						setMouseState(MOUSESTATE_DEPLOY_DEFENCE);
						deploy_defence = start_epoch + i;
						ASSERT_EPOCH(deploy_defence);
					}
				}
			}
		}
	}
	else if( this->c_page == STATE_ATTACK ) {
		int n_nukes = 0;
		if( m_left ) {
			n_nukes = gamestate->getCurrentSector()->getAssembledArmy()->getSoldiers(nuclear_epoch_c);
		}
		if( m_left && click && this->button_bigattack->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            // return assembled army
			this->setPage(STATE_SECTORCONTROL);
			//gamestate->getCurrentSector()->returnAssembledArmy();
			gamestate->returnAssembledArmy(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos());
			setMouseState(MOUSESTATE_NORMAL);
		}
		else if( m_left && click && this->button_return_attackers->mouseOver(m_x,m_y) ) {
            done = true;
            // return assembled army
			//gamestate->getCurrentSector()->returnAssembledArmy();
			gamestate->returnAssembledArmy(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos());
			setMouseState(MOUSESTATE_NORMAL);
		}
		else if( m_left && !click && this->button_deploy_unarmedmen->mouseOver(m_x,m_y) && n_nukes == 0 ) {
            done = true;
            // add unarmed man to assembled army
			int n_spare = gamestate->getCurrentSector()->getAvailablePopulation();
			int n = getNClicks();
			ASSERT(n >= 1);
			if( n_spare < n )
				n = 1;
			if( n_spare >= n ) {
				//gamestate->getCurrentSector()->getAssembledArmy()->add(n_epochs_c, n);
				//gamestate->getCurrentSector()->setPopulation( n_population - n );
				gamestate->assembleArmyUnarmed(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos(), n);
				setMouseState(MOUSESTATE_DEPLOY_WEAPON);
			}
		}
		// add weapons to assembled army?
        for(int i=0;i<n_sub_epochs && m_left && !click && !done;i++) {
			bool ok = true;
			if( start_epoch + i == nuclear_epoch_c && n_nukes == 0 ) {
				int n_selected = gamestate->getCurrentSector()->getAssembledArmy()->getTotal();
				if( n_selected > 0 )
					ok = false;
			}
			else if( n_nukes > 0 ) {
				ok = false;
			}
			if( ok && this->button_deploy_attackers[i]->mouseOver(m_x,m_y) ) {
                done = true;
                int n = getNClicks();
				//if( gamestate->getCurrentSector()->assembleArmy(start_epoch + i, n) ) {
				if( gamestate->assembleArmy(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos(), start_epoch + i, n) ) {
					setMouseState(MOUSESTATE_DEPLOY_WEAPON);
				}
			}
		}
	}
	else if( this->c_page == STATE_ELEMENTSTOCKS ) {
		if( m_left && click && this->button_bigelementstocks->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            this->setPage(STATE_SECTORCONTROL);
		}
        for(int i=0;i<4 && !done;i++) {
			//Id element = (Id)this->element_index[i];
			/*int n_elements = 0, fraction = 0;
			gamestate->getCurrentSector()->getElements(&n_elements, &fraction, element);
			if( ( n_elements > 0 || fraction > 0 ) && elements[element]->type != Element::GATHERABLE && ( m_left || m_right ) && this->button_nminers2[i]->mouseOver(m_x,m_y) ) {*/
			if( ( m_left || m_right ) && this->button_nminers2[i]->mouseOver(m_x,m_y) ) {
                done = true;
                ASSERT_ELEMENT_ID( this->element_index[i] );
				/*int n = getNClicks();
				changeMiners(element, m_left, n);*/
				//processClick(buttonNMinersClick, this, i, button_nminers2[i], m_left, m_middle, m_right);
				processClick(buttonNMinersClick, this->get(this->c_page), this, i, button_nminers2[i], m_left, m_middle, m_right, click);
			}
		}
	}
	else if( this->c_page == STATE_BUILD ) {
		if( m_left && click && this->button_bigbuild->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            this->setPage(STATE_SECTORCONTROL);
		}
		else if( m_left || m_right ) {
            for(int i=0;i<N_BUILDINGS && !done;i++) {
				if( button_nbuilders2[i] != NULL && this->button_nbuilders2[i]->mouseOver(m_x,m_y) ) {
                    done = true;
                    /*int n_builders = gamestate->getCurrentSector()->getBuilders((Type)i);
					int n_spare = gamestate->getCurrentSector()->getAvailablePopulation();
					int n = getNClicks();
					if( m_left && n_builders > 0 ) {
						n_builders -= n;
						if( n_builders < 0 )
							n_builders = 0;
						gamestate->getCurrentSector()->setBuilders((Type)i, n_builders);
					}
					else if( m_right && n_spare > 0 ) {
						if( n > n_spare )
							n = n_spare;
						n_builders += n;
						gamestate->getCurrentSector()->setBuilders((Type)i, n_builders);
					}*/
					//processClick(buttonNBuildersClick, this, i, button_nbuilders2[i], m_left, m_middle, m_right);
					processClick(buttonNBuildersClick, this->get(this->c_page), this, i, button_nbuilders2[i], m_left, m_middle, m_right, click);
				}
			}
		}
	}
	else if( this->c_page == STATE_KNOWNDESIGNS ) {
		if( m_left && click && this->button_bigknowndesigns->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            this->setPage(STATE_SECTORCONTROL);
		}
        for(int i=0;i<n_sub_epochs && !done;i++) {
			if( m_left && click && this->button_knownshields[i]->mouseOver(m_x,m_y) ) {
                done = true;
                registerClick();
                this->setPage(STATE_DESIGNINFO);
				this->designinfo = Invention::getInvention(Invention::SHIELD,start_epoch+i);
			}
			else if( m_left && click && this->button_knowndefences[i]->mouseOver(m_x,m_y) ) {
                done = true;
                registerClick();
                this->setPage(STATE_DESIGNINFO);
				this->designinfo = Invention::getInvention(Invention::DEFENCE,start_epoch+i);
			}
			else if( m_left && click && this->button_knownweapons[i]->mouseOver(m_x,m_y) ) {
                done = true;
                registerClick();
                this->setPage(STATE_DESIGNINFO);
				this->designinfo = Invention::getInvention(Invention::WEAPON,start_epoch+i);
			}
		}
	}
	else if( this->c_page == STATE_DESIGNINFO ) {
		if( m_left && click && this->button_bigdesigninfo->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            this->setPage(STATE_KNOWNDESIGNS);
		}
		//if( ( onemousebutton ? m_left : ( m_left && m_right ) ) && click && this->button_trashdesign->mouseOver(m_x,m_y) ) {
		if( m_left && click && this->button_trashdesign->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            this->setPage(STATE_KNOWNDESIGNS);
			ASSERT(this->designinfo != NULL);
			//gamestate->getCurrentSector()->trashDesign(this->designinfo);
			gamestate->trashDesign(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos(), this->designinfo);
		}
	}
	else if( this->c_page == STATE_FACTORY ) {
		if( m_left && click && this->button_bigfactory->mouseOver(m_x,m_y) ) {
            done = true;
            registerClick();
            this->setPage(STATE_SECTORCONTROL);
		}
        for(int i=0;i<n_sub_epochs && !done && click;i++) {
			if( m_left && this->button_fshields[i]->mouseOver(m_x,m_y)
				&& ( gamestate->getCurrentSector()->getCurrentManufacture() == NULL
				|| gamestate->getCurrentSector()->getCurrentManufacture()->getInvention() != invention_shields[start_epoch + i] )
				) {
                    done = true;
                    Design *design = gamestate->getCurrentSector()->knownDesign(Invention::SHIELD, start_epoch + i);
					//gamestate->getCurrentSector()->setCurrentManufacture( design );
					gamestate->setCurrentManufacture(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos(), design);
					{
						stringstream str;
						str << "start manufacturing a " << design->getInvention()->getName();
						addTextEffect(new TextEffect(str.str(), help_x_c, help_y_c, help_delay_c));
					}
			}
			else if( m_left && this->button_fdefences[i]->mouseOver(m_x,m_y)
				&& ( gamestate->getCurrentSector()->getCurrentManufacture() == NULL
				|| gamestate->getCurrentSector()->getCurrentManufacture()->getInvention() != invention_defences[start_epoch + i] )
				) {
                    done = true;
                    Design *design = gamestate->getCurrentSector()->knownDesign(Invention::DEFENCE, start_epoch + i);
					//gamestate->getCurrentSector()->setCurrentManufacture( design );
					gamestate->setCurrentManufacture(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos(), design);
					{
						stringstream str;
						str << "start manufacturing a " << design->getInvention()->getName();
						addTextEffect(new TextEffect(str.str(), help_x_c, help_y_c, help_delay_c));
					}
			}
			else if( m_left && this->button_fweapons[i]->mouseOver(m_x,m_y)
				&& ( gamestate->getCurrentSector()->getCurrentManufacture() == NULL
				|| gamestate->getCurrentSector()->getCurrentManufacture()->getInvention() != invention_weapons[start_epoch + i] )
				) {
                    done = true;
                    Design *design = gamestate->getCurrentSector()->knownDesign(Invention::WEAPON, start_epoch + i);
					//gamestate->getCurrentSector()->setCurrentManufacture( design );
					gamestate->setCurrentManufacture(gamestate->getCurrentSector()->getXPos(), gamestate->getCurrentSector()->getYPos(), design);
					{
						stringstream str;
						str << "start manufacturing a " << design->getInvention()->getName();
						addTextEffect(new TextEffect(str.str(), help_x_c, help_y_c, help_delay_c));
					}
			}
		}
        if( ( m_left || m_right ) && !done && this->button_workers->mouseOver(m_x,m_y) ) {
            done = true;
            /*int n_workers = gamestate->getCurrentSector()->getWorkers();
			int n_spare = gamestate->getCurrentSector()->getAvailablePopulation();
			int n = getNClicks();
			if( m_left ) {
				n_workers -= n;
				if( n_workers < 0 )
					n_workers = 0;
				gamestate->getCurrentSector()->setWorkers( n_workers );
			}
			else if( m_right ) {
				if( n > n_spare )
					n = n_spare;
				n_workers += n;
				gamestate->getCurrentSector()->setWorkers( n_workers );
			}*/
			//processClick(buttonNManufacturersClick, this, 0, button_workers, m_left, m_middle, m_right);
			processClick(buttonNManufacturersClick, this->get(this->c_page), this, 0, button_workers, m_left, m_middle, m_right, click);
		}
        else if( ( m_left || m_right ) && !done && this->button_famount->mouseOver(m_x,m_y) ) {
            done = true;
            /*int famount = gamestate->getCurrentSector()->getFAmount();
			if( m_left ) {
				if( famount > 1 )
					gamestate->getCurrentSector()->setFAmount( famount - 1 );
			}
			else if( m_right && famount != infinity_c ) {
				gamestate->getCurrentSector()->setFAmount( famount + 1 );
			}*/
			//processClick(buttonFAmountClick, this, 0, button_famount, m_left, m_middle, m_right);
			processClick(buttonFAmountClick, this->get(this->c_page), this, 0, button_famount, m_left, m_middle, m_right, click);
		}
	}
}
