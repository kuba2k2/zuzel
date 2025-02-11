// Copyright (c) Kuba Szczodrzyński 2024-12-27.

#include "game.h"

static int game_thread(game_t *game);
static net_err_t game_select_read_cb(net_endpoint_t *endpoint, game_t *game);
static void game_select_err_cb(net_endpoint_t *endpoint, game_t *game, net_err_t err);

static game_t *game_list		  = NULL;
static SDL_mutex *game_list_mutex = NULL;

game_t *game_init(pkt_game_data_t *pkt_data) {
	game_t *game;
	MALLOC(game, sizeof(*game), goto cleanup);

	SDL_WITH_MUTEX(game->mutex) {
		// create an expiry timer (initially 5000 ms)
		game->expiry_timer = SDL_AddTimer(5000, (SDL_TimerCallback)game_expiry_cb, game);
		// create semaphores
		game->ready_sem	   = SDL_CreateSemaphore(0);
		game->start_at_sem = SDL_CreateSemaphore(0);
		// set some default settings
		game->is_public = false;
		game->speed		= SETTINGS->game_speed;
		game->state		= GAME_IDLE;
		game->rounds	= 15;

		if (pkt_data == NULL) {
			// new game created, set the server's default options
			game->is_server = true;
			game_set_default_player_options(game);
			// generate a game key
			do {
				char *ch = game->key;
				for (int i = 0; i < sizeof(game->key) - 1; i++) {
					int num = '0' + rand() % 36;
					if (num > '9')
						num += 'A' - '9' - 1;
					*ch++ = (char)num;
				}
			} while (game_get_by_key(game->key) != NULL);
		} else {
			// joined a game, apply data from PKT_GAME_DATA
			game_process_packet(game, (pkt_t *)pkt_data, NULL);
			// fetch the local IP addresses (for UI)
			game->local_ips = net_get_local_ips();
		}
	}

	// create a pipe for incoming packets
	{
		net_endpoint_t pipe = {0};
		net_endpoint_pipe(&pipe);
		// servers shouldn't send SDL events as response to pipe packets
		pipe.pipe.no_sdl = game->is_server;
		game_add_endpoint(game, &pipe);
	}

	// finally, start the game thread
	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)game_thread, "game", game);
	SDL_DetachThread(thread);
	if (thread == NULL)
		SDL_ERROR("SDL_CreateThread()", goto cleanup);

	if (game->is_server) {
		// only use game_list server-side
		SDL_WITH_MUTEX(game_list_mutex) {
			DL_APPEND(game_list, game);
		}
	}

	return game;

cleanup:
	game_free(game);
	return NULL;
}

game_t *game_get_list(SDL_mutex **mutex) {
	if (mutex != NULL)
		*mutex = game_list_mutex;
	return game_list;
}

uint32_t game_expiry_cb(uint32_t interval, game_t *game) {
	LT_I("Game: expired '%s' (key: %s)", game->name, game->key);
	SDL_WITH_MUTEX(game->mutex) {
		game_stop(game);
	}
	return 0;
}

void game_stop(game_t *game) {
	if (game == NULL)
		return;
	game->stop	   = true;
	pkt_ping_t pkt = {
		.hdr.type = PKT_PING,
	};
	net_pkt_send_pipe(game->endpoints, (pkt_t *)&pkt);
}

void game_free(game_t *game) {
	if (game == NULL)
		return;
	// remove the game from the global list
	if (game->is_server) {
		SDL_WITH_MUTEX(game_list_mutex) {
			DL_DELETE(game_list, game);
		}
	}
	// stop the match thread
	match_stop(game);
	// close and free all endpoints
	SDL_WITH_MUTEX(game->mutex) {
		net_endpoint_t *endpoint, *tmp;
		DL_FOREACH_SAFE(game->endpoints, endpoint, tmp) {
			DL_DELETE(game->endpoints, endpoint);
			net_endpoint_free(endpoint);
			SDL_DestroyMutex(endpoint->mutex);
			free(endpoint);
		}
	}
	// free all players
	SDL_WITH_MUTEX(game->mutex) {
		player_t *player, *tmp;
		DL_FOREACH_SAFE(game->players, player, tmp) {
			DL_DELETE(game->players, player);
			player_free(player);
		}
	}
	// free remaining members
	SDL_DestroyMutex(game->mutex);
	SDL_DestroySemaphore(game->ready_sem);
	SDL_DestroySemaphore(game->start_at_sem);
	SDL_RemoveTimer(game->expiry_timer);
	free(game->local_ips);
	free(game);
}

static int game_thread(game_t *game) {
	if (game == NULL)
		return -1;
	char thread_name[19];
	snprintf(thread_name, sizeof(thread_name), "game-%s-%s", game->is_server ? "server" : "client", game->key);
	lt_log_set_thread_name(thread_name);
	srand((unsigned int)time(NULL));

	LT_I("Game: starting '%s' (key: %s)", game->name, game->key);

	while (!game->stop) {
		// wait for incoming data
		net_err_t err = net_endpoint_select(
			game->endpoints,
			game->mutex,
			(net_select_read_cb_t)game_select_read_cb,
			(net_select_err_cb_t)game_select_err_cb,
			game
		);
		if (err != NET_ERR_OK)
			goto cleanup;
		if (SETTINGS->net_slowdown)
			SDL_Delay(100);

		// server: check if all players are ready now
		if (game->is_server && game->state == GAME_IDLE && match_check_ready(game) && !game->match_stop &&
			!game->stop) {
			// start the match
			game->state = GAME_STARTING;
			if (game->match_thread != NULL) {
				// if there is a running match thread, quit
				LT_E("Game: match thread already running");
				game_send_error(game, NULL, GAME_ERR_SERVER_ERROR);
				goto cleanup;
			}
			if (!match_init(game)) {
				// if thread creation failed, quit
				LT_E("Game: match thread creation failed");
				game_send_error(game, NULL, GAME_ERR_SERVER_ERROR);
				goto cleanup;
			}
		}
	}

cleanup:
	game->stop = true;
	if (!game->is_server) {
		// send game stop event to UI
		SDL_Event event = {
			.user.type	= SDL_USEREVENT_GAME,
			.user.data1 = NULL,
		};
		SDL_PushEvent(&event);
	}
	LT_I("Game: stopping '%s' (key: %s)", game->name, game->key);
	game_free(game);
	LT_I("Game: thread stopped");
	return 0;
}

static net_err_t game_select_read_cb(net_endpoint_t *endpoint, game_t *game) {
	net_err_t ret = net_pkt_recv(endpoint);
	if (ret < NET_ERR_OK)
		return ret;
	if (ret != NET_ERR_OK_PACKET)
		// continue if packet is not fully received yet
		return NET_ERR_OK;

	// valid packet received, process it and send a response
	if (game_process_packet(game, &endpoint->recv.pkt, endpoint) == false)
		// if 'false', packet was consumed by processing
		return NET_ERR_OK;

	// otherwise, broadcast the packet to other endpoints
	SDL_WITH_MUTEX(game->mutex) {
		net_pkt_broadcast(game->endpoints, &endpoint->recv.pkt, endpoint);
	}
	return NET_ERR_OK;
}

static void game_select_err_cb(net_endpoint_t *endpoint, game_t *game, net_err_t err) {
	if (err == NET_ERR_CLIENT_CLOSED)
		LT_I("Game: connection closed from %s", net_endpoint_str(endpoint));
	else
		LT_E("Game: connection error from %s", net_endpoint_str(endpoint));
	game_del_endpoint(game, endpoint);
}
