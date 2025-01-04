// Copyright (c) Kuba Szczodrzyński 2025-1-3.

#include "net.h"

static int net_client_connect(char *address);
static net_err_t net_client_select_cb(net_endpoint_t *endpoint, net_t *net);

static net_t *client = NULL;

net_endpoint_t *net_client_start(const char *address, bool use_tls) {
	if (client != NULL)
		return &client->endpoint;
	MALLOC(client, sizeof(*client), return NULL);

	// set the connection protocol
	client->endpoint.type = use_tls ? NET_ENDPOINT_TLS : NET_ENDPOINT_TCP;

	// create a pipe for UI communication
	net_endpoint_t *pipe;
	MALLOC(pipe, sizeof(*pipe), return NULL);
	net_endpoint_pipe(pipe);
	// append to doubly-linked list for net_endpoint_select()
	pipe->prev			  = &client->endpoint;
	client->endpoint.next = pipe;
	client->endpoint.prev = pipe;

	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)net_client_connect, "client", strdup(address));
	SDL_DetachThread(thread);
	if (thread == NULL) {
		SDL_ERROR("SDL_CreateThread()", );
		free(pipe);
		free(client);
		client = NULL;
		return NULL;
	}

	return &client->endpoint;
}

void net_client_stop() {
	if (client == NULL)
		return;
	client->stop = true;
	net_endpoint_close(client->endpoint.next);
	net_endpoint_close(&client->endpoint);
}

static int net_client_connect(char *address) {
	if (client == NULL)
		return -1;
	lt_log_set_thread_name("client");

#if WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	SDL_Event event = {
		.user.type = SDL_USEREVENT_CLIENT,
	};

	// check port number if specified
	int port	   = SETTINGS->server_port;
	char *port_str = strchr(address, ':');
	if (port_str != NULL) {
		*port_str = '\0';
		port	  = atoi(port_str + 1);
	}

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
		.sin_port	= htons(port),
		.sin_addr	= ((struct sockaddr_in *)info->ai_addr)->sin_addr,
	};
	freeaddrinfo(info);

	// start the TCP client
	client->endpoint.addr = saddr;
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

	while (!client->stop) {
		// wait for incoming data
		if (net_endpoint_select(&client->endpoint, NULL, (net_select_cb_t)net_client_select_cb, client) != NET_ERR_OK)
			goto cleanup;
	}

error_start:
	LT_E("Couldn't start the game client");

cleanup:
	// send a 'closed' event
	event.user.code = false;
	SDL_PushEvent(&event);
	// close and free the pipe
	net_endpoint_free(client->endpoint.next);
	free(client->endpoint.next);
	// stop the client
	net_endpoint_free(&client->endpoint);
	// free the server's structure
	free(client);
	client = NULL;
#if WIN32
	WSACleanup();
#endif
	LT_I("Client: thread stopped");
	return 0;
}

static net_err_t net_client_select_cb(net_endpoint_t *endpoint, net_t *net) {
	net_err_t ret = net_pkt_recv(endpoint);
	if (ret == NET_ERR_RECV)
		return ret;
	if (ret == NET_ERR_CLIENT_CLOSED) {
		LT_E(
			"Client: connection closed with %s:%d",
			inet_ntoa(endpoint->addr.sin_addr),
			ntohs(endpoint->addr.sin_port)
		);
		return ret;
	}
	if (ret != NET_ERR_OK_PACKET)
		// continue if packet is not fully received yet
		return NET_ERR_OK;

	return net_pkt_broadcast(&net->endpoint, &net->endpoint.recv.pkt, endpoint);
}
