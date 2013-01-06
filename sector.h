#pragma once

/** Handles an individual sector.
*/

namespace Gigalomania {
	class Image;
	class PanelPage;
	class Button;
}

using namespace Gigalomania;

class Feature;
class Sector;
class PlayingGameState;
class Invention;

using std::vector;
using std::string;

#include "common.h"

bool isAirUnit(int epoch);
bool defenceNeedsMan(int epoch);

class Particle {
	float xpos, ypos; // floats to allow for movement
	int birth_time;
public:
	Particle();

	float getX() const {
		return this->xpos;
	}
	float getY() const {
		return this->ypos;
	}
	void setPos(float xpos, float ypos) {
		this->xpos = xpos;
		this->ypos = ypos;
	}
	int getBirthTime() const {
		return this->birth_time;
	}
};

class ParticleSystem {
protected:
	vector<Particle> particles;
	const Image *image;

public:
	ParticleSystem(const Image *image) : image(image) {
	}

	void draw(int xpos, int ypos) const;
	virtual void update()=0;
};

class SmokeParticleSystem : public ParticleSystem {
	float birth_rate;
	int life_exp;
	int last_emit_time;
public:
	SmokeParticleSystem(const Image *image);
	void setBirthRate(float birth_rate);

	virtual void update();
};

class Army {
	int soldiers[n_epochs_c+1]; // unarmed men are soldiers[n_epochs_c];
	int player;
	Sector *sector;
	PlayingGameState *gamestate;

public:
	Army(PlayingGameState *gamestate, Sector *sector, int player);
	~Army() {
	}

	int getPlayer() const {
		return this->player;
	}
	Sector *getSector() const {
		return this->sector;
	}
	int getTotal() const;
	int getTotalMen() const;
	bool any(bool include_unarmed) const;
	int getStrength() const;
	int getBombardStrength() const;
	int getSoldiers(int index) const {
		return this->soldiers[index];
	}
	void add(int i,int n);
	void add(Army *army);
	void remove(int i,int n);
	void kill(int index);
	void empty() {
		for(int i=0;i<n_epochs_c+1;i++)
			soldiers[i] = 0;
	}
	bool canLeaveSafely(int a_player) const;
	void retreat(bool only_air);

	static int getIndividualStrength(int i);
	static int getIndividualBombardStrength(int i);
};

const int element_multiplier_c = 2;

class Element {
	string name;
public:
	enum Type {
		GATHERABLE = 0,
		OPENPITMINE = 1,
		DEEPMINE = 2
	};
private:
	Id id;
	Type type;

public:
	Element(const char *name,Id id,Type type);
	~Element();

	Image *getImage() const;
	const char *getName() const {
		return this->name.c_str();
	}
	Type getType() const {
		return this->type;
	}
};

class Design {
	Invention *invention;
	bool ergonomically_terrific;
	int cost[N_ID];

public:
	Design(Invention *invention,bool ergonomically_terrific);

	void setCost(Id id, float cost) {
		this->cost[(int)id] = (int)(cost * element_multiplier_c);
	}
	int getCost(Id id) {
		return this->cost[(int)id];
	}
	bool isErgonomicallyTerrific() const {
		return this->ergonomically_terrific;
	}
	Invention *getInvention() const {
		return this->invention;
	}

	static bool setupDesigns();
};

class Invention {
protected:
	string name;
public:
	enum Type {
		SHIELD = 0,
		DEFENCE = 1,
		WEAPON = 2
	};

protected:
	Type type;
	int epoch;
	vector<Design *> *designs;

public:
	Invention(const char *name,Type type,int epoch);
	~Invention();

	//int getRelativeEpoch();
	Image *getImage() const;
	//void addDesign(Design *design);
	const char *getName() const {
		return this->name.c_str();
	}
	Type getType() const {
		return this->type;
	}
	int getEpoch() const {
		return this->epoch;
	}
	void addDesign(Design *design) {
		this->designs->push_back(design);
	}
	size_t getNDesigns() const {
		return this->designs->size();
	}
	Design *getDesign(size_t i) const {
		return this->designs->at(i);
	}

	static Invention *getInvention(Invention::Type type,int epoch);
};

class Weapon : public Invention {
	int n_men;
public:
	Weapon(const char *name,int epoch,int n_men) : Invention(name,WEAPON,epoch) {
		this->n_men = n_men;
	}
	~Weapon() {
	}
	int getNMen() const {
		return this->n_men;
	}
};

const int max_building_turrets_c = 4;

class Building {
private:
	Type type;
	Sector *sector;
	int health, max_health;
	int pos_x, pos_y;
	int n_turrets;
	int turret_man[max_building_turrets_c];
	int turret_mandir[max_building_turrets_c];
	PanelPage *building_button;
	PanelPage *turret_buttons[max_building_turrets_c];
	PlayingGameState *gamestate;

public:
	Building(PlayingGameState *gamestate, Sector *sector, Type type);
	~Building();

	Type getType() const {
		return this->type;
	}
	int getX() const {
		return this->pos_x;
	}
	int getY() const {
		return this->pos_y;
	}
	int getHealth() const {
		return this->health;
	}
	int getMaxHealth() const {
		return this->max_health;
	}
	void addHealth(int v) {
		this->health += v;
		if( this->health > this->max_health )
			this->health = this->max_health;
	}
	Image **getImages();
	void rotateDefenders();
	int getNDefenders() const {
		int n = 0;
		for(int i=0;i<n_turrets;i++) {
			if( turret_man[i] != -1 )
				n++;
		}
		return n;
	}
	int getNDefenders(int type) const {
		int n = 0;
		for(int i=0;i<n_turrets;i++) {
			if( turret_man[i] == type )
				n++;
		}
		return n;
	}
	int getDefenderStrength() const;
	void killIthDefender(int i);
	void killDefender(int index);
	PanelPage *getBuildingButton() const {
		return this->building_button;
	}
	int getNTurrets() const {
		return this->n_turrets;
	}
	int getTurretMan(int turret) const;
	int getTurretManDir(int turret) const;
	PanelPage *getTurretButton(int turret) const;
	void clearTurretMan(int turret);
	void setTurretMan(int turret, int epoch);
};

class Sector {
	//Vector *features;
	vector<Feature *> *features;
	int xpos, ypos;
	int epoch;
	int player;
	bool is_shutdown;
	//int shutdown_player;

	bool nuked;
	bool trees_nuked; // separate flag, as if a sector is "shutdown", we don't ever set the nuked flag to true
	int nuke_by_player;
	int nuke_time;

	int population;
	int n_designers;
	int n_miners[N_ID];
	int n_builders[N_BUILDINGS];
	int n_workers;
	int n_famount;
	Design *current_design;
	Design *current_manufacture;
	int researched;
	int researched_lasttime;
	int manufactured;
	int manufactured_lasttime;
	int growth_lasttime;
	int mined_lasttime;
	int built_towers[n_players_c]; // for neutral sectors
	int built[N_BUILDINGS]; // NB: built[BUILDING_TOWER] should never be used
	int built_lasttime;

	int elements[N_ID]; // elements remaining
	int elementstocks[N_ID]; // elements mined
	int partial_elementstocks[N_ID];

	void initTowerStuff();
	void consumeStocks(Design *design);
	void buildDesign(Design *design);

	int getInventionCost() const;
	int getManufactureCost() const;
	bool inventions_known[3][n_epochs_c];
	//Vector *designs;
	vector<Design *> *designs;

	int getBuildingCost(Type type) const;
	void destroyBuilding(Type building_type);
	void destroyBuilding(Type building_type,bool silent);

	int getDefenceStrength() const;
	void doCombat();
	void doPlayer();

	Building *buildings[N_BUILDINGS];
	Army *assembled_army;
	Army *stored_army;
	Army *armies[n_players_c];
	int stored_defenders[n_epochs_c];
	int stored_shields[4];
	SmokeParticleSystem *smokeParticleSystem;

	PlayingGameState *gamestate;
public:

	Sector(PlayingGameState *gamestate, int epoch, int xpos, int ypos);
	~Sector();

	void createTower(int player,int population);
	void destroyTower(bool nuked);
	bool canShutdown() const;
	void shutdown();
	bool isShutdown() const {
		return this->is_shutdown;
	}
	Building *getBuilding(Type type) const;
	int getNDefenders() const;
	int getNDefenders(int type) const;
	int getDefenderStrength() const;
	void killDefender(int index);
	bool canBuild(Type type) const;
	bool canMine(Id id) const;
	Design *canBuildDesign(Invention::Type type,int epoch) const;
	bool canBuildDesign(Design *design) const;
	bool canEverBuildDesign(Design *design) const;
	void autoTrashDesigns();
	bool tryMiningMore() const;
	bool usedUp() const;
	Design *knownDesign(Invention::Type type,int epoch) const;
	Design *bestDesign(Invention::Type type,int epoch) const;
	Design *canResearch(Invention::Type type,int epoch) const;
	int getNDesigns() const;
	//void consumeStocks(Design *design);
	bool assembleArmy(int epoch,int n);
	bool deployDefender(Building *building,int turret,int epoch);
	void returnDefender(Building *building,int turret);
	int getStoredDefenders(int epoch) const;
	bool useShield(Building *building,int shield);
	int getStoredShields(int shield) const;
	void update();

	int getNFeatures() const {
		return this->features->size();
	}
	Feature *getFeature(int i) const {
		return this->features->at(i);
	}
	const ParticleSystem *getParticleSystem() const {
		return this->smokeParticleSystem;
	}
	ParticleSystem *getParticleSystem() {
		return this->smokeParticleSystem;
	}
	void setEpoch(int epoch);
	int getEpoch() const;
	int getBuildingEpoch() const;
	int getXPos() const {
		return xpos;
	}
	int getYPos() const {
		return ypos;
	}

	int getPlayer() const;
	int getActivePlayer() const;

	void setCurrentDesign(Design *current_design);
	Design *getCurrentDesign() const;
	void setCurrentManufacture(Design *current_manufacture);
	Design *getCurrentManufacture() const;
	void inventionTimeLeft(int *halfdays,int *hours) const;
	void manufactureTotalTime(int *halfdays,int *hours) const;
	void manufactureTimeLeft(int *halfdays,int *hours) const;
	bool inventionKnown(Invention::Type type,int epoch) const;
	void trashDesign(Invention *invention);
	void trashDesign(Design *design);
	bool nuke(Sector *source);
	int beingNuked(int *nuke_time) const {
		*nuke_time = this->nuke_time;
		return nuke_by_player;
	}
	bool isNuked() const {
		return this->nuked;
	}
	bool isTreesNuked() const {
		return this->trees_nuked;
	}

	void setElements(int id,int n_elements);
	void getElements(int *n,int *fraction,int id) const;
	bool anyElements(int id) const;
	void reduceElementStocks(Id id,int reduce);
	void getElementStocks(int *n,int *fraction,Id id) const;
	void getTotalElements(int *n,int *fraction,int id) const;

	void buildingTowerTimeLeft(int player,int *halfdays,int *hours) const;
	void buildingTimeLeft(Type type,int *halfdays,int *hours) const;

	void setPopulation(int population);
	void setDesigners(int n_designers);
	void setWorkers(int n_workers);
	void setFAmount(int n_famount);
	void setMiners(Id id,int n_miners);
	void setBuilders(Type type,int n_builders);

	int getPopulation() const;
	int getSparePopulation() const;
	int getAvailablePopulation() const;
	int getDesigners() const;
	int getWorkers() const;
	int getFAmount() const;
	int getMiners(Id id) const;
	int getBuilders(Type type) const;
	Army *getAssembledArmy() const {
		return assembled_army;
	}
	Army *getStoredArmy() const {
		return stored_army;
	}
	Army *getArmy(int player) const;
	bool enemiesPresent() const;
	bool enemiesPresentWithBombardment() const;
	bool enemiesPresent(int player) const;
	bool enemiesPresent(int player,bool include_unarmed) const;
	void returnAssembledArmy();
	void returnArmy();
	void returnArmy(Army *army);
	bool moveArmy(int player, Army *army);
	void evacuate();

	//void doAIUpdate();

	void printDebugInfo() const;
};
