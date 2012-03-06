#pragma once

const int n_epochs_c = 10;
const int n_players_c = 4;
const int map_width_c = 5;
const int map_height_c = 5;
const int n_slots_c = 10;

enum AmmoDirection {
	ATTACKER_AMMO_DOWN = 0,
	ATTACKER_AMMO_UP = 1,
	ATTACKER_AMMO_RIGHT = 2,
	ATTACKER_AMMO_LEFT = 3,
	ATTACKER_AMMO_BOMB = 4,
	N_ATTACKER_AMMO_DIRS = 5
};

enum Type {
	BUILDING_TOWER = 0,
	BUILDING_MINE = 1,
	BUILDING_FACTORY = 2,
	BUILDING_LAB = 3,
	N_BUILDINGS = 4
};

enum Id {
	UNDEFINED = -1,
	WOOD = 0,
	ROCK,
	BONE,
	SLATE,
	MOONLITE,
	PLANETARIUM,
	BETHLIUM,
	SOLARIUM,
	ARULDITE,
	HERBIRITE,
	YERIDIUM,
	VALIUM,
	PARASITE,
	AQUARIUM,
	PALADIUM,
	ONION,
	TEDIUM,
	MORON,
	MARMITE,
	ALIEN,
	N_ID
};
