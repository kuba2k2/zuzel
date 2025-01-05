// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#pragma once

enum {
	SDL_USEREVENT_FIRST = SDL_USEREVENT + 0x100,
	SDL_USEREVENT_SERVER, //!< Server starting result
	SDL_USEREVENT_CLIENT, //!< Client started/stopped
	SDL_USEREVENT_PACKET, //!< Packet has been received
	SDL_USEREVENT_GAME,	  //!< Instance of game_t* is available
};
