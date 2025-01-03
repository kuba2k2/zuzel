// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#pragma once

enum {
	SDL_USEREVENT_FIRST = SDL_USEREVENT + 0x100,
	SDL_USEREVENT_SERVER,	  //!< Server starting result
	SDL_USEREVENT_CONNECTION, //!< Client connection result
	SDL_USEREVENT_ERROR,	  //!< An error has occurred
};
