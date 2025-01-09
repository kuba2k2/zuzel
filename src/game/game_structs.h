// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#pragma once

#include "include.h"

typedef struct net_endpoint_t net_endpoint_t;
typedef struct player_t player_t;

typedef enum game_err_t {
	GAME_ERR_OK			   = 0, //!< No error
	GAME_ERR_INVALID_STATE = 1, //!< Operation invalid in the current game state
	GAME_ERR_NOT_FOUND	   = 2, //!< Game not found by the specified key
} game_err_t;

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

typedef enum game_state_t {
	GAME_IDLE	 = 0, //!< Players are in the lobby
	GAME_READY	 = 1, //!< Game is starting, players can't join anymore
	GAME_PLAYING = 2, //!< A match is being played by one or more players
} game_state_t;

typedef struct game_t {
	SDL_mutex *mutex;		  //!< Mutex locking the game (players list and other options)
	SDL_TimerID expiry_timer; //!< Expiry timer for the game
	bool stop;				  //!< Whether to stop the game thread
	bool is_server;			  //!< Whether this game is servers other players (clients)
	bool is_public;			  //!< Whether this game is public (searchable)
	bool is_local;			  //!< Whether this game is served by/connected to a LAN server
	char *local_ips;		  //!< Local IP addresses (for UI, client-only)

	net_endpoint_t *endpoints; //!< Communication pipe and other connected devices
	player_t *players;		   //!< Players in the room
	int player_count;		   //!< Number of players in the room (used for UI)

	// game options
	char name[GAME_NAME_LEN + 1]; //!< Room name
	char key[GAME_KEY_LEN + 1];	  //!< Room key
	unsigned int speed;			  //!< Game speed, 1..9
	unsigned int delay;			  //!< Loop delay based on speed (ms)

	// game and match state
	game_state_t state; //!< Current game state
	unsigned int time;	//!< Game time (ticks)
	unsigned int round; //!< Round number, 1..15

	struct game_t *prev, *next;
} game_t;

typedef struct player_pos_t {
	unsigned int time;		  //!< Position timestamp (ticks)
	double x;				  //!< Point X
	double y;				  //!< Point Y
	player_pos_state_t state; //!< Type of the position
} player_pos_t;

typedef struct player_t {
	SDL_sem *mutex;		  //!< Mutex locking this player's data
	SDL_Texture *texture; //!< SDL texture

	net_endpoint_t *endpoint; //!< Client handle (server only)
	player_state_t state;	  //!< Current player state

	// client-only parameters
	bool is_local; //!< Whether player is controlled on this device
	int turn_key;  //!< Key (or touch position) for turning left (local only)

	// player options
	unsigned int id;				//!< Unique ID within the game
	char name[PLAYER_NAME_LEN + 1]; //!< Player's nickname
	unsigned int color;				//!< Player's line color

	// round state
	unsigned int time;	   //!< Total playing time (ticks)
	player_pos_t pos[100]; //!< Position history
	unsigned int angle;	   //!< Turning angle, 0..359, CCW (0: left)
	double speed;		   //!< Moving speed, 1.0..7.0
	unsigned int lap;	   //!< Lap number, 1..4
	bool lap_can_advance;  //!< Whether the player moved through half a lap
	int finished_at;	   //!< Winning/losing position

	// scores
	int round_points; //!< Points in the current round
	int match_points; //!< Points in the current match
	int game_points;  //!< Points in the current game

	struct player_t *prev, *next;
} player_t;
