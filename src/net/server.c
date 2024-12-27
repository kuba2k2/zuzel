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
	closesocket(server->endpoint.fd);
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
			if (server->stop) {
				// stop requested, exit without error
				LT_I("Server: stopping gracefully");
				break;
			}
			// fail on actual errors
			goto cleanup;
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
	struct sockaddr_in caddr = net->endpoint.addr;

#if WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	while (1) {
		net_err_t ret = net_pkt_recv(&net->endpoint);
		if (ret == NET_ERR_RECV)
			break;
		if (ret == NET_ERR_CLIENT_CLOSED) {
			LT_I("Server: connection closed from %s:%d", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
			break;
		}
		if (ret != NET_ERR_OK_PACKET)
			// continue if packet is not fully received yet
			continue;

		// valid packet received, process it and send a response
		if (net_server_respond(&net->endpoint, &net->endpoint.recv.pkt) != NET_ERR_OK)
			break;
	}

	closesocket(net->endpoint.fd);
	free(net);
#if WIN32
	WSACleanup();
#endif
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

		case PKT_GAME_LIST:
			return NET_ERR_OK;
		case PKT_GAME_NEW:
			return NET_ERR_OK;
		case PKT_GAME_JOIN:
			return NET_ERR_OK;

		default: {
			pkt_error_t pkt = {
				.hdr.type = PKT_ERROR,
				.error	  = GAME_ERR_INVALID_STATE,
			};
			return net_pkt_send(endpoint, (pkt_t *)&pkt);
		}
	}
}
