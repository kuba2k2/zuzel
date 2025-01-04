// Copyright (c) Kuba Szczodrzyński 2025-1-3.

#include "net.h"

static int net_client_connect(char *address);

static net_t *client = NULL;

net_t *net_client_start(const char *address, bool use_tls) {
	if (client != NULL)
		return client;
	MALLOC(client, sizeof(*client), return NULL);

	client->endpoint.type = use_tls ? NET_ENDPOINT_TLS : NET_ENDPOINT_TCP;

	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)net_client_connect, "client", strdup(address));
	SDL_DetachThread(thread);
	if (thread == NULL) {
		SDL_ERROR("SDL_CreateThread()", );
		free(client);
		client = NULL;
	}

	return client;
}

void net_client_stop() {
	if (client == NULL)
		return;
	client->stop = true;
	net_endpoint_close(&client->endpoint);
}

static int net_client_connect(char *address) {
	if (client == NULL)
		return -1;

#if WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	SDL_Event event = {
		.user.type = SDL_USEREVENT_CLIENT,
	};

	// resolve the host name
	struct addrinfo hints = {
		.ai_family	 = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP,
	};
	struct addrinfo *info = NULL;
	int gai_error		  = getaddrinfo(address, NULL, &hints, &info);
	free(address);
#if WIN32
	WSASetLastError(gai_error); // fix for WSANOTINITIALISED not being delivered otherwise
#endif
	if (gai_error != 0)
		SOCK_ERROR("getaddrinfo()", goto error_start);

	// build the destination server address
	struct sockaddr_in saddr = {
		.sin_family = AF_INET,
		.sin_port	= htons(1234),
		.sin_addr	= ((struct sockaddr_in *)info->ai_addr)->sin_addr,
	};
	freeaddrinfo(info);

	// start the TCP client
	client->endpoint.addr = saddr;
	client->endpoint.type = NET_ENDPOINT_TCP;
	if (net_endpoint_connect(&client->endpoint) != NET_ERR_OK)
		goto error_start;

	LT_I(
		"Client: connected to %s:%d with fd=%d",
		inet_ntoa(saddr.sin_addr),
		ntohs(saddr.sin_port),
		client->endpoint.fd
	);
	event.user.code = true;
	SDL_PushEvent(&event);

	net_t *net = client;
	while (1) {
		net_err_t ret = net_pkt_recv(&net->endpoint);
		if (ret == NET_ERR_RECV)
			goto cleanup;
		if (ret == NET_ERR_CLIENT_CLOSED) {
			LT_I(
				"Client: connection closed with %s:%d",
				inet_ntoa(net->endpoint.addr.sin_addr),
				ntohs(net->endpoint.addr.sin_port)
			);
			goto cleanup;
		}
		if (ret != NET_ERR_OK_PACKET)
			// continue if packet is not fully received yet
			continue;
	}

error_start:
	LT_E("Couldn't start the game client");
	event.user.code = false;
	SDL_PushEvent(&event);

cleanup:
	// stop the client
	net_endpoint_close(&client->endpoint);
	// free the server's structure
	free(client);
	client = NULL;
#if WIN32
	WSACleanup();
#endif
	return 0;
}
