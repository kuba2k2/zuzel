// Copyright (c) Kuba Szczodrzyński 2025-1-17.

#include "match.h"

static int match_thread(game_t *game);
static void match_run(game_t *game);

static const unsigned int ping_timeout		= 2000;
static const unsigned int speed_to_delay[9] = {
	40 * 1.5, // Speed 1
	32 * 1.5, // Speed 2
	25 * 1.5, // Speed 3
	17 * 1.6, // Speed 4
	11 * 2,	  // Speed 5
	7 * 2.7,  // Speed 6
	4 * 3.6,  // Speed 7
	2 * 5,	  // Speed 8
	0 + 8,	  // Speed 9
};

bool match_init(game_t *game) {
	// reset the 'start_at' semaphore
	SDL_SemReset(game->start_at_sem);
	game->match_stop = false;

	// start the match thread
	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)match_thread, "match", game);
	game->match_thread = thread;
	if (thread == NULL)
		SDL_ERROR("SDL_CreateThread()", return false);

	return true;
}

void match_stop(game_t *game) {
	if (game->match_thread == NULL)
		return;
	// wake up the match thread, wait for it to quit
	game->match_stop = true;
	SDL_SemPost(game->start_at_sem);
	SDL_SemPost(game->ready_sem);
	SDL_WaitThread(game->match_thread, NULL);
	game->match_thread = NULL;
	game->match_stop   = false;
}

static int match_thread(game_t *game) {
	char thread_name[20];
	snprintf(thread_name, sizeof(thread_name), "match-%s-%s", game->is_server ? "server" : "client", game->key);
	lt_log_set_thread_name(thread_name);
	srand(time(NULL));

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

	for (game->round = 1; game->round <= game->rounds && !game->match_stop;) {
		match_run(game);
		game->round++;
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

static void match_run(game_t *game) {
	LT_I("Match (round %u): initializing round", game->round);

	SDL_WITH_MUTEX(game->mutex) {
		// initialize match state and clear player data
		game->delay = speed_to_delay[game->speed];
		game->lap	= 1;
		// reset player's data, set as PLAYING
		player_reset_round(game);
	}

	// make the UI redraw everything
	game->state = GAME_STARTING;
	match_send_sdl_event(game, MATCH_UPDATE_REDRAW_ALL);

	unsigned long long count_at = 0, start_at = 0;

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
		int max_rtt		 = 0;
		DL_FOREACH_SAFE(game->endpoints, endpoint, tmp) {
			if (endpoint->type == NET_ENDPOINT_PIPE)
				continue;
			if (SDL_SemWaitTimeout(endpoint->ping_sem, ping_timeout) == 0) {
				max_rtt = max(max_rtt, endpoint->ping_rtt);
				endpoints_ok++;
			} else {
				// endpoint didn't respond, disconnect it
				LT_W(
					"Match (round %u): ping timed out after %u ms - disconnecting %s",
					game->round,
					ping_timeout,
					net_endpoint_str(endpoint)
				);
				net_endpoint_close(endpoint);
			}
			if (game->match_stop)
				return;
		}

		if (endpoints_ok == 0) {
			LT_W("Match (round %u): no endpoints responded! Stopping the thread", game->round);
			game->match_stop = true;
			return;
		}

		LT_I("Match (round %u): all clients' ping check finished", game->round);

		// calculate the local timestamp based on max RTT of all endpoints
		// add 100 ms (overhead)
		count_at = millis() + max_rtt + 100;
		// calculate the actual match start timestamp
		// add the countdown timer
		start_at = count_at + GAME_COUNTDOWN_SEC * 1000;
		// send to clients
		pkt_game_start_round_t pkt = {
			.hdr.type = PKT_GAME_START_ROUND,
			.count_at = count_at,
			.start_at = start_at,
		};
		net_pkt_send_pipe(game->endpoints, (pkt_t *)&pkt);
	} else {
		// client: wait for 'start_at' packet from server
		SDL_SemWait(game->start_at_sem);
		if (game->match_stop)
			return;
		count_at = game->count_at;
		start_at = game->start_at;
	}

	// wait until the synchronized countdown
	LT_I("Match (round %u): counting at %llu...", game->round, count_at);
	unsigned long long local_time = millis();
	if (count_at > local_time) {
		SDL_Delay(count_at - local_time);
		if (game->match_stop)
			return;
	} else {
		LT_W("Match (round %u): system clock is behind count_at time!", game->round);
	}

	// run the countdown with approximate delays
	game->state	   = GAME_COUNTING;
	game->start_in = GAME_COUNTDOWN_SEC;
	do {
		// update UI state
		match_send_sdl_event(game, MATCH_UPDATE_STATE);
		// the last delay is unnecessary
		if (game->start_in == 1)
			break;
		// wait for at most 1000 ms
		local_time = millis();
		if (local_time >= start_at)
			break;
		int to_start = start_at - local_time;
		SDL_Delay(min(to_start, 1000));
		if (game->match_stop)
			return;
	} while (--game->start_in);

	// wait until the synchronized match start
	LT_I("Match (round %u): starting at %llu...", game->round, start_at);
	local_time = millis();
	if (start_at > local_time) {
		SDL_Delay(start_at - local_time);
		if (game->match_stop)
			return;
	} else {
		LT_W("Match (round %u): system clock is behind start_at time!", game->round);
	}

	game->state = GAME_PLAYING;
	match_send_sdl_event(game, MATCH_UPDATE_STATE);

	LT_I("Match (round %u): starting now!", game->round);

	// calculate performance delays
	uint64_t perf_freq		 = SDL_GetPerformanceFrequency();
	uint64_t perf_cur		 = SDL_GetPerformanceCounter();
	uint64_t perf_loop_delay = perf_freq * game->delay / 1000;
	uint64_t perf_loop_next	 = perf_cur + perf_loop_delay;
	uint64_t perf_ui_delay	 = perf_freq * 16 / 1000;
	uint64_t perf_ui_next	 = perf_cur + perf_ui_delay;

	LT_D("Match (round %u): performance frequency: %" PRIu64, game->round, perf_freq);

	// run the main game loop
	bool any_in_round		= false;
	bool any_playing		= false;
	bool match_update_state = false;
	do {
		// process all players
		SDL_WITH_MUTEX(game->mutex) {
			player_t *player;
			any_in_round = false;
			DL_FOREACH(game->players, player) {
				if (!player->is_in_round)
					continue;
				SDL_WITH_MUTEX(player->mutex) {
					if (player_loop(player)) {
						// player state changed (lap advanced, crashed, finished, etc.)
						match_update_state = true;
						// save the leading player's lap number
						game->lap = max(game->lap, player->pos[0].lap);
					}
					if (player->is_in_round) {
						any_in_round = true;
						any_playing	 = true;
					}
				}
			}
		}

		// client: update the UI
		perf_cur = SDL_GetPerformanceCounter();
		if (!game->is_server && perf_cur >= perf_ui_next) {
			if (match_update_state) {
				match_send_sdl_event(game, MATCH_UPDATE_STATE);
				match_update_state = false;
			} else {
				match_send_sdl_event(game, MATCH_UPDATE_STEP_PLAYERS);
			}
			perf_ui_next += perf_ui_delay;
		}

		// wait until the next loop timestamp
		perf_cur = SDL_GetPerformanceCounter();
		if (perf_cur < perf_loop_next) {
			uint64_t perf_diff = perf_loop_next - perf_cur;
			SDL_Delay(perf_diff * 1000 / perf_freq);
		} else {
			LT_W("Match (round %u): can't keep up! %" PRIu64 " >= %" PRIu64, game->round, perf_cur, perf_loop_next);
		}
		// increment the next loop timestamp
		perf_loop_next += perf_loop_delay;
	} while (!game->match_stop && any_in_round);

	if (!any_playing) {
		// stop the match if no player was ever playing in this round
		LT_I("Match (round %u): played with no players! Stopping the match...", game->round);
		game->match_stop = true;
	}

	game->state = game->match_stop ? GAME_IDLE : GAME_FINISHED;
	match_send_sdl_event(game, MATCH_UPDATE_STATE);

	LT_I("Match (round %u): finished", game->round);
}
