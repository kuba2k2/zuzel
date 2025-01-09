// Copyright (c) Kuba Szczodrzyński 2025-1-5.

#include "game.h"

typedef bool (*game_process_t)(game_t *game, pkt_t *recv_pkt, net_endpoint_t *source);
static bool process_invalid_state(game_t *game, pkt_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_ping(game_t *game, pkt_ping_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_game_data(game_t *game, pkt_game_data_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_send_game_data(game_t *game, pkt_send_game_data_t *recv_pkt, net_endpoint_t *source);

const game_process_t process_list[] = {
	NULL,
	(game_process_t)process_pkt_ping,			// PKT_PING
	(game_process_t)process_invalid_state,		// PKT_SUCCESS
	NULL,										// PKT_ERROR
	(game_process_t)process_invalid_state,		// PKT_GAME_LIST
	(game_process_t)process_invalid_state,		// PKT_GAME_NEW
	(game_process_t)process_invalid_state,		// PKT_GAME_JOIN
	(game_process_t)process_pkt_game_data,		// PKT_GAME_DATA
	(game_process_t)process_invalid_state,		// PKT_GAME_STATE
	(game_process_t)process_invalid_state,		// PKT_PLAYER_LIST
	(game_process_t)process_invalid_state,		// PKT_PLAYER_NEW
	(game_process_t)process_invalid_state,		// PKT_PLAYER_DATA
	(game_process_t)process_invalid_state,		// PKT_PLAYER_STATE
	(game_process_t)process_invalid_state,		// PKT_PLAYER_KEYPRESS
	(game_process_t)process_invalid_state,		// PKT_PLAYER_UPDATE
	(game_process_t)process_invalid_state,		// PKT_PLAYER_LEAVE
	(game_process_t)process_pkt_send_game_data, // PKT_SEND_GAME_DATA
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
		// no processing function defined, simply ignore the packet
		return false;
	// call the processing function
	return func(game, pkt, source);
}

static bool process_invalid_state(game_t *game, pkt_t *recv_pkt, net_endpoint_t *source) {
	pkt_error_t pkt = {
		.hdr.type = PKT_ERROR,
		.error	  = GAME_ERR_INVALID_STATE,
	};
	net_pkt_send(source, (pkt_t *)&pkt);
	return false;
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
			strncpy(game->key, recv_pkt->key, sizeof(game->key) - 1);
		game->state = recv_pkt->state;
	}

	// update game name
	if (recv_pkt->name[0] != '\0')
		strncpy(game->name, recv_pkt->name, sizeof(game->name) - 1);

	game->is_public	   = recv_pkt->is_public;
	game->is_local	   = recv_pkt->is_local;
	game->player_count = recv_pkt->players;
	game->speed		   = recv_pkt->speed;

	return true;
}

static bool process_pkt_send_game_data(game_t *game, pkt_send_game_data_t *recv_pkt, net_endpoint_t *source) {
	pkt_game_data_t pkt = {
		.hdr.type = PKT_GAME_DATA,
		.is_list  = false,
	};
	game_fill_game_data(game, &pkt);

	SDL_WITH_MUTEX(game->mutex) {
		net_pkt_broadcast(game->endpoints, (pkt_t *)&pkt, source);
	}
	return false;
}
