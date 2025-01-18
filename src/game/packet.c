// Copyright (c) Kuba Szczodrzyński 2025-1-5.

#include "game.h"

typedef bool (*game_process_t)(game_t *game, pkt_t *recv_pkt, net_endpoint_t *source);
static bool send_err_invalid_state(game_t *game, pkt_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_ping(game_t *game, pkt_ping_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_game_data(game_t *game, pkt_game_data_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_game_start(game_t *game, pkt_game_start_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_game_stop(game_t *game, pkt_game_stop_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_game_start_round(game_t *game, pkt_game_start_round_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_player_new(game_t *game, pkt_player_new_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_player_data(game_t *game, pkt_player_data_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_player_leave(game_t *game, pkt_player_leave_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_request_send_data(game_t *game, pkt_request_send_data_t *recv_pkt, net_endpoint_t *source);
static bool process_pkt_request_time_sync(game_t *game, pkt_request_time_sync_t *recv_pkt, net_endpoint_t *source);

const game_process_t process_list[] = {
	NULL,
	(game_process_t)process_pkt_ping,			   // PKT_PING
	(game_process_t)send_err_invalid_state,		   // PKT_SUCCESS
	NULL,										   // PKT_ERROR
	(game_process_t)send_err_invalid_state,		   // PKT_GAME_LIST (server-only)
	(game_process_t)send_err_invalid_state,		   // PKT_GAME_NEW (server-only)
	(game_process_t)send_err_invalid_state,		   // PKT_GAME_JOIN (server-only)
	(game_process_t)process_pkt_game_data,		   // PKT_GAME_DATA
	(game_process_t)process_pkt_game_start,		   // PKT_GAME_START
	(game_process_t)process_pkt_game_stop,		   // PKT_GAME_STOP
	(game_process_t)process_pkt_game_start_round,  // PKT_GAME_START_ROUND
	(game_process_t)send_err_invalid_state,		   // PKT_PLAYER_LIST
	(game_process_t)process_pkt_player_new,		   // PKT_PLAYER_NEW
	(game_process_t)process_pkt_player_data,	   // PKT_PLAYER_DATA
	(game_process_t)send_err_invalid_state,		   // PKT_PLAYER_KEYPRESS
	(game_process_t)send_err_invalid_state,		   // PKT_PLAYER_UPDATE
	(game_process_t)process_pkt_player_leave,	   // PKT_PLAYER_LEAVE
	(game_process_t)process_pkt_request_send_data, // PKT_REQUEST_SEND_DATA
	(game_process_t)process_pkt_request_time_sync, // PKT_REQUEST_TIME_SYNC
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

static bool send_err_invalid_state(game_t *game, pkt_t *recv_pkt, net_endpoint_t *source) {
	if (!game->is_server)
		// do not send from client
		return false;
	pkt_error_t pkt = {
		.hdr.type = PKT_ERROR,
		.error	  = GAME_ERR_INVALID_STATE,
	};
	net_pkt_send(source, (pkt_t *)&pkt);
	return false;
}

static bool process_pkt_ping(game_t *game, pkt_ping_t *recv_pkt, net_endpoint_t *source) {
	unsigned long long local_time = millis();

	if (recv_pkt->recv_time == 0) {
		// ping: request
		LT_I("Ping: request from %s", net_endpoint_str(source));
		recv_pkt->recv_time = local_time;
		net_pkt_send(source, (pkt_t *)recv_pkt);
	} else if (recv_pkt->send_time == source->ping_time) {
		// ping: response
		LT_I("Ping: response from %s", net_endpoint_str(source));
		source->ping_rtt   = local_time - recv_pkt->send_time;
		source->time_delta = (long long)recv_pkt->send_time - (long long)recv_pkt->recv_time + source->ping_rtt / 2;
		LT_D("Ping: RTT = %u ms, delta = %lld ms", source->ping_rtt, source->time_delta);
		if (source->ping_sem)
			SDL_SemPost(source->ping_sem);
	}

	return false;
}

static bool process_pkt_game_data(game_t *game, pkt_game_data_t *recv_pkt, net_endpoint_t *source) {
	if (game->is_server && game->state != GAME_IDLE)
		// can't modify a running game
		return false;
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

static bool process_pkt_game_start(game_t *game, pkt_game_start_t *recv_pkt, net_endpoint_t *source) {
	if (game->is_server)
		// server: send to all clients if received on pipe - otherwise ignore
		return source->type == NET_ENDPOINT_PIPE;
	if (game->state != GAME_IDLE)
		// game is already started
		return false;
	game->state = GAME_STARTING;

	// client: start the match thread, send event to UI
	match_init(game);
	return true;
}

static bool process_pkt_game_stop(game_t *game, pkt_game_stop_t *recv_pkt, net_endpoint_t *source) {
	if (game->state == GAME_IDLE)
		// game is already stopped
		return false;
	game->state = GAME_IDLE;
	if (game->is_server)
		// server: send to all clients if received on pipe - otherwise ignore
		return source->type == NET_ENDPOINT_PIPE;

	// client: force player states to IDLE
	player_t *player;
	DL_FOREACH(game->players, player) {
		player->state = PLAYER_IDLE;
	}

	// client: send event to UI
	return true;
}

static bool process_pkt_game_start_round(game_t *game, pkt_game_start_round_t *recv_pkt, net_endpoint_t *source) {
	if (game->state == GAME_IDLE)
		// game is not running
		return false;
	if (game->is_server) {
		if (source->type != NET_ENDPOINT_PIPE)
			// server: ignore if not received on pipe
			return false;
		// server: send to all clients, while adjusting their 'start_at' timestamp
		unsigned long long start_at = recv_pkt->start_at;
		net_endpoint_t *endpoint;
		DL_FOREACH(game->endpoints, endpoint) {
			if (endpoint == source)
				continue;
			recv_pkt->start_at = start_at - endpoint->time_delta - endpoint->ping_rtt / 2;
			net_pkt_send(endpoint, (pkt_t *)recv_pkt);
		}
	}

	// client: post start_at to match thread
	game->start_at = recv_pkt->start_at;
	SDL_SemPost(game->start_at_sem);
	return false;
}

static player_t *get_player_with_endpoint(game_t *game, unsigned int id, net_endpoint_t *source) {
	player_t *player = game_get_player_by_id(game, id);
	if (game->is_server) {
		if (player == NULL)
			// player not found
			return NULL;
		if (source != player->endpoint)
			// disallow modifying other players' data
			return NULL;
	}
	return player;
}

static bool process_pkt_player_new(game_t *game, pkt_player_new_t *recv_pkt, net_endpoint_t *source) {
	if (!game->is_server)
		// client: send packet to other endpoint
		return true;

	player_t *player = player_init(game, recv_pkt->name);
	if (player == NULL)
		goto error;

	if (game->state != GAME_IDLE)
		// players joining ongoing games can only spectate
		player->state = PLAYER_SPECTATING;

	// add to players list
	player->endpoint = source;
	game_add_player(game, player);
	return false;

error:
	free(player);
	if (game->is_server)
		// do not send from client
		game_send_error(game, source, GAME_ERR_SERVER_ERROR);
	return false;
}

static bool process_pkt_player_data(game_t *game, pkt_player_data_t *recv_pkt, net_endpoint_t *source) {
	player_t *player = get_player_with_endpoint(game, recv_pkt->id, source);
	if (game->is_server && player == NULL)
		return false;

	// client: create a player if not found
	bool is_new_player = false;
	if (player == NULL) {
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

	if (game->is_server) {
		if (recv_pkt->state != PLAYER_READY)
			// clients can only set READY state
			recv_pkt->state = player->state;
		else if (player->state != PLAYER_IDLE && player->state != PLAYER_CRASHED && player->state != PLAYER_FINISHED)
			// ...only if they are IDLE, CRASHED or FINISHED
			recv_pkt->state = player->state;
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

	// client: add to players list
	if (is_new_player)
		game_add_player(game, player);

	if (game->is_server)
		// before broadcasting from server, clear 'is_local'
		player->is_local = false;

	// post the ready state semaphore
	SDL_SemPost(game->ready_sem);

	return true;
}

static bool process_pkt_player_leave(game_t *game, pkt_player_leave_t *recv_pkt, net_endpoint_t *source) {
	player_t *player = game_get_player_by_id(game, recv_pkt->id);
	if (player == NULL)
		// player not found
		// client: send to UI anyway
		return !game->is_server;

	// allow leaving as self, as well as kicking others out
	if (game->is_server || source->type != NET_ENDPOINT_PIPE) {
		// server: delete player, send leave event
		// client: wait for leave event from server (if not PIPE)
		game_del_player(game, player);
	}

	// client: send to other endpoint
	return !game->is_server;
}

static bool process_pkt_request_send_data(game_t *game, pkt_request_send_data_t *recv_pkt, net_endpoint_t *source) {
	if (source->type != NET_ENDPOINT_PIPE)
		// only accept packets on pipe
		return false;

	net_endpoint_t *join_endpoint = (void *)recv_pkt->join_endpoint;
	bool updated_game			  = recv_pkt->updated_game;
	player_t *updated_player	  = NULL;
	if (recv_pkt->updated_player)
		DL_SEARCH_SCALAR(game->players, updated_player, id, recv_pkt->updated_player);

	if (join_endpoint != NULL || updated_game) {
		// server: endpoint joined
		// client: data updated locally
		pkt_game_data_t pkt = {
			.hdr.type = PKT_GAME_DATA,
			.is_list  = false,
		};
		game_fill_data_pkt(game, &pkt);
		if (join_endpoint != NULL)
			net_pkt_send(join_endpoint, (pkt_t *)&pkt);
		else
			net_pkt_broadcast(game->endpoints, (pkt_t *)&pkt, source);
	}

	if (join_endpoint != NULL) {
		// server: endpoint joined
		player_t *player;
		DL_FOREACH(game->players, player) {
			pkt_player_data_t pkt = {
				.hdr.type = PKT_PLAYER_DATA,
				.is_local = player->endpoint == join_endpoint,
			};
			player_fill_data_pkt(game, player, &pkt);
			net_pkt_send(join_endpoint, (pkt_t *)&pkt);
		}
	}

	if (updated_player != NULL) {
		pkt_player_data_t pkt = {
			.hdr.type = PKT_PLAYER_DATA,
		};
		player_fill_data_pkt(game, updated_player, &pkt);
		net_endpoint_t *endpoint;
		DL_FOREACH(game->endpoints, endpoint) {
			if (endpoint == source)
				// skip the pipe
				continue;
			pkt.is_local = updated_player->endpoint == endpoint;
			net_pkt_send(endpoint, (pkt_t *)&pkt);
		}
	} else if (recv_pkt->updated_player != 0) {
		pkt_player_leave_t pkt = {
			.hdr.type = PKT_PLAYER_LEAVE,
			.id		  = recv_pkt->updated_player,
		};
		net_pkt_broadcast(game->endpoints, (pkt_t *)&pkt, source);
	}

	return false;
}

static bool process_pkt_request_time_sync(game_t *game, pkt_request_time_sync_t *recv_pkt, net_endpoint_t *source) {
	unsigned long long local_time = millis();

	pkt_ping_t pkt = {
		.hdr.type  = PKT_PING,
		.send_time = local_time,
		.recv_time = 0,
	};

	net_endpoint_t *endpoint;
	DL_FOREACH(game->endpoints, endpoint) {
		if (endpoint == source)
			continue;
		endpoint->ping_time = local_time;
		net_pkt_send(endpoint, (pkt_t *)&pkt);
	}

	return false;
}
