//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>
#include <cerrno> // n.b., needed on Linux at least

#include <sstream>
using std::stringstream;

#include "gamestate.h"
#include "game.h"
#include "utils.h"
#include "sector.h"
#include "gui.h"
#include "player.h"

#include "screen.h"
#include "image.h"
#include "sound.h"

//---------------------------------------------------------------------------

const int defenders_frames_per_update_c = (int)(2.0 * ticks_per_frame_c * time_ratio_c); // consider a turn every this number of frames
const int soldier_move_rate_c = (int)(0.6 * ticks_per_frame_c * time_ratio_c); // ticks per pixel
const int air_move_rate_c = (int)(0.2 * ticks_per_frame_c * time_ratio_c); // ticks per pixel
const int soldier_turn_rate_c = (int)(20.0 * ticks_per_frame_c * time_ratio_c); // mean ticks per turn

//const int shield_step_y_c = 16;
const int shield_step_y_c = 20;

class Soldier {
	static int sort_soldier_pair(const void *v1,const void *v2);
public:
	int player;
	int epoch;
	int xpos, ypos;
	AmmoDirection dir;
	Soldier(int player,int epoch,int xpos,int ypos) {
		ASSERT_S_EPOCH(epoch);
		this->player = player;
		this->epoch = epoch;
		this->xpos = xpos;
		this->ypos = ypos;
		this->dir = (AmmoDirection)(rand() % 4);
	}
	static void sortSoldiers(Soldier **soldiers,int n_soldiers) {
		qsort(soldiers, n_soldiers, sizeof( Soldier *), sort_soldier_pair);
	}
};

int Soldier::sort_soldier_pair(const void *v1,const void *v2) {
	Soldier *s1 = *(Soldier **)v1;
	Soldier *s2 = *(Soldier **)v2;
	/*if( s1->epoch >= 6 )
	return 1;
	else if( s2->epoch >= 6 )
	return -1;
	else*/
	return (s1->ypos - s2->ypos);
}

void Feature::draw() const {
	const int ticks_per_frame_c = 110; // tree animation looks better if offset from main animation, and if slightly slower
	int counter = getRealTime() / ticks_per_frame_c;
	image[counter % n_frames]->draw(xpos, ypos);
}

TimedEffect::TimedEffect() {
	this->timeset = getRealTime();
	this->func_finish = NULL;
}

TimedEffect::TimedEffect(int delay, void (*func_finish)()) {
	this->timeset = getRealTime() + delay;
	this->func_finish = func_finish;
}

const int ammo_time_c = 1000;
const float ammo_speed_c = 1.5f; // higher is faster

AmmoEffect::AmmoEffect(PlayingGameState *gamestate,int epoch, AmmoDirection dir, int xpos, int ypos) : TimedEffect(), gamestate(gamestate) {
	ASSERT_EPOCH(epoch);
	this->gametimeset = getGameTime();
	this->epoch = epoch;
	this->dir = dir;
	this->xpos = xpos;
	this->ypos = ypos;
}

bool AmmoEffect::render() const {
	int time = getRealTime() - this->timeset;
	if( time < 0 )
		return false;
	int gametime = getGameTime() - this->gametimeset;
	int x = xpos;
	int y = ypos;
	int dist = (int)(gametime * ammo_speed_c);
	if( dir == ATTACKER_AMMO_BOMB )
		dist /= 2;
	if( dir == ATTACKER_AMMO_DOWN )
		y += dist;
	else if( dir == ATTACKER_AMMO_UP )
		y -= dist;
	else if( dir == ATTACKER_AMMO_LEFT )
		x -= dist;
	else if( dir == ATTACKER_AMMO_RIGHT )
		x += dist;
	else if( dir == ATTACKER_AMMO_BOMB )
		y += dist;
	else {
		ASSERT(0);
	}
	Image *image = attackers_ammo[epoch][dir];
	if( dir == ATTACKER_AMMO_BOMB && dist > 24 ) {
		if( explosions[0] != NULL ) {
			int w = image->getScaledWidth();
			int w2 = explosions[0]->getScaledWidth();
			gamestate->explosionEffect(offset_land_x_c + x + (w-w2)/2, offset_land_y_c + y);
		}
		return true;
	}
	if( x < 0 || y < 0 )
		return true;
	x += offset_land_x_c;
	y += offset_land_y_c;
	if( x + image->getScaledWidth() >= screen->getWidth() || y + image->getScaledHeight() >= screen->getHeight() )
		return true;
	image->draw(x, y);
	if( time > ammo_time_c )
		return true;
	return false;
}

const int fade_time_c = 1000;
const int whitefade_time_c = 1000;

FadeEffect::FadeEffect(bool white,bool out,int delay, void (*func_finish)()) : TimedEffect(delay, func_finish) {
	this->white = white;
	this->out = out;
#if SDL_MAJOR_VERSION == 1
	this->image = Image::createBlankImage(screen->getWidth(), screen->getHeight(), 24);
	int r = 0, g = 0, b = 0;
	if( white ) {
		r = g = b = 255;
	}
	else {
		r = g = b = 0;
	}
	image->fillRect(0, 0, screen->getWidth(), screen->getHeight(), r, g, b);
	this->image->convertToDisplayFormat();
#else
	image = NULL;
#endif
}

FadeEffect::~FadeEffect() {
#if SDL_MAJOR_VERSION == 1
	delete image;
#endif
}

bool FadeEffect::render() const {
	int time = getRealTime() - this->timeset;
	int length = white ? whitefade_time_c : fade_time_c;
	if( time < 0 )
		return false;
	double alpha = 0.0;
	if( white ) {
		alpha = ((double)time) / (0.5 * (double)length);
		if( alpha > 2.0 )
			alpha = 2.0;
		if( time > length/2.0 ) {
			alpha = 2.0 - alpha;
		}
	}
	else {
		alpha = ((double)time) / (double)length;
		if( alpha > 1.0 )
			alpha = 1.0;
		if( !out )
			alpha = 1.0 - alpha;
	}
#if SDL_MAJOR_VERSION == 1
	image->drawWithAlpha(0, 0, (unsigned char)(alpha * 255));
#else
	unsigned char value = white ? 255 : 0;
	screen->fillRectWithAlpha(0, 0, screen->getWidth(), screen->getHeight(), value, value, value, (unsigned char)(alpha * 255));
#endif
	if( time > length ) // we still need to draw the fade, on the last time
		return true;
	return false;
}

const int flashingsquare_flash_time_c = 250;
const int flashing_square_n_flashes_c = 8;

bool FlashingSquare::render() const {
	int time = getRealTime() - this->timeset;
	if( time < 0 )
		return false;
	if( time > flashingsquare_flash_time_c * flashing_square_n_flashes_c ) {
		return true;
	}

	bool flash = ( time / flashingsquare_flash_time_c ) % 2 == 0;
	if( flash ) {
		int map_x = offset_map_x_c + 16 * this->xpos;
		int map_y = offset_map_y_c + 16 * this->ypos;
		flashingmapsquare->draw(map_x, map_y);
	}
	return false;
}

bool AnimationEffect::render() const {
	int time = getRealTime() - this->timeset;
	if( time < 0 )
		return false;
	int frame = time / time_per_frame;
	if( frame >= n_images )
		return true;
	//this->finished = true;
	else {
		if( !dir )
			frame = n_images - 1 - frame;
		images[frame]->draw(xpos, ypos);
	}
	return false;
}

bool TextEffect::render() const {
	int time = getRealTime() - this->timeset;
	if( time < 0 )
		return false;
	else if( time > duration )
		return true;
	Image::write(xpos, ypos, letters_small, text.c_str(), Image::JUSTIFY_CENTRE);
	return false;
}

GameState::GameState(int client_player) : client_player(client_player) {
	this->fade = NULL;
	this->whitefade = NULL;
	//this->effects = new Vector();
	//this->effects = new vector<TimedEffect *>();
	this->screen_page = new PanelPage(0, 0);
	this->screen_page->setTolerance(0);
	this->mobile_ui_display_mouse = false;
	this->mouse_image = NULL;
	this->mouse_off_x = 0;
	this->mouse_off_y = 0;
	this->confirm_type = CONFIRMTYPE_UNKNOWN;
    this->confirm_window = NULL;
    this->confirm_yes_button = NULL;
    this->confirm_no_button = NULL;
}

GameState::~GameState() {
	LOG("~GameState()\n");
	/*for(int i=0;i<effects->size();i++) {
		//TimedEffect *effect = (TimedEffect *)effects->get(i);
		TimedEffect *effect = effects->at(i);
		delete effect;
	}
	delete effects;*/

	if( fade != NULL )
		delete fade;
	if( whitefade != NULL )
		delete whitefade;

	if( this->screen_page )
		delete screen_page;

	LOG("~GameState() done\n");
}

void GameState::reset() {
    //LOG("GameState::reset()\n");
	this->screen_page->free(true);
}

void GameState::setDefaultMouseImage() {
	if( isDemo() )
		mouse_image = mouse_pointers[0];
	else
		mouse_image = mouse_pointers[client_player];
	mobile_ui_display_mouse = false;
}

void GameState::draw() {
	if( mouse_image != NULL ) {
		bool touch_mode = mobile_ui || application->isBlankMouse();
		if( touch_mode && mobile_ui_display_mouse ) {
			mouse_image->draw(default_width_c - mouse_image->getScaledWidth(), 0);
		}
		else if( !touch_mode ) {
			int m_x = 0, m_y = 0;
			screen->getMouseCoords(&m_x, &m_y);
			m_x = (int)(m_x / scale_width);
			m_y = (int)(m_y / scale_height);
			m_x += mouse_off_x;
			m_y += mouse_off_y;
			mouse_image->draw(m_x, m_y);
		}
	}

	if( fade ) {
		if( fade->render() ) {
			FadeEffect *copy = fade;
			delete fade;
			if( copy == fade )
				fade = NULL;
		}
	}
	if( whitefade ) {
		if( whitefade->render() ) {
			FadeEffect *copy = whitefade;
			delete whitefade;
			if( copy == whitefade )
				whitefade = NULL;
		}
	}

	if( isPaused() ) {
		string str = mobile_ui ? "touch screen\nto unpause game" : "press p or click\nmouse to unpause game";
		// n.b., don't use 120 for y pos, need to avoid collision with quit game message
		// and offset x pos slightly, to avoid overlapping with GUI
		Image::write(120, 100, letters_small, str.c_str(), Image::JUSTIFY_LEFT);
	}

	if( application->hasFPS() ) {
		float fps = application->getFPS();
		if( fps > 0.0f ) {
			stringstream str;
			str << fps;
			Image::writeMixedCase(4, default_height_c - 16, letters_large, letters_small, numbers_white, str.str().c_str(), Image::JUSTIFY_LEFT);
		}
	}

	screen->refresh();
}

void GameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	this->screen_page->input(m_x, m_y, m_left, m_middle, m_right, click);
}

void GameState::requestQuit() {
	application->setQuit();
}

void GameState::createQuitWindow() {
    if( confirm_window == NULL && !state_changed ) {
		confirm_type = CONFIRMTYPE_QUITGAME;
		confirm_window = new PanelPage(120, 120, 64, 32);
		Button *text_button = new Button(0, 0, "REALLY QUIT?", letters_small);
		confirm_window->add(text_button);
		confirm_yes_button = new Button(0, 16, "YES", letters_small);
		confirm_window->add(confirm_yes_button);
		confirm_no_button = new Button(32, 16, "NO", letters_small);
		confirm_window->add(confirm_no_button);
		screen_page->add(confirm_window);
	}
}

void GameState::closeConfirmWindow() {
    //LOG("GameState::closeConfirmWindow()\n");
    if( confirm_window != NULL ) {
        delete confirm_window;
        confirm_window = NULL;
        confirm_yes_button = NULL;
        confirm_no_button = NULL;
    }
    //LOG("GameState::closeConfirmWindow() done\n");
}

ChooseGameTypeGameState::ChooseGameTypeGameState(int client_player) : GameState(client_player) {
	this->choosegametypePanel = NULL;
}

ChooseGameTypeGameState::~ChooseGameTypeGameState() {
	LOG("~ChooseGameTypeGameState()\n");
	if( this->choosegametypePanel )
		delete choosegametypePanel;
	LOG("~ChooseGameTypeGameState() done\n");
}

ChooseGameTypePanel *ChooseGameTypeGameState::getChooseGameTypePanel() {
	return this->choosegametypePanel;
}

void ChooseGameTypeGameState::reset() {
    //LOG("ChooseGameTypeGameState::reset()\n");
	this->screen_page->free(true);

	if( this->choosegametypePanel != NULL ) {
		delete this->choosegametypePanel;
		this->choosegametypePanel = NULL;
	}
	this->choosegametypePanel = new ChooseGameTypePanel();
}

void ChooseGameTypeGameState::draw() {
#if defined(__ANDROID__)
	screen->clear(); // SDL on Android requires screen be cleared (otherwise we get corrupt regions outside of the main area)
#endif
	background->draw(0, 0);

	this->choosegametypePanel->draw();

	this->screen_page->draw();
	//this->screen_page->drawPopups();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void ChooseGameTypeGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	this->choosegametypePanel->input(m_x, m_y, m_left, m_middle, m_right, click);
}

ChooseDifficultyGameState::ChooseDifficultyGameState(int client_player) : GameState(client_player) {
	this->choosedifficultyPanel = NULL;
}

ChooseDifficultyGameState::~ChooseDifficultyGameState() {
	LOG("~ChooseDifficultyGameState()\n");
	if( this->choosedifficultyPanel )
		delete choosedifficultyPanel;
	LOG("~ChooseDifficultyGameState() done\n");
}

ChooseDifficultyPanel *ChooseDifficultyGameState::getChooseDifficultyPanel() {
	return this->choosedifficultyPanel;
}

void ChooseDifficultyGameState::reset() {
    //LOG("ChooseDifficultyGameState::reset()\n");
	this->screen_page->free(true);

	if( this->choosedifficultyPanel != NULL ) {
		delete this->choosedifficultyPanel;
		this->choosedifficultyPanel = NULL;
	}
	this->choosedifficultyPanel = new ChooseDifficultyPanel();
}

void ChooseDifficultyGameState::draw() {
#if defined(__ANDROID__)
	screen->clear(); // SDL on Android requires screen be cleared (otherwise we get corrupt regions outside of the main area)
#endif
	background->draw(0, 0);

	this->choosedifficultyPanel->draw();

	this->screen_page->draw();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void ChooseDifficultyGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	this->choosedifficultyPanel->input(m_x, m_y, m_left, m_middle, m_right, click);
}

ChoosePlayerGameState::ChoosePlayerGameState(int client_player) : GameState(client_player), button_red(NULL), button_yellow(NULL), button_green(NULL), button_blue(NULL) {
}

ChoosePlayerGameState::~ChoosePlayerGameState() {
	LOG("~ChoosePlayerGameState()\n");
	LOG("~ChoosePlayerGameState() done\n");
}

void ChoosePlayerGameState::reset() {
	this->screen_page->free(true);

	const int xpos = 64;
	int ypos = 64;
	const int ydiff = 48;

	button_red = new Button(xpos, ypos, "CONTROLLER OF THE RED PEOPLE", letters_large);
	ypos += ydiff;
	screen_page->add(button_red);

	button_yellow = new Button(xpos, ypos, "CONTROLLER OF THE YELLOW PEOPLE", letters_large);
	ypos += ydiff;
	screen_page->add(button_yellow);

	button_green = new Button(xpos, ypos, "CONTROLLER OF THE GREEN PEOPLE", letters_large);
	ypos += ydiff;
	screen_page->add(button_green);

	button_blue = new Button(xpos, ypos, "CONTROLLER OF THE BLUE PEOPLE", letters_large);
	ypos += ydiff;
	screen_page->add(button_blue);
}

void ChoosePlayerGameState::draw() {
#if defined(__ANDROID__)
	screen->clear(); // SDL on Android requires screen be cleared (otherwise we get corrupt regions outside of the main area)
#endif
	//player_select->draw(0, 0, false);
	background->draw(0, 0);
    Image::writeMixedCase(160, 16, letters_large, letters_small, NULL, "Select a Player", Image::JUSTIFY_CENTRE);

	this->screen_page->draw();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

PlaceMenGameState::PlaceMenGameState(int client_player) : GameState(client_player), start_map_x(-1), start_map_y(-1) {
	this->off_x = (int)(0.25 * default_width_c);
	this->choosemenPanel = NULL;
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			map_panels[x][y] = NULL;
		}
	}
}

PlaceMenGameState::~PlaceMenGameState() {
	LOG("~PlaceMenGameState()\n");
	if( this->choosemenPanel )
		delete choosemenPanel;
	LOG("~PlaceMenGameState() done\n");
}

ChooseMenPanel *PlaceMenGameState::getChooseMenPanel() {
	return this->choosemenPanel;
}

const PanelPage *PlaceMenGameState::getMapPanel(int x, int y) const {
	ASSERT( x >= 0 && x < map_width_c );
	ASSERT( y >= 0 && y < map_height_c );
	return this->map_panels[x][y];
}

PanelPage *PlaceMenGameState::getMapPanel(int x, int y) {
	ASSERT( x >= 0 && x < map_width_c );
	ASSERT( y >= 0 && y < map_height_c );
	return this->map_panels[x][y];
}

void PlaceMenGameState::reset() {
    //LOG("PlaceMenGameState::reset()\n");
	/*if( !_CrtCheckMemory() ) {
		throw "_CrtCheckMemory FAILED";
	}*/
	this->screen_page->free(true);

	if( this->choosemenPanel != NULL ) {
		delete this->choosemenPanel;
		this->choosemenPanel = NULL;
	}

	this->choosemenPanel = new ChooseMenPanel(this);

	// setup screen_page buttons
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			map_panels[x][y] = NULL;
			if( getMap()->isSectorAt(x, y) ) {
				//int map_x = offset_map_x_c + 16 * x;
				int map_x = this->off_x - 8 * map_width_c + 16 * x;
				int map_y = offset_map_y_c + 16 * y;
				PanelPage *panel = new PanelPage(map_x, map_y, 16, 16);
				panel->setInfoLMB("place starting tower\nin this sector");
				panel->setEnabled(false);
				screen_page->add(panel);
				map_panels[x][y] = panel;
			}
		}
	}
	/*if( !_CrtCheckMemory() ) {
		throw "_CrtCheckMemory FAILED";
	}*/
}

void PlaceMenGameState::draw() {
	char buffer[256] = "";

#if defined(__ANDROID__)
	screen->clear(); // SDL on Android requires screen be cleared (otherwise we get corrupt regions outside of the main area)
#endif
	background_islands->draw(0, 0);

	if( !using_old_gfx ) {
		sprintf(buffer, "Gigalomania v%d.%d", majorVersion, minorVersion);
	    Image::writeMixedCase(160, 228, letters_large, letters_small, numbers_white, buffer, Image::JUSTIFY_CENTRE);
	}

    /*this->choosemenPanel->draw();
    this->screen_page->draw();
    GameState::draw();
    return;*/

    int l_h = letters_large[0]->getScaledHeight();
	int s_h = letters_small[0]->getScaledHeight();
	const int cx = this->off_x;
    //int cy = 100;
    int cy = 120;
    Image::writeMixedCase(cx, cy, letters_large, letters_small, NULL, map->getName(), Image::JUSTIFY_CENTRE);
	cy += s_h + 2;
	Image::writeMixedCase(cx, cy, letters_large, letters_small, NULL, "of the", Image::JUSTIFY_CENTRE);
	cy += l_h + 2;
	sprintf(buffer, "%s AGE", epoch_names[start_epoch]);
	Image::writeMixedCase(cx, cy, letters_large, letters_small, NULL, buffer, Image::JUSTIFY_CENTRE);
    cy += l_h + 2;

	int year = epoch_dates[start_epoch];
	bool shiny = start_epoch == n_epochs_c-1;
	Image::writeNumbers(cx+8, cy, shiny ? numbers_largeshiny : numbers_largegrey, abs(year),Image::JUSTIFY_RIGHT);
	Image *era = ( year < 0 ) ? icon_bc :
		shiny ? icon_ad_shiny : icon_ad;
	era->draw(cx+8, cy);
    cy += l_h + 2;

    if( !isDemo() && gameType == GAMETYPE_ALLISLANDS ) {
		int n_suspended = getNSuspended();
        if( n_suspended > 0 )
		{
			sprintf(buffer, "Saved Men %d", n_suspended);
            Image::writeMixedCase(cx, cy, letters_large, letters_small, numbers_white, buffer, Image::JUSTIFY_CENTRE);
		}
	}

    /*cy += l_h + 2;
	cy += l_h + 2;
	if( choosemenPanel->getPage() == ChooseMenPanel::STATE_LOADGAME ) {
		Image::write(cx, cy, letters_large, "LOAD", Image::JUSTIFY_CENTRE, true);
	}
	else if( choosemenPanel->getPage() == ChooseMenPanel::STATE_SAVEGAME ) {
		Image::write(cx, cy, letters_large, "SAVE", Image::JUSTIFY_CENTRE, true);
	}
    cy += s_h + 2;*/

	if( choosemenPanel->getPage() == ChooseMenPanel::STATE_CHOOSEMEN ) {
		cy = 60;
		Image::writeMixedCase(200, cy, letters_large, letters_small, NULL, "Click on the icon below", Image::JUSTIFY_CENTRE);
		cy += l_h + 2;
		Image::writeMixedCase(200, cy, letters_large, letters_small, NULL, "to choose how many men", Image::JUSTIFY_CENTRE);
		cy += l_h + 2;
		Image::writeMixedCase(200, cy, letters_large, letters_small, NULL, "to play with", Image::JUSTIFY_CENTRE);
		cy += l_h + 2;
		Image::writeMixedCase(200, cy, letters_large, letters_small, NULL, "then click on the map", Image::JUSTIFY_CENTRE);
		cy += l_h + 2;
		Image::writeMixedCase(200, cy, letters_large, letters_small, NULL, "to the left", Image::JUSTIFY_CENTRE);
		cy += l_h + 2;
	}

	// map
	/*for(int y=0;y<map_height_c;y++) {
	for(int x=0;x<map_width_c;x++) {
	if( map->sector_at[x][y] ) {
	int map_x = offset_map_x_c - 3 + 16 * x;
	int map_y = offset_map_y_c - 3 + 16 * y;
	map_sq[15]->draw(map_x, map_y, true);
	}
	}
	}*/
	//map->draw(offset_map_x_c, offset_map_y_c);
	map->draw(cx - 8*map_width_c, offset_map_y_c);

	this->choosemenPanel->draw();
	//this->choosemenPanel->drawPopups();

    this->screen_page->draw();
	//this->screen_page->drawPopups();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void PlaceMenGameState::requestNewGame() {
	if( confirm_window != NULL ) {
		this->closeConfirmWindow();
	}
	confirm_window = new PanelPage(120, 120, 64, 32);
	confirm_type = CONFIRMTYPE_NEWGAME;
	Button *text_button = new Button(0, 0, "NEW GAME", letters_small);
	confirm_window->add(text_button);
	confirm_yes_button = new Button(0, 16, "YES", letters_small);
	confirm_window->add(confirm_yes_button);
	confirm_no_button = new Button(32, 16, "NO", letters_small);
	confirm_window->add(confirm_no_button);
	this->screen_page->add(confirm_window);
}

void PlaceMenGameState::requestQuit() {
	if( choosemenPanel->getPage() == ChooseMenPanel::STATE_CHOOSEMEN && !state_changed ) {
		choosemenPanel->setPage(ChooseMenPanel::STATE_CHOOSEISLAND);
	}
	else {
		this->createQuitWindow();
	}
}


PlayingGameState::PlayingGameState(int client_player) : GameState(client_player) {
	this->current_sector = NULL;
	this->flag_frame_step = 0;
	this->defenders_last_frame_update = 0;
	this->soldiers_last_time_moved_x = 0;
	this->soldiers_last_time_moved_y = 0;
	this->soldiers_last_time_turned = 0;
	this->air_last_time_moved = 0;

	this->text_effect = NULL;
	this->speed_button = NULL;
	for(int i=0;i<n_players_c;i++) {
		this->shield_buttons[i] = NULL;
		this->shield_number_panels[i] = NULL;
	}
	this->shield_blank_button = NULL;
	this->land_panel = NULL;
	this->pause_button = NULL;
	this->quit_button = NULL;
	this->gamePanel = NULL;
	this->selected_army = NULL;
	this->map_display = MAPDISPLAY_MAP;
	//this->map_display = MAPDISPLAY_UNITS;
	this->player_asking_alliance = -1;
	/*for(i=0;i<n_players_c;i++)
	this->n_soldiers[i] = 0;*/
	for(int i=0;i<n_players_c;i++)
		for(int j=0;j<=n_epochs_c;j++)
			this->n_deaths[i][j] = 0;
	//this->refreshSoldiers(false);
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			map_panels[x][y] = NULL;
		}
	}
	alliance_yes = NULL;
	alliance_no = NULL;
}

PlayingGameState::~PlayingGameState() {
	LOG("~PlayingGameState()\n");
	for(size_t i=0;i<effects.size();i++) {
		TimedEffect *effect = effects.at(i);
		delete effect;
	}
	for(size_t i=0;i<ammo_effects.size();i++) {
		TimedEffect *effect = ammo_effects.at(i);
		delete effect;
	}
	if( text_effect != NULL ) {
		delete text_effect;
	}
	if( this->gamePanel )
		delete gamePanel;
	LOG("~PlayingGameState() done\n");
}

bool PlayingGameState::readSectorsProcessLine(Map *map, char *line, bool *done_header, int *sec_x, int *sec_y) {
	bool ok = true;
	line[ strlen(line) - 1 ] = '\0'; // trim new line
	line[ strlen(line) - 1 ] = '\0'; // trim carriage return
	if( !(*done_header) ) {
		if( line[0] != '#' ) {
			LOG("expected first character to be '#'\n");
			ok = false;
			return ok;
		}
		// ignore rest of header
		*done_header = true;
	}
	else {
		char *line_ptr = line;
		while( *line_ptr == ' ' || *line_ptr == '\t' ) // avoid initial whitespace
			line_ptr++;
		char *comment = strchr(line_ptr, '#');
		if( comment != NULL ) { // trim comments
			*comment = '\0';
		}
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
			*sec_x = atoi(ptr);
			if( *sec_x < 0 || *sec_x >= map_width_c ) {
				LOG("invalid map x %d\n", *sec_x);
				ok = false;
				return ok;
			}

			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find sec_y\n");
				ok = false;
				return ok;
			}
			*sec_y = atoi(ptr);
			if( *sec_y < 0 || *sec_y >= map_height_c ) {
				LOG("invalid map y %d\n", *sec_y);
				ok = false;
				return ok;
			}
		}
		else if( strcmp(ptr, "ELEMENT") == 0 ) {
			if( *sec_x == -1 || *sec_y == -1 ) {
				LOG("sector not defined\n");
				ok = false;
				return ok;
			}
			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find element name\n");
				ok = false;
				return ok;
			}
			/*char elementname[MAX_LINE+1] = "";
			strcpy(elementname, ptr);*/
			string elementname = ptr;

			ptr = strtok(NULL, " ");
			if( ptr == NULL ) {
				LOG("can't find n_elements\n");
				ok = false;
				return ok;
			}
			int n_elements = atoi(ptr);

			Id element = UNDEFINED;
			for(int i=0;i<N_ID;i++) {
				if( strcmp( elements[i]->getName(), elementname.c_str() ) == 0 ) {
					element = (Id)i;
				}
			}
			if( element == UNDEFINED ) {
				LOG("unknown element: %s\n", elementname.c_str());
				ok = false;
				return ok;
			}

			map->getSector(*sec_x, *sec_y)->setElements(element, n_elements);
		}
		else {
			LOG("unknown word: %s\n", ptr);
			ok = false;
			return ok;
		}
	}
	return ok;
}

bool PlayingGameState::readSectors(Map *map) {
	bool ok = true;
	const int MAX_LINE = 4096;
	char line[MAX_LINE+1] = "";
	int sec_x = -1, sec_y = -1;
	bool done_header = false;

    char fullname[4096] = "";
	sprintf(fullname, "%s/%s", maps_dirname, map->getFilename());
	// open in binary mode, so that we parse files in an OS-independent manner
	// (otherwise, Windows will parse "\r\n" as being "\n", but Linux will still read it as "\n")
	//FILE *file = fopen(fullname, "rb");
	SDL_RWops *file = SDL_RWFromFile(fullname, "rb");
#if !defined(__ANDROID__) && defined(__linux)
	if( file == NULL ) {
		LOG("searching in /usr/share/gigalomania/ for islands folder\n");
		sprintf(fullname, "%s/%s", alt_maps_dirname, map->getFilename());
		file = SDL_RWFromFile(fullname, "rb");
	}
#endif
	if( file == NULL ) {
		LOG("failed to open file: %s\n", fullname);
		//perror("perror returns: ");
		return false;
	}
	char buffer[MAX_LINE+1] = "";
	int buffer_offset = 0;
	bool reached_end = false;
	int newline_index = 0;
	while( ok ) {
		bool done = readLineFromRWOps(ok, file, buffer, line, MAX_LINE, buffer_offset, newline_index, reached_end);
		if( !ok )  {
			LOG("failed to read line\n");
		}
		else if( done ) {
			break;
		}
		else {
			ok = readSectorsProcessLine(map, line, &done_header, &sec_x, &sec_y);
		}
	}
	file->close(file);

	return ok;
}

void PlayingGameState::createSectors(int x, int y, int n_men) {
	LOG("PlayingGameState::createSectors(%d, %d, %d)\n", x, y, n_men);

	map->createSectors(this, start_epoch);
	Sector *sector = map->getSector(x, y);
	current_sector = sector;
	if( !isDemo() ) {
		sector->createTower(client_player, n_men);
	}
	//current_sector->createTower(human_player, 10);

	//current_sector->getArmy(enemy_player)->soldiers[10] = 0;
	//map->sectors[2][2]->getArmy(enemy_player)->soldiers[0] = 10;

	//Sector *enemy_sector = map->sectors[1][2];

	for(int i=0;i<n_players_c;i++) {
		if( i == client_player || players[i] == NULL )
			continue;
		int ex = 0, ey = 0;
		while( true ) {
			map->findRandomSector(&ex, &ey);
			ASSERT( map->isSectorAt(ex, ey) );
			if( map->getSector(ex, ey)->getPlayer() == -1 && !map->isReserved(ex, ey) )
				break;
		}
		//Sector *enemy_sector = map->sectors[ex][ey];
		Sector *enemy_sector = map->getSector(ex, ey);
		//enemy_sector->getArmy(enemy_player)->soldiers[10] = 30;
		//enemy_sector->createTower(enemy_player, 12);
		if( start_epoch == end_epoch_c ) {
			//players[i]->n_men_for_this_island = n_suspended[i];
			players[i]->setNMenForThisIsland(100);
		}
		else {
			players[i]->setNMenForThisIsland(20 + 5*start_epoch);
			// total: 360*3 + 65 = 1145 men
		}
		LOG("Enemy %d created at %d , %d\n", i, ex, ey);
		enemy_sector->createTower(i, players[i]->getNMenForThisIsland());
		//enemy_sector->createTower(enemy_player, 20);
		//enemy_sector->createTower(enemy_player, 200);
	}

	if( !readSectors(map) ) {
		LOG("failed to read map sector info!\n");
		ASSERT(false);
	}
}

GamePanel *PlayingGameState::getGamePanel() {
	return this->gamePanel;
}

/*Sector *PlayingGameState::getCurrentSector() {
	return current_sector;
}*/

const Sector *PlayingGameState::getCurrentSector() const {
	return current_sector;
}

bool PlayingGameState::viewingActiveClientSector() const {
	return this->getCurrentSector()->getActivePlayer() == client_player;
}

bool PlayingGameState::viewingAnyClientSector() const {
	// includes shutdown sectors
	return this->getCurrentSector()->getPlayer() == client_player;
}

bool PlayingGameState::openPitMine() {
	if( current_sector->getActivePlayer() != -1 && current_sector->getBuilding(BUILDING_MINE) == NULL && current_sector->getEpoch() >= 1 ) {
		for(int i=0;i<N_ID;i++) {
			if( current_sector->anyElements((Id)i) ) {
				Element *element = ::elements[i];
				if( element->getType() == Element::OPENPITMINE )
					return true;
			}
		}
	}
	return false;
}

void PlayingGameState::setupMapGUI() {
	if( alliance_yes != NULL ) {
		delete alliance_yes;
		alliance_yes = NULL;
	}
	if( alliance_no != NULL ) {
		delete alliance_no;
		alliance_no = NULL;
	}
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			if( map_panels[x][y] != NULL ) {
				delete map_panels[x][y];
				map_panels[x][y] = NULL;
			}
		}
	}
	if( this->player_asking_alliance != -1 ) {
		alliance_yes = new Button(16, 64, "YES", letters_small);
		alliance_yes->setInfoLMB("join the alliance");
		screen_page->add(alliance_yes);
		alliance_no = new Button(48, 64, "NO", letters_small);
		alliance_no->setInfoLMB("refuse the alliance");
		screen_page->add(alliance_no);
	}
	else if( this->map_display == MAPDISPLAY_MAP ) {
		for(int y=0;y<map_height_c;y++) {
			for(int x=0;x<map_width_c;x++) {
				if( map->isSectorAt(x, y) ) {
					int map_x = offset_map_x_c + 16 * x;
					int map_y = offset_map_y_c + 16 * y;
					PanelPage *panel = new PanelPage(map_x, map_y, 16, 16);
					panel->setTolerance(0); // since map sectors are aligned, better for touchscreens not to use the "tolerance"
					panel->setInfoLMB("view this sector");
					screen_page->add(panel);
					//map->panels[x][y] = panel;
					map_panels[x][y] = panel;
				}
			}
		}
	}
}

void PlayingGameState::reset() {
    //LOG("PlayingGameState::reset()\n");
	if( current_sector == NULL )
		return;

	this->screen_page->free(true);
	alliance_yes = NULL;
	alliance_no = NULL;
	for(int y=0;y<map_height_c;y++) {
		for(int x=0;x<map_width_c;x++) {
			map_panels[x][y] = NULL;
		}
	}
	confirm_type = CONFIRMTYPE_UNKNOWN;
	confirm_window = NULL;
	confirm_yes_button = NULL;
	confirm_no_button = NULL;

	if( this->gamePanel != NULL ) {
		delete this->gamePanel;
		this->gamePanel = NULL;
	}

	this->land_panel = new PanelPage(offset_land_x_c, offset_land_y_c, default_width_c - offset_land_x_c, default_height_c - offset_land_y_c - quit_button_offset_c);
	screen_page->add(this->land_panel);

	// setup screen_page buttons
	this->setupMapGUI();

	if( !isDemo() ) {
		speed_button = new ImageButton(offset_map_x_c + 16 * map_width_c + 4, 4, icon_speeds[time_rate-1]);
		if( onemousebutton ) {
			speed_button->setInfoLMB("cycle through different time rates");
		}
		else {
			speed_button->setInfoLMB("decrease the rate of time");
			speed_button->setInfoRMB("increase the rate of time");
		}
		screen_page->add(speed_button);

		//if( mobile_ui )
		{
			pause_button = new Button(default_width_c - 80, default_height_c - quit_button_offset_c, "PAUSE", letters_small);
			screen_page->add(pause_button);
			quit_button = new Button(default_width_c - 32, default_height_c - quit_button_offset_c, "QUIT", letters_small);
			screen_page->add(quit_button);
		}
	}

	for(int i=0;i<n_players_c;i++) {
		shield_buttons[i] = NULL;
		shield_number_panels[i] = NULL;
	}
	this->shield_blank_button = NULL;

	resetShieldButtons();

	for(int i=0;i<N_BUILDINGS;i++) {
		Building *building = current_sector->getBuilding((Type)i);
		if( building != NULL ) {
			addBuilding(building);
		}
	}

	// must be done after creating shield_number_panels
	this->refreshSoldiers(false);

	/*if( smokeParticleSystem != NULL ) {
		delete smokeParticleSystem;
	}
	Building *building_factory = current_sector->getBuilding(BUILDING_FACTORY);
	//Building *building_factory = current_sector->getBuilding(BUILDING_TOWER);
	if( building_factory != NULL ) {
		//smokeParticleSystem = new SmokeParticleSystem(smoke_image, offset_land_x_c + building_factory->getX(), offset_land_y_c + building_factory->getY());
		smokeParticleSystem = new SmokeParticleSystem(smoke_image, offset_land_x_c + building_factory->getX() + 20, offset_land_y_c + building_factory->getY());
	}*/
	/*if( smokeParticleSystem == NULL ) {
		smokeParticleSystem = new SmokeParticleSystem(smoke_image);
	}*/

	this->gamePanel = new GamePanel(this, this->client_player);
	// must call setup last, in case it recalls member functions of PlayingGameState, that requires the buttons to have been initialised
	this->gamePanel->setup();

	if( LOGGING ) {
		current_sector->printDebugInfo();
	}
}

void PlayingGameState::resetShieldButtons() {
	bool done_shield[n_players_c];
	for(int i=0;i<n_players_c;i++)
		done_shield[i] = false;
	for(int i=0;i<n_players_c;i++) {
		if( shield_buttons[i] != NULL ) {
			screen_page->remove(shield_buttons[i]);
			delete shield_buttons[i];
			shield_buttons[i] = NULL;
		}
		if( shield_number_panels[i] != NULL ) {
			screen_page->remove(shield_number_panels[i]);
			delete shield_number_panels[i];
			shield_number_panels[i] = NULL;
		}
	}
	if( shield_blank_button != NULL ) {
		screen_page->remove(shield_blank_button);
		delete shield_blank_button;
		shield_blank_button = NULL;
	}
	int n_sides = 0;
	for(int i=0;i<n_players_c;i++) {
		if( !done_shield[i] && players[i] != NULL && !players[i]->isDead() ) {
			bool allied[n_players_c];
			for(int j=0;j<n_players_c;j++)
				allied[j] = false;
			allied[i] = true;
			done_shield[i] = true;
			for(int j=i+1;j<n_players_c;j++) {
				if( Player::isAlliance(i, j) ) {
					ASSERT( players[j] != NULL );
					ASSERT( !players[j]->isDead() );
					allied[j] = true;
					done_shield[j] = true;
				}
			}
			n_sides++;
			shield_buttons[i] = new ImageButton(offset_map_x_c + 16 * map_width_c + 4, offset_map_y_c + shield_step_y_c * i + 8, playershields[ Player::getShieldIndex(allied) ]);
			screen_page->add(shield_buttons[i]);
			//shield_number_panels[i] = new PanelPage(offset_map_x_c + 16 * map_width_c + 4 + 16, offset_map_y_c + shield_step_y_c * i + 8, 20, 10);
			shield_number_panels[i] = new PanelPage(offset_map_x_c + 16 * map_width_c + 4 + 16, offset_map_y_c + shield_step_y_c * i + 8, 20, shield_step_y_c);
			screen_page->add(shield_number_panels[i]);
		}
	}
	if( !isDemo() && n_sides > 2 ) {
		for(int i=0;i<n_players_c;i++) {
			if( shield_buttons[i] != NULL && i != client_player && !Player::isAlliance(i, client_player) ) {
				shield_buttons[i]->setInfoLMB("make an alliance");
			}
		}
	}
	bool any_alliances = false;
	for(int i=0;i<n_players_c && !any_alliances && !isDemo();i++) {
		if( i != client_player && Player::isAlliance(i, client_player) ) {
			any_alliances = true;
			ASSERT( players[i] != NULL );
			ASSERT( !players[i]->isDead() );
		}
	}
	if( any_alliances ) {
		shield_blank_button = new ImageButton(offset_map_x_c + 16 * map_width_c + 4, offset_map_y_c + 4*shield_step_y_c + 8, playershields[0]);
		shield_blank_button->setInfoLMB("break current alliance");
		screen_page->add(shield_blank_button);
	}

	refreshShieldNumberPanels();
}

void PlayingGameState::addBuilding(Building *building) {
	for(int j=0;j<building->getNTurrets();j++) {
		screen_page->add(building->getTurretButton(j));
	}
	if( building->getBuildingButton() != NULL )
		screen_page->add(building->getBuildingButton());
}

void PlayingGameState::setFlashingSquare(int xpos,int ypos) {
	//this->flashingSquare.set(xpos, ypos);
	if( this->player_asking_alliance == -1 && map_display == MAPDISPLAY_MAP ) {
		FlashingSquare *square = new FlashingSquare(xpos, ypos);
		//square->set(xpos, ypos);
		//this->effects->add(square);
		this->effects.push_back(square);
	}
};

void GameState::fadeScreen(bool out, int delay, void (*func_finish)()) {
    if( fade != NULL )
        delete fade;
	if( is_testing ) {
		if( func_finish != NULL ) {
			func_finish();
		}
	}
	else {
	    fade = new FadeEffect(false, out, delay, func_finish);
	}
}

void GameState::whiteFlash() {
	//ASSERT( whitefade == NULL );
    if( whitefade != NULL )
        delete whitefade;
	if( !is_testing ) {
	    whitefade = new FadeEffect(true, false, 0, NULL);
	}
}

void PlayingGameState::draw() {
#if defined(__ANDROID__)
	screen->clear(); // SDL on Android requires screen be cleared (otherwise we get corrupt regions outside of the main area)
#endif

	background->draw(0, 0);
	//background->draw(0, 0, true);

	bool no_armies = true;
	for(int i=0;i<n_players_c && no_armies;i++) {
		const Army *army = current_sector->getArmy(i);
		if( army->getTotal() > 0 )  {
			no_armies = false;
		}
	}
	if( no_armies && this->map_display == MAPDISPLAY_UNITS ) {
		this->map_display = MAPDISPLAY_MAP;
		this->reset();
	}

	if( this->player_asking_alliance != -1 ) {
		// ask alliance
		stringstream str;
		str << PlayerType::getName((PlayerType::PlayerTypeID)player_asking_alliance);
		Image::write(offset_map_x_c + 8, offset_map_y_c + 8, letters_small, str.str().c_str(), Image::JUSTIFY_LEFT);
		str.str("asks for an");
		Image::write(offset_map_x_c + 8, offset_map_y_c + 16, letters_small, str.str().c_str(), Image::JUSTIFY_LEFT);
		str.str("alliance");
		Image::write(offset_map_x_c + 8, offset_map_y_c + 24, letters_small, str.str().c_str(), Image::JUSTIFY_LEFT);
	}
	else if( this->map_display == MAPDISPLAY_MAP ) {
		// map

		map->draw(offset_map_x_c, offset_map_y_c);
		for(int y=0;y<map_height_c;y++) {
			for(int x=0;x<map_width_c;x++) {
				if( map->getSector(x, y) != NULL ) {
					int map_x = offset_map_x_c + 16 * x;
					int map_y = offset_map_y_c + 16 * y;
					//map_sq[15]->draw(map_x, map_y, true);
					if( map->getSector(x, y)->getPlayer() != -1 ) {
						icon_towers[ map->getSector(x, y)->getPlayer() ]->draw(map_x + 5, map_y + 5);
					}
					else if( map->getSector(x, y)->isNuked() ) {
						icon_nuke_hole->draw(map_x + 4, map_y + 4);
					}
					for(int i=0;i<n_players_c;i++) {
						Army *army = map->getSector(x, y)->getArmy(i);
						int n_army = army->getTotal();
						if( n_army > 0 ) {
							int off_step = 5;
							int off_step_x = ( i == 0 || i == 2 ) ? -off_step : off_step;
							int off_step_y = ( i == 0 || i == 1 ) ? -off_step : off_step;
							icon_armies[i]->draw(map_x + 6 + off_step_x, map_y + 6 + off_step_y);
						}
					}
				}
			}
		}
		int map_x = offset_map_x_c + 16 * current_sector->getXPos();
		int map_y = offset_map_y_c + 16 * current_sector->getYPos();
		mapsquare->draw(map_x, map_y);
	}
	else if( this->map_display == MAPDISPLAY_UNITS ) {
		// unit stats
		const int gap = 18;
		const int extra = 0;
		for(int i=0;i<=n_sub_epochs;i++) {
			Image *image = (i==0) ? unarmed_man : numbered_weapons[start_epoch + i - 1];
			//Image *image = (i==0) ? men[start_epoch] : numbered_weapons[start_epoch + i - 1];
			image->draw(offset_map_x_c + gap * i + extra, offset_map_y_c + 2 - 16 + 8);
		}
		for(int i=0;i<n_players_c;i++) {
			if( shield_buttons[i] == NULL ) {
				continue;
			}
			int off = 0;
			for(int j=i;j<n_players_c;j++) {
				if( j == i || Player::isAlliance(i, j) ) {
					const Army *army = current_sector->getArmy(j);
					if( army->getTotal() > 0 ) {
						for(int k=0;k<=n_sub_epochs;k++) {
							int idx = (k==0) ? 10 : start_epoch + k - 1;
							int n_men = army->getSoldiers(idx);
							if( n_men > 0 ) {
								//Image::writeNumbers(offset_map_x_c + 16 * k + 4, offset_map_y_c + 2 + 16 * i + 8 * off + 8, numbers_small[j], n_men, Image::JUSTIFY_LEFT, true);
								Image::writeNumbers(offset_map_x_c + gap * k + extra, offset_map_y_c + 2 + 16 * i + 8 * off + 8, numbers_small[j], n_men, Image::JUSTIFY_LEFT);
							}
						}
						off++;
					}
				}
			}
		}
	}

	// land area
	land[map->getColour()]->draw(offset_land_x_c, offset_land_y_c);

	// trees etc (not at front)
	for(int i=0;i<current_sector->getNFeatures();i++) {
		const Feature *feature = current_sector->getFeature(i);
		/*const int tree_ticks_per_frame_c = 110; // tree animation looks better if offset from main animation, and if slightly slower
		int tree_counter = getRealTime() / tree_ticks_per_frame_c;
		const Image *image = current_sector->isTreesNuked() ? icon_trees[2][0] : feature->getImage(tree_counter);
		image->draw(feature->getX(), feature->getY(), true);*/
		if( !feature->isAtFront() ) {
			feature->draw();
		}
	}

	if( current_sector->getActivePlayer() != -1 )
	{
		if( openPitMine() )
			icon_openpitmine->draw(offset_land_x_c + offset_openpitmine_x_c, offset_land_y_c + offset_openpitmine_y_c);

		bool rotate_defenders = false;
		if( frame_counter - defenders_last_frame_update > defenders_frames_per_update_c ) {
			rotate_defenders = true;
			defenders_last_frame_update = frame_counter;
		}

		for(int i=0;i<N_BUILDINGS;i++) {
			Building *building = current_sector->getBuilding((Type)i);
			if( building == NULL )
				continue;

			// draw building
			Image **images = building->getImages();
			images[ current_sector->getBuildingEpoch() ]->draw(offset_land_x_c + building->getX(), offset_land_y_c + building->getY());

			if( rotate_defenders )
				building->rotateDefenders();

			// draw defenders
			for(int j=0;j<building->getNTurrets();j++) {
				if( building->getTurretMan(j) != -1 ) {
					Image *image = NULL;
					if( building->getTurretMan(j) == nuclear_epoch_c ) {
						image = nuke_defences[current_sector->getPlayer()];
					}
					else {
						image = defenders[current_sector->getPlayer()][ building->getTurretMan(j) ][ building->getTurretManDir(j) ];
					}
					image->draw(building->getTurretButton(j)->getLeft(), building->getTurretButton(j)->getTop() - 4);
				}
			}

			if( i == BUILDING_TOWER )
				flags[ current_sector->getPlayer() ][frame_counter % n_flag_frames_c]->draw(offset_land_x_c + building->getX() + offset_flag_x_c, offset_land_y_c + building->getY() + offset_flag_y_c);

			const int health_xpos = offset_land_x_c + building->getX() + 4;
			//const int health_ypos = offset_land_y_c + building->getY() - 16;
			const int health_ypos = offset_land_y_c + building->getY() + images[ current_sector->getBuildingEpoch() ]->getScaledHeight() + 2;
			const int health_width = 36;
			const int health_height = 4;
			/*float health = (health_width-2) * ((float)building->getHealth()) / (float)building->getMaxHealth();
			screen->fillRectWithAlpha(scale_width*health_xpos, scale_height*health_ypos, scale_width*health_width, scale_height*health_height, 255, 255, 255, 127);
			screen->fillRectWithAlpha(scale_width*(health_xpos+1), scale_height*(health_ypos+1), scale_width*(health_width-2), scale_height*(health_height-2), 0, 0, 0, 127);
			screen->fillRectWithAlpha(scale_width*(health_xpos+1), scale_height*(health_ypos+1), scale_width*health, scale_height*(health_height-2), 255, 0, 0, 127);*/
			float health = (scale_width*health_width-2) * ((float)building->getHealth()) / (float)building->getMaxHealth();
			screen->fillRect((short)(scale_width*health_xpos), (short)(scale_height*health_ypos), (short)(scale_width*health_width), (short)(scale_height*health_height), 255, 255, 255);
			screen->fillRect((short)(scale_width*health_xpos+1), (short)(scale_height*health_ypos+1), (short)(scale_width*health_width-2), (short)(scale_height*health_height-2), 0, 0, 0);
			screen->fillRect((short)(scale_width*health_xpos+1), (short)(scale_height*health_ypos+1), (short)health, (short)(scale_height*health_height-2), 255, 0, 0);
		}
	}
	else if( current_sector->getPlayer() != -1 ) {
		ASSERT( current_sector->isShutdown() );
		Building *building = current_sector->getBuilding(BUILDING_TOWER);
		ASSERT( building != NULL );
		Image **images = building->getImages();
		images[ current_sector->getBuildingEpoch() ]->draw(offset_land_x_c + building->getX(), offset_land_y_c + building->getY());
		flags[ current_sector->getPlayer() ][frame_counter % n_flag_frames_c]->draw(offset_land_x_c + building->getX() + offset_flag_x_c, offset_land_y_c + building->getY() + offset_flag_y_c);
	}

	//Vector soldier_list(n_players_c * 250);
	int n_total_soldiers = 0;
	for(int i=0;i<n_players_c;i++) {
		n_total_soldiers += soldiers[i].size();
	}
	Soldier **soldier_list = new Soldier *[n_total_soldiers];
	for(int i=0,c=0;i<n_players_c;i++) {
		//for(int j=0;j<n_soldiers[i];j++) {
		for(size_t j=0;j<soldiers[i].size();j++) {
			//Soldier *soldier = soldiers[i][j];
			//Soldier *soldier = (Soldier *)soldiers[i]->get(j);
			Soldier *soldier = soldiers[i].at(j);
			//soldier_list.add(soldier);
			soldier_list[c++] = soldier;
		}
	}
	//Soldier::sortSoldiers((Soldier **)soldier_list.getData(), soldier_list.size());
	Soldier::sortSoldiers(soldier_list, n_total_soldiers);
	// draw land units
	/*for(int i=0;i<soldier_list.size();i++) {
	Soldier *soldier = (Soldier *)soldier_list.elementAt(i);*/
	for(int i=0;i<n_total_soldiers;i++) {
		Soldier *soldier = soldier_list[i];
		ASSERT(soldier->epoch != nuclear_epoch_c);
		if( !isAirUnit(soldier->epoch) ) {
			//int frame = soldier->dir * 4 + ( frame_counter % 3 );
			//Image *image = attackers_walking[soldier->player][soldier->epoch][frame];
			int n_frames = n_attacker_frames[soldier->epoch][soldier->dir];
			Image *image = attackers_walking[soldier->player][soldier->epoch][soldier->dir][frame_counter % n_frames];
			image->draw(offset_land_x_c + soldier->xpos, offset_land_y_c + soldier->ypos);
		}
	}

	// trees etc (at front)
	for(int i=0;i<current_sector->getNFeatures();i++) {
		const Feature *feature = current_sector->getFeature(i);
		if( feature->isAtFront() ) {
			feature->draw();
		}
	}

	for(int i=effects.size()-1;i>=0;i--) {
		TimedEffect *effect = effects.at(i);
		if( effect->render() ) {
			effects.erase(effects.begin() + i);
			delete effect;
		}
	}
	for(int i=ammo_effects.size()-1;i>=0;i--) {
		TimedEffect *effect = ammo_effects.at(i);
		if( effect->render() ) {
			ammo_effects.erase(ammo_effects.begin() + i);
			delete effect;
		}
	}

	// draw air units
	/*for(int i=0;i<soldier_list.size();i++) {
	Soldier *soldier = (Soldier *)soldier_list.elementAt(i);*/
	for(int i=0;i<n_total_soldiers;i++) {
		Soldier *soldier = soldier_list[i];
		ASSERT(soldier->epoch != nuclear_epoch_c);
		if( isAirUnit(soldier->epoch) ) {
			Image *image = NULL;
			if( soldier->epoch == 6 || soldier->epoch == 7 ) {
				image = planes[soldier->player][soldier->epoch];
			}
			else if( soldier->epoch == 9 ) {
				int frame = frame_counter % 3;
				image = saucers[soldier->player][frame];
			}
			ASSERT(image != NULL);
			image->draw(offset_land_x_c + soldier->xpos, offset_land_y_c + soldier->ypos);
		}
	}
	delete [] soldier_list;

	// nuke
	int nuke_time = -1;
	int nuke_by_player = current_sector->beingNuked(&nuke_time);
	if( nuke_by_player != -1 ) {
		ASSERT( nuke_time != -1 );
		float alpha = ((float)( getGameTime() - nuke_time )) / (float)nuke_delay_c;
		ASSERT( alpha >= 0.0 );
		if( alpha > 1.0 )
			alpha = 1.0;
		int sx = 300, sy = 32;
		int ex = 128, ey = 144;
		int xpos = (int)(alpha * ex + (1.0 - alpha) * sx);
		int ypos = (int)(alpha * ey + (1.0 - alpha) * sy);
		nukes[nuke_by_player][1]->draw(xpos, ypos);
	}

	// playershields etc
	for(int i=0;i<n_players_c;i++) {
		if( players[i] != NULL && !players[i]->isDead() ) {
			//ASSERT( shield_buttons[i] != NULL );
			if( shield_buttons[i] == NULL ) {
				continue;
			}
			shield_number_panels[i]->setEnabled(false);
			/*int n_allied = 1;
			for(j=i+1;j<n_players_c;j++) {
			if( Player::isAlliance(i, j) ) {
			n_allied++;
			}
			}*/
			//playershields[ players[i]->getShieldIndex()  ]->draw(offset_map_x_c + 16 * map_width_c + 4, offset_map_y_c + shield_step_y_c * i + 8, true);
			int off = 0;
			for(int j=i;j<n_players_c;j++) {
				if( j == i || Player::isAlliance(i, j) ) {
					const Army *army = current_sector->getArmy(j);
					int n_army = army->getTotal();
					if( n_army > 0 ) {
						shield_number_panels[i]->setEnabled(true);
						//Image::writeNumbers(offset_map_x_c + 16 * map_width_c + 20, offset_map_y_c + 2 + shield_step_y_c * i + 8, numbers_small[i], n_army, Image::JUSTIFY_LEFT, true);
						Image::writeNumbers(offset_map_x_c + 16 * map_width_c + 20, offset_map_y_c + 2 + shield_step_y_c * i + 8 * off + 8, numbers_small[j], n_army, Image::JUSTIFY_LEFT);
						off++;
					}
				}
			}
		}
	}

	// panel
	this->gamePanel->draw();
	//this->gamePanel->drawPopups();

	this->screen_page->draw();
	//this->screen_page->drawPopups();

	/*if( smokeParticleSystem != NULL ) {
		const Building *building_factory = current_sector->getBuilding(BUILDING_FACTORY);
		//const Building *building_factory = current_sector->getBuilding(BUILDING_TOWER);
		if( building_factory != NULL ) {
			const SmokeParticleSystem *ps = ( current_sector->getWorkers() > 0 ) ? smokeParticleSystem_busy : smokeParticleSystem;
			ps->draw(offset_land_x_c + building_factory->getX() + 17, offset_land_y_c + building_factory->getY() + 2);
		}
	}*/
	if( current_sector->getParticleSystem() != NULL ) {
		const Building *building_factory = current_sector->getBuilding(BUILDING_FACTORY);
		//const Building *building_factory = current_sector->getBuilding(BUILDING_TOWER);
		if( building_factory != NULL ) {
			current_sector->getParticleSystem()->draw(offset_land_x_c + building_factory->getX() + 17, offset_land_y_c + building_factory->getY() + 2);
		}
	}

	if( text_effect != NULL && text_effect->render() ) {
		delete text_effect;
		text_effect = NULL;
	}

	// mouse pointer
	/*int m_x, m_y;
	SDL_GetMouseState(&m_x, &m_y);
	m_x /= scale_width;
	m_y /= scale_height;*/
	GameState::setDefaultMouseImage();
	mouse_off_x = 0;
	mouse_off_y = 0;
	if( gameStateID == GAMESTATEID_PLAYING ) {
		GamePanel::MouseState mousestate = gamePanel->getMouseState();
		if( mousestate == GamePanel::MOUSESTATE_DEPLOY_WEAPON || selected_army != NULL ) {
			ASSERT( mousestate != GamePanel::MOUSESTATE_DEPLOY_WEAPON || selected_army == NULL );
			bool bloody = false;
			const Sector *this_sector = ( selected_army == NULL ) ? current_sector : selected_army->getSector();
			if( this_sector->getPlayer() != client_player ) {
				if( this_sector->getPlayer() != PLAYER_NONE && !Player::isAlliance(this_sector->getPlayer(), client_player) )
					bloody = true;
				for(int i=0;i<n_players_c && !bloody;i++) {
					if( i != client_player && !Player::isAlliance(i, client_player) ) {
						if( this_sector->getArmy(i)->any(true) )
							bloody = true;
					}
				}
			}
			if( bloody )
				mouse_image = panel_bloody_attack;
			else
				mouse_image = panel_attack;
			mobile_ui_display_mouse = true;
		}
		else if( mousestate == GamePanel::MOUSESTATE_DEPLOY_DEFENCE ) {
			mouse_image = panel_defence;
			//m_x -= mouse_image->getScaledWidth() / 2;
			//m_y -= mouse_image->getScaledHeight() / 2;
			mouse_off_x = - mouse_image->getScaledWidth() / 2;
			mouse_off_y = - mouse_image->getScaledHeight() / 2;
			mobile_ui_display_mouse = true;
		}
		else if( mousestate == GamePanel::MOUSESTATE_DEPLOY_SHIELD ) {
			mouse_image = panel_shield;
			mobile_ui_display_mouse = true;
			//m_x -= mouse_image->getScaledWidth() / 2;
			//m_y -= mouse_image->getScaledHeight() / 2;
			mouse_off_x = - mouse_image->getScaledWidth() / 2;
			mouse_off_y = - mouse_image->getScaledHeight() / 2;
		}
		else if( mousestate == GamePanel::MOUSESTATE_SHUTDOWN ) {
			mouse_image = men[n_epochs_c-1];
			mouse_off_x = - mouse_image->getScaledWidth() / 2;
			mouse_off_y = - mouse_image->getScaledHeight() / 2;
			mobile_ui_display_mouse = true;
		}
	}

	GameState::draw();
}

void PlayingGameState::update() {
	/*if( this->smokeParticleSystem != NULL ) {
		if( current_sector->getWorkers() > 0 ) {
			this->smokeParticleSystem->setBirthRate(0.008f);
		}
		else {
			this->smokeParticleSystem->setBirthRate(0.004f);
		}
	}*/
	int move_soldiers_step_x = 0;
	int move_soldiers_step_y = 0;
	int move_air_step = 0;
	// move twice as fast in x direction, to simulate 3D look
	while( getGameTime() - soldiers_last_time_moved_x > soldier_move_rate_c ) {
		move_soldiers_step_x++;
		soldiers_last_time_moved_x += soldier_move_rate_c;
	}
	while( getGameTime() - soldiers_last_time_moved_y > 2*soldier_move_rate_c ) {
		move_soldiers_step_y++;
		soldiers_last_time_moved_y += 2*soldier_move_rate_c;
	}
	while( getGameTime() - air_last_time_moved > air_move_rate_c ) {
		move_air_step++;
		air_last_time_moved += air_move_rate_c;
	}
	/*bool move_soldiers = ( getGameTime() - soldiers_last_time_moved > soldier_move_rate_c );
	bool move_air = ( getGameTime() - air_last_time_moved > air_move_rate_c );
	if( move_soldiers )
	soldiers_last_time_moved = getGameTime();
	if( move_air )
	air_last_time_moved = getGameTime();*/
	int time_interval = getLoopTime();

	int n_armies = 0;
	for(int i=0;i<n_players_c;i++) {
		const Army *army = current_sector->getArmy(i);
		if( army->getTotal() > 0 )
			n_armies++;
	}
	bool combat = false;
	if( n_armies >= 2 || ( current_sector->getPlayer() != -1 && current_sector->enemiesPresent() ) ) {
		combat = true;
	}

	int fire_prob = poisson(soldier_turn_rate_c, time_interval);
	for(int i=0;i<n_players_c;i++) {
		//for(int j=0;j<n_soldiers[i];j++) {
		for(size_t j=0;j<soldiers[i].size();j++) {
			//Soldier *soldier = soldiers[i][j];
			//Soldier *soldier = (Soldier *)soldiers[i]->get(j);
			Soldier *soldier = soldiers[i].at(j);
			//if( soldier->epoch == 6 || soldier->epoch == 7 || soldier->epoch == 9 ) {
			if( isAirUnit(soldier->epoch) ) {
				// air unit
				if( move_air_step > 0 ) {
					soldier->xpos -= move_air_step;
					soldier->ypos -= move_air_step;
					if( soldier->xpos < -32 )
						soldier->xpos = land_width_c + 64;
					if( soldier->ypos < - offset_land_y_c - 32 )
						soldier->ypos = land_height_c + 64;
				}
				if( combat ) {
					int fire_random = rand() % RAND_MAX;
					if( fire_random <= fire_prob ) {
						// fire!
						AmmoEffect *ammoeffect = new AmmoEffect( this, soldier->epoch, ATTACKER_AMMO_BOMB, soldier->xpos + 4, soldier->ypos + 8 );
						//this->ammo_effects->add(ammoeffect);
						this->ammo_effects.push_back(ammoeffect);
					}
				}
			}
			else {
				if( !validSoldierLocation(soldier->epoch,soldier->xpos, soldier->ypos) ) {
					/* Soldier is already invalid location. This usually happens if the scenery suddenly
					* changes (eg, new building appearing). If this happens, find a new valid locaation.
					*/
					bool found_loc = false;
					while(!found_loc) {
						soldier->xpos = rand() % land_width_c;
						soldier->ypos = rand() % land_height_c;
						found_loc = validSoldierLocation(soldier->epoch, soldier->xpos, soldier->ypos);
					}
				}
				/* Turns are modelled as a Poisson distribution - so soldier_turn_rate_c is the mean number of
				* ticks that elapse per turn. Therefore we are interested in the probability that at least one
				* turn occured within this time interval.
				*/
				/*double prob = 1.0 - exp( - ((double)time_interval) / soldier_turn_rate_c );
				double random = ((double)( rand() % RAND_MAX )) / (double)RAND_MAX;*/
				//double prob = RAND_MAX * ( 1.0 - exp( - ((double)time_interval) / soldier_turn_rate_c ) );
				int prob = poisson(soldier_turn_rate_c, time_interval);
				int random = rand() % RAND_MAX;
				if( random <= prob ) {
					// turn!
					soldier->dir = (AmmoDirection)(rand() % 4);
				}
				int move_soldiers_step = (soldier->dir == 0 || soldier->dir == 1) ? move_soldiers_step_y : move_soldiers_step_x;
				if( move_soldiers_step > 0  ) {
					int step_x = 0;
					int step_y = 0;
					if( soldier->dir == 0 )
						step_y = move_soldiers_step;
					else if( soldier->dir == 1 )
						step_y = - move_soldiers_step;
					else if( soldier->dir == 2 )
						step_x = move_soldiers_step;
					else if( soldier->dir == 3 )
						step_x = - move_soldiers_step;

					int new_xpos = soldier->xpos + step_x;
					int new_ypos = soldier->ypos + step_y;
					if( !validSoldierLocation(soldier->epoch,new_xpos, new_ypos) ) {
						// path blocked, so turn around
						new_xpos = soldier->xpos;
						new_ypos = soldier->ypos;
						if( soldier->dir == 0 )
							soldier->dir = (AmmoDirection)1;
						else if( soldier->dir == 1 )
							soldier->dir = (AmmoDirection)0;
						else if( soldier->dir == 2 )
							soldier->dir = (AmmoDirection)3;
						else if( soldier->dir == 3 )
							soldier->dir = (AmmoDirection)2;
					}
					soldier->xpos = new_xpos;
					soldier->ypos = new_ypos;
				}

				if( combat && soldier->epoch != n_epochs_c ) {
					int fire_random = rand() % RAND_MAX;
					if( fire_random <= fire_prob ) {
						// fire!
						Image *image = attackers_walking[soldier->player][soldier->epoch][soldier->dir][0];
						AmmoEffect *ammoeffect = new AmmoEffect( this, soldier->epoch, soldier->dir, soldier->xpos + image->getScaledWidth()/2, soldier->ypos );
						//this->ammo_effects->add(ammoeffect);
						this->ammo_effects.push_back(ammoeffect);
					}
				}
			}
		}
	}
}

bool PlayingGameState::buildingMouseClick(int s_m_x,int s_m_y,bool m_left,bool m_right,Building *building) {
	bool done = false;
	if( building == NULL )
		return done;
	if( !m_left && !m_right )
		return done;

	//Image *base_image = building->getImages()[0];
	Image *base_image = building->getImages()[current_sector->getBuildingEpoch()];
	ASSERT( base_image != NULL );
	if( s_m_x < offset_land_x_c + building->getX() || s_m_x >= offset_land_x_c + building->getX() + base_image->getScaledWidth() ||
		s_m_y < offset_land_y_c + building->getY() || s_m_y >= offset_land_y_c + building->getY() + base_image->getScaledHeight() ) {
			return done;
	}

	if( !done && this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_SHUTDOWN && building->getType() == BUILDING_TOWER ) {
		//current_sector->shutdown();
		this->shutdown(current_sector->getXPos(), current_sector->getYPos());
		done = true;
	}

	for(int i=0;i<building->getNTurrets() && !done;i++) {
		if( m_left
			/*&& s_m_x >= offset_land_x_c + building->pos_x + building->turret_pos[i].x
			&& s_m_x < offset_land_x_c + building->pos_x + building->turret_pos[i].getRight()
			&& s_m_y >= offset_land_y_c + building->pos_y + building->turret_pos[i].y
			&& s_m_y < offset_land_y_c + building->pos_y + building->turret_pos[i].getBottom()*/
			&& s_m_x >= building->getTurretButton(i)->getLeft()
			&& s_m_x < building->getTurretButton(i)->getRight()
			&& s_m_y >= building->getTurretButton(i)->getTop()
			&& s_m_y < building->getTurretButton(i)->getBottom()
			) {
				done = true;
				//int n_population = current_sector->getPopulation();
				//int n_spare = current_sector->getSparePopulation();
				if( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_DEFENCE ) {
					// set new defender
					int deploy_defence = this->getGamePanel()->getDeployDefence();
					ASSERT(deploy_defence != -1);
					//current_sector->deployDefender(building, i, deploy_defence);
					this->deployDefender(current_sector->getXPos(), current_sector->getYPos(), building->getType(), i, deploy_defence);
				}
				else if( building->getTurretMan(i) != -1 ) {
					// remove existing defender
					// return current defender to stocks
					//current_sector->returnDefender(building, i);
					this->returnDefender(current_sector->getXPos(), current_sector->getYPos(), building->getType(), i);
				}
		}
	}

	if( !done && this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_SHIELD ) {
		if( building->getHealth() < building->getMaxHealth() ) {
			//bool ok = true;
			int deploy_shield = this->getGamePanel()->getDeployShield();
			ASSERT(deploy_shield != -1);
			/*if( current_sector->stored_shields[deploy_shield] == 0 ) {
			// not enough - try to make one
			Design *design = current_sector->canBuildDesign(Invention::SHIELD, deploy_shield);
			if( design == NULL )
			ok = false; // can't make a new one
			else {
			current_sector->consumeStocks(design);
			current_sector->stored_shields[deploy_shield]++;
			}
			}
			if( ok ) {
			building->health += 10 * ( deploy_shield + 1 );
			if( building->health > building->max_health )
			building->health = building->max_health;
			current_sector->stored_shields[deploy_shield]--;
			}*/
			//current_sector->useShield(building, deploy_shield);
			this->useShield(current_sector->getXPos(), current_sector->getYPos(), building->getType(), deploy_shield);
		}
	}

	return done;
}

void PlayingGameState::moveTo(int map_x,int map_y) {
	current_sector = map->getSector(map_x, map_y);
	this->getGamePanel()->setPage( GamePanel::STATE_SECTORCONTROL );
	this->reset();
	for(size_t i=0;i<ammo_effects.size();i++) {
		TimedEffect *effect = ammo_effects.at(i);
		delete effect;
	}
	ammo_effects.clear();
}

void ChoosePlayerGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	int player = -1;
    if( m_left && click && button_red->mouseOver(m_x, m_y) ) {
		player = 0;
	}
    else if( m_left && click && button_yellow->mouseOver(m_x, m_y) ) {
		player = 2;
	}
    else if( m_left && click && button_green->mouseOver(m_x, m_y) ) {
		player = 1;
	}
    else if( m_left && click && button_blue->mouseOver(m_x, m_y) ) {
		player = 3;
	}

	if( player != -1 ) {
		//human_player = player;
		//human_player = PLAYER_DEMO; // force demo mode
		::setClientPlayer(player);
		setGameStateID(GAMESTATEID_PLACEMEN);
		newGame();
	}
}

void PlaceMenGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);
	/*int s_m_x = (int)(m_x / scale_width);
	int s_m_y = (int)(m_y / scale_height);*/

    bool done = false;
    if( !done && m_left && click && confirm_yes_button != NULL && confirm_yes_button->mouseOver(m_x, m_y) ) {
        LOG("confirm yes clicked\n");
        done = true;
        registerClick();
        ASSERT( confirm_window != NULL );
        this->closeConfirmWindow();
		if( confirm_type == CONFIRMTYPE_NEWGAME ) {
			newGame();
		}
		else if( confirm_type == CONFIRMTYPE_QUITGAME ) {
	        application->setQuit();
		}
		else {
			T_ASSERT(false);
		}
    }
    else if( !done && m_left && click && confirm_no_button != NULL && confirm_no_button->mouseOver(m_x, m_y) ) {
        LOG("confirm no clicked\n");
        done = true;
        registerClick();
        ASSERT( confirm_window != NULL );
        this->closeConfirmWindow();
    }

    if( !done && m_left && click && this->choosemenPanel->getPage() == ChooseMenPanel::STATE_CHOOSEMEN && this->choosemenPanel->getNMen() > 0 ) {
		bool found = false;
		int map_x = -1;
		int map_y = -1;
		for(int y=0;y<map_height_c && !found;y++) {
			for(int x=0;x<map_width_c && !found;x++) {
				if( map->isSectorAt(x, y) ) {
					ASSERT( this->map_panels[x][y] != NULL );
					if( this->map_panels[x][y]->mouseOver(m_x, m_y) ) {
						found = true;
						map_x = x;
						map_y = y;
					}
				}
			}
		}
		if( found ) {
			LOG("starting epoch %d island %s at %d, %d\n", start_epoch, map->getName(), map_x, map_y);
			this->setStartMapPos(map_x, map_y);
			return;
		}
	}

    if( !done )
        this->choosemenPanel->input(m_x, m_y, m_left, m_middle, m_right, click);
}

void PlaceMenGameState::setStartMapPos(int start_map_x, int start_map_y ) {
	this->start_map_x = start_map_x;
	this->start_map_y = start_map_y;
	if( !isDemo() ) {
		players[client_player]->setNMenForThisIsland( this->choosemenPanel->getNMen() );
		ASSERT( players[client_player]->getNMenForThisIsland() <= getMenAvailable() );
		LOG("human is player %d, starting with %d men\n", client_player, players[client_player]->getNMenForThisIsland());
	}
	else {
		LOG("DEMO mode\n");
		//placeTower(map_x, map_y, 0);
	}
	placeTower();
}

bool PlayingGameState::canRequestAlliance(int player,int i) const {
	ASSERT(player != i);
	ASSERT(players[player] != NULL);
	ASSERT(!players[player]->isDead());
	bool ok = true;
	// check not already allied
	for(int j=0;j<n_players_c && ok;j++) {
		if( j == player || players[j] == NULL || players[j]->isDead() ) {
		}
		else if( j == i || Player::isAlliance(i, j) ) {
			if( Player::isAlliance(player, j) )
				ok = false;
		}
	}

	// check still two sides
	bool allied_all_others = ok;
	for(int j=0;j<n_players_c && allied_all_others;j++) {
		if( j == player || players[j] == NULL || players[j]->isDead() ) {
		}
		else if( j == i || Player::isAlliance(i, j) ) {
			// player on the side that we are requesting an alliance with
		}
		else if( !Player::isAlliance(player, j) ) {
			allied_all_others = false;
		}
	}
	if( allied_all_others )
		ok = false;
	return ok;
}

void PlayingGameState::requestAlliance(int player,int i,bool human) {
	// 'player' requests alliance with 'i'
	/*if( !human ) {
		// AIs only supported in non-player mode
		ASSERT(gameMode == GAMEMODE_SINGLEPLAYER);
	}*/
	ASSERT(gameMode == GAMEMODE_SINGLEPLAYER); // blocked for now
	ASSERT(player != i);
	ASSERT(players[player] != NULL);
	ASSERT(!players[player]->isDead());
	//ASSERT(i != human_player); // todo: for requesting with human player
	bool ok = true;
	bool ask_human_player = false;
	int playing_asking_human = -1; // which player do we need to ask the human?
	int human_player = -1;
	// okay to request?
	// check i, and those who are allied with i
	for(int j=0;j<n_players_c && ok;j++) {
		if( j == player || players[j] == NULL || players[j]->isDead() ) {
		}
		else if( j == i || Player::isAlliance(i, j) ) {
			//if( j == human_player ) {
			if( players[j]->isHuman() ) {
				// request if human is part of alliance
				ask_human_player = true;
				playing_asking_human = player;
				human_player = j;
			}
			else if( !players[j]->requestAlliance(player) ) {
				ok = false;
				if( human )
					playSample(s_alliance_no[j]);
			}
		}
	}
	// check those who are allied with player
	for(int j=0;j<n_players_c && ok;j++) {
		if( j == player || players[j] == NULL || players[j]->isDead() ) {
		}
		else if( Player::isAlliance(player, j) ) {
			//if( j == human_player ) {
			if( players[j]->isHuman() ) {
				// request if human is part of alliance
				//ok = false;
				ask_human_player = true;
				playing_asking_human = i;
				human_player = j;
			}
			else if( !players[j]->requestAlliance(i) ) {
				ok = false;
				if( human )
					playSample(s_alliance_no[j]);
			}
		}
	}

	if( ok && ask_human_player ) {
		if( this->player_asking_alliance != -1 ) {
			// someone else asking, so don't ask
		}
		else {
			// askHuman() is called to avoid the cpu player repeatedly asking
			if( players[playing_asking_human]->askHuman() && players[playing_asking_human]->requestAlliance(human_player) ) {
				playSample(s_alliance_ask[playing_asking_human]);
				this->player_asking_alliance = playing_asking_human;
				//this->reset();
				this->setupMapGUI(); // needed to change the map GUI to ask player; call this rather than reset(), to avoid resetting the entire GUI (which causes the GUI to return to main sector control)
			}
		}
	}
	else if( ok ) {
		if( human )
			playSample(s_alliance_yes[i]);
		makeAlliance(player, i);
	}
}

void PlayingGameState::makeAlliance(int player,int i) {
	for(int j=0;j<n_players_c;j++) {
		if( j == player || players[j] == NULL || players[j]->isDead() ) {
		}
		else if( j == i || Player::isAlliance(i, j) ) {
			// bring player j into the alliance
			for(int k=0;k<n_players_c;k++) {
				if( k != j && ( k == player || Player::isAlliance(k, player) ) ) {
					Player::setAlliance(k, j, true);
				}
			}
		}
	}
	//gamestate->reset(); // reset shield buttons
	//((PlayingGameState *)gamestate)->resetShieldButtons(); // needed to update player shield buttons
	this->resetShieldButtons(); // needed to update player shield buttons
	this->cancelPlayerAskingAlliance(); // need to do this even if AIs make an alliance between themselves, as it may mean the player-alliance is no longer possible!
}

void PlayingGameState::cancelPlayerAskingAlliance() {
	if( this->player_asking_alliance != -1 ) {
		this->player_asking_alliance = -1;
		//this->reset();
		this->setupMapGUI(); // call this rather than reset(), to avoid the GUI going back to main sector control!
	}
}

/*void PlayingGameState::buttonSpeedClick(void *data, int arg, bool m_left, bool m_middle, bool m_right) {
	PlayingGameState *gamestate = static_cast<PlayingGameState *>(data);
	if( m_left ) {
		if( time_rate > 1 )
			time_rate--;
	}
	else if( m_right ) {
		if( time_rate < 3 )
			time_rate++;
	}
	LOG("set time_rate to %d\n", time_rate);
	gamestate->speed_button->setImage( icon_speeds[ time_rate-1 ] );
}*/

void PlayingGameState::refreshTimeRate() {
	speed_button->setImage( icon_speeds[ time_rate-1 ] );
}

void PlayingGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	if( !isDemo() && players[client_player]->isDead() ) {
		return;
	}
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);
	int s_m_x = (int)(m_x / scale_width);
	int s_m_y = (int)(m_y / scale_height);

	bool done = false;
	bool clear_selected_army = true;

	//int s_m_x = m_x / 1;
	//int s_m_y = m_y / 1;
	int map_x = ( s_m_x - offset_map_x_c ) / 16;
	int map_y = ( s_m_y - offset_map_y_c ) / 16;
	/*if( m_x >= offset_map_x_c && m_x < offset_map_x_c + map_width_c * map_sq->getScaledWidth() &&
	m_y >= offset_map_y_c && m_y < offset_map_y_c + map_height_c * map_sq->getScaledHeight() ) {*/
	if( !done && m_left && click && this->player_asking_alliance != -1 ) {
		ASSERT( this->alliance_yes != NULL );
		ASSERT( this->alliance_no != NULL );
		if( this->alliance_yes->mouseOver(m_x, m_y) ) {
			ASSERT( players[player_asking_alliance] != NULL );
			ASSERT( !players[player_asking_alliance]->isDead() );
			this->makeAlliance(player_asking_alliance, client_player);
			// makeAlliance also cancels
			done = true;
		}
		else if( this->alliance_no->mouseOver(m_x, m_y ) ) {
			this->cancelPlayerAskingAlliance();
			done = true;
		}
		if( done ) {
			registerClick();
		}
	}
	if( !done && click && map_x >= 0 && map_x < map_width_c && map_y >= 0 && map_y < map_height_c ) {
		if( this->player_asking_alliance == -1 && map_display == MAPDISPLAY_MAP && map->isSectorAt(map_x, map_y) ) {
			done = true;
			if( m_left && selected_army != NULL ) {
				if( selected_army->getSector() != map->getSector(map_x, map_y) ) {
					int n_nukes = selected_army->getSoldiers(nuclear_epoch_c);
					ASSERT( n_nukes == 0 );
					// move selected army
					/*if( map->getSector(map_x, map_y)->moveArmy(selected_army) ) {
						this->moveTo(map_x,map_y);
					}*/
					if( this->moveArmyTo(selected_army->getSector()->getXPos(), selected_army->getSector()->getYPos(), map_x, map_y) ) {
						this->moveTo(map_x, map_y);
					}
					else {
						// (some of) army too far - don't lose selection
						clear_selected_army = false;
					}
				}
			}
			else if( m_left && this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_WEAPON ) {
				// deploy assembled army
				ASSERT( current_sector->getAssembledArmy() != NULL );
				Sector *target_sector = map->getSector(map_x, map_y);
				if( target_sector->isNuked() ) {
					//clear_selected_army = false;
				}
				else {
					int n_nukes = current_sector->getAssembledArmy()->getSoldiers(nuclear_epoch_c);
					if( n_nukes > 0 ) {
						// nuke!
						LOG("nuke sector %d, %d (%d)\n", map_x, map_y, n_nukes);
						ASSERT( n_nukes == 1 );
						if( target_sector->getActivePlayer() != -1 && target_sector->getPlayer() == current_sector->getPlayer() ) {
							// don't nuke own sector
							LOG("don't nuke own sector: %d\n", target_sector->getActivePlayer());
						}
						else if( target_sector->getActivePlayer() != -1 && Player::isAlliance(current_sector->getPlayer(), target_sector->getPlayer()) ) {
							// don't nuke allied sectors
							LOG("don't nuke allied sector\n");
							playSample(s_cant_nuke_ally);
						}
						else {
							if( this->nukeSector(current_sector->getXPos(), current_sector->getYPos(), map_x, map_y) ) {
								this->moveTo(map_x,map_y);
							}
						}
					}
					//else if( map->getSector(map_x, map_y)->moveArmy(current_sector->getAssembledArmy() ) ) {
					else if( this->moveAssembledArmyTo(current_sector->getXPos(), current_sector->getYPos(), map_x, map_y) ) {
						this->getGamePanel()->setMouseState(GamePanel::MOUSESTATE_NORMAL);
						this->moveTo(map_x,map_y);
					}
				}
			}
			else if( m_left ) {
				//if( map->sectors[map_x][map_y] != current_sector )
				{
					// move to viewing a different sector
					if( current_sector->getPlayer() == client_player ) {
						//current_sector->returnAssembledArmy();
						this->returnAssembledArmy(current_sector->getXPos(), current_sector->getYPos());
					}
					this->getGamePanel()->setMouseState(GamePanel::MOUSESTATE_NORMAL);
					this->moveTo(map_x,map_y);
				}
			}
			else if( m_right && !isDemo() ) {
				// select an army
				Army *army = map->getSector(map_x, map_y)->getArmy(client_player);
				if( army->getTotal() > 0 ) {
					done = true;
					selected_army = army;
					clear_selected_army = false;
				}
			}
		}
	}

	if( !done && ( m_left || m_right ) && click && speed_button != NULL && speed_button->mouseOver(m_x, m_y) ) {
        done = true;
        registerClick();
        if( oneMouseButtonMode() ) {
			// cycle through the speeds
			time_rate++;
			if( time_rate > 3 )
				time_rate = 1;
		}
		else {
			if( m_left ) {
				if( time_rate > 1 )
					time_rate--;
			}
			else if( m_right ) {
				if( time_rate < 3 )
					time_rate++;
			}
		}
		LOG("set time_rate to %d\n", time_rate);
		refreshTimeRate();
		//processClick(buttonSpeedClick, this->screen_page, this, 0, speed_button, m_left, m_middle, m_right);
	}
	else if( !done && m_left && click && quit_button != NULL && quit_button->mouseOver(m_x, m_y) ) {
        done = true;
        registerClick();
        requestQuit();
	}
	else if( !done && m_left && click && confirm_yes_button != NULL && confirm_yes_button->mouseOver(m_x, m_y) ) {
        done = true;
        registerClick();
        ASSERT( confirm_window != NULL );
        this->closeConfirmWindow();
        if( !state_changed ) {
			gameResult = GAMERESULT_QUIT;
			fadeMusic(1000);
			state_changed = true;
			this->fadeScreen(true, 0, endIsland);
		}
	}
	else if( !done && m_left && click && confirm_no_button != NULL && confirm_no_button->mouseOver(m_x, m_y) ) {
        done = true;
        registerClick();
        ASSERT( confirm_window != NULL );
        this->closeConfirmWindow();
    }
	else if( !done && m_left && click && pause_button != NULL && pause_button->mouseOver(m_x, m_y) ) {
		// should always be non-paused if we are here!
		if( !isPaused() ) {
            done = true;
            registerClick();
			togglePause();
		}
	}

	// switch map display
	for(int i=0;i<n_players_c && !done && m_left && click;i++) {
		if( shield_buttons[i] == NULL ) {
			continue;
		}
		ASSERT( shield_number_panels[i] != NULL );
		/*int bx = offset_map_x_c + 16 * map_width_c + 20;
		int by = offset_map_y_c + 2 + 16 * i + 8;
		if( s_m_x >= bx && s_m_x < bx + 16 && s_m_y >= by && s_m_y < by + 16 ) {*/
		if( shield_number_panels[i]->mouseOver(m_x, m_y) ) {
			bool ok = false;
			for(int j=i;j<n_players_c && !ok;j++) {
				if( j == i || Player::isAlliance(i, j) ) {
					const Army *army = current_sector->getArmy(j);
					if( army->getTotal() > 0 )
						ok = true;
				}
			}

			if( ok ) {
                done = true;
                registerClick();
                if( this->map_display == MAPDISPLAY_MAP )
					this->map_display = MAPDISPLAY_UNITS;
				else if( this->map_display == MAPDISPLAY_UNITS )
					this->map_display = MAPDISPLAY_MAP;
				this->reset();
			}
		}
	}

	// alliances
	for(int i=0;i<n_players_c && !done && m_left && click && !isDemo();i++) {
		if( i != client_player && players[i] != NULL && !players[i]->isDead() ) {
			if( shield_buttons[i] != NULL && shield_buttons[i]->mouseOver(m_x, m_y) ) {
				if( this->player_asking_alliance != -1 && this->player_asking_alliance == i ) {
					// automatically accept
					this->makeAlliance(client_player, i);
				}
				else {
					// request alliance
					if( canRequestAlliance(client_player, i) ) {
						requestAlliance(client_player, i, true);
					}
				}
				// automatically cancel any alliance being asked for
				if( this->player_asking_alliance != -1 ) {
					this->cancelPlayerAskingAlliance();
				}
				done = true;
			}
		}
	}
	if( !done && m_left && click && shield_blank_button != NULL && shield_blank_button->mouseOver(m_x, m_y) ) {
		// break alliance
		bool any = false;
		for(int i=0;i<n_players_c;i++) {
			if( i != client_player && Player::isAlliance(i, client_player) ) {
				Player::setAlliance(i, client_player, false);
				any = true;
			}
		}
		ASSERT( any );
		//gamestate->reset(); // reset shield buttons
		//((PlayingGameState *)gamestate)->resetShieldButtons(); // needed to update player shield buttons
		this->resetShieldButtons(); // needed to update player shield buttons
		done = true;
	}

	//if( !done && s_m_x >= offset_land_x_c + 16 && s_m_y >= offset_land_y_c ) {
	if( !done && click && !isDemo() && this->land_panel->mouseOver(m_x, m_y) ) {
		const Army *army_in_sector = current_sector->getArmy(client_player);
		Building *building = current_sector->getBuilding(BUILDING_TOWER);
		bool clicked_fortress = building != NULL && s_m_x >= offset_land_x_c + building->getX() && s_m_x < offset_land_x_c + building->getX() + fortress[ current_sector->getBuildingEpoch() ]->getScaledWidth() &&
			s_m_y >= offset_land_y_c + building->getY() && s_m_y < offset_land_y_c + building->getY() + fortress[ current_sector->getBuildingEpoch() ]->getScaledHeight();
		if( m_left ) {
            if( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_WEAPON ) {
                ASSERT( current_sector->getAssembledArmy() != NULL );
				int n_nukes = current_sector->getAssembledArmy()->getSoldiers(nuclear_epoch_c);
				if( n_nukes == 0 ) {
					// deploy assembled army
                    done = true;
                    registerClick();
                    //army_in_sector->add( current_sector->getAssembledArmy() );
					this->moveAssembledArmyTo(current_sector->getXPos(), current_sector->getYPos(), current_sector->getXPos(), current_sector->getYPos());
					this->getGamePanel()->setMouseState(GamePanel::MOUSESTATE_NORMAL);
                }
				else {
					// can't nuke own sector!
					int n_selected = current_sector->getAssembledArmy()->getTotal();
					ASSERT( n_nukes == 1 );
					ASSERT( n_selected == n_nukes );
				}
			}
			else if( selected_army != NULL ) {
				done = true;
                registerClick();
                // move selected army
				if( clicked_fortress && current_sector->getPlayer() == selected_army->getPlayer() ) {
					this->returnArmy(current_sector->getXPos(), current_sector->getYPos(), selected_army->getSector()->getXPos(), selected_army->getSector()->getYPos());
				}
				else {
					if( selected_army->getSector() != current_sector ) {
						// move selected army
						this->moveArmyTo(selected_army->getSector()->getXPos(), selected_army->getSector()->getYPos(), current_sector->getXPos(), current_sector->getYPos());
					}
				}
			}
		}
		if( !done && ( oneMouseButtonMode() ? m_left : m_right ) && !clicked_fortress && army_in_sector->getTotal() > 0 ) {
			done = true;
            registerClick();
            //selected_army = army_in_sector;
			selected_army = map->getSector(current_sector->getXPos(), current_sector->getYPos())->getArmy(client_player);
			clear_selected_army = false;
			if( current_sector->getPlayer() == client_player ) {
				//current_sector->returnAssembledArmy();
				this->returnAssembledArmy(current_sector->getXPos(), current_sector->getYPos());
			}
			this->getGamePanel()->setMouseState(GamePanel::MOUSESTATE_NORMAL);
		}
	}

	if( current_sector->getPlayer() == client_player && click ) {
		for(int i=0;i<N_BUILDINGS && !done;i++) {
			done = buildingMouseClick(s_m_x, s_m_y, m_left, m_right, current_sector->getBuilding((Type)i));
		}
	}

	if( !done )
		this->getGamePanel()->input(m_x, m_y, m_left, m_middle, m_right, click);

	if( clear_selected_army && ( m_left || m_right ) && click ) {
		selected_army = NULL;
		refreshButtons();
	}
	else if( selected_army != NULL ) {
		refreshButtons();
	}

}

bool PlayingGameState::validSoldierLocation(int epoch,int xpos,int ypos) {
	ASSERT_S_EPOCH(epoch);
	bool okay = true;
	if( epoch == 6 || epoch == 7 || epoch == 9 )
		return true;

	//int size_x = attackers_walking[0][ epoch ][0]->getScaledWidth();
	//int size_y = attackers_walking[0][ epoch ][0]->getScaledHeight();
	int size_x = attackers_walking[0][ epoch ][0][0]->getScaledWidth();
	int size_y = attackers_walking[0][ epoch ][0][0]->getScaledHeight();
	if( xpos < 0 || xpos + size_x >= land_width_c || ypos < 0 || ypos + size_y >= land_height_c )
		okay = false;
	else if( current_sector->getPlayer() != -1 ) {
		for(int i=0;i<N_BUILDINGS && okay;i++) {
			Building *building = current_sector->getBuilding((Type)i);
			if( building == NULL )
				continue;
			Image *image = building->getImages()[current_sector->getBuildingEpoch()];
			if( xpos + size_x >= building->getX() && xpos < building->getX() + image->getScaledWidth() &&
				ypos + size_y >= building->getY() && ypos < building->getY() + image->getScaledHeight() )
				okay = false;
		}
		if( okay && openPitMine() && xpos + size_x >= offset_openpitmine_x_c && xpos < offset_openpitmine_x_c + icon_openpitmine->getScaledWidth() &&
			ypos + size_y >= offset_openpitmine_y_c && ypos < offset_openpitmine_y_c + icon_openpitmine->getScaledHeight() )
			okay = false;
		/*Building *building = current_sector->getBuilding(BUILDING_TOWER);
		Building *building_mine = current_sector->getBuilding(BUILDING_MINE);
		if( xpos + size_x >= building->pos_x && xpos < building->pos_x + fortress[ current_sector->getBuildingEpoch() ]->getScaledWidth() &&
		ypos + size_y >= building->pos_y && ypos < building->pos_y + fortress[ current_sector->getBuildingEpoch() ]->getScaledHeight() )
		okay = false;
		else if( building_mine != NULL && xpos + size_x >= building_mine->pos_x && xpos < building_mine->pos_x + mine[ current_sector->getBuildingEpoch() ]->getScaledWidth() &&
		ypos + size_y >= building_mine->pos_y && ypos < building_mine->pos_y + mine[ current_sector->getBuildingEpoch() ]->getScaledHeight() )
		okay = false;
		else if( openPitMine() && xpos + size_x >= offset_openpitmine_x_c && xpos < offset_openpitmine_x_c + icon_openpitmine->getScaledWidth() &&
		ypos + size_y >= offset_openpitmine_y_c && ypos < offset_openpitmine_y_c + icon_openpitmine->getScaledHeight() )
		okay = false;*/
	}
	return okay;
}

void PlayingGameState::refreshSoldiers(bool flash) {
	for(int i=0;i<n_players_c;i++) {
		int n_soldiers_type[n_epochs_c+1];
		for(int j=0;j<=n_epochs_c;j++)
			n_soldiers_type[j] = 0;
		/*for(j=0;j<n_soldiers[i];j++)
		n_soldiers_type[ soldiers[i][j]->epoch ]++;*/
		//for(j=0;j<n_soldiers[i];j++) {
		for(size_t j=0;j<soldiers[i].size();j++) {
			Soldier *soldier = soldiers[i].at(j);
			n_soldiers_type[ soldier->epoch ]++;
		}
		const Army *army = current_sector->getArmy(i);
		for(int j=0;j<=n_epochs_c;j++) {
			int diff = army->getSoldiers(j) - n_soldiers_type[j];
			if( diff > 0 ) {
				// create some more
				for(int k=0;k<diff;k++) {
					int xpos = 0, ypos = 0;
					bool found_loc = false;
					while(!found_loc) {
						xpos = rand() % land_width_c;
						ypos = rand() % land_height_c;
						found_loc = validSoldierLocation(j, xpos, ypos);
					}
					Soldier *soldier = new Soldier(i, j, xpos, ypos);
					soldiers[i].push_back( soldier );
					if( flash && !isAirUnit( soldier->epoch ) ) {
						blueEffect(offset_land_x_c + soldier->xpos, offset_land_y_c + soldier->ypos, true);
					}
				}
			}
			else if( diff < 0 ) {
				// remove some
				//for(int k=0;k<n_soldiers[i];) {
				for(size_t k=0;k<soldiers[i].size();) {
					Soldier *soldier = soldiers[i].at(k);
					if( soldier->epoch == j ) {
						if( n_deaths[i][j] > 0 ) {
							if( flash && !isAirUnit( soldier->epoch ) ) {
								deathEffect(offset_land_x_c + soldier->xpos, offset_land_y_c + soldier->ypos);
								if( !isPlaying(SOUND_CHANNEL_FX) ) {
									// only play if sound fx channel is free, to avoid too many death samples sounding
									playSample(s_scream, SOUND_CHANNEL_FX);
								}
							}
							n_deaths[i][j]--;
						}
						else if( flash && !isAirUnit( soldier->epoch ) ) {
							blueEffect(offset_land_x_c + soldier->xpos, offset_land_y_c + soldier->ypos, false);
						}
						soldiers[i].erase(soldiers[i].begin() + k);
						delete soldier;
						diff++;
						if( diff == 0 )
							break;
					}
					else
						k++;
				}
			}
		}
	}

	//this->refreshShieldNumberPanels();
}

/*void GameState::clearSoldiers() {
for(int i=0;i<n_players_c;i++) {
n_soldiers[i] = 0;
}
}*/

void PlayingGameState::deathEffect(int xpos,int ypos) {
	AnimationEffect *animationeffect = new AnimationEffect(xpos, ypos, death_flashes, n_death_flashes_c, 100, true);
	this->effects.push_back(animationeffect);
}

void PlayingGameState::blueEffect(int xpos,int ypos,bool dir) {
	AnimationEffect *animationeffect = new AnimationEffect(xpos, ypos, blue_flashes, n_blue_flashes_c, 50, dir);
	this->effects.push_back(animationeffect);
}

void PlayingGameState::explosionEffect(int xpos,int ypos) {
	if( explosions[0] != NULL ) { // not available with "old" graphics
		AnimationEffect *animationeffect = new AnimationEffect(xpos, ypos, explosions, n_explosions_c, 50, true);
		this->effects.push_back(animationeffect);
	}
}

void PlayingGameState::refreshButtons() {
	for(int i=0;i<N_BUILDINGS;i++) {
		Building *building = current_sector->getBuilding((Type)i);
		if( building != NULL ) {
			for(int j=0;j<building->getNTurrets();j++) {
				PanelPage *panel = building->getTurretButton(j);
				panel->setInfoLMB("");
			}
			if( building->getBuildingButton() != NULL ) {
				building->getBuildingButton()->setInfoLMB("");
			}

			if( current_sector->getPlayer() != client_player ) {
				// no text
			}
			else if( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_SHUTDOWN ) {
				if( building->getBuildingButton() != NULL ) {
					building->getBuildingButton()->setInfoLMB("shutdown the sector");
				}
			}
			else {
				for(int j=0;j<building->getNTurrets();j++) {
					PanelPage *panel = building->getTurretButton(j);
					if( this->getGamePanel()->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_DEFENCE ) {
						panel->setInfoLMB("place a defender here");
					}
					else if( building->getTurretMan(j) != -1 ) {
						panel->setInfoLMB("return defender to tower"); // todo: check text
					}
				}
			}
		}
	}

	if( selected_army != NULL || this->gamePanel->getMouseState() == GamePanel::MOUSESTATE_DEPLOY_WEAPON ) {
		bool is_nukes = current_sector->getAssembledArmy() != NULL && current_sector->getAssembledArmy()->getSoldiers(nuclear_epoch_c) > 0;
		this->land_panel->setInfoLMB(is_nukes ? "" : "move army here");
		if( this->player_asking_alliance == -1 && this->map_display == MAPDISPLAY_MAP ) {
			for(int y=0;y<map_height_c;y++) {
				for(int x=0;x<map_width_c;x++) {
					if( map->isSectorAt(x, y) ) {
						if( is_nukes ) {
							if( map->getSector(x, y)->getPlayer() == current_sector->getPlayer() || map->getSector(x, y)->isBeingNuked() || map->getSector(x, y)->isNuked() ) {
								map_panels[x][y]->setInfoLMB("");
							}
							else {
								map_panels[x][y]->setInfoLMB("nuke sector");
							}
						}
						else {
							map_panels[x][y]->setInfoLMB("move army to this sector");
						}
						//map_panels[x][y]->setInfoLMB(!is_nukes ? "move army to this sector" : map->getSector(x, y) == current_sector ? "" : "nuke sector");
					}
				}
			}
		}
	}
	else {
		this->land_panel->setInfoLMB("");
		if( this->player_asking_alliance == -1 && this->map_display == MAPDISPLAY_MAP ) {
			for(int y=0;y<map_height_c;y++) {
				for(int x=0;x<map_width_c;x++) {
					if( map->isSectorAt(x, y) ) {
						map_panels[x][y]->setInfoLMB("view this sector");
					}
				}
			}
		}
	}
}

void PlayingGameState::refreshShieldNumberPanels() {
	if( this->map_display == MAPDISPLAY_MAP ) {
		for(int i=0;i<n_players_c;i++) {
			if( shield_number_panels[i] != NULL ) {
				//shield_number_panels[i]->setEnabled(false);
				shield_number_panels[i]->setInfoLMB("display numbers in each army");
			}
		}
	}
	else {
		for(int i=0;i<n_players_c;i++) {
			if( shield_number_panels[i] != NULL ) {
				shield_number_panels[i]->setInfoLMB("display map");
			}
		}
	}
}

void PlayingGameState::setNDesigners(int sector_x, int sector_y, int n_designers) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->getCurrentDesign() != NULL ) {
			sector->setDesigners(n_designers);
		}
	}
}

void PlayingGameState::setNWorkers(int sector_x, int sector_y, int n_workers) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->getCurrentManufacture() != NULL ) {
			sector->setWorkers(n_workers);
		}
	}
}

void PlayingGameState::setFAmount(int sector_x, int sector_y, int n_famount) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->getCurrentManufacture() != NULL ) {
			sector->setFAmount(n_famount);
		}
	}
}

void PlayingGameState::setNMiners(int sector_x, int sector_y, Id element, int n_miners) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->canMine(element) ) {
			sector->setMiners(element, n_miners);
		}
	}
}

void PlayingGameState::setNBuilders(int sector_x, int sector_y, Type type, int n_builders) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->canBuild(type) ) {
			sector->setBuilders(type, n_builders);
		}
	}
}

void PlayingGameState::setCurrentDesign(int sector_x, int sector_y, Design *design) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->setCurrentDesign(design);
	}
}

void PlayingGameState::setCurrentManufacture(int sector_x, int sector_y, Design *design) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->setCurrentManufacture(design);
	}
}

void PlayingGameState::assembledArmyEmpty(int sector_x, int sector_y) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->getAssembledArmy()->empty();
	}
}

bool PlayingGameState::assembleArmyUnarmed(int sector_x, int sector_y, int n) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		int n_spare = sector->getAvailablePopulation();
		if( n_spare >= n ) {
			int n_population = sector->getPopulation();
			sector->getAssembledArmy()->add(n_epochs_c, n);
			sector->setPopulation(n_population - n);
			return true;
		}
	}
	return false;
}

bool PlayingGameState::assembleArmy(int sector_x, int sector_y, int epoch, int n) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		if( sector->assembleArmy(epoch, n) ) {
			return true;
		}
	}
	return false;
}

void PlayingGameState::returnAssembledArmy(int sector_x, int sector_y) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->returnAssembledArmy();
	}
}

bool PlayingGameState::returnArmy(int sector_x, int sector_y, int src_x, int src_y) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		Sector *src = map->getSector(src_x, src_y);
		Army *army = src->getArmy(client_player);
		return sector->returnArmy(army);
	}
	return false;
}

bool PlayingGameState::moveArmyTo(int src_x, int src_y, int target_x, int target_y) {
	Sector *src = map->getSector(src_x, src_y);
	Sector *target = map->getSector(target_x, target_y);
	ASSERT(src != NULL);
	ASSERT(target != NULL);
	Army *army = src->getArmy(client_player);
	return target->moveArmy(army);
}

bool PlayingGameState::moveAssembledArmyTo(int src_x, int src_y, int target_x, int target_y) {
	Sector *src = map->getSector(src_x, src_y);
	ASSERT(src != NULL);
	if( src->getActivePlayer() == client_player ) {
		Army *army = src->getAssembledArmy();
		Sector *target = map->getSector(target_x, target_y);
		ASSERT(target != NULL);
		return target->moveArmy(army);
	}
	return false;
}

bool PlayingGameState::nukeSector(int src_x, int src_y, int target_x, int target_y) {
	Sector *src = map->getSector(src_x, src_y);
	Sector *target = map->getSector(target_x, target_y);
	ASSERT(src != NULL);
	ASSERT(target != NULL);
	if( src->getActivePlayer() == client_player ) {
		if( target->nukeSector(src) ) {
			this->assembledArmyEmpty(src_x, src_y);
			return true;
		}
	}
	return false;
}

void PlayingGameState::deployDefender(int sector_x, int sector_y, Type type, int turret, int epoch) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		Building *building = sector->getBuilding(type);
		if( building != NULL ) {
			sector->deployDefender(building, turret, epoch);
		}
	}
}

void PlayingGameState::returnDefender(int sector_x, int sector_y, Type type, int turret) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		Building *building = sector->getBuilding(type);
		if( building != NULL ) {
			sector->returnDefender(building, turret);
		}
	}
}

void PlayingGameState::useShield(int sector_x, int sector_y, Type type, int shield) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		Building *building = sector->getBuilding(type);
		if( building != NULL ) {
			sector->useShield(building, shield);
		}
	}
}

void PlayingGameState::trashDesign(int sector_x, int sector_y, Invention *invention) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->trashDesign(invention);
	}
}

void PlayingGameState::shutdown(int sector_x, int sector_y) {
	Sector *sector = map->getSector(sector_x, sector_y);
	ASSERT(sector != NULL);
	if( sector->getActivePlayer() == client_player ) {
		sector->shutdown(client_player);
	}
}

void EndIslandGameState::reset() {
    //LOG("EndIslandGameState::reset()\n");
	this->screen_page->free(true);
}

void EndIslandGameState::draw() {
#if defined(__ANDROID__)
	screen->clear(); // SDL on Android requires screen be cleared (otherwise we get corrupt regions outside of the main area)
#endif
	background->draw(0, 0);
	char text[4096] = "";
	if( gameResult == GAMERESULT_QUIT )
		strcpy(text, "QUITTER!");
	else if( gameResult == GAMERESULT_LOST )
		strcpy(text, "LOSER!");
	else if( gameResult == GAMERESULT_WON )
		strcpy(text, "CONGRATULATIONS!");
	else {
		ASSERT(false);
	}
	Image::write(160, 120, letters_small, text, Image::JUSTIFY_CENTRE);

	bool suspend = false;
	if( start_epoch >= 6 && gameResult == GAMERESULT_WON )
		suspend = true;

	Image::write(40, 140, letters_small, "PLAYER", Image::JUSTIFY_LEFT);
	Image::write(100, 140, letters_small, "START", Image::JUSTIFY_LEFT);
	Image::write(140, 140, letters_small, "BIRTHS", Image::JUSTIFY_LEFT);
	Image::write(180, 140, letters_small, "DEATHS", Image::JUSTIFY_LEFT);
	Image::write(220, 140, letters_small, "END", Image::JUSTIFY_LEFT);
	if( suspend )
		Image::write(260, 140, letters_small, "SAVED", Image::JUSTIFY_LEFT);

	int ypos = 150;
	//int r = 0, g = 0, b = 0, col = 0;
	int r = 0, g = 0, b = 0;
	/*SDL_Rect rect;
	rect.x = (Sint16)(20 * scale_width);
	rect.y = (Sint16)(ypos * scale_height);
	rect.w = (Uint16)(16 * scale_width);
	rect.h = (Uint16)(8 * scale_height);*/
	int rect_x = (int)(20 * scale_width);
	int rect_y = (int)(ypos * scale_height);
	int rect_w = (int)(16 * scale_width);
	int rect_h = (int)(8 * scale_height);

	if( !isDemo() ) {
		PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)client_player);
		/*col = SDL_MapRGB(screen->getSurface()->format, r, g, b);
		SDL_FillRect(screen->getSurface(), &rect, col);*/
		screen->fillRect(rect_x, rect_y, rect_w, rect_h, r, g, b);

		//Image::write(40, ypos, letters_small, "HUMAN", Image::JUSTIFY_LEFT, true);
		Image::write(40, ypos, letters_small, PlayerType::getName((PlayerType::PlayerTypeID)client_player), Image::JUSTIFY_LEFT);

		Image::writeNumbers(110, ypos, numbers_yellow, players[client_player]->getNMenForThisIsland(), Image::JUSTIFY_LEFT);
		Image::writeNumbers(150, ypos, numbers_yellow, players[client_player]->getNBirths(), Image::JUSTIFY_LEFT);
		Image::writeNumbers(190, ypos, numbers_yellow, players[client_player]->getNDeaths(), Image::JUSTIFY_LEFT);
		Image::writeNumbers(230, ypos, numbers_yellow, players[client_player]->getFinalMen(), Image::JUSTIFY_LEFT);
		if( suspend )
			Image::writeNumbers(270, ypos, numbers_yellow, players[client_player]->getNSuspended(), Image::JUSTIFY_LEFT);
		ypos += 10;
	}

	for(int i=0;i<n_players_c;i++) {
		if( i == client_player || players[i] == NULL )
			continue;
		PlayerType::getColour(&r, &g, &b, (PlayerType::PlayerTypeID)i);
		/*col = SDL_MapRGB(screen->getSurface()->format, r, g, b);
		rect.y = (Sint16)(ypos * scale_height);
		SDL_FillRect(screen->getSurface(), &rect, col);*/
		rect_y = (int)(ypos * scale_height);
		screen->fillRect(rect_x, rect_y, rect_w, rect_h, r, g, b);

		//Image::write(40, ypos, letters_small, "COMPUTER", Image::JUSTIFY_LEFT, true);
		Image::write(40, ypos, letters_small, PlayerType::getName((PlayerType::PlayerTypeID)i), Image::JUSTIFY_LEFT);
		Image::writeNumbers(110, ypos, numbers_yellow, players[i]->getNMenForThisIsland(), Image::JUSTIFY_LEFT);
		Image::writeNumbers(150, ypos, numbers_yellow, players[i]->getNBirths(), Image::JUSTIFY_LEFT);
		Image::writeNumbers(190, ypos, numbers_yellow, players[i]->getNDeaths(), Image::JUSTIFY_LEFT);
		Image::writeNumbers(230, ypos, numbers_yellow, players[i]->getFinalMen(), Image::JUSTIFY_LEFT);
		if( suspend )
			Image::writeNumbers(270, ypos, numbers_yellow, players[i]->getNSuspended(), Image::JUSTIFY_LEFT);
		ypos += 10;
	}

	this->screen_page->draw();
	//this->screen_page->drawPopups();

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void EndIslandGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);

	if( ( m_left || m_right ) && click && !state_changed ) {
		this->requestQuit();
	}
}

void EndIslandGameState::requestQuit() {
	state_changed = true;
	this->fadeScreen(true, 0, returnToChooseIsland);
}

void GameCompleteGameState::reset() {
    //LOG("GameCompleteGameState::reset()\n");
	this->screen_page->free(true);
}

void GameCompleteGameState::draw() {
#if defined(__ANDROID__)
	screen->clear(); // SDL on Android requires screen be cleared (otherwise we get corrupt regions outside of the main area)
#endif
	background->draw(0, 0);

	this->screen_page->draw();
	//this->screen_page->drawPopups();

	if( !isDemo() ) {
		stringstream str;
		int l_h = letters_large[0]->getScaledHeight();
		int y = 80;

		Image::writeMixedCase(160, y, letters_large, letters_small, numbers_white, "GAME COMPLETE", Image::JUSTIFY_CENTRE);
		y += l_h + 2;

		if( difficulty_level == DIFFICULTY_EASY )
			str.str("Easy");
		else if( difficulty_level == DIFFICULTY_MEDIUM )
			str.str("Medium");
		else if( difficulty_level == DIFFICULTY_HARD )
			str.str("Hard");
		else {
			ASSERT(false);
		}
		Image::writeMixedCase(160, y, letters_large, letters_small, numbers_white, str.str().c_str(), Image::JUSTIFY_CENTRE);
		y += l_h + 2;

		y += l_h + 2;

		str << "Men Remaining " << getMenAvailable();
		Image::writeMixedCase(160, y, letters_large, letters_small, numbers_white, str.str().c_str(), Image::JUSTIFY_CENTRE);
		y += l_h + 2;

		str.str("");
		str << "Men Saved " << getNSuspended();
		Image::writeMixedCase(160, y, letters_large, letters_small, numbers_white, str.str().c_str(), Image::JUSTIFY_CENTRE);
		y += l_h + 2;

		int score = getMenAvailable() + getNSuspended();
		str.str("");
		str << "Total Score " << score;
		Image::writeMixedCase(160, y, letters_large, letters_small, numbers_white, str.str().c_str(), Image::JUSTIFY_CENTRE);
		y += l_h + 2;
	}

	GameState::setDefaultMouseImage();
	GameState::draw();
}

void GameCompleteGameState::mouseClick(int m_x,int m_y,bool m_left,bool m_middle,bool m_right,bool click) {
	GameState::mouseClick(m_x, m_y, m_left, m_middle, m_right, click);

	//bool m_left = mouse_left(m_b);
	//bool m_right = mouse_right(m_b);

	if( ( m_left || m_right ) && click && !state_changed ) {
		this->requestQuit();
	}
}

void GameCompleteGameState::requestQuit() {
	state_changed = true;
	this->fadeScreen(true, 0, startNewGame);
}
