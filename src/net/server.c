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
	(void)param;

#if WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	int sfd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sfd == INVALID_SOCKET)
		SOCK_ERROR("socket()", goto cleanup);

	// enable address reuse
	char on = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0)
		SOCK_ERROR("setsockopt()", goto cleanup);

	// bind the server socket to an address
	struct sockaddr_in saddr = {
		.sin_family = AF_INET,
		.sin_port	= htons(1234),
		.sin_addr	= {{{0}}},
	};
	if (bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0)
		SOCK_ERROR("bind()", goto cleanup);

	// listen for incoming connections
	if (listen(sfd, 10) != 0)
		SOCK_ERROR("listen()", goto cleanup);

	// server started successfully, fill net_t*
	LT_I("Server: listening on %s:%d with fd=%d", inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port), sfd);
	server->endpoint.addr = saddr;
	server->endpoint.fd	  = sfd;

	while (1) {
		int cfd;
		struct sockaddr_in caddr = {0};
		int addrlen				 = sizeof(caddr);

		// accept an incoming connection
		if ((cfd = (int)accept(sfd, (struct sockaddr *)&caddr, &addrlen)) < 0) {
#if WIN32
			if (WSAGetLastError() == WSAECONNRESET) {
#else
			if (errno == ECONNABORTED) {
#endif
				LT_W("Server: connection reset during accept()");
				continue;
			}
			if (server->stop)
				// stop requested, exit without error
				break;
			SOCK_ERROR("accept()", );
			goto cleanup;
		}

		// connection was received
		LT_I("Server: connection from %s:%d with fd=%d", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), cfd);

		// create a net_t* structure for the client
		net_t *net;
		MALLOC(net, sizeof(*net), goto cleanup_client);
		net->endpoint.addr = caddr;
		net->endpoint.fd   = cfd;

		// create a network thread for the client
		SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)net_server_accept, "server-accept", net);
		SDL_DetachThread(thread);
		if (thread == NULL)
			SDL_ERROR("SDL_CreateThread()", goto cleanup_client);

		// if successful, run the loop again
		continue;
	cleanup_client:
		closesocket(cfd);
		free(net);
	}

	LT_I("Server: stopping gracefully");

cleanup:
	closesocket(sfd);
	free(server);
	server = NULL;
#if WIN32
	WSACleanup();
#endif
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
		if (ret == NET_ERR_CONN_CLOSED) {
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
