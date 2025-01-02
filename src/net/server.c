// Copyright (c) Kuba Szczodrzyński 2024-12-22.

#include "include.h"

static int net_server_listen(void *param);
static int net_server_accept(net_t *net);
static net_err_t net_server_respond(net_endpoint_t *endpoint, pkt_t *recv_pkt);

static net_t *server = NULL;

net_t *net_server_start() {
	if (server != NULL)
		return server;
	MALLOC(server, sizeof(*server), return NULL);

	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)net_server_listen, "server", NULL);
	SDL_DetachThread(thread);
	if (thread == NULL) {
		SDL_ERROR("SDL_CreateThread()", );
		free(server);
		server = NULL;
	}

	return server;
}

void net_server_stop() {
	if (server == NULL)
		return;
	server->stop = true;
	net_endpoint_close(&server->endpoint);
}

static int net_server_listen(void *param) {
	if (server == NULL)
		return -1;
	(void)param;

	// create a net_t* structure for the first client
	net_t *client;
	MALLOC(client, sizeof(*client), goto cleanup);

	// start the TCP server
	struct sockaddr_in saddr = {
		.sin_family = AF_INET,
		.sin_port	= htons(1234),
		.sin_addr	= {{{0}}},
	};
	server->endpoint.addr = saddr;
	server->endpoint.type = NET_ENDPOINT_TCP;
	if (net_endpoint_listen(&server->endpoint) != NET_ERR_OK)
		goto cleanup;

	LT_I(
		"Server: listening on %s:%d with fd=%d",
		inet_ntoa(saddr.sin_addr),
		ntohs(saddr.sin_port),
		server->endpoint.fd
	);

	while (1) {
		// accept an incoming connection
		net_err_t err;
		if ((err = net_endpoint_accept(&server->endpoint, &client->endpoint)) != NET_ERR_OK) {
			if (err == NET_ERR_CLIENT_CLOSED) {
				// non-fatal server error
				LT_W("Server: connection closed during accept()");
				continue;
			}
			if (err == NET_ERR_ACCEPT) {
				// fail on accept() errors
				LT_E("Server: stopping on error");
				goto cleanup;
			}
			if (server->stop) {
				// stop requested, exit without error
				LT_I("Server: stopping gracefully");
				break;
			}
			// disconnect the client on any other error
			LT_E("Server: client connection failed");
			continue;
		}

		// connection was received
		LT_I(
			"Server: connection from %s:%d with fd=%d",
			inet_ntoa(client->endpoint.addr.sin_addr),
			ntohs(client->endpoint.addr.sin_port),
			client->endpoint.fd
		);

		// create a network thread for the client
		SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)net_server_accept, "server-accept", client);
		SDL_DetachThread(thread);
		if (thread == NULL) {
			// thread creation failed, close the connection
			SDL_ERROR("SDL_CreateThread()", );
			net_endpoint_close(&client->endpoint);
		} else {
			// thread successfully created, make a new structure for the next client
			MALLOC(client, sizeof(*client), goto cleanup);
		}
	}

cleanup:
	// stop the server
	net_endpoint_close(&server->endpoint);
	// free the next client's structure
	free(client);
	// free the server's structure
	free(server);
	server = NULL;
	return 0;
}

static int net_server_accept(net_t *net) {
	if (net == NULL)
		return -1;

	while (1) {
		net_err_t ret = net_pkt_recv(&net->endpoint);
		if (ret == NET_ERR_RECV)
			break;
		if (ret == NET_ERR_CLIENT_CLOSED) {
			LT_I(
				"Server: connection closed from %s:%d",
				inet_ntoa(net->endpoint.addr.sin_addr),
				ntohs(net->endpoint.addr.sin_port)
			);
			break;
		}
		if (ret != NET_ERR_OK_PACKET)
			// continue if packet is not fully received yet
			continue;

		// valid packet received, process it and send a response
		ret = net_server_respond(&net->endpoint, &net->endpoint.recv.pkt);
		if (ret == NET_ERR_OK)
			continue;
		if (ret == NET_ERR_OK_PACKET) {
			// endpoint handed over to game thread
			LT_I(
				"Server: connection with %s:%d handed to game thread",
				inet_ntoa(net->endpoint.addr.sin_addr),
				ntohs(net->endpoint.addr.sin_port)
			);
			// exit without closing the connection
			goto exit_thread;
		}
		// break on other errors
		break;
	}

	// disconnect the client
	net_endpoint_close(&net->endpoint);
exit_thread:
	// free the client's structure
	free(net);
	return 0;
}

static net_err_t net_server_respond(net_endpoint_t *endpoint, pkt_t *recv_pkt) {
	switch (recv_pkt->hdr.type) {
		case PKT_PING: {
			pkt_ping_t pkt = {
				.hdr.type	 = PKT_PING,
				.seq		 = recv_pkt->ping.seq,
				.is_response = true,
			};
			return net_pkt_send(endpoint, (pkt_t *)&pkt);
		}

		case PKT_GAME_LIST: {
			SDL_mutex *game_list_mutex;
			game_t *game_list = game_get_list(&game_list_mutex);
			// count the active games
			int total_count;
			SDL_WITH_MUTEX(game_list_mutex) {
				game_t *game;
				DL_COUNT(game_list, game, total_count);
			}
			// send game list response
			pkt_game_list_t pkt = {
				.hdr.type	 = PKT_GAME_LIST,
				.page		 = recv_pkt->game_list.page,
				.per_page	 = recv_pkt->game_list.per_page,
				.total_count = total_count,
			};
			net_pkt_send(endpoint, (pkt_t *)&pkt);
			// send updates for the requested range
			SDL_WITH_MUTEX(game_list_mutex) {
				game_t *game;
				int index = 0;
				unsigned int first = recv_pkt->game_list.page * recv_pkt->game_list.per_page;
				unsigned int last  = first + recv_pkt->game_list.per_page;
				DL_FOREACH(game_list, game) {
					if (index < first) {
						index++;
						continue;
					} else {
						game_send_update(game, NULL, endpoint);
						if (++index >= last)
							break;
					}
				}
			}
			return NET_ERR_OK;
		}

		case PKT_GAME_NEW: {
			game_t *game = game_init();
			// pass the endpoint to the game thread, duplicating it
			game_add_endpoint(game, endpoint);
			return NET_ERR_OK_PACKET;
		}

		case PKT_GAME_JOIN: {
			game_t *game = game_get_by_key(recv_pkt->game_join.key);
			if (game == NULL) {
				pkt_error_t pkt = {
					.hdr.type = PKT_ERROR,
					.error	  = GAME_ERR_NOT_FOUND,
				};
				return net_pkt_send(endpoint, (pkt_t *)&pkt);
			}
			// game found, pass endpoint to game thread
			game_add_endpoint(game, endpoint);
			return NET_ERR_OK_PACKET;
		}

		default: {
			pkt_error_t pkt = {
				.hdr.type = PKT_ERROR,
				.error	  = GAME_ERR_INVALID_STATE,
			};
			return net_pkt_send(endpoint, (pkt_t *)&pkt);
		}
	}
}
