// Copyright (c) Kuba Szczodrzyński 2025-1-17.

#include "match.h"

static int match_thread(game_t *game);
static int match_run(game_t *game);

static const unsigned int ping_timeout		= 2000;
static const unsigned int speed_to_delay[9] = {40, 32, 25, 17, 11, 7, 4, 2, 0};

bool match_init(game_t *game) {
	// reset the 'start_at' semaphore
	SDL_SemReset(game->start_at_sem);

	// start the match thread
	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)match_thread, "match", game);
	SDL_DetachThread(thread);
	if (thread == NULL)
		SDL_ERROR("SDL_CreateThread()", return false);

	return true;
}

static int match_thread(game_t *game) {
	char thread_name[20];
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

	player_t *player;
	DL_FOREACH(game->players, player) {
		player->match_points = 0;
	}

	for (game->round = 1; game->round <= 15; game->round++) {
		match_run(game);
		match_wait_ready(game);
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

static int match_run(game_t *game) {
	SDL_WITH_MUTEX(game->mutex) {
		// initialize match state and clear player data
		game->delay = speed_to_delay[game->speed];
		game->time	= 0;
		game->lap	= 1;
		// reset player's data, set as PLAYING
		player_reset_round(game);
	}

	// make the UI redraw everything
	match_send_sdl_event(game, MATCH_UPDATE_REDRAW_ALL);

	unsigned long long start_at = 0;

	if (game->is_server) {
		// server: ping all clients
		net_endpoint_t *endpoint, *tmp;
		// create endpoint ping semaphores, reset them to 0
		DL_FOREACH_SAFE(game->endpoints, endpoint, tmp) {
			if (endpoint->type == NET_ENDPOINT_PIPE)
				continue;
			if (endpoint->ping_sem == NULL)
				endpoint->ping_sem = SDL_CreateSemaphore(0);
			SDL_SemReset(endpoint->ping_sem);
		}
		// request ping-based time sync for every endpoint
		game_request_time_sync(game);
		// wait for all clients to report their RTT
		int endpoints_ok = 0;
		DL_FOREACH_SAFE(game->endpoints, endpoint, tmp) {
			if (endpoint->type == NET_ENDPOINT_PIPE)
				continue;
			if (SDL_SemWaitTimeout(endpoint->ping_sem, ping_timeout) == 0) {
				start_at = max(start_at, endpoint->ping_rtt);
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

		LT_I("Match: all clients' ping check finished");

		// 'start_at' is the max RTT of the endpoints
		// calculate the local timestamp based on that
		start_at += millis();
		// add 100 ms (overhead)
		start_at += 100;
		// send to clients
		pkt_game_start_round_t pkt = {
			.hdr.type = PKT_GAME_START_ROUND,
			.start_at = start_at,
		};
		net_pkt_send_pipe(game->endpoints, (pkt_t *)&pkt);
	} else {
		// client: wait for 'start_at' packet from server
		SDL_SemWait(game->start_at_sem);
		start_at = game->start_at;
	}

	LT_I("Match: starting at %llu...", start_at);
	unsigned long long local_time = millis();
	if (start_at > local_time) {
		SDL_Delay(start_at - local_time);
	} else {
		LT_W("Match: clock is behind match time!");
	}
	LT_I("Match: starting now!");

	game->state	   = GAME_COUNTING;
	game->start_in = 3;
	do {
		// update UI state
		match_send_sdl_event(game, MATCH_UPDATE_STATE);
		SDL_Delay(1000);
	} while (--game->start_in);
	game->state = GAME_PLAYING;

	match_send_sdl_event(game, MATCH_UPDATE_STATE);

	// calculate performance delays
	uint64_t perf_freq		 = SDL_GetPerformanceFrequency();
	uint64_t perf_cur		 = SDL_GetPerformanceCounter();
	uint64_t perf_loop_delay = perf_freq * game->delay / 1000;
	uint64_t perf_loop_next	 = perf_cur + perf_loop_delay;
	uint64_t perf_ui_delay	 = perf_freq * 16 / 1000;
	uint64_t perf_ui_next	 = perf_cur + perf_ui_delay;

	// run the main game loop
	bool playing = false;
	do {
		// process all players
		SDL_WITH_MUTEX(game->mutex) {
			player_t *player;
			playing = false;
			DL_FOREACH(game->players, player) {
				if ((player->state & PLAYER_IN_MATCH_MASK) == 0)
					continue;
				SDL_WITH_MUTEX(player->mutex) {
					player_loop(game, player);
					if (player->state == PLAYER_PLAYING)
						playing = true;
				}
			}
		}

		// client: update the UI
		perf_cur = SDL_GetPerformanceCounter();
		if (!game->is_server && perf_cur >= perf_ui_next) {
			if (game->update_state)
				match_send_sdl_event(game, MATCH_UPDATE_STATE);
			if (game->update_redraw_players)
				match_send_sdl_event(game, MATCH_UPDATE_REDRAW_ALL);
			else
				match_send_sdl_event(game, MATCH_UPDATE_STEP_PLAYERS);
			perf_ui_next += perf_ui_delay;
		}

		// wait until the next loop timestamp
		perf_cur = SDL_GetPerformanceCounter();
		if (perf_cur < perf_loop_next) {
			uint64_t perf_diff = perf_loop_next - perf_cur;
			SDL_Delay(perf_diff * 1000 / perf_freq);
		}
		// increment the next loop timestamp
		perf_loop_next += perf_loop_delay;
	} while (playing);

	game->state = GAME_FINISHED;

	LT_I("Match: finished");

	return 0;
}
