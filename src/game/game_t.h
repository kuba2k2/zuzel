// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#pragma once

#include "include.h"

typedef struct net_endpoint_t net_endpoint_t;
typedef struct player_t player_t;

typedef enum game_err_t {
	GAME_ERR_OK			   = 0,	 //!< No error
	GAME_ERR_INVALID_STATE = 1,	 //!< Operation invalid in the current game state
	GAME_ERR_NOT_FOUND	   = 2,	 //!< Game not found by the specified key
	GAME_ERR_NO_PLAYER	   = 3,	 //!< Player not found by the specified ID
	GAME_ERR_SERVER_ERROR  = 99, //!< Internal server error
} game_err_t;

typedef enum game_state_t {
	GAME_IDLE	  = 0, //!< Players are in the lobby
	GAME_STARTING = 1, //!< Game is starting, players can't join anymore
	GAME_PLAYING  = 2, //!< A match is being played by one or more players
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
