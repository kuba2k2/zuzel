// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#pragma once

enum {
	SDL_USEREVENT_FIRST = SDL_USEREVENT + 0x100,
	SDL_USEREVENT_SERVER, //!< Server starting result
	SDL_USEREVENT_CLIENT, //!< Client started/stopped
	SDL_USEREVENT_PACKET, //!< Packet has been received
	SDL_USEREVENT_GAME,	  //!< Instance of game_t* is available, or game just stopped
	SDL_USEREVENT_MATCH,  //!< Match update
};

enum {
	MATCH_UPDATE_REDRAW_ALL = 1, //!< Redraw the board and all players
	MATCH_UPDATE_STATE,			 //!< Present the game state
	MATCH_UPDATE_STEP_PLAYERS,	 //!< Draw the players' heads, erase tails
	MATCH_UPDATE_REDRAW_PLAYERS, //!< Redraw all players
};
