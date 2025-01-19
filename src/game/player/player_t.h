// Copyright (c) Kuba Szczodrzyński 2025-1-15.

#pragma once

#include "include.h"

typedef struct game_t game_t;
typedef struct net_endpoint_t net_endpoint_t;

typedef enum player_state_t {
	PLAYER_IDLE			= (1 << 0), //!< Waiting for the race/spectating
	PLAYER_READY		= (1 << 1), //!< Ready for the race
	PLAYER_PLAYING		= (1 << 2), //!< Currently playing in a race
	PLAYER_CRASHED		= (1 << 3), //!< Crashed into a wall
	PLAYER_FINISHED		= (1 << 4), //!< Finished the race
	PLAYER_DISCONNECTED = (1 << 5), //!< Disconnected from the room
	PLAYER_SPECTATING	= (1 << 6), //!< Spectating the game (waiting for a new match)
} player_state_t;

#define PLAYER_NOT_READY_MASK (PLAYER_IDLE | PLAYER_CRASHED | PLAYER_FINISHED)
#define PLAYER_IN_GAME_MASK	  (PLAYER_READY | PLAYER_PLAYING | PLAYER_CRASHED | PLAYER_FINISHED)
#define PLAYER_IN_MATCH_MASK  (PLAYER_PLAYING | PLAYER_CRASHED | PLAYER_FINISHED)

typedef enum player_pos_dir_t {
	PLAYER_POS_FORWARD = 0, //!< Player is moving in a straight line
	PLAYER_POS_LEFT	   = 1, //!< Player is turning left
} player_pos_dir_t;

typedef struct player_pos_t {
	unsigned int time;	//!< Position timestamp (ticks)
	unsigned int angle; //!< Turning angle, 0..359, CCW (0: left)
	double speed;		//!< Moving speed, 1.0..7.0
	double x;			//!< Position X
	double y;			//!< Position Y
	int direction;		//!< Movement direction for the next position
	bool confirmed;		//!< Whether the remote player's movement direction is confirmed
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
	unsigned int lap;				  //!< Lap number, 1..4
	bool lap_can_advance;			  //!< Whether the player moved through half a lap

	// scores, controlled by the match thread
	int round_points; //!< Points in the current round
	int match_points; //!< Points in the current match
	int game_points;  //!< Points in the current game

	struct player_t *prev, *next;
} player_t;
