// Copyright (c) Kuba Szczodrzyński 2025-1-17.

#include "match.h"

static int match_thread(game_t *game);

static const unsigned int ping_timeout		= 2000;
static const unsigned int speed_to_delay[9] = {40, 32, 25, 17, 11, 7, 4, 2, 0};

bool match_check_ready(game_t *game) {
	int players_count = 0;
	int ready_count	  = 0;
	player_t *player;
	DL_FOREACH(game->players, player) {
		players_count++;
		if (player->state == PLAYER_READY)
			ready_count++;
	}
	return players_count != 0 && players_count == ready_count;
}

bool match_init(game_t *game) {
	// start the match thread
	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)match_thread, "match", game);
	SDL_DetachThread(thread);
	if (thread == NULL)
		SDL_ERROR("SDL_CreateThread()", return false);

	return true;
}

static int match_thread(game_t *game) {
	char thread_name[19];
	snprintf(thread_name, sizeof(thread_name), "match-%s-%s", game->is_server ? "server" : "client", game->key);
	lt_log_set_thread_name(thread_name);

	LT_I("Match: starting match in game '%s' (%s)", game->name, game->key);

	// server: send game start signal
	if (game->is_server) {
		pkt_game_start_t pkt = {
			.hdr.type = PKT_GAME_START,
		};
		net_pkt_send_pipe(game->endpoints, (pkt_t *)&pkt);
	}

	SDL_WITH_MUTEX(game->mutex) {
		// initialize match state and clear player data
		game->delay = speed_to_delay[game->speed];
		game->time	= 0;
		game->match = 1;
		game->round = 1;
		player_t *player;
		DL_FOREACH(game->players, player) {
			player->match_points = 0;
		}
		player_reset_round(game);
	}

	// server: ping all clients
	if (game->is_server) {
		net_endpoint_t *endpoint, *tmp;
		// create endpoint ping semaphores, reset them to 0
		DL_FOREACH_SAFE(game->endpoints, endpoint, tmp) {
			if (endpoint->type == NET_ENDPOINT_PIPE)
				continue;
			if (endpoint->ping_sem == NULL)
				endpoint->ping_sem = SDL_CreateSemaphore(0);
			while (SDL_SemTryWait(endpoint->ping_sem) == 0) {
				/* reset the semaphore */
			}
		}
		// request ping-based time sync for every endpoint
		game_request_time_sync(game);
		// wait for all clients to report their RTT
		int endpoints_ok = 0;
		DL_FOREACH_SAFE(game->endpoints, endpoint, tmp) {
			if (endpoint->type == NET_ENDPOINT_PIPE)
				continue;
			if (SDL_SemWaitTimeout(endpoint->ping_sem, ping_timeout) == 0) {
				endpoints_ok++;
			} else {
				// endpoint didn't respond, disconnect it
				LT_W("Match: ping timed out after %u ms - disconnecting %s", ping_timeout, net_endpoint_str(endpoint));
				net_endpoint_close(endpoint);
			}
		}

		if (endpoints_ok == 0) {
			LT_W("Match: no endpoints responded! Stopping the thread");
			return 1;
		}
		LT_I("Match: ping completed");
	}

	// server: send game stop signal
	LT_I("Match: thread stopping");
	if (game->is_server) {
		pkt_game_stop_t pkt = {
			.hdr.type = PKT_GAME_STOP,
		};
		net_pkt_send_pipe(game->endpoints, (pkt_t *)&pkt);
	}

	return 0;
}
