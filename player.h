#pragma once

#include "common.h"

/** Handles the players, including all the AI.
*/

class Sector;
class PlayingGameState;

class PlayerType {
public:
	enum PlayerTypeID {
		PLAYER_RED = 0,
		PLAYER_GREEN = 1,
		PLAYER_YELLOW = 2,
		PLAYER_BLUE = 3
	};
	static const char *getName(PlayerTypeID id);
	static void getColour(int *r,int *g,int *b,PlayerTypeID id);
};

class Player {
	//char name[256];
	int index;
	bool dead;
	//int personality;

	int n_births;
	int n_deaths;
	int n_men_for_this_island;
	int n_suspended;

	void doSectorAI(PlayingGameState *gamestate, Sector *sector);
	static bool alliances[n_players_c][n_players_c];
	static int alliance_last_asked[n_players_c][n_players_c];

	int alliance_last_asked_human; // time we last asked human player, to avoid continually asking

	static void setAllianceLastAsked(int a, int b,int time);
	static int allianceLastAsked(int a, int b);
public:
	//Player(int index, char *name);
	//Player(int index, int personality);
	Player(int index);
	~Player();

	//int getShieldIndex();
	static int getShieldIndex(bool allied[n_players_c]);
	/*int getPersonality() {
	return personality;
	}*/
	void doAIUpdate(PlayingGameState *gamestate);
	int getFinalMen() const;
	bool isDead() const {
		return this->dead;
	}
	void kill(PlayingGameState *gamestate);
	bool askHuman();
	bool requestAlliance(int player);

	int getNBirths() const {
		return this->n_births;
	}
	int getNDeaths() const {
		return this->n_deaths;
	}
	int getNMenForThisIsland() const {
		return this->n_men_for_this_island;
	}
	int getNSuspended() const {
		return this->n_suspended;
	}

	void registerBirths(int n) {
		this->n_births += n;
	}
	void setNDeaths(int n) {
		this->n_deaths = n;
	}
	void addNDeaths(int n) {
		this->n_deaths += n;
	}
	void setNMenForThisIsland(int n) {
		this->n_men_for_this_island = n;
	}
	void setNSuspended(int n) {
		this->n_suspended = n;
	}
	void addNSuspended(int n) {
		this->n_suspended += n;
	}

	static void setAlliance(int a, int b, bool alliance);
	static bool isAlliance(int a, int b);
	static void resetAllAlliances();
};
