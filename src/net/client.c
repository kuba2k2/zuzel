// Copyright (c) Kuba Szczodrzyński 2025-1-3.

#include "net.h"

static int net_client_connect(char *address);
static net_err_t net_client_select_read_cb(net_endpoint_t *endpoint, net_t *net);
static void net_client_select_err_cb(net_endpoint_t *endpoint, net_t *net, net_err_t err);

static net_t *client = NULL;

net_endpoint_t *net_client_start(const char *address, bool use_tls) {
	if (client != NULL)
		return &client->endpoint;
	if (address == NULL)
		return NULL;
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
		net_endpoint_free(pipe);
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
	if (client->stop == true)
		return;
	client->stop = true;
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

	// build the destination server address
	struct sockaddr_in saddr = {
		.sin_family = AF_INET,
		.sin_port	= htons(port),
	};
	// resolve the host name
	if (!net_resolve_ip(address, &saddr.sin_addr))
		goto error_start;

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
		net_err_t err = net_endpoint_select(
			&client->endpoint,
			NULL,
			(net_select_read_cb_t)net_client_select_read_cb,
			(net_select_err_cb_t)net_client_select_err_cb,
			client
		);
		if (err != NET_ERR_OK)
			goto cleanup;
	}

cleanup:
	// mark this client as 'stopping'
	client->stop = true;
	net_t *net	 = client;
	client		 = NULL;
	// close and free the pipe
	SDL_WITH_MUTEX(net->endpoint.mutex) {
		net_endpoint_free(net->endpoint.next);
		SDL_DestroyMutex(net->endpoint.next->mutex);
		free(net->endpoint.next);
	}
	if (net->game == NULL) {
		// no game joined - close the pipe
		net_endpoint_free(&net->endpoint);
		SDL_DestroyMutex(net->endpoint.mutex);
		// send a 'closed' event to UI
		event.user.code = false;
		SDL_PushEvent(&event);
	} else {
		// game was joined - pass endpoint to game thread
		game_add_endpoint(net->game, &net->endpoint);
		// send a 'game' event to UI
		event.user.type	 = SDL_USEREVENT_GAME;
		event.user.data1 = net->game;
		SDL_PushEvent(&event);
	}
	// free the client's structure
	free(net);
	free(address);
#if WIN32
	WSACleanup();
#endif
	LT_I("Client: thread stopped");
	return 0;

error_start:
	LT_E("Couldn't start the game client");
	goto cleanup;
}

static net_err_t net_client_select_read_cb(net_endpoint_t *endpoint, net_t *net) {
	net_err_t ret = net_pkt_recv(endpoint);
	if (ret < NET_ERR_OK)
		return ret;
	if (ret != NET_ERR_OK_PACKET)
		// continue if packet is not fully received yet
		return NET_ERR_OK;
	pkt_t *pkt = &endpoint->recv.pkt;

	if (pkt->hdr.type == PKT_GAME_DATA && !pkt->game_data.is_list) {
		// game joined - hand over to game thread
		net->game = game_init((pkt_game_data_t *)pkt);
		if (net->game == NULL)
			return NET_ERR_CLIENT_CLOSED;
		LT_I("Client: joined game %s", net->game->key);
		// request the client to stop
		net->stop = true;
		return NET_ERR_OK;
	}

	return net_pkt_broadcast(&net->endpoint, pkt, endpoint);
}

static void net_client_select_err_cb(net_endpoint_t *endpoint, net_t *net, net_err_t err) {
	if (err == NET_ERR_CLIENT_CLOSED && net->stop)
		LT_I("Client: connection closed from %s", net_endpoint_str(endpoint));
	else if (err == NET_ERR_CLIENT_CLOSED)
		LT_E("Client: disconnected from %s", net_endpoint_str(endpoint));
	else
		LT_E("Client: connection error from %s", net_endpoint_str(endpoint));
	net->stop = true;
}
