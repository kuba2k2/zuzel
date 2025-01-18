// Copyright (c) Kuba Szczodrzyński 2025-1-15.

#pragma once

#include "include.h"

typedef struct game_t game_t;
typedef struct net_endpoint_t net_endpoint_t;

typedef enum player_state_t {
	PLAYER_IDLE			= 0, //!< Waiting for the race/spectating
	PLAYER_READY		= 1, //!< Ready for the race
	PLAYER_PLAYING		= 2, //!< Currently playing in a race
	PLAYER_CRASHED		= 3, //!< Crashed into a wall
	PLAYER_FINISHED		= 4, //!< Finished the race
	PLAYER_DISCONNECTED = 5, //!< Disconnected from the room
} player_state_t;

typedef enum player_pos_state_t {
	PLAYER_POS_UNCONFIRMED = 0, //!< Position is subject to recalculation
	PLAYER_POS_FORWARD	   = 1, //!< Player is moving in a straight line
	PLAYER_POS_TURNING	   = 2, //!< Player is turning left
} player_pos_state_t;

typedef struct player_pos_t {
	unsigned int time;		  //!< Position timestamp (ticks)
	double x;				  //!< Point X
	double y;				  //!< Point Y
	player_pos_state_t state; //!< Type of the position
} player_pos_t;

typedef struct player_t {
	SDL_mutex *mutex;	  //!< Mutex locking this player's data
	SDL_Texture *texture; //!< SDL texture

	game_t *game;			  //!< Handle to the game
	net_endpoint_t *endpoint; //!< Client handle (server only)
	player_state_t state;	  //!< Current player state

	// client-only parameters
	bool is_local; //!< Whether player is controlled on this device
	int turn_key;  //!< Key (or touch position) for turning left (local only)

	// player options
	unsigned int id;				//!< Unique ID within the game
	char name[PLAYER_NAME_LEN + 1]; //!< Player's nickname
	unsigned int color;				//!< Player's line color

	// round state, controlled by the match thread
	unsigned int time;				  //!< Total playing time (ticks)
	player_pos_t pos[PLAYER_POS_NUM]; //!< Position history
	unsigned int angle;				  //!< Turning angle, 0..359, CCW (0: left)
	double speed;					  //!< Moving speed, 1.0..7.0
	unsigned int lap;				  //!< Lap number, 1..4
	bool lap_can_advance;			  //!< Whether the player moved through half a lap
	int finished_at;				  //!< Winning/losing position

	// scores, controlled by the match thread
	int round_points; //!< Points in the current round
	int match_points; //!< Points in the current match
	int game_points;  //!< Points in the current game

	struct player_t *prev, *next;
} player_t;
