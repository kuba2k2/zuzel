// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "game.h"

static void game_check_empty(game_t *game, bool endpoint_deleted);

/**
 * Duplicate an endpoint and add to the game.
 * Clear the game expiry timer.
 */
void game_add_endpoint(game_t *game, net_endpoint_t *endpoint) {
	// this needs to be thread-safe - it's used in net_client and net_server
	net_endpoint_t *item = net_endpoint_dup(endpoint);
	if (item == NULL)
		return;
	LT_I("Game: adding endpoint %s", net_endpoint_str(endpoint));
	SDL_WITH_MUTEX(game->mutex) {
		DL_APPEND(game->endpoints, item);
	}
	// check if game is empty
	game_check_empty(game, false);
	if (!game->is_server || endpoint->type == NET_ENDPOINT_PIPE)
		// clients don't send data updates
		// pipes don't need data updates
		return;
	// send a data update to the newly-joined endpoint
	pkt_request_send_data_t pkt = {
		.hdr.type	   = PKT_REQUEST_SEND_DATA,
		.join_endpoint = (uintptr_t)item,
	};
	net_pkt_send_pipe(game->endpoints, (pkt_t *)&pkt);
}

/**
 * Close/free and remove an endpoint from the game.
 * Call game_del_player() on all players using this endpoint.
 * Schedule the game expiry timer.
 */
void game_del_endpoint(game_t *game, net_endpoint_t *endpoint) {
	LT_I("Game: deleting endpoint %s", net_endpoint_str(endpoint));
	net_endpoint_type_t type = endpoint->type;
	DL_DELETE(game->endpoints, endpoint);
	net_endpoint_free(endpoint);
	free(endpoint);
	// check if game is empty
	game_check_empty(game, true);
	if (type == NET_ENDPOINT_PIPE || game->stop)
		return;
	// delete all players using this endpoint
	player_t *player, *tmp;
	DL_FOREACH_SAFE(game->players, player, tmp) {
		if (player->endpoint != endpoint)
			continue;
		player->endpoint = NULL;
		game_del_player(game, player);
	}
}

/**
 * Add a player to the game.
 * If server, send player data to every endpoint (incl. the one creating the player).
 */
void game_add_player(game_t *game, player_t *player) {
	LT_I("Game: adding player #%d '%s'", player->id, player->name);
	DL_APPEND(game->players, player);
	if (game->is_server) {
		// only servers send player list updates
		pkt_request_send_data_t pkt = {
			.hdr.type		= PKT_REQUEST_SEND_DATA,
			.updated_player = player->id,
		};
		net_pkt_send_pipe(game->endpoints, (pkt_t *)&pkt);
	}
}

/**
 * Remove (and free) a player from the game.
 * If player is in game, mark as disconnected.
 * If server, send player updates to every endpoint (incl. the one leaving).
 */
void game_del_player(game_t *game, player_t *player) {
	net_endpoint_t *player_endpoint = player->endpoint;
	if (player->state <= PLAYER_READY) {
		// player can be deleted safely
		LT_I("Game: deleting player #%d '%s'", player->id, player->name);
		DL_DELETE(game->players, player);
		if (game->is_server) {
			// only servers send player list updates
			pkt_request_send_data_t pkt = {
				.hdr.type	  = PKT_REQUEST_SEND_DATA,
				.leave_player = player->id,
			};
			net_pkt_send_pipe(game->endpoints, (pkt_t *)&pkt);
		}
		player_free(player);
	} else {
		// player is in game; mark as disconnected
		// keep the player in game_t*
		LT_I("Game: player disconnected #%d '%s'", player->id, player->name);
		player->state = PLAYER_DISCONNECTED;
		if (game->is_server) {
			// only servers send player list updates
			game_request_send_update(game, false, player->id);
		}
	}

	// search any other players on the same endpoint
	if (player_endpoint == NULL)
		// endpoint cleared in game_del_endpoint() - nothing to do
		return;
	DL_SEARCH_SCALAR(game->players, player, endpoint, player_endpoint);
	// close the endpoint if no more players are connected
	if (player == NULL)
		game_del_endpoint(game, player_endpoint);
}

static void game_check_empty(game_t *game, bool endpoint_deleted) {
	int endpoints = 0;
	SDL_WITH_MUTEX(game->mutex) {
		net_endpoint_t *item;
		DL_COUNT(game->endpoints, item, endpoints);
	}
	if (!endpoint_deleted && endpoints == 1)
		// avoid messing with the timer on pipe endpoint adding
		return;

	if (game->expiry_timer != 0) {
		LT_I("Game: clearing expiry timer");
		SDL_RemoveTimer(game->expiry_timer);
		game->expiry_timer = 0;
	}
	if (!endpoint_deleted)
		return;
	if (endpoints > 1)
		return;

	if (game->is_server && !game->is_local) {
		LT_I("Game: empty, setting expiry timer");
		game->expiry_timer = SDL_AddTimer(60000, (SDL_TimerCallback)game_expiry_cb, game);
	} else {
		// clients should immediately stop if the server disconnects
		LT_I("Game: empty, stopping immediately");
		game->stop = true;
	}
}
