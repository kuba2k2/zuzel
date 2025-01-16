// Copyright (c) Kuba Szczodrzyński 2025-1-5.

#include "game.h"

typedef bool (*game_process_t)(game_t *game, pkt_t *recv_pkt, net_endpoint_t *source);
bool send_err_invalid_state(game_t *game, pkt_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_ping(game_t *game, pkt_ping_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_game_data(game_t *game, pkt_game_data_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_player_new(game_t *game, pkt_player_new_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_player_data(game_t *game, pkt_player_data_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_request_send_data(game_t *game, pkt_request_send_data_t *recv_pkt, net_endpoint_t *source);

const game_process_t process_list[] = {
	NULL,
	(game_process_t)process_pkt_ping,			   // PKT_PING
	(game_process_t)send_err_invalid_state,		   // PKT_SUCCESS
	NULL,										   // PKT_ERROR
	(game_process_t)send_err_invalid_state,		   // PKT_GAME_LIST (server-only)
	(game_process_t)send_err_invalid_state,		   // PKT_GAME_NEW (server-only)
	(game_process_t)send_err_invalid_state,		   // PKT_GAME_JOIN (server-only)
	(game_process_t)process_pkt_game_data,		   // PKT_GAME_DATA
	(game_process_t)send_err_invalid_state,		   // PKT_GAME_STATE
	(game_process_t)send_err_invalid_state,		   // PKT_PLAYER_LIST
	(game_process_t)process_pkt_player_new,		   // PKT_PLAYER_NEW
	(game_process_t)process_pkt_player_data,	   // PKT_PLAYER_DATA
	(game_process_t)send_err_invalid_state,		   // PKT_PLAYER_STATE
	(game_process_t)send_err_invalid_state,		   // PKT_PLAYER_KEYPRESS
	(game_process_t)send_err_invalid_state,		   // PKT_PLAYER_UPDATE
	(game_process_t)send_err_invalid_state,		   // PKT_PLAYER_LEAVE
	(game_process_t)process_pkt_request_send_data, // PKT_REQUEST_SEND_DATA
};

/**
 * Process a single game packet, updating the game's state as well.
 *
 * @param game game instance
 * @param pkt received packet
 * @param source sender endpoint
 * @return whether the packet should be broadcast to other endpoints
 */
bool game_process_packet(game_t *game, pkt_t *pkt, net_endpoint_t *source) {
	BUILD_BUG_ON(sizeof(process_list) != sizeof(*process_list) * PKT_MAX);
	game_process_t func = process_list[pkt->hdr.type];
	if (func == NULL)
		// no processing function defined
		// server: ignore the packet
		// client: send packet to other endpoint
		return game->is_server ? false : true;
	// call the processing function
	return func(game, pkt, source);
}

static bool process_pkt_ping(game_t *game, pkt_ping_t *recv_pkt, net_endpoint_t *source) {
	pkt_ping_t pkt = {
		.hdr.type	 = PKT_PING,
		.seq		 = recv_pkt->seq,
		.is_response = true,
	};
	net_pkt_send(source, (pkt_t *)&pkt);
	return false;
}

static bool process_pkt_game_data(game_t *game, pkt_game_data_t *recv_pkt, net_endpoint_t *source) {
	if (game->key[0] == '\0') {
		// first-time initialization
		if (recv_pkt->key[0] != '\0')
			memcpy(game->key, recv_pkt->key, sizeof(game->key));
		game->state = recv_pkt->state;
	}

	// update game data
	game->is_public = recv_pkt->is_public;
	game->is_local	= recv_pkt->is_local;
	game->speed		= recv_pkt->speed;
	// update game name
	if (recv_pkt->name[0] != '\0')
		memcpy(game->name, recv_pkt->name, sizeof(game->name));

	return true;
}

static bool process_pkt_player_new(game_t *game, pkt_player_new_t *recv_pkt, net_endpoint_t *source) {
	if (!game->is_server)
		// client: send packet to other endpoint
		return true;

	player_t *player = player_init(game, recv_pkt->name);
	if (player == NULL)
		goto error;
	LT_I("Game: created player #%d '%s'", player->id, player->name);
	player->endpoint = source;

	// add to players list
	SDL_WITH_MUTEX(game->mutex) {
		DL_APPEND(game->players, player);
	}
	// broadcast all players' data
	game_request_send_data(game, false, true);

	return false;

error:
	free(player);
	if (game->is_server)
		// do not send from client
		game_send_error(source, GAME_ERR_SERVER_ERROR);
	return false;
}

static bool process_pkt_player_data(game_t *game, pkt_player_data_t *recv_pkt, net_endpoint_t *source) {
	player_t *player = NULL;
	// find player by ID
	SDL_WITH_MUTEX(game->mutex) {
		DL_SEARCH_SCALAR(game->players, player, id, recv_pkt->id);
	}

	// check if player found
	bool is_new_player = false;
	if (player == NULL) {
		if (game->is_server) {
			// server can't create player here
			game_send_error(source, GAME_ERR_NO_PLAYER);
			return false;
		}
		// client has to create missing players
		player = player_init(game, recv_pkt->name);
		if (player == NULL)
			return false;
		LT_I(
			"Game client: created %s player #%d '%s'",
			recv_pkt->is_local ? "local" : "remote",
			player->id,
			player->name
		);
		is_new_player = true;
	}

	// update player data
	player->id	  = recv_pkt->id; // safe; either a new player or the ID is already the same
	player->color = recv_pkt->color;
	player->state = recv_pkt->state;
	// update player name
	if (recv_pkt->name[0] != '\0')
		memcpy(player->name, recv_pkt->name, sizeof(player->name));

	if (!game->is_server && recv_pkt->is_local) {
		// server will never set 'is_local' in data updates/broadcasts
		// so if it's set, this packet is from process_pkt_send_game_data()
		player->is_local = true;
	}

	// add to players list (client-only)
	if (is_new_player) {
		SDL_WITH_MUTEX(game->mutex) {
			DL_APPEND(game->players, player);
		}
	}

	if (game->is_server)
		// before broadcasting from server, clear 'is_local'
		player->is_local = false;

	return true;
}

static bool process_pkt_request_send_data(game_t *game, pkt_request_send_data_t *recv_pkt, net_endpoint_t *source) {
	if (source->type != NET_ENDPOINT_PIPE)
		// only accept packets on pipe
		return false;

	SDL_LOCK_MUTEX(game->mutex);

	// make a game data packet
	pkt_game_data_t pkt_game_data = {
		.hdr.type = PKT_GAME_DATA,
		.is_list  = false,
	};
	game_fill_data_pkt(game, &pkt_game_data);

	// iterate over all connected endpoints
	net_endpoint_t *endpoint;
	DL_FOREACH(game->endpoints, endpoint) {
		if (endpoint == source)
			// skip pipe
			continue;

		// send game data
		if (recv_pkt->send_game) {
			net_pkt_send(endpoint, (pkt_t *)&pkt_game_data);
		}

		// send all players' data
		if (recv_pkt->send_players) {
			player_t *player;
			DL_FOREACH(game->players, player) {
				pkt_player_data_t pkt_player_data = {
					.hdr.type = PKT_PLAYER_DATA,
					.is_local = player->endpoint == endpoint,
				};
				player_fill_data_pkt(game, player, &pkt_player_data);
				net_pkt_send(endpoint, (pkt_t *)&pkt_player_data);
			}
		}
	}

	SDL_UNLOCK_MUTEX(game->mutex);

	return false;
}
