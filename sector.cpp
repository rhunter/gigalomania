//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>

#include <algorithm>
using std::min;
using std::max;

#include "sector.h"
#include "game.h"
#include "utils.h"
#include "gamestate.h"
#include "panel.h"
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

const int max_grow_population_c = 500;
//const int max_population_c = 999;
const int growth_rate_c = 100; // higher is slower
//const int research_rate_c = 1000; // higher is slower
//const int build_rate_c = 1000; // higher is slower
const int mine_rate_c = 60; // higher is slower
//const int combat_rate_c = 500; // higher is slower combat
const int combat_rate_c = 50; // higher is slower combat
const int bombard_rate_c = 5; // higher is slower damage
const int max_gatherables_stored_c = 22;

const int offset_fortress_x_c = 150;
const int offset_fortress_y_c = 14;
//const int offset_fortress_x_c = 100;
//const int offset_fortress_y_c = 50;
const int offset_mine_x_c = 32;
const int offset_mine_y_c = 14;
const int offset_factory_x_c = 32;
const int offset_factory_y_c = 96;
const int offset_lab_x_c = 150;
const int offset_lab_y_c = 96;

const int DEFENDER_DIR_S = 0;
const int DEFENDER_DIR_N = 1;
const int DEFENDER_DIR_E = 2;
const int DEFENDER_DIR_W = 3;

bool isAirUnit(int epoch) {
	ASSERT_S_EPOCH(epoch);
	if( epoch == n_epochs_c || epoch < 6 ) {
		return false;
	}
	return true;
}

bool defenceNeedsMan(int epoch) {
	ASSERT_EPOCH(epoch);
	if( epoch == nuclear_epoch_c || epoch == laser_epoch_c ) {
		return false;
	}
	return true;
}

Particle::Particle() : xpos(0.0f), ypos(0.0f), birth_time(0) {
	this->birth_time = getRealTime();
}

void ParticleSystem::draw(int xpos, int ypos) const {
	for(vector<Particle>::const_iterator iter = particles.begin(); iter != particles.end(); ++iter) {
		this->image->draw(xpos + (int)iter->getX(), ypos + (int)iter->getY());
	}
}

SmokeParticleSystem::SmokeParticleSystem(const Image *image) : ParticleSystem(image),
birth_rate(0.0f), life_exp(1500), last_emit_time(0) {
	this->last_emit_time = getRealTime();
}

void SmokeParticleSystem::setBirthRate(float birth_rate) {
	this->birth_rate = birth_rate;
}

void SmokeParticleSystem::update() {
	// expire old particles
	int time_now = getRealTime();
	for(int i=particles.size()-1;i>=0;i--) { // count backwards in case of deletion
		if( time_now >= particles.at(i).getBirthTime() + life_exp ) {
			// for performance, we reorder and reduce the length by 1 (as the order of the particles shouldn't matter)
			particles[i] = particles[particles.size()-1];
			particles.resize(particles.size()-1);
		}
	}

	int real_loop_time = getRealLoopTime();
	//LOG("%d\n", real_loop_time);
	// update particles
	for(int i=particles.size()-1;i>=0;i--) { // count backwards in case of deletion
		//const float xspeed = 0.01f;
		const float xspeed = 0.015f;
		//const float yspeed = 0.05f;
		const float yspeed = 0.03f;
		float xpos = particles.at(i).getX();
		float ypos = particles.at(i).getY();
		float ydiff = real_loop_time * yspeed;
		ypos -= ydiff;
		float xdiff = real_loop_time * xspeed;
		if( rand() % 2 == 0 ) {
			xdiff = - xdiff;
		}
		xpos += xdiff;
		/*if( ypos < 0 ) {
			// kill
			// for performance, we reorder and reduce the length by 1 (as the order of the particles shouldn't matter)
			particles[i] = particles[particles.size()-1];
			particles.resize(particles.size()-1);
			//LOG("resize to %d\n", particles.size());
		}
		else*/ {
			particles.at(i).setPos(xpos, ypos);
		}
	}

	// emit new particles
	int accumulated_time = getRealTime() - this->last_emit_time;
	int new_particles = (int)(this->birth_rate * accumulated_time);
	new_particles = min(1, new_particles); // helps make rate more steady
	this->last_emit_time += (int)(1.0f/birth_rate * new_particles);
	if( new_particles > 0 ) {
		//LOG("%d new particles (total will be %d)\n", new_particles, particles.size() + new_particles);
		for(int i=0;i<new_particles;i++) {
			Particle particle;
			particles.push_back(particle);
		}
	}
}

Army::Army(PlayingGameState *gamestate, Sector *sector, int player) :
gamestate(gamestate), sector(sector), player(player)
{
	ASSERT_PLAYER(player);
	this->empty();
}

int Army::getTotal() const {
	ASSERT_PLAYER(this->player);
	int n = 0;
	for(int i=0;i<n_epochs_c+1;i++)
		n += soldiers[i];
	return n;
}

int Army::getTotalMen() const {
	ASSERT_PLAYER(this->player);
	int n = 0;
	for(int i=0;i<n_epochs_c+1;i++) {
		int n_men = ( i==n_epochs_c ? 1 : invention_weapons[i]->getNMen() );
		n += soldiers[i] * n_men;
	}
	return n;
}

bool Army::any(bool include_unarmed) const {
	ASSERT_PLAYER(this->player);
	int until = include_unarmed ? n_epochs_c : n_epochs_c-1;
	for(int i=0;i<=until;i++) {
		if( soldiers[i] > 0 )
			return true;
	}
	return false;
}

int Army::getStrength() const {
	ASSERT_PLAYER(this->player);
	int n = 0;
	for(int i=0;i<=n_epochs_c;i++) {
		if( soldiers[i] > 0 ) {
			ASSERT(i != nuclear_epoch_c);
			n += getIndividualStrength(i) * soldiers[i];
		}
	}
	return n;
}

int Army::getBombardStrength() const {
	ASSERT_PLAYER(this->player);
	int n = 0;
	for(int i=0;i<=n_epochs_c;i++) {
		if( soldiers[i] > 0 )
			n += getIndividualBombardStrength(i) * soldiers[i];
	}
	return n;
}

void Army::add(int i,int n) {
	ASSERT_PLAYER(this->player);
	ASSERT_S_EPOCH(i);
	ASSERT(n > 0);
	ASSERT( !this->sector->isNuked() );
	this->soldiers[i] += n;
}

void Army::add(Army *army) {
	ASSERT_PLAYER(this->player);
	ASSERT_PLAYER(army->player);
	bool any = false;
	for(int i=0;i<=n_epochs_c;i++) {
		ASSERT(army->soldiers[i] >= 0);
		any = any || army->soldiers[i] > 0;
		this->soldiers[i] += army->soldiers[i];
		army->soldiers[i] = 0;
	}
	if( any && ( this->sector == gamestate->getCurrentSector() || army->getSector() == gamestate->getCurrentSector() ) ) {
		ASSERT( !this->sector->isNuked() );
		//((PlayingGameState *)gamestate)->refreshSoldiers(true);
		gamestate->refreshSoldiers(true);
	}
}

void Army::remove(int i,int n) {
	ASSERT_PLAYER(this->player);
	ASSERT_S_EPOCH(i);
	ASSERT(n > 0);
	this->soldiers[i] -= n;
	ASSERT(this->soldiers[i] >= 0);
}

void Army::kill(int index) {
	ASSERT_PLAYER(this->player);
	bool done = false;
	int saved_index = index;
	for(int i=0;i<=n_epochs_c && !done;i++) {
		if( index < soldiers[i] ) {
			soldiers[i]--;
			done = true;
			if( this->sector == gamestate->getCurrentSector() ) {
				//((PlayingGameState *)gamestate)->n_deaths[player][i]++;
				//((PlayingGameState *)gamestate)->registerDeath(player, i);
				gamestate->registerDeath(player, i);
			}
		}
		index -= soldiers[i];
	}
	if( !done ) {
		LOG("###can't kill index %d in army (total %d)\n",saved_index,this->getTotal());
		ASSERT(0);
	}
	if( this->sector == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->refreshSoldiers(true);
		gamestate->refreshSoldiers(true);
	}
	//((PlayingGameState *)gamestate)->getGamePanel()->refreshShutdown();
	gamestate->getGamePanel()->refreshShutdown();
}

bool Army::canLeaveSafely() const {
	bool any_enemy_attackers = false;
	for(int i=0;i<n_players_c && !any_enemy_attackers;i++) {
		if( i != this->player && !Player::isAlliance(i, this->player) )
			any_enemy_attackers = this->sector->getArmy(i)->any(true);
	}
	//if( n_enemy_attackers > 0 || ( src_sector->player != -1 && src_sector->player != a_player ) ) {
	//if( any_enemy_attackers || ( src_sector->player != -1 && src_sector->player != a_player ) ) {
	if( this->sector->getPlayer() != this->player && ( any_enemy_attackers || ( this->sector->getPlayer() != -1 && !Player::isAlliance(this->sector->getPlayer(), this->player) ) ) ) {
		return false;
	}
	return true;
}

void Army::retreat(bool only_air) {
	for(int i=0;i<=n_epochs_c;i++) {
		//if( only_air && ( i < air_epoch_c || i == n_epochs_c ) ) {
		if( this->soldiers[i] > 0 ) {
			if( only_air && !isAirUnit( i ) ) {
				; // don't do land units
			}
			else {
				//this->soldiers[i] *= 0.6;
				this->soldiers[i] = (int)(this->soldiers[i] * 0.6);
			}
		}
	}
}

int Army::getIndividualStrength(int i) {
	/*if( i < start_epoch || i > n_epochs_c ) {
	printf("");
	}*/
	ASSERT_S_EPOCH(i);
	int str = 0;
	if( i == n_epochs_c && start_epoch != end_epoch_c )
		//if( i == n_epochs_c )
		str = 1;
	else if( i == nuclear_epoch_c )
		str = 0;
	else {
		str = 1;
		for(int j=0;j<=i;j++)
			str *= 2;
		//int n_men = invention_weapons[i]->n_men;
		//str = 2 * (i+1) * n_men;
		//str *= n_men;
	}
	return str;
}

int Army::getIndividualBombardStrength(int i) {
	ASSERT_S_EPOCH(i);
	int str = 0;
	if( start_epoch == end_epoch_c ) {
		str = 1;
	}
	else if( i != n_epochs_c ) {
		// only armed men cause damage
		/*int n_men = invention_weapons[i]->n_men;
		str = (i+1) * n_men;*/
		//str = i+1;
		str = i - start_epoch + 1;
		/*str = 1;
		for(int j=0;j<i;j++)
		str *= 2;
		str *= n_men;*/
	}
	return str;
}

Element::Element(const char *name,Id id,Type type) {
	//strcpy(this->name,name);
	this->name = name;
	this->id = id;
	this->type = type;
}

Element::~Element() {
}

Image *Element::getImage() const {
	return icon_elements[this->id];
}

Design::Design(Invention *invention,bool ergonomically_terrific) {
	this->invention = invention;
	this->ergonomically_terrific = ergonomically_terrific;
	for(int i=0;i<N_ID;i++)
		this->cost[i] = 0;
	//invention->designs->add(this);
	//invention->designs->push_back(this);
	invention->addDesign(this);
};

Invention::Invention(const char *name,Type type,int epoch) {
	ASSERT_ANY_EPOCH(epoch);
	//strcpy(this->name,name);
	this->name = name;
	this->type = type;
	this->epoch = epoch;
	//this->designs = new Vector();
	this->designs = new vector<Design *>();
}

Invention::~Invention() {
	for(unsigned int i=0;i<designs->size();i++) {
		//Design *design = (Design *)designs->elementAt(i);
		Design *design = designs->at(i);
		delete design;
	}
	delete this->designs;
}

/*int Invention::getRelativeEpoch() {
if( this->type == SHIELD )
return this->epoch;
else
return this->epoch - start_epoch;
}*/

Image *Invention::getImage() const {
	if( this->type == SHIELD )
		return icon_shields[this->epoch - start_epoch];
	else if( this->type == DEFENCE )
		return icon_defences[this->epoch];
	else if( this->type == WEAPON )
		return icon_weapons[this->epoch];
	return NULL;
}

/*void Invention::addDesign(Design *design) {
this->designs->add(design);
}*/

Invention *Invention::getInvention(Invention::Type type,int epoch) {
	ASSERT_EPOCH(epoch);
	Invention *invention = NULL;
	if( type == SHIELD )
		invention = invention_shields[epoch];
	//invention = invention_shields[epoch - start_epoch];
	else if( type == DEFENCE )
		invention = invention_defences[epoch];
	else if( type == WEAPON )
		invention = invention_weapons[epoch];
	ASSERT(invention != NULL);
	return invention;
}

Building::Building(PlayingGameState *gamestate, Sector *sector, Type type) :
gamestate(gamestate), sector(sector), type(type)
{
    //LOG("Building::Building(Sector [%d: %d, %d], Type %d)\n", sector->getPlayer(), sector->getXPos(), sector->getYPos(), type);
	Rect2D turret_pos[max_building_turrets_c];

	if( this->type == BUILDING_TOWER )
		this->max_health = 150;
	else
		this->max_health = 100;
	this->health = this->max_health;
	this->n_turrets = 0;
	for(int i=0;i<max_building_turrets_c;i++) {
		this->turret_man[i] = -1;
		this->turret_mandir[i] = 0;
	}

	if( this->type == BUILDING_TOWER ) {
		this->pos_x = offset_fortress_x_c;
		this->pos_y = offset_fortress_y_c;
		this->n_turrets = 4;
		turret_pos[0].set(0, 0, 15, 12);
		turret_pos[1].set(27, 0, 15, 12);
		turret_pos[2].set(0, 24, 15, 12);
		turret_pos[3].set(27, 24, 15, 12);
	}
	else if( this->type == BUILDING_MINE ) {
		this->pos_x = offset_mine_x_c;
		this->pos_y = offset_mine_y_c;
		this->n_turrets = 2;
		turret_pos[0].set(0, 17, 15, 12);
		turret_pos[1].set(27, 17, 15, 12);
	}
	else if( this->type == BUILDING_FACTORY ) {
		this->pos_x = offset_factory_x_c;
		this->pos_y = offset_factory_y_c;
		this->n_turrets = 3;
		turret_pos[0].set(1, 9, 15, 12);
		turret_pos[1].set(33, 9, 15, 12);
		turret_pos[2].set(16, 26, 15, 12);
	}
	else if( this->type == BUILDING_LAB ) {
		this->pos_x = offset_lab_x_c;
		this->pos_y = offset_lab_y_c;
		this->n_turrets = 1;
		turret_pos[0].set(10, 4, 15, 12);
	}
	ASSERT(this->n_turrets != 0);

	building_button = NULL;
	if( this->type == BUILDING_TOWER ) {
		building_button = new PanelPage(offset_land_x_c + this->pos_x, offset_land_y_c + this->pos_y, 58, 60);
		building_button->setSurviveOwner(true);
	}

	for(int j=0;j<this->n_turrets;j++) {
		Rect2D rect = turret_pos[j];
		turret_buttons[j] = new PanelPage(offset_land_x_c + this->pos_x + rect.x, offset_land_y_c + this->pos_y + rect.y, rect.w, rect.h);
		//turret_buttons[j]->setInfoLMB("place a defender here");
		turret_buttons[j]->setSurviveOwner(true);
	}
	if( sector == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->addBuilding(this);
		gamestate->addBuilding(this);
	}
    //LOG("Building::Building done\n");
}

Building::~Building() {
    //LOG("Building::~Building()\n");
	for(int i=0;i<this->n_turrets;i++) {
		delete turret_buttons[i];
	}
	if( building_button != NULL )
		delete building_button;
    //LOG("Building::~Building done\n");
}

Image **Building::getImages() {
	Image **images = NULL;
	if( type == BUILDING_TOWER )
		images = fortress;
	else if( type == BUILDING_MINE )
		images = mine;
	else if( type == BUILDING_FACTORY )
		images = factory;
	else if( type == BUILDING_LAB )
		images = lab;
	ASSERT(images != NULL);
	return images;
}

void Building::rotateDefenders() {
	if( this->type == BUILDING_TOWER ) {
		this->turret_mandir[0] = ( rand() % 2 ) == 0 ? DEFENDER_DIR_W : DEFENDER_DIR_N;
		this->turret_mandir[1] = ( rand() % 2 ) == 0 ? DEFENDER_DIR_E : DEFENDER_DIR_N;
		this->turret_mandir[2] = ( rand() % 2 ) == 0 ? DEFENDER_DIR_W : DEFENDER_DIR_S;
		this->turret_mandir[3] = ( rand() % 2 ) == 0 ? DEFENDER_DIR_E : DEFENDER_DIR_S;
	}
	else if( this->type == BUILDING_MINE ) {
		this->turret_mandir[0] = ( rand() % 2 ) == 0 ? DEFENDER_DIR_W : DEFENDER_DIR_S;
		this->turret_mandir[1] = ( rand() % 2 ) == 0 ? DEFENDER_DIR_E : DEFENDER_DIR_S;
	}
	else if( this->type == BUILDING_FACTORY ) {
		int r0 = rand() % 3;
		this->turret_mandir[0] = r0 == 0 ? DEFENDER_DIR_W : r0 == 1 ? DEFENDER_DIR_S : DEFENDER_DIR_N;
		int r1 = rand() % 3;
		this->turret_mandir[1] = r1 == 0 ? DEFENDER_DIR_E : r1 == 1 ? DEFENDER_DIR_S : DEFENDER_DIR_N;
		int r2 = rand() % 3;
		this->turret_mandir[2] = r2 == 0 ? DEFENDER_DIR_W : r2 == 1 ? DEFENDER_DIR_E : DEFENDER_DIR_S;
	}
	else if( this->type == BUILDING_LAB ) {
		int r0 = rand() % 4;
		if( r0 == 0 )
			this->turret_mandir[0] = DEFENDER_DIR_N;
		else if( r0 == 1 )
			this->turret_mandir[0] = DEFENDER_DIR_E;
		else if( r0 == 2 )
			this->turret_mandir[0] = DEFENDER_DIR_S;
		else if( r0 == 3 )
			this->turret_mandir[0] = DEFENDER_DIR_W;
		else {
			ASSERT(false);
		}
	}
	else {
		ASSERT(false);
	}
}

int Building::getDefenderStrength() const {
	int n = 0;
	for(int i=0;i<n_turrets;i++) {
		if( turret_man[i] != -1 )
			n += Army::getIndividualStrength( turret_man[i] );
	}
	//n *= 2;
	n *= 4;
	return n;
}

void Building::killIthDefender(int i) {
	turret_man[i] = -1;
	if( gamestate->getCurrentSector() == this->sector ) {
		int xpos = this->turret_buttons[i]->getLeft();
		int ypos = this->turret_buttons[i]->getTop();
		//((PlayingGameState *)gamestate)->deathEffect(xpos, ypos - 4);
		gamestate->deathEffect(xpos, ypos - 4);
	}
}

void Building::killDefender(int index) {
	for(int i=0;i<n_turrets;i++) {
		if( turret_man[i] != -1 ) {
			if( index == 0 ) {
				killIthDefender(i);
				break;
			}
			index--;
		}
	}
}

int Building::getTurretMan(int turret) const {
	ASSERT( turret >= 0 && turret < this->n_turrets );
	return this->turret_man[turret];
}

int Building::getTurretManDir(int turret) const {
	ASSERT( turret >= 0 && turret < this->n_turrets );
	ASSERT( this->turret_man[turret] != -1 );
	return this->turret_mandir[turret];
}

PanelPage *Building::getTurretButton(int turret) const {
	ASSERT( turret >= 0 && turret < this->n_turrets );
	return this->turret_buttons[turret];
}

void Building::clearTurretMan(int turret) {
	ASSERT( turret >= 0 && turret < this->n_turrets );
	this->turret_man[turret] = -1;
}

void Building::setTurretMan(int turret, int epoch) {
	ASSERT( turret >= 0 && turret < this->n_turrets );
	this->turret_man[turret] = epoch;
}

Sector::Sector(PlayingGameState *gamestate, int epoch, int xpos, int ypos, MapColour map_colour) :
gamestate(gamestate), epoch(epoch), xpos(xpos), ypos(ypos)
{
    //LOG("Sector::Sector(%d,%d,%d)\n",epoch,xpos,ypos);
	ASSERT_EPOCH(epoch);
	this->designs = new vector<Design *>();

	for(int i=0;i<N_ID;i++) {
		this->elements[i] = 0;
	}
	for(int i=0;i<n_players_c;i++) {
		this->armies[i] = new Army(gamestate, this, i);
	}

	this->features = new vector<Feature *>();

	// rocks etc
	int n_clutter = icon_clutter[0] != NULL ? (1 + rand() % 4) : 0;
	for(int i=0;i<n_clutter;i++) {
		int land_width = land[(int)map_colour]->getScaledWidth() - 32;
		int land_height = land[(int)map_colour]->getScaledHeight() - 32;
		int xpos = offset_land_x_c + rand() % land_width;
		int ypos = offset_land_y_c + rand() % land_height;
		Image **image_ptr = &icon_clutter[rand() % n_clutter_c];
		Feature *feature = new Feature(image_ptr, 1, xpos, ypos);
		this->features->push_back(feature);
	}
	// trees (should be after clutter, so they are drawn over them if overlapping)
	int cx = offset_land_x_c + 16;
	for(;;) {
		int treetype = rand() % 3;
		if( treetype == 2 )
			treetype = 3;
		//Image *image = icon_trees[treetype];
		Image *image = icon_trees[treetype][0];
		cx += rand() % 16;
		if( cx + image->getScaledWidth() > offset_land_x_c + land_width_c )
			break;
		//int ypos = offset_land_y_c - image->getScaledHeight() + 12 + rand() % 12;
		int ypos = offset_land_y_c - image->getScaledHeight() + 16 + rand() % 8;
		//Feature *feature = new Feature(icon_trees[treetype], cx, ypos);
		Feature *feature = new Feature(icon_trees[treetype], n_tree_frames_c, cx, ypos);
		//Feature *feature = new Feature(image, cx, ypos);
		//this->features->add(feature);
		this->features->push_back(feature);
		cx += image->getScaledWidth() - 8;
	}
	cx = offset_land_x_c + 16;
	for(;;) {
		int treetype = rand() % 3;
		if( treetype == 2 )
			treetype = 3;
		Image *image = icon_trees[treetype][0];
		cx += rand() % 32;
		if( cx + image->getScaledWidth() > offset_land_x_c + land_width_c )
			break;
		int ypos = offset_land_y_c + land[(int)map_colour]->getScaledHeight() - image->getScaledHeight() - 8 - rand() % 4;
		Feature *feature = new Feature(icon_trees[treetype], n_tree_frames_c, cx, ypos);
		feature->setAtFront(true);
		this->features->push_back(feature);
		cx += image->getScaledWidth() - 8;
	}

	this->nuked = false;
	this->trees_nuked = false;
	this->nuke_by_player = -1;
	this->nuke_time = -1;

	this->smokeParticleSystem = NULL;
	if( smoke_image != NULL ) {
		this->smokeParticleSystem = new SmokeParticleSystem(smoke_image);
	}

	initTowerStuff();
}

void Sector::initTowerStuff() {
    //LOG("Sector::initTowerStuff() [%d, %d]\n", xpos, ypos);
	this->player = PLAYER_NONE;
	this->is_shutdown = false;
	//this->shutdown_player = -1;
	this->population = 0;
	this->n_designers = 0;
	this->n_famount = 0;
	for(int i=0;i<N_BUILDINGS;i++) {
		this->n_builders[i] = 0;
		this->buildings[i] = NULL;
	}
	this->current_design = NULL;
	this->researched = 0;
	this->researched_lasttime = -1;
	this->current_manufacture = NULL;
	this->manufactured = 0;
	this->manufactured_lasttime = -1;
	this->growth_lasttime = -1;
	this->mined_lasttime = -1;
	for(int i=0;i<n_players_c;i++)
		this->built_towers[i] = 0;
	for(int i=0;i<N_BUILDINGS;i++)
		this->built[i] = 0;
	this->built_lasttime = -1;
	for(int i=0;i<3;i++) {
		for(int j=0;j<n_epochs_c;j++) {
			this->inventions_known[i][j] = false;
			//this->inventions_known[i][j] = true;
		}
	}
	this->designs->clear();
	for(int i=0;i<N_ID;i++) {
		this->n_miners[i] = 0;
		this->elementstocks[i] = 0;
		this->partial_elementstocks[i] = 0;
	}

	//this->assembled_army = new Army(this, this->getPlayer());
	//this->stored_army = new Army(this, this->getPlayer());
	this->assembled_army = NULL;
	this->stored_army = NULL;
	for(int i=0;i<n_epochs_c;i++)
		this->stored_defenders[i] = 0;
	for(int i=0;i<4;i++)
		this->stored_shields[i] = 0;

	//this->n_workers = 0;
	this->setWorkers(0); // call to also set the particle system rate; must be done after setting the buildings to NULL
}

Sector::~Sector() {
    //LOG("Sector::~Sector() [%d: %d, %d]\n", player, xpos, ypos);
	for(unsigned int i=0;i<this->features->size();i++) {
		//Feature *feature = (Feature *)this->features->get(i);
		Feature *feature = (Feature *)this->features->at(i);
		delete feature;
	}
	delete this->features;

	for(unsigned int i=0;i<N_BUILDINGS;i++) {
		if(this->buildings[i] != NULL )
			delete this->buildings[i];
	}
	if( this->assembled_army != NULL )
		delete this->assembled_army;
	if( this->stored_army != NULL )
		delete this->stored_army;
	for(int i=0;i<n_players_c;i++)
		delete this->armies[i];
	delete this->designs;

	if( smokeParticleSystem != NULL ) {
		delete smokeParticleSystem;
	}
}

void Sector::createTower(int player,int population) {
    //LOG("Sector::createTower(%d,%d) [%d, %d]\n", player, population, xpos, ypos);
	ASSERT( !nuked );
	this->player = player;
	this->assembled_army = new Army(gamestate, this, this->getPlayer());
	this->stored_army = new Army(gamestate, this, this->getPlayer());
	this->population = population;
	this->buildings[BUILDING_TOWER] = new Building(gamestate, this, BUILDING_TOWER);
}

void Sector::destroyTower(bool nuked, int client_player) {
	LOG("Sector::destroyTower(%d) [%d: %d, %d]\n", nuked, player, xpos, ypos);
	ASSERT_PLAYER(this->player);
	if( !nuked ) {
		this->evacuate();
	}
	/*delete this->buildings[BUILDING_TOWER];
	this->buildings[BUILDING_TOWER] = NULL;*/
	for(int i=0;i<N_BUILDINGS;i++) {
		if(this->buildings[i] != NULL) {
			delete this->buildings[i];
			this->buildings[i] = NULL;
		}
	}

	delete this->assembled_army;
	this->assembled_army = NULL;
	delete this->stored_army;
	this->stored_army = NULL;

	int this_player = this->player; // keep a copy

	initTowerStuff();
	if( this == gamestate->getCurrentSector() ) {
		gamestate->getGamePanel()->setPage(GamePanel::STATE_SECTORCONTROL);
		gamestate->getGamePanel()->setup();
	}
	gamestate->getGamePanel()->refreshShutdown();

	if( nuked || playerAlive(this_player) ) {
		// player has other sectors
		if( isDemo() ) {
		}
		else if( this_player == client_player ) {
			if( nuked )
				playSample(s_weve_been_nuked);
			else
				playSample(s_sector_destroyed);
			//((PlayingGameState *)gamestate)->setFlashingSquare(this->xpos, this->ypos);
			gamestate->setFlashingSquare(this->xpos, this->ypos);
		}
		else if( ( nuked && nuke_by_player == client_player ) || ( !nuked && this->getArmy(client_player)->getTotal() > 0 ) ) {
			if( nuked && nuke_by_player == client_player )
				playSample(s_weve_nuked_them);
			else
				playSample(s_conquered);
			//((PlayingGameState *)gamestate)->setFlashingSquare(this->xpos, this->ypos);
			gamestate->setFlashingSquare(this->xpos, this->ypos);
		}
	}
	// game win/lose is handled in main game loop now
	LOG("Sector::destroyTower() exit\n");
}

void Sector::destroyBuilding(Type building_type,int client_player) {
	this->destroyBuilding(building_type, false, client_player);
}

void Sector::destroyBuilding(Type building_type,bool silent,int client_player) {
	LOG("Sector::destroyBuilding(%d) [%d: %d, %d]\n", building_type, player, xpos, ypos);
	if( this == gamestate->getCurrentSector() && !silent ) {
		playSample(s_buildingdestroyed, SOUND_CHANNEL_FX);
	}
	if( building_type == BUILDING_TOWER ) {
		this->destroyTower(false, client_player);
		return;
	}
	else if( building_type == BUILDING_MINE ) {
		if( this->player == client_player && !silent ) {
			playSample(s_mine_destroyed);
			//((PlayingGameState *)gamestate)->setFlashingSquare(this->xpos, this->ypos);
			gamestate->setFlashingSquare(this->xpos, this->ypos);
		}
		for(int i=0;i<N_ID;i++) {
			/*
			// n.b., we kill the open pit element miners too, as the mine supersedes the open pit mine
			this->population -= this->n_miners[i];
			*/
			this->n_miners[i] = 0;
		}
		/*if( this->population < 0 ) {
			// shouldn't happen!
			this->population = 0;
		}*/
	}
	else if( building_type == BUILDING_FACTORY ) {
		if( this->player == client_player && !silent ) {
			playSample(s_factory_destroyed);
			//((PlayingGameState *)gamestate)->setFlashingSquare(this->xpos, this->ypos);
			gamestate->setFlashingSquare(this->xpos, this->ypos);
		}
		/*this->population -= this->n_workers;
		if( this->population < 0 ) {
			// shouldn't happen!
			this->population = 0;
		}*/
		this->setWorkers(0); // call to also set the particle system rate
		this->current_manufacture = NULL; // so the player can't try to set workers again!
	}
	else if( building_type == BUILDING_LAB ) {
		if( this->player == client_player && !silent ) {
			playSample(s_lab_destroyed);
			//((PlayingGameState *)gamestate)->setFlashingSquare(this->xpos, this->ypos);
			gamestate->setFlashingSquare(this->xpos, this->ypos);
		}
		if( this->current_design != NULL && this->current_design->getInvention()->getEpoch() > lab_epoch_c ) {
			// for pre-lab epoch designs, we can imagine the designers can work without being in the lab...
			/*this->population -= this->n_designers;
			if( this->population < 0 ) {
				// shouldn't happen!
				this->population = 0;
			}*/
			this->setDesigners(0);
			this->current_design = NULL; // so the player can't try to set designers again!
		}
	}
	else {
		ASSERT(0);
	}

	// also return defenders
	for(int j=0;j<this->buildings[(int)building_type]->getNTurrets();j++) {
		if( this->buildings[(int)building_type]->getTurretMan(j) != -1 )
			this->returnDefender(this->buildings[(int)building_type], j);
	}

	delete this->buildings[(int)building_type];
	this->buildings[(int)building_type] = NULL;
	this->built[(int)building_type] = 0;

	if( this == gamestate->getCurrentSector() && this->player == client_player ) {
		gamestate->getGamePanel()->refresh();
	}
}

bool Sector::canShutdown() const {
	//ASSERT( !this->is_shutdown );
	if( this->is_shutdown )
		return false;

	if( this->getEpoch() == n_epochs_c-1 && start_epoch != end_epoch_c ) // in 2100AD
	{
		for(int y=0;y<map_height_c;y++) {
			for(int x=0;x<map_width_c;x++) {
				//if( map->sector_at[x][y] ) {
				if( getMap()->isSectorAt(x, y) ) {
					const Sector *sector = getMap()->getSector(x, y);
					if( ( sector != gamestate->getCurrentSector() && sector->getPlayer() == this->getPlayer() )
						|| sector->getArmy(this->getPlayer())->any(true) ) {
							return true;
					}
				}
			}
		}
	}
	return false;
}

void Sector::shutdown() {
	LOG("Sector::shutdown [%d: %d, %d]\n", player, xpos, ypos);
	ASSERT_PLAYER( this->player );
	ASSERT( this->player != -1 );
	ASSERT( !this->is_shutdown );
	/*this->shutdown_player = this->player;
	this->player = -1;*/
	for(int i=0;i<N_BUILDINGS;i++) {
		if( this->buildings[i] != NULL ) {
			for(int j=0;j<this->buildings[i]->getNTurrets();j++) {
				if( this->buildings[i]->getTurretMan(j) != -1 )
					this->returnDefender(this->buildings[i], j);
			}
		}
		if( i != BUILDING_TOWER )
			this->destroyBuilding((Type)i, true);
	}

	this->is_shutdown = true;
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->reset();
	}
}

Building *Sector::getBuilding(Type type) const {
	//LOG("Sector::getBuilding(%d)\n",type);
	return this->buildings[type];
}

int Sector::getNDefenders() const {
	//LOG("Sector::getNDefenders()\n");
	int n = 0;
	for(int i=0;i<N_BUILDINGS;i++) {
		if( this->buildings[i] != NULL )
			n += buildings[i]->getNDefenders();
	}
	return n;
}

int Sector::getNDefenders(int type) const {
	int n = 0;
	for(int i=0;i<N_BUILDINGS;i++) {
		if( this->buildings[i] != NULL )
			n += buildings[i]->getNDefenders(type);
	}
	return n;
}

int Sector::getDefenderStrength() const {
	//LOG("Sector::getDefenderStrength()\n");
	int n = 0;
	for(int i=0;i<N_BUILDINGS;i++) {
		if( this->buildings[i] != NULL )
			n += buildings[i]->getDefenderStrength();
	}
	return n;
}

void Sector::killDefender(int index) {
	LOG("Sector::killDefender(%d) [%d: %d, %d]\n", index, player, xpos, ypos);
	for(int i=0;i<N_BUILDINGS;i++) {
		if( this->buildings[i] != NULL ) {
			if( index < buildings[i]->getNDefenders() ) {
				buildings[i]->killDefender(index);
				break;
			}
			index -= buildings[i]->getNDefenders();
		}
	}
}

bool Sector::canBuild(Type type) const {
	//LOG("Sector::canBuild(%d)\n",type);
	if( start_epoch == end_epoch_c )
		return false;

	if( getBuilding(type) != NULL )
		return false;

	if( type == BUILDING_MINE ) {
		return this->epoch >= mine_epoch_c;
	}
	if( type == BUILDING_FACTORY ) {
		return this->epoch >= factory_epoch_c;
	}
	if( type == BUILDING_LAB ) {
		return this->epoch >= lab_epoch_c;
	}

	return false;
}

bool Sector::canMine(Id id) const {
	ASSERT_ELEMENT_ID(id);
	//LOG("Sector::canMine(%d)\n",id);
	if( !this->anyElements(id) )
		return false;

	if( ::elements[id]->getType() == Element::GATHERABLE )
		return true;
	else if( ::elements[id]->getType() == Element::OPENPITMINE )
		return ( this->epoch >= 1 );
	else if( ::elements[id]->getType() == Element::DEEPMINE )
		return (this->buildings[BUILDING_MINE] != NULL);

	// error
	LOG("###Didn't expect element id %d of type %d\n",id,::elements[id]->getType());
	ASSERT(0);
	return false;
}

Design *Sector::canBuildDesign(Invention::Type type,int epoch) const {
	//LOG("Sector::canBuildDesign(%d,%d)\n",type,epoch);
	ASSERT_EPOCH(epoch);
	if(epoch < start_epoch || epoch > start_epoch + 3 )
		return NULL; // can't build this
	if( epoch >= factory_epoch_c && this->getBuilding(BUILDING_FACTORY) == NULL )
		return NULL; // need a factory for this

	Invention *invention = Invention::getInvention(type, epoch);
	ASSERT(invention != NULL);
	Design *best_design = NULL;
	for(unsigned int i=0;i<this->designs->size() && best_design==NULL;i++) {
		//Design *design = (Design *)this->designs->elementAt(i);
		Design *design = this->designs->at(i);
		// design should be non-NULL, but to satisfy VS Code Analysis...
		if( design == NULL )
			continue;
		if( design->getInvention() != invention )
			continue;
		bool ok = canBuildDesign(design);
		if( ok )
			best_design = design;
	}
	return best_design;
}

bool Sector::canBuildDesign(Design *design) const {
	//LOG("Sector::canBuildDesign(%d)\n",design);
	if( design->getInvention()->getEpoch() >= factory_epoch_c && this->getBuilding(BUILDING_FACTORY) == NULL )
		return false; // need a factory for this

	bool ok = true;
	for(int j=0;j<N_ID && ok;j++) {
		if( design->getCost((Id)j) > this->elementstocks[j] ) {
			// not enough elements
			ok = false;
		}
	}
	return ok;
}

bool Sector::canEverBuildDesign(Design *design) const {
	//LOG("Sector::canBuildDesign(%d)\n",design);
	bool ok = true;
	for(int j=0;j<N_ID && ok;j++) {
		if( design->getCost((Id)j) > this->elementstocks[j] + this->elements[j] ) {
			// not enough elements
			ok = false;
		}
	}
	return ok;
}

void Sector::autoTrashDesigns() {
	for(unsigned int i=0;i<this->designs->size();i++) {
		//Design *design = (Design *)this->designs->elementAt(i);
		Design *design = this->designs->at(i);
		if( !canEverBuildDesign(design) ) {
			// trash
			this->trashDesign(design);
		}
	}
}

bool Sector::tryMiningMore() const {
	// todo: only check elements we can mine (we might not be able to advance to that epoch)
	for(int i=0;i<N_ID;i++) {
		//Element *element = ::elements[i];
		if( this->elements[i] > 0 &&
			this->elementstocks[i] < 6 * element_multiplier_c
			//( element->type != Element::GATHERABLE || this->elementstocks[i] < max_gatherables_stored_c * element_multiplier_c )
			) {
				// if there are less than 6 of this element, and we can mine more, then do so
				return true;
		}
	}
	return false;
}

bool Sector::usedUp() const {
	// a sector is used up iff:
	//   we have no weapon designs (they are all trashed)
	//   we can't design anything new
	//   a lab could be built, but it hasn't been built
	//   there are no more element stocks to mine
	//   a mine could be built, but it hasn't been built
	/*if( this->designs->size() > 0 )
	return false;*/
	if( this->canBuild(BUILDING_LAB) ) {
		// should try building a lab
		return false;
	}
	if( this->canBuild(BUILDING_MINE) ) {
		// should try building a mine
		return false;
	}
	for(unsigned int i=0;i<this->designs->size();i++) {
		//Design *design = (Design *)this->designs->elementAt(i);
		Design *design = this->designs->at(i);
		if( design->getInvention()->getType() == Invention::WEAPON )
			return false;
	}
	for(int i=0;i<n_epochs_c;i++) {
		if( this->canResearch(Invention::WEAPON, i) )
			return false;
		if( this->canResearch(Invention::DEFENCE, i) )
			return false;
		if( this->canResearch(Invention::SHIELD, i) )
			return false;
	}
	if( tryMiningMore() ) {
		return false;
	}
	return true;
}

Design *Sector::knownDesign(Invention::Type type,int epoch) const {
	//LOG("Sector::knownDesign(%d,%d)\n",type,epoch);
	ASSERT_EPOCH(epoch);
	Invention *invention = Invention::getInvention(type, epoch);
	ASSERT(invention != NULL);
	Design *best_design = NULL;
	//for(unsigned int i=0;i<this->designs->size();i++) {
	for(unsigned int i=0;i<this->designs->size() && best_design==NULL;i++) {
		Design *design = this->designs->at(i);
		// design should be non-NULL, but to satisfy VS Code Analysis...
		if( design == NULL )
			continue;
		if( design->getInvention() == invention ) {
			//ASSERT( best_design == NULL );
			best_design = design;
		}
	}
	return best_design;
}

Design *Sector::bestDesign(Invention::Type type,int epoch) const {
	//LOG("Sector::bestDesign(%d,%d)\n",type,epoch);
	ASSERT_EPOCH(epoch);
	Invention *invention = Invention::getInvention(type, epoch);
	ASSERT(invention != NULL);
	Design *best_design = NULL;
	// todo: prefer gatherables; prefer more elements (eg, 0.5 wood + 0.5 bone is preferred over 1 wood or 1 bone);
	for(size_t i=0;i<invention->getNDesigns();i++) {
		//Design *design = (Design *)invention->designs->elementAt(i);
		Design *design = invention->getDesign(i);
		bool ok = true;
		for(int j=0;j<N_ID && ok;j++) {
			if( design->getCost((Id)j) > this->elementstocks[j] ) {
				// not enough elements
				ok = false;
			}
		}
		if( ok ) {
			if( best_design == NULL )
				best_design = design;
			else if( design->isErgonomicallyTerrific() )
				best_design = design;
		}
	}
	return best_design;
}

void Sector::trashDesign(Invention *invention) {
	LOG("Sector::trashDesign(%d) [%d: %d, %d]\n", invention, player, xpos, ypos);
	this->inventions_known[ invention->getType() ][ invention->getEpoch() ] = false;
	for(unsigned int i=0;i<this->designs->size();i++) {
		//Design *design = (Design *)this->designs->elementAt(i);
		Design *design = this->designs->at(i);
		if( design->getInvention() == invention ) {
			//this->designs->remove(i);
			this->designs->erase(this->designs->begin() + i);
			break;
		}
	}
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
}

void Sector::trashDesign(Design *design) {
	LOG("Sector::trashDesign(%d) [%d: %d, %d]\n", design, player, xpos, ypos);
	this->inventions_known[ design->getInvention()->getType() ][ design->getInvention()->getEpoch() ] = false;
	for(unsigned int i=0;i<this->designs->size();i++) {
		//Design *this_design = (Design *)this->designs->elementAt(i);
		Design *this_design = this->designs->at(i);
		if( this_design == design ) {
			//this->designs->remove(i);
			this->designs->erase(this->designs->begin() + i);
			break;
		}
	}
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
}

void Sector::nukeSector(Sector *source) {
	LOG("Sector::nuke(%d) [%d: %d, %d]\n", source, player, xpos, ypos);
	ASSERT( source->player != -1 );
	ASSERT( this->player != source->player );
	ASSERT( this->player == -1 || !Player::isAlliance( this->player, source->player ) );
	if( isBeingNuked() ) {
		ASSERT( this->nuke_time != -1 );
		//return false;
		return;
	}

	// we still start the timer, even if a laser will shoot it
	this->nuke_by_player = source->player;
	this->nuke_time = getGameTime();

	if( this->getActivePlayer() != -1 ) {
		// nuke defences
		bool done = false;
		for(int i=0;i<N_BUILDINGS && !done;i++) {
			if( this->buildings[i] != NULL ) {
				for(int j=0;j<this->buildings[i]->getNTurrets() && !done;j++) {
					if( this->buildings[i]->getTurretMan(j) == nuclear_epoch_c ) {
						this->buildings[i]->clearTurretMan(j);
						source->nukeSector(this);
						done = true;
					}
				}
			}
		}
	}

	//return true;
}

Design *Sector::canResearch(Invention::Type type,int epoch) const {
	//LOG("Sector::canResearch(%d,%d)\n",type,epoch);
	ASSERT_EPOCH(epoch);
	if( epoch < start_epoch || epoch > start_epoch + 3 )
		return NULL; // can't research this
	if( epoch > lab_epoch_c && this->getBuilding(BUILDING_LAB) == NULL )
		return NULL; // need a lab for this

	if( this->inventionKnown(type, epoch) )
		return NULL; // already know it, so can't reserch it again

	return this->bestDesign(type, epoch);
}

int Sector::getNDesigns() const {
	//LOG("Sector::getNDesigns()\n");
	return this->designs->size();
}

void Sector::consumeStocks(Design *design) {
    // disable logging, performance issue on mobile devices (Symbian)
    //LOG("Sector::consumeStocks(%d) [%d: %d, %d]\n", design, player, xpos, ypos);
	for(int j=0;j<N_ID;j++) {
		// we should have enough elements here!
		ASSERT(this->elementstocks[j] >= design->getCost((Id)j));
		this->reduceElementStocks((Id)j, design->getCost((Id)j));
	}
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
}

void Sector::buildDesign(Design *design) {
	//LOG("Sector::buildDesign(%d : %s) [%d: %d, %d]\n", design, design->getInvention()->getName(), player, xpos, ypos);
/*#ifdef _DEBUG
	LOG("### Sector::buildDesign a\n");
	map->checkSectors();
#endif*/
	Invention *invention = design->getInvention();
	if( invention->getType() == Invention::WEAPON ) {
		this->getStoredArmy()->add(invention->getEpoch(), 1);
	}
	else if( invention->getType() == Invention::DEFENCE ) {
		this->stored_defenders[invention->getEpoch()]++;
	}
	else if( invention->getType() == Invention::SHIELD ) {
		int shield = invention->getEpoch() - start_epoch;
		ASSERT_SHIELD(shield);
		this->stored_shields[shield]++;
	}
	else {
		ASSERT(0);
	}
/*#ifdef _DEBUG
	//LOG("### Sector::buildDesign b\n");
	map->checkSectors();
#endif*/
}

bool Sector::assembleArmy(int epoch,int n) {
	//LOG("Sector::assembleArmy(%d,%d)\n",epoch,n);
	ASSERT_EPOCH(epoch);
	int n_population = this->getPopulation();
	int n_spare = this->getAvailablePopulation();
	int n_men = invention_weapons[epoch]->getNMen() * n;
	if( n_spare < n_men ) {
		n = 1;
		n_men = invention_weapons[epoch]->getNMen() * n;
	}
	if( n_spare < n_men )
		return false;

	int n_make = this->getStoredArmy()->getSoldiers(epoch);
	if( n_make > n ) {
		n_make = n;
	}
	while( n_make < n ) {
		if( epoch >= factory_epoch_c )
			break;
		// make a new one?
		Design *design = this->canBuildDesign(Invention::WEAPON, epoch);
		if( design == NULL )
			break;
		this->consumeStocks(design);
		this->getStoredArmy()->add(epoch, 1);
		n_make++;
	}
	if( n_make > 0 ) {
		this->getAssembledArmy()->add(epoch, n_make);
		this->getStoredArmy()->remove(epoch, n_make);
		n_men = invention_weapons[epoch]->getNMen() * n_make;
		this->setPopulation( n_population - n_men );
	}
	return (n_make == n);
}

bool Sector::deployDefender(Building *building,int turret,int epoch) {
	//LOG("Sector::deployDefender(%d,%d,%d) [%d: %d, %d]\n", building->type, turret, epoch, player, xpos, ypos);
	ASSERT_EPOCH(epoch);
	int n_population = this->getPopulation();
	int n_spare = this->getAvailablePopulation();

	bool need_new_man = false;
	if( building->getTurretMan(turret) == -1 && defenceNeedsMan(epoch) ) {
		// need a new man
		if( n_spare <= 0 )
			return false;
		else
			need_new_man = true;
	}

	if( this->stored_defenders[epoch] == 0 ) {
		if( epoch >= factory_epoch_c ) {
			return false;
		}
		// not enough - try to make one
		Design *design = this->canBuildDesign(Invention::DEFENCE, epoch);
		if( design == NULL )
			return false;
		this->consumeStocks(design);
		this->stored_defenders[epoch]++;
	}

	LOG("deploying a new defender(%d,%d,%d) [%d: %d, %d]\n", building->getType(), turret, epoch, player, xpos, ypos);
	if( building->getTurretMan(turret) != -1 ) {
		// return current defender to stocks
		this->stored_defenders[ building->getTurretMan(turret) ]++;
	}
	this->stored_defenders[epoch]--;
	building->setTurretMan(turret, epoch);
	if( need_new_man )
		this->setPopulation( n_population - 1 );
	return true;
}

void Sector::returnDefender(Building *building,int turret) {
	LOG("Sector::returnDefender(%d,%d) [%d: %d, %d]\n", building->getType(), turret, player, xpos, ypos);
	ASSERT( building->getTurretMan(turret) != -1 );
	if( defenceNeedsMan( building->getTurretMan(turret) ) ) {
		int n_population = this->getPopulation();
		this->setPopulation( n_population + 1 );
	}
	this->stored_defenders[ building->getTurretMan(turret) ]++;
	building->clearTurretMan(turret);
}

int Sector::getStoredDefenders(int epoch) const {
	//LOG("Sector::getStoredDefenders(%d)\n",epoch);
	ASSERT_EPOCH(epoch);
	return this->stored_defenders[epoch];
}

bool Sector::useShield(Building *building,int shield) {
	ASSERT_SHIELD(shield);
	if( this->stored_shields[shield] == 0 ) {
		if( start_epoch + shield >= factory_epoch_c )
			return false;
		// not enough - try to make one
		Design *design = this->canBuildDesign(Invention::SHIELD, start_epoch + shield);
		if( design == NULL )
			return false; // can't make a new one
		this->consumeStocks(design);
		this->stored_shields[shield]++;
	}
	LOG("-> Use Shield %d on building %d type %d\n", shield, building, building->getType());
	//building->addHealth( 10 * ( shield + 1 ) );
	building->addHealth( 5 * ( shield + 1 ) );
	this->stored_shields[shield]--;
	return true;
}

int Sector::getStoredShields(int shield) const {
	ASSERT_SHIELD(shield);
	return this->stored_shields[shield];
}

int Sector::getDefenceStrength() const {
	//LOG("Sector::getDefenceStrength() [%d: %d, %d]\n", player, xpos, ypos);
	/*int str = 1;
	for(int i=0;i<=this->epoch;i++)
	str *= 2;*/
	//int str = this->epoch + 1;
	int str = 0;
	if( start_epoch == end_epoch_c ) {
		str = 4;
	}
	else {
		//str = ( this->epoch - start_epoch ) + 1;
		str = 1;
	}
	return str;
}

void Sector::doCombat(int client_player) {
	//LOG("Sector::doCombat()\n");
	int looptime = getLoopTime();

	/*int army_strengths[n_players_c];
	int n_armies = 0;
	for(i=0;i<n_players_c;i++) {
	army_strengths[i] = this->getArmy(i)->getStrength();
	if( army_strengths[i] > 0 )
	n_armies++;
	}
	if( n_armies == 2 ) {
	Army *friendly = this->getArmy(human_player);
	Army *enemy = this->getArmy(enemy_player);
	}*/
	bool died[n_players_c];
	//int random = rand () % RAND_MAX;
	for(int i=0;i<n_players_c;i++) {
		died[i] = false;
		Army *army = this->getArmy(i);
		int this_strength = army->getStrength();
		int this_total = army->getTotal();
		//LOG(">>> %d , %d\n", this_strength, this_total);
		if( this->player == i ) {
			this_total += this->getNDefenders();
			this_strength += this->getDefenderStrength();
		}
		//LOG("    %d , %d\n", this_strength, this_total);
		if( this_total > 0 ) {
			int enemy_strength = 0;
			int enemy_total = 0;
			for(int j=0;j<n_players_c;j++) {
				if( i != j && !Player::isAlliance(i,j) ) {
					enemy_strength += this->getArmy(j)->getStrength();
					enemy_total += this->getArmy(j)->getTotal();
					if( this->player == j ) {
						enemy_strength += this->getDefenderStrength();
						enemy_total += this->getNDefenders();
					}
				}
			}
			if( enemy_strength > 0 ) {
				//int death_rate = ( combat_rate_c * gameticks_per_hour_c * this_strength ) / enemy_strength;
				//int death_rate = ( combat_rate_c * gameticks_per_hour_c ) / enemy_strength;
				//int death_rate = ( combat_rate_c * gameticks_per_hour_c * this_strength ) / ( enemy_total * enemy_strength );
				// need to use floats, to avoid possible overflow!!!
				float death_rate = ((float)this_strength) / ((float)enemy_total);
				death_rate = death_rate * ((float)(combat_rate_c * gameticks_per_hour_c)) / ((float)enemy_strength);
				int prob = poisson((int)death_rate, looptime);
				int random = rand() % RAND_MAX;
				if( random <= prob ) {
					// soldier has died
					died[i] = true;
				}
			}
		}
	}
	for(int i=0;i<n_players_c;i++) {
		if( died[i] ) {
			Army *army = this->getArmy(i);
			int this_total = army->getTotal();
			if( this->player == i )
				this_total += this->getNDefenders();

			int die = rand() % this_total;
			if( die < army->getTotal() ) {
				army->kill(die);
			}
			else {
				// kill a defender
				this->killDefender(die - army->getTotal());
			}
		}
	}

	// damage to buildings
	if( this->player != -1 ) {
		int bombard = 0;
		for(int i=0;i<n_players_c;i++) {
			if( this->player != i && !Player::isAlliance(this->player, i) ) {
				bombard += this->getArmy(i)->getBombardStrength();
			}
		}
		if( bombard > 0 ) {
			int bombard_rate = ( bombard_rate_c * gameticks_per_hour_c ) / bombard;
			//bombard_rate *= ( this->epoch + 1 );
			bombard_rate *= this->getDefenceStrength();
			int prob = poisson(bombard_rate, looptime);
			int random = rand() % RAND_MAX;
			if( random <= prob ) {
				// caused some damage
				int n_buildings = 0;
				for(int i=0;i<N_BUILDINGS;i++) {
					if( this->buildings[i] != NULL )
						n_buildings++;
				}
				ASSERT( n_buildings > 0 );
				int b = rand() % n_buildings;
				for(int i=0;i<N_BUILDINGS;i++) {
					Building *building = this->buildings[i];
					if( building != NULL ) {
						if( b == 0 ) {
							building->addHealth(-1);
#ifdef _DEBUG
                            // disable in Release mode as possible performance issue on mobile devices (Symbian)
                            LOG("Sector [%d: %d, %d] caused some damage on building %d, type %d, %d remaining\n", player, xpos, ypos, building, building->getType(), building->getHealth());
#endif
							if( building->getHealth() <= 0 ) {
								// destroy building
								/*delete building;
								this->buildings[i] = NULL;*/
								destroyBuilding((Type)i, client_player);
							}
							else if( this->player == client_player && building->getHealth() == 10 && building->getType() == BUILDING_TOWER ) {
								playSample(s_tower_critical);
								//((PlayingGameState *)gamestate)->setFlashingSquare(this->xpos, this->ypos);
								gamestate->setFlashingSquare(this->xpos, this->ypos);
							}
							break;
						}
						b--;
					}
				}
			}
		}
	}
}

void Sector::doPlayer(int client_player) {
	//LOG("Sector::doPlayer()\n");
	// stuff for sectors owned by a player

	if( this->getParticleSystem() != NULL ) {
		this->getParticleSystem()->update();
	}

	if( gameMode == GAMEMODE_MULTIPLAYER_CLIENT ) {
		// rest of function is for game logic done by server
		return;
	}

	int time = getGameTime();
	int looptime = getLoopTime();

	if( this->current_design != NULL ) {
		if( this->researched_lasttime == -1 )
			this->researched_lasttime = time;
		while( time - this->researched_lasttime > gameticks_per_hour_c ) {
			this->researched += this->getDesigners();
			this->researched_lasttime += gameticks_per_hour_c;
		}
		int cost = this->getInventionCost();
		if( this->researched > cost ) {
			//LOG("Sector [%d: %d, %d] has made a %s\n", player, xpos, ypos, this->current_design->getInvention()->getName());
			bool done_sound = false;
			if( this->player != client_player )
				done_sound = true;
			this->inventions_known[ this->current_design->getInvention()->getType() ][ this->current_design->getInvention()->getEpoch() ] = true;
			//this->designs->add( this->current_design );
			this->designs->push_back( this->current_design );

			if( this->epoch < start_epoch + 3 && epoch < n_epochs_c-1 ) {
				//int levels[4] = {0, 0, 0, 0};
				int score = 0;
				for(int i=0;i<3;i++) {
					int val = 1;
					for(int j=0;j<4;j++) {
						if( start_epoch+j < n_epochs_c ) {
							if( this->inventions_known[i][start_epoch+j] ) {
								//levels[j]++;
								score += val;
							}
						}
						val *= 2;
					}
				}
				/*if( levels[0] >= 3 || levels[1] >= 2 || levels[2] >= 1 || levels[3] >= 1 ) {
				// advance a tech level!
				this->epoch++;
				if( !done_sound ) {
				playSample(s_advanced_tech);
				done_sound = true;
				}
				}*/
				score = ( score + 3 ) / 3;
				int new_epoch = start_epoch;
				while( score > 1 ) {
					new_epoch++;
					score /= 2;
				}
				if( new_epoch > this->epoch ) {
					// advance a tech level!
					this->epoch = new_epoch;
					LOG("Sector [%d: %d, %d] has advanced to tech level %d\n", player, xpos, ypos, epoch);
					if( !done_sound ) {
						playSample(s_advanced_tech);
						done_sound = true;
					}
				}
			}
			if( this == gamestate->getCurrentSector() ) {
				//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
				gamestate->getGamePanel()->refresh();
			}
			if( !done_sound ) {
				if( this->current_design->isErgonomicallyTerrific() )
					playSample(s_ergo);
				else
					playSample(s_design_is_ready);
			}
			if( this->player == client_player ) {
				//((PlayingGameState *)gamestate)->setFlashingSquare(this->xpos, this->ypos);
				gamestate->setFlashingSquare(this->xpos, this->ypos);
			}

			this->setCurrentDesign(NULL);
		}
	}

	if( this->current_manufacture != NULL ) {
		if( this->manufactured_lasttime == -1 )
			this->manufactured_lasttime = time;
		if( this->manufactured == 0 && this->getWorkers() > 0 ) {
			if( !this->canBuildDesign( this->current_manufacture ) ) {
				// not enough elements
				// therefore production run completed
				if( this->player == client_player ) {
					playSample(s_fcompleted);
					gamestate->setFlashingSquare(this->xpos, this->ypos);
				}
				this->setCurrentManufacture(NULL);
				if( this == gamestate->getCurrentSector() ) {
					//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
					gamestate->getGamePanel()->refresh();
				}
			}
			else {
				this->consumeStocks( this->current_manufacture );
			}
		}
	}
	// a new if statement, as production run may have ended due to lack of elements
	if( this->current_manufacture != NULL ) {
		while( time - this->manufactured_lasttime > gameticks_per_hour_c ) {
			this->manufactured += this->getWorkers();
			this->manufactured_lasttime += gameticks_per_hour_c;
		}
		if( this->manufactured == 0 && this->getWorkers() > 0 ) {
			this->manufactured++; // a bit hacky; just to avoid consuming stocks again
		}
		int cost = this->getManufactureCost();
		if( this->manufactured > cost ) {
			this->buildDesign( this->current_manufacture );
			this->manufactured = 0;
			if( this->n_famount != infinity_c )
				this->setFAmount( this->n_famount - 1 );
			if( this->n_famount == 0 ) {
				// production run completed
				//LOG("production run completed\n");
				if( this->player == client_player ) {
					playSample(s_fcompleted);
					gamestate->setFlashingSquare(this->xpos, this->ypos);
				}
				this->setCurrentManufacture(NULL);
			}
			if( this == gamestate->getCurrentSector() ) {
				//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
				gamestate->getGamePanel()->refresh();
			}
		}
	}

	bool new_stocks = false;
	for(int i=0;i<N_ID;i++) {
		if( this->elements[i] == 0 ) {
			continue; // no more of this element
		}
		Element *element = ::elements[i];
		if( element->getType() != Element::GATHERABLE || this->elementstocks[i] < max_gatherables_stored_c * element_multiplier_c )
		{
			int n_gatherers = 0;
			if( element->getType() == Element::GATHERABLE ) {
				//n_gatherers = 80;
				n_gatherers = 500;
				//n_gatherers = this->getPopulation();
			}
			else
				n_gatherers = this->getMiners((Id)i);
			this->partial_elementstocks[i] += 2 * element_multiplier_c * n_gatherers * looptime;
			while( this->partial_elementstocks[i] > mine_rate_c * gameticks_per_hour_c ) {
				new_stocks = true;
				this->partial_elementstocks[i] -= mine_rate_c * gameticks_per_hour_c;
				this->elementstocks[i]++;
				this->elements[i]--;
				if( this->elements[i] == 0 ) {
					if( element->getType() != Element::GATHERABLE )
						this->setMiners((Id)i, 0);
					LOG("Sector [%d: %d, %d] running out of element %d : %s\n", player, xpos, ypos, i, ::elements[i]->getName());
					if( this->player == client_player ) {
						playSample(s_running_out_of_elements);
						//((PlayingGameState *)gamestate)->setFlashingSquare(this->xpos, this->ypos);
						gamestate->setFlashingSquare(this->xpos, this->ypos);
					}
					break;
				}
			}
		}
	}
	if( new_stocks && this == gamestate->getCurrentSector() ) {
		/*((PlayingGameState *)gamestate)->getGamePanel()->refreshCanDesign();
		((PlayingGameState *)gamestate)->getGamePanel()->refreshDesignInventions();
		//((PlayingGameState *)gamestate)->getGamePanel()->refreshDeployInventions();
		((PlayingGameState *)gamestate)->getGamePanel()->refreshManufactureInventions();*/

		gamestate->getGamePanel()->refreshCanDesign();
		gamestate->getGamePanel()->refreshDesignInventions();
		gamestate->getGamePanel()->refreshManufactureInventions();
	}

	if( this->built_lasttime == -1 )
		this->built_lasttime = time;
	while( time - this->built_lasttime > gameticks_per_hour_c ) {
		for(int i=0;i<N_BUILDINGS;i++) {
			this->built[i] += this->getBuilders((Type)i);
		}
		this->built_lasttime += gameticks_per_hour_c;
	}
	for(int i=0;i<N_BUILDINGS;i++) {
		int cost = this->getBuildingCost((Type)i);
		if( this->built[i] > cost ) {
			LOG("Sector [%d: %d, %d] has built building type %d\n", player, xpos, ypos, i);
			this->setBuilders((Type)i, 0);
			this->built[i] = 0;
			if( i == BUILDING_MINE ) {
				this->buildings[BUILDING_MINE] = new Building(gamestate, this, BUILDING_MINE);
			}
			else if( i == BUILDING_FACTORY ) {
				this->buildings[BUILDING_FACTORY] = new Building(gamestate, this, BUILDING_FACTORY);
				this->setWorkers(0); // call to also set the particle system rate
			}
			else if( i == BUILDING_LAB ) {
				this->buildings[BUILDING_LAB] = new Building(gamestate, this, BUILDING_LAB);
			}
			else {
				// error
				LOG("###Didn't expect completion of building type %d\n",i);
				ASSERT(0);
			}
			if( this == gamestate->getCurrentSector() ) {
				//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
				//((PlayingGameState *)gamestate)->addBuilding( this->buildings[(Type)i] ); // now done in Building constructor
				gamestate->getGamePanel()->refresh();
			}
		}
	}

	if( this->growth_lasttime == -1 )
		this->growth_lasttime = time;
	else if( this->getSparePopulation() > 0 ) {
		int delay = ( growth_rate_c * gameticks_per_hour_c ) / this->getSparePopulation();
		/*if( delay == 0 )
		delay = 1;
		while( time - this->growth_lasttime > delay ) {
		this->population++;
		if( this->population > max_population_c ) {
		this->population = max_population_c;
		break;
		}
		this->growth_lasttime += delay;
		delay = ( growth_rate_c * gameticks_per_hour_c ) / this->getSparePopulation();
		if( delay == 0 )
		delay = 1;
		}*/
		if( time - this->growth_lasttime > delay ) {
			int old_pop = this->population;
			if( this->getSparePopulation() < max_grow_population_c )
				this->population++;
			this->growth_lasttime = time;
			/*if( this->getSparePopulation() > max_population_c ) {
			int diff = this->getSparePopulation() - max_population_c;
			this->population -= diff;
			}*/
			int births = this->population - old_pop;
			//ASSERT(births >= 0);
			if( births > 0 ) {
				//players[this->player]->n_births += births;
				players[this->player]->registerBirths(births);
			}
		}
	}
}

void Sector::update(int client_player) {
	//LOG("Sector::update()\n");
	if( gameMode != GAMEMODE_MULTIPLAYER_CLIENT ) {
		this->doCombat(client_player);
	}

	if( this->player != -1 ) {
		if( !this->is_shutdown )
			this->doPlayer(client_player); // still call for clients, to update particle system
	}
	else if( gameMode != GAMEMODE_MULTIPLAYER_CLIENT ) {
		int time = getGameTime();
		int n_players_in_sector = 0;
		int player_in_sector = -1;
		for(int i=0;i<n_players_c;i++) {
			//if( this->getArmy(i)->getTotal() > 0 ) {
			if( this->getArmy(i)->any(true) ) {
				player_in_sector = i;
				n_players_in_sector++;
			}
		}

		// build new tower?
		if( n_players_in_sector == 1 ) {
			if( this->built_lasttime == -1 )
				this->built_lasttime = time;
			while( time - this->built_lasttime > gameticks_per_hour_c ) {
				this->built_towers[player_in_sector] += this->getArmy(player_in_sector)->getTotal();
				this->built_lasttime += gameticks_per_hour_c;
			}

			int cost = this->getBuildingCost(BUILDING_TOWER);
			if( this->built_towers[player_in_sector] > cost ) {
				/*if( ((PlayingGameState *)gamestate)->getSelectedArmy() == this->getArmy(player_in_sector) ) {
					((PlayingGameState *)gamestate)->clearSelectedArmy();
				}*/
				if( gamestate->getSelectedArmy() == this->getArmy(player_in_sector) ) {
					gamestate->clearSelectedArmy();
				}
				this->createTower(player_in_sector, 0);
				this->returnArmy();
				if( this == gamestate->getCurrentSector() ) {
					//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
					gamestate->getGamePanel()->refresh();
				}
			}
		}
	}

	if( gameMode == GAMEMODE_MULTIPLAYER_CLIENT ) {
		return;
	}
	// rest of function is done by server

	if( this->nuke_by_player != -1 ) {
		ASSERT( this->nuke_time != -1 );
		if( getGameTime() >= this->nuke_time + nuke_delay_c ) {
			this->nuked = true;

			if( this->getActivePlayer() != -1 ) {
				// lasers
				for(int i=0;i<N_BUILDINGS && nuked;i++) {
					if( this->buildings[i] != NULL ) {
						for(int j=0;j<this->buildings[i]->getNTurrets() && nuked;j++) {
							if( this->buildings[i]->getTurretMan(j) == laser_epoch_c ) {
								//this->buildings[i]->turret_man[j] = -1;
								this->buildings[i]->killIthDefender(j);
								this->nuked = false;
							}
						}
					}
				}
			}

			if( this->nuked ) {
				playSample(s_explosion, SOUND_CHANNEL_FX);
				this->trees_nuked = this->nuked;
				if( this->getActivePlayer() != -1 ) {
					this->destroyTower(true, client_player);
				}
				for(int i=0;i<n_players_c;i++) {
					this->armies[i]->empty();
				}
				// replace tree icons with burnt tree
				for(vector<Feature *>::iterator iter = this->features->begin(); iter != this->features->end(); ++iter) {
					Feature *feature = *iter;
					const Image *image = feature->getImage(0);
					// bit of a hacky way of finding trees...
					bool is_tree = false;
					for(int i=0;i<n_trees_c && !is_tree;i++) {
						if( image == icon_trees[i][0] ) {
							is_tree = true;
						}
					}
					if( is_tree ) {
						feature->setImage(icon_trees[2], 1);
					}
				}
				if( this == gamestate->getCurrentSector() ) {
					//((PlayingGameState *)gamestate)->refreshSoldiers(true);
					//((PlayingGameState *)gamestate)->whiteFlash();
					gamestate->refreshSoldiers(true);
					gamestate->whiteFlash();
				}
				if( this->isShutdown() ) {
					// shutdown sectors are never marked as "nuked"
					this->nuked = false;
				}
			}

			this->nuke_by_player = -1;
			this->nuke_time = -1;
		}
	}
}

int Sector::getInventionCost() const {
	//LOG("Sector::getInventionCost()\n");
	const int factor_c = 12;
	int diff = this->current_design->getInvention()->getEpoch() - this->epoch;
	int cost = -1;

	if( diff == -3 )
		cost = factor_c * DESIGNTIME_M3;
	else if( diff == -2 )
		cost = factor_c * DESIGNTIME_M2;
	else if( diff == -1 )
		cost = factor_c * DESIGNTIME_M1;
	else if( diff == 0 )
		cost = factor_c * DESIGNTIME_0;
	else if( diff == 1 )
		cost = factor_c * DESIGNTIME_1;
	else if( diff == 2 )
		cost = factor_c * DESIGNTIME_2;
	else if( diff == 3 )
		cost = factor_c * DESIGNTIME_3;

	ASSERT(cost != -1);
	if( this->current_design->isErgonomicallyTerrific() )
		cost /= 2;
	return cost;
}

int Sector::getManufactureCost() const {
	//LOG("Sector::getManufactureCost()\n");
	const int factor_c = 12;
	int diff = this->current_manufacture->getInvention()->getEpoch() - this->epoch;
	int cost = -1;

	if( diff <= 0 )
		cost = factor_c * MANUFACTURETIME_0;
	else if( diff == 1 )
		cost = factor_c * MANUFACTURETIME_1;
	else if( diff == 2 )
		cost = factor_c * MANUFACTURETIME_2;
	else if( diff == 3 )
		cost = factor_c * MANUFACTURETIME_3;

	ASSERT(cost != -1);

	return cost;
}

int Sector::getBuildingCost(Type type) const {
	//LOG("Sector::getBuildingcost(%d)\n",type);
	if( type == BUILDING_TOWER )
		return hours_per_day_c * BUILDTIME_TOWER;
	else if( type == BUILDING_MINE )
		return hours_per_day_c * BUILDTIME_MINE;
	else if( type == BUILDING_FACTORY )
		return hours_per_day_c * BUILDTIME_FACTORY;
	else if( type == BUILDING_LAB )
		return hours_per_day_c * BUILDTIME_LAB;

	// error
	ASSERT(0);
	return -1;
}

void Sector::setEpoch(int epoch) {
	LOG("Sector::setEpoch(%d) [%d: %d,%d]\n", epoch, player, xpos, ypos);
	ASSERT_EPOCH(epoch);
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
	this->epoch = epoch;
}

int Sector::getEpoch() const {
	//LOG("Sector::getEpoch()\n");
	/*if( epoch >= n_epochs_c ) {
		printf("!!!\n");
	}*/
	ASSERT_EPOCH(epoch);
	return epoch;
}

int Sector::getBuildingEpoch() const {
	int eph = this->getEpoch();
	/*if( this->is_shutdown )
		eph = end_epoch_c;
	else if( start_epoch == end_epoch_c )
		eph = end_epoch_c;
	else if( eph == n_epochs_c-1 )
		eph = n_epochs_c-2;*/
	if( this->is_shutdown )
		eph = n_epochs_c-1;
	else if( start_epoch == end_epoch_c )
		eph = n_epochs_c-1;
	else if( eph == n_epochs_c-1 )
		eph = n_epochs_c-2;
	return eph;
}

int Sector::getPlayer() const {
	//LOG("Sector::getPlayer()\n");
	return player;
}

int Sector::getActivePlayer() const {
	if( this->is_shutdown )
		return PLAYER_NONE;
	return player;
}

void Sector::setCurrentDesign(Design *current_design) {
	//LOG("Sector::setCurrentDesign(%d : %s) [%d: %d, %d]\n", current_design, current_design==NULL?"NONE":current_design->getInvention()->getName(), player, xpos, ypos);
	ASSERT( current_design == NULL || current_design->getInvention()->getEpoch() <= lab_epoch_c || this->getBuilding(BUILDING_LAB ) != NULL );
	this->current_design = current_design;
	this->n_designers = 0;
	this->researched = 0;
	this->researched_lasttime = getGameTime();
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
}

const Design *Sector::getCurrentDesign() const {
	//LOG("Sector::getCurrentDesign()\n");
	return this->current_design;
}

void Sector::setCurrentManufacture(Design *current_manufacture) {
	//LOG("Sector::setCurrentManufacture(%d : %s) [%d: %d, %d]\n", current_manufacture, current_manufacture==NULL?"NONE":current_manufacture->getInvention()->getName(), player, xpos, ypos);
/*#ifdef _DEBUG
	LOG("### Sector::setCurrentManufacture a\n");
	map->checkSectors();
#endif*/
	ASSERT( current_manufacture == NULL || this->getBuilding(BUILDING_FACTORY ) != NULL );
	ASSERT( current_manufacture == NULL || current_manufacture->getInvention()->getEpoch() >= factory_epoch_c );
	this->current_manufacture = current_manufacture;
	//this->n_workers = 0;
	this->setWorkers(0); // call to also set the particle system rate
	this->n_famount = 1;
	this->manufactured = 0;
	this->manufactured_lasttime = getGameTime();
/*#ifdef _DEBUG
	LOG("### Sector::setCurrentManufacture b\n");
	map->checkSectors();
#endif*/
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
/*#ifdef _DEBUG
	LOG("### Sector::setCurrentManufacture c\n");
	map->checkSectors();
#endif*/
}

const Design *Sector::getCurrentManufacture() const {
	//LOG("Sector::getCurrentManufature()\n");
	return this->current_manufacture;
}

void Sector::inventionTimeLeft(int *halfdays,int *hours) const {
	//LOG("Sector::inventionTimeLeft()\n");
	int cost = this->getInventionCost() - this->researched;
	int time = cost / this->getDesigners();
	*halfdays = (int)( time / 12 );
	*hours = time % 12;
}

void Sector::manufactureTimeLeft(int *halfdays,int *hours) const {
	//LOG("Sector::manufactureTimeLeft()\n");
	int cost = this->getManufactureCost() - this->manufactured;
	int time = cost / this->getWorkers();
	*halfdays = (int)( time / 12 );
	*hours = time % 12;
}

void Sector::manufactureTotalTime(int *halfdays,int *hours) const {
	//LOG("Sector::manufactureTotalTime()\n");
	int cost = this->getManufactureCost();
	int time = cost / this->getWorkers();
	*halfdays = (int)( time / 12 );
	*hours = time % 12;
}

bool Sector::inventionKnown(Invention::Type type,int epoch) const {
	//LOG("Sector::inventionKnown(%d,%d)\n",type,epoch);
	ASSERT_EPOCH(epoch);
	return this->inventions_known[(int)type][epoch];
}

// Set Elements Remaining
void Sector::setElements(int id,int n_elements) {
	ASSERT_ELEMENT_ID(id);
	this->elements[id] = n_elements * element_multiplier_c;
}

// Get Elements Remaining
void Sector::getElements(int *n,int *fraction,int id) const {
	ASSERT_ELEMENT_ID(id);
	*n = this->elements[id] / element_multiplier_c;
	*fraction = this->elements[id] % element_multiplier_c;
}

bool Sector::anyElements(int id) const {
	ASSERT_ELEMENT_ID(id);
	return this->elements[id] > 0;
}

void Sector::reduceElementStocks(Id id,int reduce) {
	// reduce should be already multiplied by element_multiplier_c !
	ASSERT_ELEMENT_ID(id);
	this->elementstocks[(int)id] -= reduce;
}

void Sector::getElementStocks(int *n,int *fraction,Id id) const {
	ASSERT_ELEMENT_ID(id);
	*n = this->elementstocks[(int)id] / element_multiplier_c;
	*fraction = this->elementstocks[(int)id] % element_multiplier_c;
}

void Sector::getTotalElements(int *n,int *fraction,int id) const {
	ASSERT_ELEMENT_ID(id);
	int total = this->elements[id] + this->elementstocks[id];
	*n = total / element_multiplier_c;
	*fraction = total % element_multiplier_c;
}

void Sector::buildingTowerTimeLeft(int player,int *halfdays,int *hours) const {
	//LOG("Sector::buildingTowerTimeLeft(%d)\n",player);
	ASSERT(player != -1);
	int cost = this->getBuildingCost(BUILDING_TOWER) - this->built_towers[player];
	int n_builders = this->getArmy(player)->getTotal();
	ASSERT(n_builders != 0);
	int time = cost / n_builders;
	*halfdays = (int)( time / 12 );
	*hours = time % 12;
}

void Sector::buildingTimeLeft(Type type,int *halfdays,int *hours) const {
	//LOG("Sector::buildingTimeLeft(%d)\n",type);
	ASSERT(type != BUILDING_TOWER);
	int cost = this->getBuildingCost(type) - this->built[type];
	int n_builders = this->getBuilders(type);
	ASSERT(n_builders != 0);
	int time = cost / n_builders;
	*halfdays = (int)( time / 12 );
	*hours = time % 12;
}

void Sector::setPopulation(int population) {
	//LOG("Sector::setPopulation(%d)\n",population);
	ASSERT(population >= 0);
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
	this->population = population;
}

void Sector::setDesigners(int n_designers) {
	//LOG("Sector::setDesigners(%d)\n",n_designers);
	ASSERT( n_designers == 0 || this->current_design != NULL );
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
	this->n_designers = n_designers;
}

void Sector::setWorkers(int n_workers) {
	//LOG("Sector::setWorkers(%d)\n",n_workers);
	ASSERT( n_workers == 0 || this->current_manufacture != NULL );
	if( this == gamestate->getCurrentSector() ) {
		gamestate->getGamePanel()->refresh();
	}
	this->n_workers = n_workers;

	if( this->smokeParticleSystem != NULL ) {
		if( this->getBuilding(BUILDING_FACTORY) == NULL ) {
			this->smokeParticleSystem->setBirthRate(0.0f);
		}
		else if( this->n_workers > 0 ) {
			this->smokeParticleSystem->setBirthRate(0.008f);
		}
		else {
			this->smokeParticleSystem->setBirthRate(0.002f);
		}
	}
}

void Sector::setFAmount(int n_famount) {
	//LOG("Sector::setFAmount(%d) [%d,%d]\n", n_famount, xpos, ypos);
	ASSERT(n_famount <= infinity_c);
	ASSERT( this->current_manufacture != NULL );
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
	this->n_famount = n_famount;
}

void Sector::setMiners(Id id,int n_miners) {
	//LOG("Sector::setMiners(%d,%d)\n",id,n_miners);
	ASSERT_ELEMENT_ID(id);
	ASSERT( ::elements[id]->getType() != Element::GATHERABLE );
	ASSERT( n_miners == 0 || canMine(id) );
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
	this->n_miners[id] = n_miners;
}

void Sector::setBuilders(Type type,int n_builders) {
	//LOG("Sector::setBuilders(%d,%d)\n",type,n_builders);
	ASSERT( n_builders == 0 || canBuild(type) );
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
	this->n_builders[type] = n_builders;
}

int Sector::getPopulation() const {
	//LOG("Sector::getPopulation()\n");
	return this->population;
}

int Sector::getSparePopulation() const {
	//LOG("Sector::getSparePopulation()\n");
	int n_spare = this->population;
	n_spare -= this->n_designers;
	n_spare -= this->n_workers;
	for(int i=0;i<N_ID;i++)
		n_spare -= this->n_miners[i];
	for(int i=0;i<N_BUILDINGS;i++)
		n_spare -= this->n_builders[i];
	ASSERT( n_spare >= 0 );
	ASSERT( n_spare <= this->population );
	return n_spare;
}

int Sector::getAvailablePopulation() const {
	//LOG("Sector::getAvailablePopulation()\n");
	return this->getSparePopulation() - 1;
}

int Sector::getDesigners() const {
	//LOG("Sector::getDesigners()\n");
	return this->n_designers;
}

int Sector::getWorkers() const {
	//LOG("Sector::getWorkers()\n");
	return this->n_workers;
}

int Sector::getFAmount() const {
	//LOG("Sector::getFAmount()\n");
	ASSERT(n_famount <= infinity_c);
	return this->n_famount;
}

int Sector::getMiners(Id id) const {
	//LOG("Sector::getMiners(%d)\n",id);
	ASSERT_ELEMENT_ID(id);
	return this->n_miners[id];
}

int Sector::getBuilders(Type type) const {
	//LOG("Sector::getBuilders(%d)\n",type);
	return this->n_builders[type];
}

const Army *Sector::getArmy(int player) const {
	ASSERT_PLAYER(player);
	return armies[player];
}

Army *Sector::getArmy(int player) {
	ASSERT_PLAYER(player);
	return armies[player];
}

bool Sector::enemiesPresent() const {
	//LOG("Sector::enemiesPresent()\n");
	return enemiesPresent(this->getPlayer());
}

bool Sector::enemiesPresentWithBombardment() const {
	return enemiesPresent(this->getPlayer(), false);
}

bool Sector::enemiesPresent(int player) const {
	return enemiesPresent(player, true);
}

bool Sector::enemiesPresent(int player,bool include_unarmed) const {
	//LOG("Sector::enemiesPresent(%d)\n",player);
	// todo: improve performance
	ASSERT( player != -1 );
	bool rtn = false;
	for(int i=0;i<n_players_c && !rtn;i++) {
		if( i != player && !Player::isAlliance(i, player) ) {
			if( this->getArmy(i)->any(include_unarmed) ) {
				rtn = true;
			}
		}
	}
	return rtn;
}

void Sector::returnAssembledArmy() {
	//LOG("Sector::returnAssembledArmy()\n");
	ASSERT(assembled_army != NULL);
	returnArmy(assembled_army);
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
}

void Sector::returnArmy() {
	LOG("Sector::returnArmy() [%d: %d,%d]\n", player, xpos, ypos);
	ASSERT( this->player != -1 );
	this->returnArmy( this->getArmy( this->player ) );
}

void Sector::returnArmy(Army *army) {
	//LOG("Sector::returnArmy(%d)\n",army);
	ASSERT( this->player != -1 );

	Sector *src_sector = army->getSector();
	bool temp[map_width_c][map_height_c];
	getMap()->canMoveTo(temp, src_sector->xpos, src_sector->ypos, army->getPlayer());
	//bool adj = map->temp[this->xpos][this->ypos];
	bool adj = temp[this->xpos][this->ypos];

	if( !army->canLeaveSafely() ) {
		// retreat
		army->retreat(!adj);
	}

	for(int i=0;i<=n_epochs_c;i++) {
			int n_soldiers = army->getSoldiers(i);
			if( n_soldiers > 0 ) {
				if( !adj && !isAirUnit( i ) ) {
					// can't move
				}
				else {
					int n_men = ( i==n_epochs_c ? 1 : invention_weapons[i]->getNMen() );
					this->population += army->getSoldiers(i) * n_men;
				}
			}
	}
	this->stored_army->add(army);
	if( this == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
		gamestate->getGamePanel()->refresh();
	}
}

/** Move Army 'army' into this sector.
*/
bool Sector::moveArmy(Army *army) {
	//LOG("Sector::moveArmy(%d,%d)\n",a_player,army);
	if( this->isNuked() ) {
		return false;
	}
	Sector *src_sector = army->getSector();
	bool temp[map_width_c][map_height_c];
	getMap()->canMoveTo(temp, src_sector->xpos, src_sector->ypos, army->getPlayer());
	//bool adj = map->temp[this->xpos][this->ypos];
	bool adj = temp[this->xpos][this->ypos];
	bool moved_all = true;
	if( !army->canLeaveSafely() ) {
		// retreat
		army->retreat(!adj);
	}

	if( this->getArmy(army->getPlayer()) != army ) {
		for(int i=0;i<=n_epochs_c;i++) {
			int n_soldiers = army->getSoldiers(i);
			if( n_soldiers > 0 ) {
				if( !adj && !isAirUnit( i ) ) {
					moved_all = false;
				}
				else {
					this->getArmy( army->getPlayer() )->add(i, n_soldiers);
					army->remove(i, n_soldiers);
				}
			}
		}
	}

	if( this == gamestate->getCurrentSector() || army->getSector() == gamestate->getCurrentSector() ) {
		//((PlayingGameState *)gamestate)->refreshSoldiers(true);
		gamestate->refreshSoldiers(true);
	}

	return moved_all;
}

void Sector::evacuate() {
	for(int i=0;i<N_BUILDINGS;i++) {
		Building *building = this->getBuilding((Type)i);
		if( building != NULL ) {
			for(int j=0;j<building->getNTurrets();j++) {
				if( building->getTurretMan(j) != -1 )
					this->returnDefender(building, j);
			}
		}
		this->setBuilders((Type)i, 0);
	}
	this->setDesigners(0);
	this->setWorkers(0);
	for(int i=0;i<N_ID;i++) {
		this->n_miners[i] = 0;
	}

	for(int i=n_epochs_c-1;i>=start_epoch;i--) {
		if( i == nuclear_epoch_c )
			continue;
		while( this->assembleArmy(i, 1) ) {
		}
	}

	int men = this->getAvailablePopulation();
	if( men > 0 ) {
		this->getAssembledArmy()->add(n_epochs_c, men);
		int n_pop = this->getPopulation() - men;
		this->setPopulation(n_pop);
	}

	this->getArmy(this->getPlayer())->add(this->getAssembledArmy());
}

void Sector::printDebugInfo() const {
#ifdef _DEBUG
	printf("*** Sector Information        ***\n");
	if( player == -1 ) {
		printf("    No player in this sector\n");
	}
	else {
		printf("    player %d\n", player);
		printf("    population %d\n", population);
		printf("    designers %d\n", n_designers);
		if( current_design != NULL )
			printf("    researching %s\n", current_design->getInvention()->getName());
		printf("    workers %d\n", n_workers);
		if( current_manufacture != NULL )
			printf("    manufacturing %s\n", current_manufacture->getInvention()->getName());
		for(int i=0;i<N_ID;i++) {
			if( this->canMine((Id)i) ) {
				Element *element = ::elements[i];
				if( element->getType() != Element::GATHERABLE )
					printf("   mining %s %d\n", element->getName(), this->getMiners((Id)i));
			}
		}
		for(int i=0;i<N_ID;i++) {
			int stocks = this->elementstocks[i];
			Element *element = ::elements[i];
			if( stocks > 0 )
				printf("stocks of %s: %d\n", element->getName(), stocks);
		}
		//printf("    \n", );
	}
	printf("*** End Of Sector Information ***\n");
#endif
}

bool Design::setupDesigns() {
	Design *design = NULL;
	// Weapons
	// 0 - rock (1.5)
	// rock, wood, bone, slate, herbirite, valium, bethlium, parasite, moonlite, aquarium, solarium, aruldite
	// ergo: wood, bone
	// no mixing ?
	// from tips: rock, wood, bone 
	design = new Design(invention_weapons[0], true);
	design->setCost(ROCK, 1.5);
	design = new Design(invention_weapons[0], true);
	design->setCost(WOOD, 1.5);
	design = new Design(invention_weapons[0], true);
	design->setCost(BONE, 1.5);
	design = new Design(invention_weapons[0], false);
	design->setCost(SLATE, 1.5);
	design = new Design(invention_weapons[0], false);
	design->setCost(HERBIRITE, 1.5);
	design = new Design(invention_weapons[0], false);
	design->setCost(AQUARIUM, 1.5);
	design = new Design(invention_weapons[0], false);
	design->setCost(VALIUM, 1.5);
	design = new Design(invention_weapons[0], false);
	design->setCost(BETHLIUM, 1.5);
	design = new Design(invention_weapons[0], false);
	design->setCost(PARASITE, 1.5);
	design = new Design(invention_weapons[0], false);
	design->setCost(MOONLITE, 1.5);
	design = new Design(invention_weapons[0], false);
	design->setCost(SOLARIUM, 1.5);
	design = new Design(invention_weapons[0], false);
	design->setCost(ARULDITE, 1.5);
	// 1 - catapult (1)
	// wood, rock, slate, bone, herbirite, bethlium, parasite, moonlite, onion, solarium, aruldite, aquarium, planetarium
	// ergo: wood, bethlium, wood+solarium, rock+aruldite, bone+solarium
	// from tips: solarium, aruldite
	design = new Design(invention_weapons[1], false);
	design->setCost(WOOD, 1);
	design = new Design(invention_weapons[1], false);
	design->setCost(BONE, 1);
	design = new Design(invention_weapons[1], false);
	design->setCost(ROCK, 1);
	design = new Design(invention_weapons[1], false);
	design->setCost(BETHLIUM, 1);
	design = new Design(invention_weapons[1], false);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_weapons[1], false);
	design->setCost(ARULDITE, 1);

	design = new Design(invention_weapons[1], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_weapons[1], true);
	design->setCost(WOOD, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[1], true);
	design->setCost(WOOD, 0.5);
	design->setCost(BETHLIUM, 0.5);

	design = new Design(invention_weapons[1], false);
	design->setCost(SLATE, 0.5);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(MOONLITE, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(HERBIRITE, 0.5);

	design = new Design(invention_weapons[1], false);
	design->setCost(ROCK, 0.5);
	design->setCost(BONE, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(ROCK, 0.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(ROCK, 0.5);
	design->setCost(SLATE, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(BONE, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(BONE, 0.5);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(WOOD, 0.5);
	design->setCost(BONE, 0.5);
	design = new Design(invention_weapons[1], true);
	design->setCost(WOOD, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[1], true);
	design->setCost(BONE, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[1], true);
	design->setCost(ROCK, 0.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(SLATE, 0.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(BONE, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(WOOD, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[1], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(ARULDITE, 0.5);
	// ***
	/*design = new Design(invention_weapons[1], false);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 0.5);*/
	// 2 - pike (1.5)
	// rock, wood, slate, bone, moonlite, bethlium, planetarium, solarium, moonlite, parasite, onion, tedium, valium, aruldite, aquarium, herbirite
	// ergo: slate, solarium, moonlite, bethlium, slate+aruldite, planetarium+aruldite, planetarium+solarium
	// from tips: slate, moonlite, planetarium, bethlium, solarium, aruldite
	design = new Design(invention_weapons[2], false);
	design->setCost(SLATE, 1.5);
	design = new Design(invention_weapons[2], true);
	design->setCost(MOONLITE, 1);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_weapons[2], true);
	design->setCost(MOONLITE, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(SLATE, 0.5);
	design->setCost(MOONLITE, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(ROCK, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(MOONLITE, 1);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(WOOD, 1);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_weapons[2], true);
	design->setCost(PLANETARIUM, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[2], true);
	design->setCost(PLANETARIUM, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[2], true);
	design->setCost(MOONLITE, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[2], true);
	design->setCost(SLATE, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(ARULDITE, 1.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(MOONLITE, 1.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(SLATE, 1);
	design->setCost(PLANETARIUM, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(BONE, 0.5);
	design->setCost(MOONLITE, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(VALIUM, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(VALIUM, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(MOONLITE, 1);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_weapons[2], true);
	design->setCost(SLATE, 1);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_weapons[2], true);
	design->setCost(SLATE, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(BONE, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[2], true);
	design->setCost(PLANETARIUM, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(BETHLIUM, 1.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(SOLARIUM, 1.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(PARASITE, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(PLANETARIUM, 1.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(SLATE, 1);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(ONION, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(MOONLITE, 1);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(MOONLITE, 1);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(MOONLITE, 1);
	design->setCost(PLANETARIUM, 0.5);
	design = new Design(invention_weapons[2], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(HERBIRITE, 1);
	design = new Design(invention_weapons[2], false);
	design->setCost(BONE, 1);
	design->setCost(BETHLIUM, 0.5);
	// 3 - longbow (2)
	// rock, wood, slate, moonlite, bethlium, aruldite, planetarium, herbirite, parasite, solarium, yeridium, marmite, aquarium, moron, tedium
	// ergo: slate+bethlium+yeridium
	// from tips: yeridium
	design = new Design(invention_weapons[3], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(MOONLITE, 1);
	design->setCost(BETHLIUM, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_weapons[3], false);
	design->setCost(SLATE, 1.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(MOONLITE, 1);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(SLATE, 1.5);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(SLATE, 1);
	design->setCost(BETHLIUM, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(SLATE, 1);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(MOONLITE, 1);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(HERBIRITE, 1.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(PARASITE, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_weapons[3], true);
	design->setCost(SLATE, 1);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(SLATE, 1);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(MOONLITE, 1);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(SLATE, 1);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SLATE, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(WOOD, 0.5);
	design->setCost(PLANETARIUM, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(BONE, 0.5);
	design->setCost(MOONLITE, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(ROCK, 0.5);
	design->setCost(SLATE, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(ROCK, 0.5);
	design->setCost(SLATE, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(ROCK, 0.5);
	design->setCost(MOONLITE, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 1);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(MARMITE, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(YERIDIUM, 1.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(MOONLITE, 1);
	design->setCost(YERIDIUM, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(AQUARIUM, 1.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SLATE, 1);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(ROCK, 1);
	design->setCost(ARULDITE, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(HERBIRITE, 1.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(SLATE, 1);
	design->setCost(HERBIRITE, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(SLATE, 1);
	design->setCost(YERIDIUM, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(MORON, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(MOONLITE, 1);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[3], true);
	design->setCost(MOONLITE, 1);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(MOONLITE, 1);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(AQUARIUM, 0.5);
	// ***
	design = new Design(invention_weapons[3], false);
	design->setCost(WOOD, 0.5);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(YERIDIUM, 1);
	design = new Design(invention_weapons[3], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 1);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(MOONLITE, 1);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_weapons[3], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(YERIDIUM, 0.5);
	// 4 - giant catapult (4.5)
	// wood, bone, slate, moonlite, bethlium, solarium, planetarium, aruldite, herbirite, valium, parasite, yeridium, moron, tedium, paladium, aquarium
	// ergo: ?
	// from tips: planetarium, herbirite, yeridium, aquarium, paladium
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 3.5);
	design->setCost(BETHLIUM, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 3);
	design->setCost(BETHLIUM, 1.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 3.5);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 3.5);
	design->setCost(BETHLIUM, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 3.5);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 3);
	design->setCost(ARULDITE, 1.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 3);
	design->setCost(SOLARIUM, 1.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(BONE, 0.5);
	design->setCost(MOONLITE, 3);
	design->setCost(BETHLIUM, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 3);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(PLANETARIUM, 3);
	design->setCost(ARULDITE, 1.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(PLANETARIUM, 3.5);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(MORON, 3);
	design = new Design(invention_weapons[4], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(YERIDIUM, 3.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(SOLARIUM, 1);
	design->setCost(YERIDIUM, 3.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(VALIUM, 3);
	design = new Design(invention_weapons[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SLATE, 3);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(WOOD, 1);
	design->setCost(SLATE, 3);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(WOOD, 3);
	design->setCost(SOLARIUM, 1);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[4], true);
	design->setCost(SLATE, 3);
	design->setCost(BETHLIUM, 1);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 3);
	design->setCost(ARULDITE, 1);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 3);
	design->setCost(ARULDITE, 1);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 3);
	design->setCost(ARULDITE, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 3);
	design->setCost(ARULDITE, 1);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 3);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 3);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 3);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(PARASITE, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(PARASITE, 3);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 4);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 3);
	design->setCost(HERBIRITE, 1.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(SOLARIUM, 4);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(ARULDITE, 1);
	design->setCost(HERBIRITE, 3.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(ARULDITE, 4);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(PLANETARIUM, 4);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(PLANETARIUM, 3);
	design->setCost(HERBIRITE, 1.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(PLANETARIUM, 3.5);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(PLANETARIUM, 3);
	design->setCost(SOLARIUM, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 3);
	design->setCost(SOLARIUM, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(BETHLIUM, 4);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 4);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 3);
	design->setCost(YERIDIUM, 1.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(SLATE, 4);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 4);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(ARULDITE, 1);
	design->setCost(YERIDIUM, 3.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(TEDIUM, 3);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 3);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 3);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_weapons[4], true);
	design->setCost(MOONLITE, 3);
	design->setCost(BETHLIUM, 1);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[4], false);
	design->setCost(SOLARIUM, 3);
	design->setCost(ARULDITE, 1);
	design->setCost(HERBIRITE, 0.5);
	// ***
	/*design = new Design(invention_weapons[4], false);
	design->setCost(MOONLITE, 1);*/
	// ***
	/*design = new Design(invention_weapons[4], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(HERBIRITE, 0.5);*/
	// ***
	/*design = new Design(invention_weapons[4], false);
	design->setCost(HERBIRITE, 1.5);*/
	// ***
	/*design = new Design(invention_weapons[4], false);
	design->setCost(YERIDIUM, 1.5);*/
	// 5 - cannon (6)
	// slate, bone, wood, bethlium, solarium, onion, tedium, aruldite, marmite, valium, planetarium, herbirite, aquarium, yeridium, paladium, moron
	// ergo: bethlium, onion, aruldite
	// from tips: bethlium, solarium, aruldite
	design = new Design(invention_weapons[5], false);
	design->setCost(BONE, 1);
	design->setCost(BETHLIUM, 5);
	design = new Design(invention_weapons[5], false);
	design->setCost(BONE, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(PARASITE, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(WOOD, 1);
	design->setCost(SOLARIUM, 5);
	design = new Design(invention_weapons[5], false);
	design->setCost(WOOD, 5);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_weapons[5], true);
	design->setCost(BETHLIUM, 5);
	design->setCost(ONION, 1);
	design = new Design(invention_weapons[5], true);
	design->setCost(BONE, 5);
	design->setCost(ONION, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(ROCK, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(ROCK, 1);
	design->setCost(BETHLIUM, 5);
	design = new Design(invention_weapons[5], false);
	design->setCost(ROCK, 1);
	design->setCost(ARULDITE, 5);
	design = new Design(invention_weapons[5], false);
	design->setCost(PLANETARIUM, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], true);
	design->setCost(ARULDITE, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], true);
	design->setCost(ARULDITE, 5);
	design->setCost(ONION, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(SOLARIUM, 5);
	design->setCost(MORON, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(ARULDITE, 5);
	design->setCost(HERBIRITE, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(ARULDITE, 5);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(ARULDITE, 5);
	design->setCost(MORON, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(WOOD, 1);
	design->setCost(ARULDITE, 5);
	design = new Design(invention_weapons[5], true);
	design->setCost(SOLARIUM, 5);
	design->setCost(ONION, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(PLANETARIUM, 5);
	design->setCost(ONION, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(HERBIRITE, 5);
	design->setCost(ONION, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(VALIUM, 5);
	design->setCost(ONION, 1);
	design = new Design(invention_weapons[5], true); // check always ergonomic? - Not in Junta!
	design->setCost(ONION, 6);
	design = new Design(invention_weapons[5], false);
	design->setCost(TEDIUM, 6);
	design = new Design(invention_weapons[5], false);
	design->setCost(SOLARIUM, 6);
	design = new Design(invention_weapons[5], false);
	design->setCost(ARULDITE, 6);
	design = new Design(invention_weapons[5], false);
	design->setCost(BETHLIUM, 5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(SOLARIUM, 5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(ARULDITE, 5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(AQUARIUM, 5);
	design->setCost(ONION, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(BETHLIUM, 5);
	design->setCost(MARMITE, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(ONION, 1);
	design->setCost(MARMITE, 5);
	design = new Design(invention_weapons[5], false);
	design->setCost(SOLARIUM, 5);
	design->setCost(MARMITE, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(ARULDITE, 5);
	design->setCost(MARMITE, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(BETHLIUM, 6);
	design = new Design(invention_weapons[5], false);
	design->setCost(BETHLIUM, 5);
	design->setCost(YERIDIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(YERIDIUM, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(SLATE, 1);
	design->setCost(BETHLIUM, 5);
	design = new Design(invention_weapons[5], false);
	design->setCost(SLATE, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(PALADIUM, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(PALADIUM, 5);
	design->setCost(ONION, 1);
	design = new Design(invention_weapons[5], true);
	design->setCost(BETHLIUM, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(AQUARIUM, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(ONION, 5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_weapons[5], false);
	design->setCost(TEDIUM, 1);
	design->setCost(MARMITE, 5);
	// 6 - biplane (6)
	// slate, wood, bethlium, solarium, yeridium, moron, onion, valium, aruldite, paladium, planetarium, tedium, aquarium
	// ergo: ?
	// from tips: bethlium, solarium, aruldite, onion, tedium, moron
	design = new Design(invention_weapons[6], false);
	design->setCost(SLATE, 4);
	design->setCost(ARULDITE, 1.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(WOOD, 4);
	design->setCost(ARULDITE, 1.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(ROCK, 4);
	design->setCost(ARULDITE, 1.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(WOOD, 4);
	design->setCost(ARULDITE, 1.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(SOLARIUM, 1.5);
	design->setCost(MORON, 4.5);
	design = new Design(invention_weapons[6], true);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(ONION, 0.5);
	design->setCost(MORON, 4);
	design = new Design(invention_weapons[6], false);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(AQUARIUM, 4);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(MORON, 4);
	design = new Design(invention_weapons[6], false);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(VALIUM, 4);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(SOLARIUM, 1.5);
	design->setCost(VALIUM, 4);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(PALADIUM, 0.5);
	design->setCost(MORON, 4);
	design = new Design(invention_weapons[6], false);
	design->setCost(PLANETARIUM, 4);
	design->setCost(SOLARIUM, 1.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(PLANETARIUM, 4);
	design->setCost(ARULDITE, 1.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(PLANETARIUM, 4);
	design->setCost(ARULDITE, 1.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(MORON, 4);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(TEDIUM, 0.5);
	design->setCost(MARMITE, 4);
	design = new Design(invention_weapons[6], true);
	design->setCost(ARULDITE, 1.5);
	design->setCost(TEDIUM, 0.5);
	design->setCost(MORON, 4);
	design = new Design(invention_weapons[6], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(MORON, 4);
	design = new Design(invention_weapons[6], false);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(ONION, 0.5);
	design->setCost(MORON, 4);
	design = new Design(invention_weapons[6], false);
	design->setCost(PALADIUM, 1.5);
	design->setCost(ONION, 0.5);
	design->setCost(MORON, 4);
	design = new Design(invention_weapons[6], false);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(TEDIUM, 4);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(PLANETARIUM, 4);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(BETHLIUM, 5.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(TEDIUM, 0.5);
	design->setCost(MORON, 5.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(TEDIUM, 2);
	design->setCost(MORON, 4);
	design = new Design(invention_weapons[6], false);
	design->setCost(SOLARIUM, 1.5);
	design->setCost(MORON, 4);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(TEDIUM, 0.5);
	design->setCost(MORON, 4);
	design->setCost(ALIEN, 1.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(ONION, 4.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(ARULDITE, 5.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(SOLARIUM, 5.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(SOLARIUM, 1.5);
	design->setCost(TEDIUM, 4.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(ARULDITE, 5.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(TEDIUM, 4.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(MORON, 4.5);
	design = new Design(invention_weapons[6], false);
	design->setCost(ARULDITE, 2);
	design->setCost(MORON, 4);
	design = new Design(invention_weapons[6], false);
	design->setCost(ONION, 2);
	design->setCost(MORON, 4);
	design = new Design(invention_weapons[6], false);
	design->setCost(BETHLIUM, 5.5);
	design->setCost(ONION, 0.5);
	// 7 - jet plane (7.5)
	// wood, slate, planetarium, onion, aquarium, tedium, marmite, paladium, valium, aruldite, moonlite
	// ergo: ?
	// from tips: slate, moonlite, planetarium, aquarium, paladium, onion, tedium
	design = new Design(invention_weapons[7], false);
	design->setCost(WOOD, 0.5);
	design->setCost(PALADIUM, 1.5);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(PALADIUM, 1.5);
	design->setCost(ONION, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(SLATE, 6);
	design->setCost(AQUARIUM, 1.5);
	design = new Design(invention_weapons[7], true);
	design->setCost(SLATE, 0.5);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(SLATE, 0.5);
	design->setCost(PARASITE, 1.5);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], true);
	design->setCost(MOONLITE, 0.5);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(MOONLITE, 0.5);
	design->setCost(YERIDIUM, 1.5);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(SLATE, 0.5);
	design->setCost(YERIDIUM, 5.5);
	design->setCost(PALADIUM, 1.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(ONION, 7);
	design = new Design(invention_weapons[7], false);
	design->setCost(PLANETARIUM, 2);
	design->setCost(ONION, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(TEDIUM, 5.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(SLATE, 0.5);
	design->setCost(ONION, 7);
	design = new Design(invention_weapons[7], false);
	design->setCost(SLATE, 2);
	design->setCost(ONION, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(SLATE, 0.5);
	design->setCost(PALADIUM, 7);
	design = new Design(invention_weapons[7], false);
	design->setCost(SLATE, 6);
	design->setCost(PALADIUM, 1.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(SLATE, 0.5);
	design->setCost(TEDIUM, 7);
	design = new Design(invention_weapons[7], false);
	design->setCost(SLATE, 2);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(MOONLITE, 2);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(MOONLITE, 6);
	design->setCost(PALADIUM, 1.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(PALADIUM, 2);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(ONION, 2);
	design->setCost(MARMITE, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(TEDIUM, 7);
	design = new Design(invention_weapons[7], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(PALADIUM, 7);
	design = new Design(invention_weapons[7], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(AQUARIUM, 7);
	design = new Design(invention_weapons[7], false);
	design->setCost(PLANETARIUM, 6);
	design->setCost(AQUARIUM, 1.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(PALADIUM, 1.5);
	design->setCost(TEDIUM, 6);
	design = new Design(invention_weapons[7], false);
	design->setCost(PALADIUM, 1.5);
	design->setCost(ONION, 6);
	design = new Design(invention_weapons[7], false);
	design->setCost(PALADIUM, 2);
	design->setCost(ONION, 5.5);
	design = new Design(invention_weapons[7], true);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(PALADIUM, 1.5);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(ARULDITE, 1.5);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(MARMITE, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(MOONLITE, 2);
	design->setCost(ONION, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(AQUARIUM, 2);
	design->setCost(ONION, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(ONION, 6);
	design = new Design(invention_weapons[7], false);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(TEDIUM, 6);
	design = new Design(invention_weapons[7], false);
	design->setCost(AQUARIUM, 2);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(ROCK, 1.5);
	design->setCost(SLATE, 0.5);
	design->setCost(TEDIUM, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(SLATE, 0.5);
	design->setCost(ARULDITE, 1.5);
	design->setCost(ONION, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(VALIUM, 0.5);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(ONION, 5.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(PALADIUM, 1.5);
	design->setCost(ONION, 5.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(PALADIUM, 1.5);
	design->setCost(TEDIUM, 5.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_weapons[7], false);
	design->setCost(PALADIUM, 1.5);
	design->setCost(ONION, 5.5);
	design->setCost(TEDIUM, 0.5);
	// 8 - nuke (5.5)
	// slate, wood, aruldite, paladium, moron, bethlium, marmite, aquarium, tedium, solarium, moonlite, planetarium
	// ergo: ?
	// from tips: bethlium, solarium, aruldite, aquarium, paladium, marmite
	design = new Design(invention_weapons[8], false);
	design->setCost(WOOD, 3);
	design->setCost(ARULDITE, 0.5);
	design->setCost(PALADIUM, 2);
	design = new Design(invention_weapons[8], false);
	design->setCost(WOOD, 2);
	design->setCost(ARULDITE, 0.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(WOOD, 2);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(PALADIUM, 2);
	design->setCost(MORON, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(ROCK, 0.5);
	design->setCost(PALADIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(ROCK, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(SLATE, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(AQUARIUM, 2.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3.5);
	design = new Design(invention_weapons[8], false);
	design->setCost(BETHLIUM, 2.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(PALADIUM, 5);
	design = new Design(invention_weapons[8], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(MARMITE, 5);
	design = new Design(invention_weapons[8], false);
	design->setCost(SOLARIUM, 2.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(MOONLITE, 2);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(SOLARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(AQUARIUM, 2);
	design->setCost(TEDIUM, 0.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(MARMITE, 5);
	design = new Design(invention_weapons[8], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(YERIDIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(ARULDITE, 2.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(AQUARIUM, 5);
	design = new Design(invention_weapons[8], false);
	design->setCost(ARULDITE, 3.5);
	design->setCost(AQUARIUM, 2);
	design = new Design(invention_weapons[8], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(TEDIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(PALADIUM, 2);
	design->setCost(MARMITE, 3.5);
	design = new Design(invention_weapons[8], false);
	design->setCost(PALADIUM, 2.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(ARULDITE, 3.5);
	design->setCost(PALADIUM, 2);
	design = new Design(invention_weapons[8], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(PALADIUM, 5);
	design = new Design(invention_weapons[8], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(PALADIUM, 5);
	design = new Design(invention_weapons[8], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(AQUARIUM, 5);
	design = new Design(invention_weapons[8], false);
	design->setCost(BETHLIUM, 3.5);
	design->setCost(AQUARIUM, 2);
	design = new Design(invention_weapons[8], true);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], true);
	design->setCost(ARULDITE, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(PALADIUM, 2);
	design->setCost(TEDIUM, 3);
	design = new Design(invention_weapons[8], true);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(YERIDIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], true);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], true);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(PALADIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(VALIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(VALIUM, 0.5);
	design->setCost(PALADIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(PALADIUM, 2);
	design->setCost(MORON, 3);
	design = new Design(invention_weapons[8], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(MARMITE, 3);
	design->setCost(ALIEN, 2);
	// 9 - saucer (8)
	// slate, aquarium, moron, alien, marmite, paladium
	// ergo: ?
	// from tips: moron, marmite, alien
	design = new Design(invention_weapons[9], false);
	design->setCost(SLATE, 2.5);
	design->setCost(MARMITE, 0.5);
	design->setCost(ALIEN, 5);
	design = new Design(invention_weapons[9], false);
	design->setCost(SOLARIUM, 2.5);
	design->setCost(MARMITE, 0.5);
	design->setCost(ALIEN, 5);
	design = new Design(invention_weapons[9], true);
	design->setCost(MORON, 2.5);
	design->setCost(MARMITE, 0.5);
	design->setCost(ALIEN, 5);
	design = new Design(invention_weapons[9], false);
	design->setCost(MORON, 7.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_weapons[9], false);
	design->setCost(MORON, 2.5);
	design->setCost(MARMITE, 5.5);
	design = new Design(invention_weapons[9], false);
	design->setCost(MORON, 2.5);
	design->setCost(ALIEN, 5.5);
	design = new Design(invention_weapons[9], true);
	design->setCost(AQUARIUM, 2.5);
	design->setCost(MARMITE, 0.5);
	design->setCost(ALIEN, 5);
	design = new Design(invention_weapons[9], false);
	design->setCost(PALADIUM, 2.5);
	design->setCost(MARMITE, 0.5);
	design->setCost(ALIEN, 5);
	design = new Design(invention_weapons[9], false);
	design->setCost(PALADIUM, 5);
	design->setCost(MORON, 2.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_weapons[9], false);
	design->setCost(MARMITE, 0.5);
	design->setCost(ALIEN, 7.5);
	design = new Design(invention_weapons[9], false);
	design->setCost(MARMITE, 3);
	design->setCost(ALIEN, 5);

	// Defences
	// 0 - stick (1.5)
	// wood, bone, rock, slate, planetarium, herbirite, valium, bethlium, parasite, solarium, aquarium
	// ergo: slate, planetarium, moonlite
	// from tips: slate, moonlite, planetarium
	design = new Design(invention_defences[0], false);
	design->setCost(WOOD, 1.5);
	design = new Design(invention_defences[0], false);
	design->setCost(BONE, 1.5);
	design = new Design(invention_defences[0], false);
	design->setCost(ROCK, 1.5);
	design = new Design(invention_defences[0], true);
	design->setCost(SLATE, 1.5);
	design = new Design(invention_defences[0], false);
	design->setCost(SOLARIUM, 1.5);
	design = new Design(invention_defences[0], false);
	design->setCost(AQUARIUM, 1.5);
	design = new Design(invention_defences[0], true);
	design->setCost(PLANETARIUM, 1.5);
	design = new Design(invention_defences[0], true);
	design->setCost(MOONLITE, 1.5);
	design = new Design(invention_defences[0], false);
	design->setCost(HERBIRITE, 1.5);
	design = new Design(invention_defences[0], false);
	design->setCost(VALIUM, 1.5);
	design = new Design(invention_defences[0], false);
	design->setCost(BETHLIUM, 1.5);
	design = new Design(invention_defences[0], false);
	design->setCost(PARASITE, 1.5);
	// 1 - spear (1.5)
	// wood, rock, bone, slate, moonlite, planetarium, valium, herbirite, parasite, onion, bethlium, solarium, aquarium, aruldite
	// ergo: rock, wood, rock+slate, moonlite, bone+planetarium, wood+planetarium
	// from tips: rock, wood, bones, slate, moonlite, planetarium
	design = new Design(invention_defences[1], false);
	design->setCost(WOOD, 1.5);
	design = new Design(invention_defences[1], false);
	design->setCost(ROCK, 1.5);
	design = new Design(invention_defences[1], true);
	design->setCost(BONE, 0.5);
	design->setCost(MOONLITE, 1);
	design = new Design(invention_defences[1], true);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 1);
	design = new Design(invention_defences[1], true);
	design->setCost(ROCK, 0.5);
	design->setCost(MOONLITE, 1);
	design = new Design(invention_defences[1], false);
	design->setCost(SLATE, 1);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_defences[1], false);
	design->setCost(BONE, 0.5);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_defences[1], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_defences[1], false);
	design->setCost(WOOD, 0.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[1], false);
	design->setCost(BONE, 1.5);
	design = new Design(invention_defences[1], false);
	design->setCost(SLATE, 1.5);
	design = new Design(invention_defences[1], false);
	design->setCost(ROCK, 0.5);
	design->setCost(BONE, 1);
	design = new Design(invention_defences[1], false);
	design->setCost(WOOD, 1.0);
	design->setCost(BONE, 0.5);
	design = new Design(invention_defences[1], true);
	design->setCost(ROCK, 0.5);
	design->setCost(SLATE, 1);
	design = new Design(invention_defences[1], true);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 1);
	design = new Design(invention_defences[1], true);
	design->setCost(WOOD, 0.5);
	design->setCost(PLANETARIUM, 1);
	design = new Design(invention_defences[1], true);
	design->setCost(SLATE, 1);
	design->setCost(PLANETARIUM, 0.5);
	design = new Design(invention_defences[1], false);
	design->setCost(BONE, 0.5);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[1], false);
	design->setCost(MOONLITE, 1);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[1], false);
	design->setCost(BONE, 0.5);
	design->setCost(HERBIRITE, 1);
	design = new Design(invention_defences[1], false);
	design->setCost(SLATE, 1);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_defences[1], false);
	design->setCost(MOONLITE, 1);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_defences[1], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_defences[1], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_defences[1], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_defences[1], false);
	design->setCost(SLATE, 1);
	design->setCost(ONION, 0.5);
	design = new Design(invention_defences[1], false);
	design->setCost(MOONLITE, 1.5);
	design = new Design(invention_defences[1], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(MOONLITE, 1);
	design = new Design(invention_defences[1], false);
	design->setCost(MOONLITE, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_defences[1], false);
	design->setCost(PLANETARIUM, 1.5);
	// 2 - bow and arrow (1.5)
	// wood, rock, slate, bone, moonlite, bethlium, planetarium, herbirite, valium, parasite, tedium
	// ergo: ?
	// from tips: rock, wood, bones, valium, parasite
	design = new Design(invention_defences[2], false);
	design->setCost(WOOD, 1);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 1);
	design = new Design(invention_defences[2], false);
	design->setCost(BONE, 1);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(BONE, 0.5);
	design->setCost(MOONLITE, 1);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 0.5);
	design->setCost(SLATE, 1);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 1);
	design->setCost(SLATE, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(WOOD, 1);
	design->setCost(SLATE, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 0.5);
	design->setCost(SLATE, 0.5);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(BONE, 0.5);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(WOOD, 0.5);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 1);
	design = new Design(invention_defences[2], false);
	design->setCost(WOOD, 1);
	design->setCost(PLANETARIUM, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 1);
	design->setCost(PLANETARIUM, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 0.5);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 0.5);
	design->setCost(BONE, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(MOONLITE, 1);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(MOONLITE, 0.5);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[2], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[2], false);
	design->setCost(SLATE, 1);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 1);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 0.5);
	design->setCost(MOONLITE, 1);
	design = new Design(invention_defences[2], false);
	design->setCost(BONE, 0.5);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[2], false);
	design->setCost(BONE, 1);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(MOONLITE, 0.5);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[2], false);
	design->setCost(BONE, 1);
	design->setCost(PLANETARIUM, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 1);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(MOONLITE, 1);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[2], true);
	design->setCost(BONE, 0.5);
	design->setCost(MOONLITE, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 0.5);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_defences[2], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[2], true);
	design->setCost(WOOD, 0.5);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_defences[2], true);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 0.5);
	design->setCost(PARASITE, 0.5);
	// ***
	design = new Design(invention_defences[2], false);
	design->setCost(ROCK, 0.5);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 0.5);
	// 3 - cauldron (3)
	// rock, wood, slate, moonlite, aruldite, herbirite, valium, planetarium, bethlium, parasite, solarium, moron, aquarium, marmite
	// ergo: bethlium+valium
	// from tips: bethlium, solarium, aruldite, valium, parasite
	design = new Design(invention_defences[3], false);
	design->setCost(ROCK, 1);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(MOONLITE, 1);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(ARULDITE, 3);
	design = new Design(invention_defences[3], false);
	design->setCost(MOONLITE, 1);
	design->setCost(BETHLIUM, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(BONE, 2);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(ROCK, 2);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[3], true);
	design->setCost(BETHLIUM, 2);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[3], true);
	design->setCost(SOLARIUM, 2);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(HERBIRITE, 2);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(SOLARIUM, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(PLANETARIUM, 2);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(AQUARIUM, 2);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(VALIUM, 3);
	design = new Design(invention_defences[3], false);
	design->setCost(SLATE, 1);
	design->setCost(BETHLIUM, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(SLATE, 1);
	design->setCost(SOLARIUM, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(BETHLIUM, 3);
	design = new Design(invention_defences[3], false);
	design->setCost(SLATE, 2);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[3], true);
	design->setCost(BETHLIUM, 2);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[3], true);
	design->setCost(ARULDITE, 2);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[3], true);
	design->setCost(SOLARIUM, 2);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(SOLARIUM, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(PARASITE, 3);
	design = new Design(invention_defences[3], false);
	design->setCost(SOLARIUM, 3);
	design = new Design(invention_defences[3], false);
	design->setCost(WOOD, 1);
	design->setCost(SOLARIUM, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(WOOD, 1);
	design->setCost(BETHLIUM, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(WOOD, 1);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(BONE, 1);
	design->setCost(SOLARIUM, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(SOLARIUM, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(SOLARIUM, 1);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(HERBIRITE, 1);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(MORON, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(ARULDITE, 2);
	design->setCost(MORON, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(ARULDITE, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(ARULDITE, 2);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(PARASITE, 1);
	design->setCost(MARMITE, 2);
	design = new Design(invention_defences[3], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[3], false);
	design->setCost(VALIUM, 1);
	design->setCost(ONION, 2);
	// 4 - crossbow (1.5)
	// rock, bone, wood, slate, bethlium, solarium, planetarium, aruldite, tedium, aquarium, paladium, moron, yeridium
	// ergo: wood+solarium+paladium, wood+bethlium+aquarium
	// from tips: rock, wood, bone, aruldite
	design = new Design(invention_defences[4], false);
	design->setCost(BONE, 0.5);
	design->setCost(BETHLIUM, 1);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 1);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 1);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 0.5);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 0.5);
	design->setCost(BETHLIUM, 1);
	design = new Design(invention_defences[4], false);
	design->setCost(ARULDITE, 1);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[4], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(BONE, 1);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(BONE, 1);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(BONE, 1);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(SOLARIUM, 1);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(BONE, 1);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SLATE, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(BONE, 0.5);
	design->setCost(MOONLITE, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_defences[4], true);
	design->setCost(WOOD, 0.5);
	design->setCost(PALADIUM, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_defences[4], true);
	design->setCost(WOOD, 0.5);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], true);
	design->setCost(BONE, 0.5);
	design->setCost(ARULDITE, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(BONE, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 1);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 1);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 0.5);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[4], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(ROCK, 0.5);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 0.5);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(BONE, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_defences[4], true);
	design->setCost(ROCK, 0.5);
	design->setCost(ARULDITE, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 0.5);
	design->setCost(ARULDITE, 0.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 0.5);
	design->setCost(ARULDITE, 0.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(ARULDITE, 0.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 0.5);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(VALIUM, 0.5);
	// ***
	/*design = new Design(invention_defences[4], false);
	design->setCost(WOOD, 1);*/
	// ***
	/*design = new Design(invention_defences[4], false);
	design->setCost(BONE, 1);*/
	// ***
	/*design = new Design(invention_defences[4], false);
	design->setCost(ROCK, 1);*/
	// 5 - musket (2)
	// wood, bone, slate, moonlite, onion, planetarium, tedium, valium, onion, herbirite, parasite, yeridium
	// ergo: ?
	// from tips: slate, moonlite, planetarium, valium, parasite
	design = new Design(invention_defences[5], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SLATE, 1);
	design->setCost(ONION, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(BONE, 0.5);
	design->setCost(SLATE, 1);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(SLATE, 1);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(SLATE, 1);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[5], false);
	design->setCost(MOONLITE, 1);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[5], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(SLATE, 1);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[5], false);
	design->setCost(SLATE, 1);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[5], false);
	design->setCost(SLATE, 1.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(ROCK, 0.5);
	design->setCost(PLANETARIUM, 1);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[5], false);
	design->setCost(VALIUM, 1.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(SLATE, 1.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(VALIUM, 0.5);
	design->setCost(TEDIUM, 1.5);
	design = new Design(invention_defences[5], false);
	design->setCost(SLATE, 1);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(MOONLITE, 1);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(YERIDIUM, 1);
	design->setCost(PARASITE, 0.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(PARASITE, 0.5);
	design->setCost(TEDIUM, 0.5);
	design->setCost(MARMITE, 1);
	design = new Design(invention_defences[5], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(MOONLITE, 1);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[5], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[5], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(ROCK, 1);
	design->setCost(VALIUM, 0.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(SLATE, 1);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_defences[5], false);
	design->setCost(SLATE, 1);
	design->setCost(TEDIUM, 0.5);
	design->setCost(MARMITE, 0.5);
	// 6 - machine gun (2.5)
	// wood, bone, slate, valium, onion, planetarium, solarium, tedium, parasite, marmite, aquarium, yeridium, moonlite
	// ergo: ?
	// from tips: slate, moonlite, planetarium, valium, parasite, onion, tedium
	design = new Design(invention_defences[6], false);
	design->setCost(WOOD, 1);
	design->setCost(SLATE, 0.5);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(BONE, 0.5);
	design->setCost(VALIUM, 1);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(ROCK, 0.5);
	design->setCost(VALIUM, 1);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 0.5);
	design->setCost(BETHLIUM, 1);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 0.5);
	design->setCost(PARASITE, 1);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[6], true);
	design->setCost(SLATE, 0.5);
	design->setCost(PARASITE, 1);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 0.5);
	design->setCost(ARULDITE, 1);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 1.5);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 1.5);
	design->setCost(VALIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 0.5);
	design->setCost(VALIUM, 2);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 0.5);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 1.5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(VALIUM, 1.5);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(VALIUM, 1);
	design->setCost(AQUARIUM, 0.5);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(VALIUM, 1.5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(MOONLITE, 0.5);
	design->setCost(AQUARIUM, 1);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[6], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(MOONLITE, 0.5);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[6], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 0.5);
	design->setCost(ONION, 2);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 1.5);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(PARASITE, 1.5);
	design->setCost(ONION, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(PARASITE, 1);
	design->setCost(TEDIUM, 1.5);
	design = new Design(invention_defences[6], false);
	design->setCost(PARASITE, 1.5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(SOLARIUM, 1);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], true);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(VALIUM, 1);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(ONION, 1);
	design->setCost(MARMITE, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(PARASITE, 1);
	design->setCost(ONION, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 0.5);
	design->setCost(YERIDIUM, 1);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 0.5);
	design->setCost(HERBIRITE, 1);
	design->setCost(PARASITE, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(SLATE, 0.5);
	design->setCost(TEDIUM, 1);
	design->setCost(MARMITE, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(MOONLITE, 0.5);
	design->setCost(YERIDIUM, 1);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(ROCK, 1);
	design->setCost(SLATE, 0.5);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(MOONLITE, 0.5);
	design->setCost(PARASITE, 1);
	design->setCost(MARMITE, 1);
	design = new Design(invention_defences[6], false);
	design->setCost(PARASITE, 1);
	design->setCost(TEDIUM, 1);
	design->setCost(MARMITE, 0.5);
	// 7 - bazooka (3)
	// slate, wood, bone, paladium, valium, onion, tedium, moron, marmite, planetarium, yeridium, aruldite, parasite, aquarium, bethlium, solarium
	// ergo: ?
	// from tips: aquarium, paladium, onion, tedium
	design = new Design(invention_defences[7], false);
	design->setCost(WOOD, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(ROCK, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(ROCK, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(WOOD, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(WOOD, 1);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(BONE, 1);
	design->setCost(ONION, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(BONE, 1);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(BONE, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(HERBIRITE, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(SOLARIUM, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(SLATE, 1);
	design->setCost(ONION, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(SLATE, 1);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(SLATE, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(PALADIUM, 1);
	design->setCost(MARMITE, 2);
	design = new Design(invention_defences[7], true);
	design->setCost(PALADIUM, 1);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[7], true);
	design->setCost(PALADIUM, 1);
	design->setCost(ONION, 2);
	design = new Design(invention_defences[7], true);
	design->setCost(AQUARIUM, 1);
	design->setCost(ONION, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(VALIUM, 1);
	design->setCost(ONION, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(VALIUM, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(ONION, 3);
	design = new Design(invention_defences[7], false);
	design->setCost(AQUARIUM, 3);
	design = new Design(invention_defences[7], false);
	design->setCost(AQUARIUM, 1);
	design->setCost(MORON, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(PALADIUM, 1);
	design->setCost(MORON, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(PALADIUM, 3);
	design = new Design(invention_defences[7], false);
	design->setCost(TEDIUM, 2);
	design->setCost(MARMITE, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(AQUARIUM, 1);
	design->setCost(MARMITE, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(ONION, 2);
	design->setCost(MARMITE, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(TEDIUM, 3);
	design = new Design(invention_defences[7], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(ONION, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(YERIDIUM, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(YERIDIUM, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(YERIDIUM, 1);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(ARULDITE, 1);
	design->setCost(ONION, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(ARULDITE, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(ARULDITE, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(ARULDITE, 1);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[7], true);
	design->setCost(AQUARIUM, 1);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(AQUARIUM, 1);
	design->setCost(PALADIUM, 2);
	design = new Design(invention_defences[7], true); // not ergo in Epoch 8, Xtra ???
	design->setCost(BETHLIUM, 1);
	design->setCost(TEDIUM, 2);
	design = new Design(invention_defences[7], false);
	design->setCost(MOONLITE, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(PARASITE, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_defences[7], false);
	design->setCost(PARASITE, 1);
	design->setCost(TEDIUM, 2);
	// 8 - nuclear defence (5.5)
	// rock, wood, bone, slate, aruldite, paladium, aquarium, marmite, onion, planetarium, alien, solarium, bethlium, valium
	// ergo: ?
	// from tips: wood, rock, bone, aquarium, paladium, marmite
	design = new Design(invention_defences[8], false);
	design->setCost(ROCK, 0.5);
	design->setCost(MARMITE, 5);
	design = new Design(invention_defences[8], false);
	design->setCost(ROCK, 2.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], true);
	design->setCost(ROCK, 0.5);
	design->setCost(PALADIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(ROCK, 0.5);
	design->setCost(YERIDIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(ROCK, 0.5);
	design->setCost(YERIDIUM, 3);
	design->setCost(PALADIUM, 2);
	design = new Design(invention_defences[8], false);
	design->setCost(ROCK, 0.5);
	design->setCost(HERBIRITE, 3);
	design->setCost(PALADIUM, 2);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SLATE, 3);
	design->setCost(PALADIUM, 2);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(ARULDITE, 3);
	design->setCost(PALADIUM, 2);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(PALADIUM, 2);
	design->setCost(TEDIUM, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(YERIDIUM, 3);
	design->setCost(PALADIUM, 2);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(VALIUM, 3);
	design->setCost(PALADIUM, 2);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 3.5);
	design->setCost(PALADIUM, 2);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 3.5);
	design->setCost(AQUARIUM, 2);
	design = new Design(invention_defences[8], false);
	design->setCost(BONE, 0.5);
	design->setCost(MARMITE, 5);
	design = new Design(invention_defences[8], false);
	design->setCost(BONE, 2.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(BONE, 0.5);
	design->setCost(SLATE, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(BONE, 0.5);
	design->setCost(VALIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(BONE, 0.5);
	design->setCost(TEDIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 3);
	design->setCost(AQUARIUM, 2);
	design = new Design(invention_defences[8], false);
	design->setCost(SLATE, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], true);
	design->setCost(ROCK, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(ARULDITE, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(AQUARIUM, 2.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3.5);
	design = new Design(invention_defences[8], false);
	design->setCost(AQUARIUM, 2);
	design->setCost(ONION, 0.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(PALADIUM, 2);
	design->setCost(MARMITE, 3.5);
	design = new Design(invention_defences[8], false);
	design->setCost(PALADIUM, 2.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(PALADIUM, 2);
	design->setCost(MARMITE, 3);
	design->setCost(ALIEN, 0.5);
	design = new Design(invention_defences[8], false);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(PALADIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(PALADIUM, 2);
	design->setCost(MORON, 0.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(AQUARIUM, 2);
	design->setCost(MORON, 0.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(AQUARIUM, 2);
	design->setCost(TEDIUM, 0.5);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(VALIUM, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MORON, 2);
	design->setCost(MARMITE, 3);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MARMITE, 3);
	design->setCost(ALIEN, 2);
	design = new Design(invention_defences[8], false);
	design->setCost(WOOD, 2.5);
	design->setCost(MARMITE, 3);
	// 9 - sdi laser (5)
	// slate, bethlium, aruldite, moron, alien, solarium, paladium
	// ergo: ?
	// from tips: moron, alien
	design = new Design(invention_defences[9], false);
	design->setCost(SLATE, 1.5);
	design->setCost(ALIEN, 3.5);
	design = new Design(invention_defences[9], false);
	design->setCost(SLATE, 3.5);
	design->setCost(MORON, 1.5);
	design = new Design(invention_defences[9], false);
	design->setCost(WOOD, 3.5);
	design->setCost(MORON, 1.5);
	design = new Design(invention_defences[9], false);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(ALIEN, 3.5);
	design = new Design(invention_defences[9], false);
	design->setCost(BETHLIUM, 3.5);
	design->setCost(MORON, 1.5);
	design = new Design(invention_defences[9], false);
	design->setCost(PALADIUM, 3.5);
	design->setCost(MORON, 1.5);
	design = new Design(invention_defences[9], false);
	design->setCost(MORON, 1.5);
	design->setCost(MARMITE, 3.5);
	design = new Design(invention_defences[9], false);
	design->setCost(MORON, 5);
	design = new Design(invention_defences[9], false);
	design->setCost(YERIDIUM, 1.5);
	design->setCost(ALIEN, 3.5);
	design = new Design(invention_defences[9], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(ALIEN, 3.5);
	design = new Design(invention_defences[9], false);
	design->setCost(ARULDITE, 3.5);
	design->setCost(ALIEN, 1.5);
	design = new Design(invention_defences[9], false);
	design->setCost(SOLARIUM, 1.5);
	design->setCost(ALIEN, 3.5);
	design = new Design(invention_defences[9], true);
	design->setCost(MORON, 1.5);
	design->setCost(ALIEN, 3.5);
	design = new Design(invention_defences[9], false);
	design->setCost(MARMITE, 1.5);
	design->setCost(ALIEN, 3.5);
	design = new Design(invention_defences[9], false);
	design->setCost(ALIEN, 5);

	// Shields (all 3 ?)
	// from af tips: wood, rock, bones, slate, moonlite, planetarium, bethlium, solarium, aruldite, herbirite, yeridium, aquarium, paladium, moron, marmite, alien
	// 0 (9500BC)
	design = new Design(invention_shields[0], false);
	design->setCost(WOOD, 3);
	design = new Design(invention_shields[0], false);
	design->setCost(BONE, 3);
	design = new Design(invention_shields[0], false);
	design->setCost(SLATE, 3);
	design = new Design(invention_shields[0], false);
	design->setCost(ROCK, 3);
	design = new Design(invention_shields[0], false);
	design->setCost(PLANETARIUM, 3);
	design = new Design(invention_shields[0], false);
	design->setCost(MOONLITE, 3);
	// 1 (3000BC)
	design = new Design(invention_shields[1], false);
	design->setCost(WOOD, 3);
	design = new Design(invention_shields[1], false);
	design->setCost(ROCK, 3);
	design = new Design(invention_shields[1], false);
	design->setCost(BONE, 3);
	design = new Design(invention_shields[1], false);
	design->setCost(ROCK, 2);
	design->setCost(BONE, 1);
	design = new Design(invention_shields[1], false);
	design->setCost(ROCK, 1);
	design->setCost(SLATE, 2);
	design = new Design(invention_shields[1], false);
	design->setCost(ROCK, 1);
	design->setCost(MOONLITE, 2);
	design = new Design(invention_shields[1], false);
	design->setCost(WOOD, 1);
	design->setCost(BETHLIUM, 2);
	design = new Design(invention_shields[1], false);
	design->setCost(WOOD, 1);
	design->setCost(AQUARIUM, 2);
	design = new Design(invention_shields[1], false);
	design->setCost(BONE, 1);
	design->setCost(VALIUM, 2);
	design = new Design(invention_shields[1], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(VALIUM, 1);
	design = new Design(invention_shields[1], false);
	design->setCost(BONE, 1);
	design->setCost(HERBIRITE, 2);
	design = new Design(invention_shields[1], false);
	design->setCost(BETHLIUM, 3);
	design = new Design(invention_shields[1], false);
	design->setCost(SOLARIUM, 3);
	design = new Design(invention_shields[1], true);
	design->setCost(WOOD, 1);
	design->setCost(SOLARIUM, 2);
	design = new Design(invention_shields[1], true);
	design->setCost(BONE, 1);
	design->setCost(SOLARIUM, 2);
	design = new Design(invention_shields[1], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(HERBIRITE, 1);
	design = new Design(invention_shields[1], false);
	design->setCost(ARULDITE, 3);
	design = new Design(invention_shields[1], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(PARASITE, 1);
	design = new Design(invention_shields[1], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[1], false);
	design->setCost(SOLARIUM, 2);
	design->setCost(PARASITE, 1);
	design = new Design(invention_shields[1], false);
	design->setCost(SOLARIUM, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[1], true);
	design->setCost(ROCK, 1);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_shields[1], true);
	design->setCost(BONE, 1);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_shields[1], false);
	design->setCost(SLATE, 1);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_shields[1], false);
	design->setCost(PLANETARIUM, 1);
	design->setCost(ARULDITE, 2);
	design = new Design(invention_shields[1], false);
	design->setCost(SOLARIUM, 2);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_shields[1], false);
	design->setCost(MOONLITE, 1);
	design->setCost(SOLARIUM, 2);
	// 2 (100AD)
	design = new Design(invention_shields[2], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SLATE, 2.5);
	design = new Design(invention_shields[2], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 2.5);
	design = new Design(invention_shields[2], false);
	design->setCost(SLATE, 2.5);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(BONE, 0.5);
	design->setCost(MOONLITE, 2.5);
	design = new Design(invention_shields[2], false);
	design->setCost(ROCK, 0.5);
	design->setCost(MOONLITE, 2.5);
	design = new Design(invention_shields[2], false);
	design->setCost(ROCK, 0.5);
	design->setCost(PLANETARIUM, 2.5);
	design = new Design(invention_shields[2], false);
	design->setCost(PLANETARIUM, 3);
	design = new Design(invention_shields[2], false);
	design->setCost(MOONLITE, 2.5);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(HERBIRITE, 3);
	design = new Design(invention_shields[2], true);
	design->setCost(PLANETARIUM, 2.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(PLANETARIUM, 2.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(SLATE, 3);
	design = new Design(invention_shields[2], false);
	design->setCost(SLATE, 2.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(SLATE, 2.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SOLARIUM, 2.5);
	design = new Design(invention_shields[2], false);
	design->setCost(SLATE, 2.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(PLANETARIUM, 2.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_shields[2], true);
	design->setCost(SLATE, 2.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[2], true);
	design->setCost(MOONLITE, 2.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(MOONLITE, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(WOOD, 2.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(BONE, 2.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(AQUARIUM, 2.5);
	design = new Design(invention_shields[2], false);
	design->setCost(BONE, 0.5);
	design->setCost(PLANETARIUM, 2.5);
	design = new Design(invention_shields[2], false);
	design->setCost(MOONLITE, 3);
	design = new Design(invention_shields[2], false);
	design->setCost(MOONLITE, 2.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(MOONLITE, 2.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[2], false);
	design->setCost(MOONLITE, 2.5);
	design->setCost(BETHLIUM, 0.5);
	// 3 (900AD)
	design = new Design(invention_shields[3], false);
	design->setCost(ROCK, 2);
	design->setCost(BONE, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_shields[3], true);
	design->setCost(BONE, 2);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(HERBIRITE, 2.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(VALIUM, 2.5);
	design = new Design(invention_shields[3], false);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(PARASITE, 2.5);
	design = new Design(invention_shields[3], false);
	design->setCost(WOOD, 2);
	design->setCost(PARASITE, 1);
	design = new Design(invention_shields[3], false);
	design->setCost(WOOD, 2);
	design->setCost(HERBIRITE, 1);
	design = new Design(invention_shields[3], false);
	design->setCost(WOOD, 2.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(WOOD, 0.5);
	design->setCost(MOONLITE, 2.5);
	design = new Design(invention_shields[3], false);
	design->setCost(WOOD, 2);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(WOOD, 2);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(BONE, 2);
	design->setCost(VALIUM, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(BONE, 2);
	design->setCost(YERIDIUM, 1);
	design = new Design(invention_shields[3], false);
	design->setCost(BONE, 2.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(ROCK, 2.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(BONE, 2.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(ROCK, 2.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(ROCK, 2);
	design->setCost(VALIUM, 1);
	design = new Design(invention_shields[3], false);
	design->setCost(YERIDIUM, 2.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(PARASITE, 2.5);
	design = new Design(invention_shields[3], false);
	design->setCost(WOOD, 2);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(WOOD, 2);
	design->setCost(BETHLIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(ROCK, 2);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(ROCK, 2);
	design->setCost(HERBIRITE, 1);
	design = new Design(invention_shields[3], false);
	design->setCost(SLATE, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_shields[3], false);
	design->setCost(WOOD, 2);
	design->setCost(MOONLITE, 0.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[3], true);
	design->setCost(BONE, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(VALIUM, 0.5);
	// ***
	design = new Design(invention_shields[3], false);
	design->setCost(ROCK, 2.5);
	design->setCost(PLANETARIUM, 0.5);
	// 4 (1400AD)
	design = new Design(invention_shields[4], false);
	design->setCost(ROCK, 3);
	design = new Design(invention_shields[4], false);
	design->setCost(BONE, 3);
	design = new Design(invention_shields[4], false);
	design->setCost(WOOD, 3);
	design = new Design(invention_shields[4], false);
	design->setCost(AQUARIUM, 3);
	design = new Design(invention_shields[4], false);
	design->setCost(BONE, 2.5);
	design->setCost(WOOD, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(ROCK, 2.5);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(WOOD, 2.5);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_shields[4], true);
	design->setCost(WOOD, 2.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[4], true);
	design->setCost(ROCK, 2.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[4], true);
	design->setCost(BONE, 2.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(BONE, 2.5);
	design->setCost(BETHLIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(BONE, 2.5);
	design->setCost(MOONLITE, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(BONE, 2.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(ROCK, 2.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(WOOD, 2.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(BONE, 2.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(BONE, 2.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(WOOD, 0.5);
	design->setCost(ROCK, 2.5);
	design = new Design(invention_shields[4], false);
	design->setCost(ROCK, 2.5);
	design->setCost(BONE, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(ROCK, 2.5);
	design->setCost(SLATE, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(ROCK, 2.5);
	design->setCost(PLANETARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(ROCK, 2.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[4], true);
	design->setCost(ROCK, 2.5);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(PALADIUM, 3);
	design = new Design(invention_shields[4], true);
	design->setCost(HERBIRITE, 2.5);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_shields[4], true);
	design->setCost(TEDIUM, 2.5);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(PALADIUM, 0.5);
	design->setCost(ONION, 2.5);
	design = new Design(invention_shields[4], false);
	design->setCost(PALADIUM, 0.5);
	design->setCost(MARMITE, 2.5);
	design = new Design(invention_shields[4], false);
	design->setCost(AQUARIUM, 0.5);
	design->setCost(TEDIUM, 2.5);
	design = new Design(invention_shields[4], false);
	design->setCost(AQUARIUM, 0.5);
	design->setCost(PALADIUM, 2.5);
	design = new Design(invention_shields[4], false);
	design->setCost(AQUARIUM, 0.5);
	design->setCost(MORON, 2.5);
	design = new Design(invention_shields[4], false);
	design->setCost(PLANETARIUM, 2.5);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(YERIDIUM, 2.5);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(YERIDIUM, 2.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(WOOD, 2.5);
	design->setCost(SLATE, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(AQUARIUM, 0.5);
	design->setCost(ONION, 2.5);
	design = new Design(invention_shields[4], false);
	design->setCost(WOOD, 2.5);
	design->setCost(SOLARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(WOOD, 2.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(BETHLIUM, 2.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(MOONLITE, 2.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(HERBIRITE, 2.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(VALIUM, 2.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(PARASITE, 2.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(ROCK, 2.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(WOOD, 2.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(WOOD, 2.5);
	design->setCost(PLANETARIUM, 0.5);
	design = new Design(invention_shields[4], false);
	design->setCost(ARULDITE, 2.5);
	design->setCost(AQUARIUM, 0.5);
	// 5 (1850AD)
	design = new Design(invention_shields[5], false);
	design->setCost(BONE, 0.5);
	design->setCost(BETHLIUM, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(BONE, 1.5);
	design->setCost(BETHLIUM, 1.5);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SOLARIUM, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 1.5);
	design->setCost(SOLARIUM, 1.5);
	design = new Design(invention_shields[5], false);
	design->setCost(BONE, 1.5);
	design->setCost(SOLARIUM, 1.5);
	design = new Design(invention_shields[5], false);
	design->setCost(ROCK, 1.5);
	design->setCost(ARULDITE, 1.5);
	design = new Design(invention_shields[5], false);
	design->setCost(ROCK, 0.5);
	design->setCost(ARULDITE, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(ROCK, 0.5);
	design->setCost(SOLARIUM, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(ROCK, 0.5);
	design->setCost(BETHLIUM, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 0.5);
	design->setCost(ARULDITE, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 1.5);
	design->setCost(ARULDITE, 1.5);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 0.5);
	design->setCost(BETHLIUM, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 0.5);
	design->setCost(AQUARIUM, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(BONE, 0.5);
	design->setCost(AQUARIUM, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(BONE, 2.5);
	design->setCost(ARULDITE, 0.5);
	design = new Design(invention_shields[5], false);
	design->setCost(BONE, 1.5);
	design->setCost(ARULDITE, 1.5);
	design = new Design(invention_shields[5], false);
	design->setCost(BONE, 0.5);
	design->setCost(ARULDITE, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(SOLARIUM, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[5], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(AQUARIUM, 1.5);
	design = new Design(invention_shields[5], true);
	design->setCost(WOOD, 0.5);
	design->setCost(ARULDITE, 1.5);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_shields[5], true);
	design->setCost(WOOD, 0.5);
	design->setCost(ARULDITE, 1.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 0.5);
	design->setCost(ARULDITE, 1.5);
	design->setCost(ONION, 1);
	design = new Design(invention_shields[5], true);
	design->setCost(WOOD, 0.5);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SLATE, 1.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[5], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(AQUARIUM, 1);
	design->setCost(ONION, 0.5);
	design = new Design(invention_shields[5], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(PALADIUM, 1);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 0.5);
	design->setCost(PALADIUM, 1);
	design->setCost(MORON, 1.5);
	design = new Design(invention_shields[5], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_shields[5], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[5], false);
	design->setCost(PALADIUM, 0.5);
	design->setCost(ONION, 2.5);
	design = new Design(invention_shields[5], false);
	design->setCost(ROCK, 0.5);
	design->setCost(HERBIRITE, 1.5);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_shields[5], true);
	design->setCost(ROCK, 0.5);
	design->setCost(ARULDITE, 1.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[5], true);
	design->setCost(BONE, 0.5);
	design->setCost(ARULDITE, 1.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[5], false);
	design->setCost(WOOD, 0.5);
	design->setCost(YERIDIUM, 1.5);
	design->setCost(AQUARIUM, 1);
	// 6 (1915AD)
	design = new Design(invention_shields[6], false);
	design->setCost(ROCK, 2.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(ROCK, 2.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2);
	design->setCost(YERIDIUM, 1);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(PALADIUM, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(PARASITE, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(ROCK, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 0.5);
	design->setCost(ROCK, 2);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 0.5);
	design->setCost(PALADIUM, 1.5);
	design->setCost(VALIUM, 1);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2);
	design->setCost(AQUARIUM, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2);
	design->setCost(AQUARIUM, 0.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2);
	design->setCost(SLATE, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(ROCK, 2);
	design->setCost(SLATE, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(BONE, 2.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(ROCK, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(ONION, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(ROCK, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(ARULDITE, 2);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(YERIDIUM, 2.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(MORON, 2.5);
	design = new Design(invention_shields[6], false);
	design->setCost(HERBIRITE, 2.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], true);
	design->setCost(ROCK, 2);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2);
	design->setCost(PALADIUM, 0.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(ROCK, 2);
	design->setCost(ARULDITE, 0.5);
	design->setCost(HERBIRITE, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(BONE, 2.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(WOOD, 2);
	design->setCost(SOLARIUM, 0.5);
	design->setCost(YERIDIUM, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(ARULDITE, 1.5);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[6], false);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(AQUARIUM, 2);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(BONE, 2);
	design->setCost(AQUARIUM, 0.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[6], false);
	design->setCost(BONE, 2);
	design->setCost(SLATE, 0.5);
	design->setCost(YERIDIUM, 0.5);
	// 7 (1945AD)
	design = new Design(invention_shields[7], true);
	design->setCost(ROCK, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(PALADIUM, 2);
	design->setCost(YERIDIUM, 0.5);
	design->setCost(MORON, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(BONE, 2);
	design->setCost(YERIDIUM, 1);
	design = new Design(invention_shields[7], false);
	design->setCost(BONE, 1);
	design->setCost(SLATE, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(BONE, 1.5);
	design->setCost(ARULDITE, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 2);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 2);
	design->setCost(BETHLIUM, 1);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 2);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 1.5);
	design->setCost(SOLARIUM, 1);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 1.5);
	design->setCost(ONION, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 1.5);
	design->setCost(PARASITE, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 1.5);
	design->setCost(MORON, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(WOOD, 0.5);
	design->setCost(SLATE, 1.5);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_shields[7], false);
	design->setCost(WOOD, 0.5);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_shields[7], false);
	design->setCost(WOOD, 2);
	design->setCost(HERBIRITE, 0.5);
	design->setCost(VALIUM, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(ROCK, 1.5);
	design->setCost(ARULDITE, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 2.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 1.5);
	design->setCost(MARMITE, 1.5);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 1.5);
	design->setCost(ARULDITE, 1);
	design->setCost(AQUARIUM, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 1.5);
	design->setCost(ARULDITE, 1);
	design->setCost(ONION, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 1.5);
	design->setCost(ARULDITE, 1);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(MARMITE, 2);
	design = new Design(invention_shields[7], false);
	design->setCost(SOLARIUM, 1);
	design->setCost(MARMITE, 2);
	design = new Design(invention_shields[7], false);
	design->setCost(SOLARIUM, 2.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(PLANETARIUM, 2);
	design->setCost(SOLARIUM, 1);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 1.5);
	design->setCost(ARULDITE, 1.5);
	design = new Design(invention_shields[7], false);
	design->setCost(MOONLITE, 2);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_shields[7], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(ARULDITE, 1.5);
	design = new Design(invention_shields[7], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(ARULDITE, 1.5);
	design = new Design(invention_shields[7], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(BETHLIUM, 1.5);
	design = new Design(invention_shields[7], false);
	design->setCost(PLANETARIUM, 2.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(ONION, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(AQUARIUM, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(SOLARIUM, 1.5);
	design = new Design(invention_shields[7], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(BETHLIUM, 1.5);
	design = new Design(invention_shields[7], false);
	design->setCost(ARULDITE, 1);
	design->setCost(MORON, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(ARULDITE, 1);
	design->setCost(ONION, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(ARULDITE, 1);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(ARULDITE, 1);
	design->setCost(MARMITE, 2);
	design = new Design(invention_shields[7], false);
	design->setCost(ARULDITE, 2.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], true);
	design->setCost(MOONLITE, 1.5);
	design->setCost(ARULDITE, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(PARASITE, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 0.5);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_shields[7], false);
	design->setCost(SLATE, 1.5);
	design->setCost(TEDIUM, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(PLANETARIUM, 0.5);
	design->setCost(ARULDITE, 1);
	design = new Design(invention_shields[7], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(YERIDIUM, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(TEDIUM, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], true);
	design->setCost(PLANETARIUM, 1.5);
	design->setCost(BETHLIUM, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(ROCK, 1.5);
	design->setCost(SOLARIUM, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[7], false);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(SOLARIUM, 1);
	design->setCost(MARMITE, 0.5);
	// 8 (1980AD)
	design = new Design(invention_shields[8], false);
	design->setCost(BONE, 1.5);
	design->setCost(ARULDITE, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(BONE, 1);
	design->setCost(PARASITE, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(WOOD, 0.5);
	design->setCost(ARULDITE, 1);
	design->setCost(PARASITE, 1.5);
	design = new Design(invention_shields[8], false);
	design->setCost(SOLARIUM, 1);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(SOLARIUM, 1);
	design->setCost(YERIDIUM, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(PARASITE, 1.5);
	design->setCost(TEDIUM, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], true);
	design->setCost(ARULDITE, 1);
	design->setCost(PARASITE, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(ARULDITE, 1);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(ARULDITE, 1);
	design->setCost(VALIUM, 1.5);
	design->setCost(TEDIUM, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(VALIUM, 1.5);
	design->setCost(PALADIUM, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(VALIUM, 1.5);
	design->setCost(MARMITE, 1.5);
	design = new Design(invention_shields[8], false);
	design->setCost(BETHLIUM, 1.5);
	design->setCost(VALIUM, 1.5);
	design = new Design(invention_shields[8], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(VALIUM, 2);
	design = new Design(invention_shields[8], false);
	design->setCost(BETHLIUM, 2.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(MARMITE, 2);
	design = new Design(invention_shields[8], false);
	design->setCost(SOLARIUM, 1);
	design->setCost(MARMITE, 2);
	design = new Design(invention_shields[8], false);
	design->setCost(SOLARIUM, 2.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(SOLARIUM, 1.5);
	design->setCost(PARASITE, 1.5);
	design = new Design(invention_shields[8], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(SOLARIUM, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(MOONLITE, 1);
	design->setCost(PARASITE, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(MOONLITE, 1.5);
	design->setCost(ARULDITE, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(ARULDITE, 2.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(ARULDITE, 1);
	design->setCost(MARMITE, 2);
	design = new Design(invention_shields[8], false);
	design->setCost(PARASITE, 1.5);
	design->setCost(PALADIUM, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(MARMITE, 0.5);
	design->setCost(ALIEN, 1.5);
	design = new Design(invention_shields[8], true);
	design->setCost(BETHLIUM, 1);
	design->setCost(VALIUM, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(AQUARIUM, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(TEDIUM, 1.5);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], false);
	design->setCost(PARASITE, 1.5);
	design->setCost(MARMITE, 1.5);
	design = new Design(invention_shields[8], false);
	design->setCost(PARASITE, 1.5);
	design->setCost(MARMITE, 0.5);
	design->setCost(ALIEN, 1);
	design = new Design(invention_shields[8], false);
	design->setCost(SLATE, 0.5);
	design->setCost(SOLARIUM, 1);
	design->setCost(PARASITE, 1.5);
	design = new Design(invention_shields[8], false);
	design->setCost(SLATE, 1.5);
	design->setCost(ARULDITE, 1);
	design->setCost(MARMITE, 0.5);
	design = new Design(invention_shields[8], true);
	design->setCost(SOLARIUM, 1);
	design->setCost(VALIUM, 1.5);
	design->setCost(MARMITE, 0.5);
	// 9 (2001AD)
	design = new Design(invention_shields[9], false);
	design->setCost(BONE, 1);
	design->setCost(HERBIRITE, 2);
	design = new Design(invention_shields[9], false);
	design->setCost(WOOD, 2);
	design->setCost(ALIEN, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(HERBIRITE, 2);
	design->setCost(TEDIUM, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(HERBIRITE, 2);
	design->setCost(MARMITE, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(HERBIRITE, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(HERBIRITE, 2);
	design->setCost(PARASITE, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(ROCK, 1);
	design->setCost(YERIDIUM, 2);
	design = new Design(invention_shields[9], false);
	design->setCost(ROCK, 1);
	design->setCost(HERBIRITE, 2);
	design = new Design(invention_shields[9], false);
	design->setCost(SLATE, 2);
	design->setCost(ALIEN, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(ALIEN, 3);
	design = new Design(invention_shields[9], true);
	design->setCost(YERIDIUM, 2);
	design->setCost(ALIEN, 1);
	design = new Design(invention_shields[9], true);
	design->setCost(YERIDIUM, 2);
	design->setCost(MARMITE, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(YERIDIUM, 2);
	design->setCost(MORON, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(YERIDIUM, 2);
	design->setCost(PALADIUM, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(YERIDIUM, 2);
	design->setCost(PARASITE, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(YERIDIUM, 2);
	design->setCost(AQUARIUM, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(YERIDIUM, 2);
	design->setCost(ONION, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(MORON, 2);
	design->setCost(ALIEN, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(YERIDIUM, 3);
	design = new Design(invention_shields[9], false);
	design->setCost(HERBIRITE, 3);
	design = new Design(invention_shields[9], false);
	design->setCost(ARULDITE, 2);
	design->setCost(ALIEN, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(AQUARIUM, 2);
	design->setCost(ALIEN, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(MARMITE, 2);
	design->setCost(ALIEN, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(PALADIUM, 2);
	design->setCost(ALIEN, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(BETHLIUM, 2);
	design->setCost(ALIEN, 1);
	design = new Design(invention_shields[9], false);
	design->setCost(BETHLIUM, 1);
	design->setCost(YERIDIUM, 2);

	return true;
}
