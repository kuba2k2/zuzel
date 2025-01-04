// Copyright (c) Kuba Szczodrzyński 2024-12-27.

#include "include.h"

net_endpoint_t *net_endpoint_dup(net_endpoint_t *endpoint) {
	// allocate memory
	net_endpoint_t *item;
	MALLOC(item, sizeof(*item), return NULL);
	*item = *endpoint;
	// adjust the recv buffer pointers
	if (item->recv.buf != NULL) {
		int buf_pos		 = endpoint->recv.buf - endpoint->recv.start;
		item->recv.start = (char *)&item->recv.pkt;
		item->recv.end	 = item->recv.start + sizeof(item->recv.pkt);
		item->recv.buf	 = item->recv.start + buf_pos;
	}
	return item;
}

net_err_t net_endpoint_pipe(net_endpoint_t *endpoint) {
	if (pipe(endpoint->pipe.fd) != 0)
		LT_ERR(F, return NET_ERR_PIPE, "Couldn't create a pipe");
	endpoint->type = NET_ENDPOINT_PIPE;
#if WIN32
	endpoint->pipe.event = WSACreateEvent();
#endif
	return NET_ERR_OK;
}

net_err_t net_endpoint_listen(net_endpoint_t *endpoint) {
	if (endpoint->type > NET_ENDPOINT_TLS)
		return NET_ERR_ENDPOINT_TYPE;
	net_err_t ret;
#if WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	if (endpoint->type == NET_ENDPOINT_TLS) {
		SSL_load_error_strings();
		SSL_library_init();
		if ((endpoint->ssl_ctx = SSL_CTX_new(TLS_server_method())) == NULL)
			SSL_ERROR("SSL_CTX_new()", ret = NET_ERR_SSL_CTX; goto cleanup);
	}

	int sfd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sfd == INVALID_SOCKET)
		SOCK_ERROR("socket()", ret = NET_ERR_SOCKET; goto cleanup);

	// enable address reuse
	char on = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0)
		SOCK_ERROR("setsockopt()", ret = NET_ERR_SETSOCKOPT; goto cleanup);

	// bind the server socket to an address
	if (bind(sfd, (struct sockaddr *)&endpoint->addr, sizeof(endpoint->addr)) != 0)
		SOCK_ERROR("bind()", ret = NET_ERR_BIND; goto cleanup);

	// listen for incoming connections
	if (listen(sfd, 10) != 0)
		SOCK_ERROR("listen()", ret = NET_ERR_LISTEN; goto cleanup);

	// server started successfully, fill net_endpoint_t*
	endpoint->fd = sfd;
#if WIN32
	endpoint->pipe.event = WSACreateEvent();
#endif

	return NET_ERR_OK;

cleanup:
	net_endpoint_free(endpoint);
	return ret;
}

net_err_t net_endpoint_accept(const net_endpoint_t *endpoint, net_endpoint_t *client) {
	if (endpoint->type > NET_ENDPOINT_TLS)
		return NET_ERR_ENDPOINT_TYPE;
	net_err_t ret;
#if WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	int addrlen = sizeof(client->addr);

	if ((client->fd = (int)accept(endpoint->fd, (struct sockaddr *)&client->addr, &addrlen)) < 0) {
		ret = NET_ERR_ACCEPT;
#if WIN32
		if (WSAGetLastError() == WSAECONNRESET)
			ret = NET_ERR_CLIENT_CLOSED;
		else if (WSAGetLastError() == WSAEINTR)
			ret = NET_ERR_SERVER_CLOSED;
#else
		if (errno == ECONNABORTED)
			ret = NET_ERR_CONN_CLOSED;
#endif
		if (ret == NET_ERR_ACCEPT)
			SOCK_ERROR("accept()", );
		goto cleanup;
	}

	client->type = NET_ENDPOINT_TCP;
	if (endpoint->type == NET_ENDPOINT_TLS) {
		client->type = NET_ENDPOINT_TLS;
		if ((client->ssl = SSL_new(endpoint->ssl_ctx)) == NULL)
			SSL_ERROR("SSL_new()", ret = NET_ERR_SSL; goto cleanup);
		if (SSL_set_fd(client->ssl, client->fd) != 1)
			SSL_ERROR("SSL_set_fd()", ret = NET_ERR_SSL; goto cleanup);
		if (SSL_accept(client->ssl) != 1)
			SSL_ERROR("SSL_accept()", ret = NET_ERR_SSL_ACCEPT; goto cleanup);
	}

#if WIN32
	client->pipe.event = WSACreateEvent();
#endif

	return NET_ERR_OK;

cleanup:
	net_endpoint_free(client);
	return ret;
}

net_err_t net_endpoint_connect(net_endpoint_t *endpoint) {
	if (endpoint->type > NET_ENDPOINT_TLS)
		return NET_ERR_ENDPOINT_TYPE;
	net_err_t ret;
#if WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	if (endpoint->type == NET_ENDPOINT_TLS) {
		SSL_load_error_strings();
		SSL_library_init();
		if ((endpoint->ssl_ctx = SSL_CTX_new(TLS_server_method())) == NULL)
			SSL_ERROR("SSL_CTX_new()", ret = NET_ERR_SSL_CTX; goto cleanup);
	}

	int cfd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (cfd == INVALID_SOCKET)
		SOCK_ERROR("socket()", ret = NET_ERR_SOCKET; goto cleanup);

	// fill net_endpoint_t* first, so that disconnection is possible
	endpoint->fd = cfd;
#if WIN32
	endpoint->pipe.event = WSACreateEvent();
#endif

	// connect to the server
	if (connect(cfd, (struct sockaddr *)&endpoint->addr, sizeof(endpoint->addr)) != 0)
		SOCK_ERROR("connect()", ret = NET_ERR_CONNECT; goto cleanup);

	// perform a TLS handshake if requested
	if (endpoint->type == NET_ENDPOINT_TLS) {
		if ((endpoint->ssl = SSL_new(endpoint->ssl_ctx)) == NULL)
			SSL_ERROR("SSL_new()", ret = NET_ERR_SSL; goto cleanup);
		if (SSL_set_fd(endpoint->ssl, endpoint->fd) != 1)
			SSL_ERROR("SSL_set_fd()", ret = NET_ERR_SSL; goto cleanup);
		if (SSL_connect(endpoint->ssl) != 1)
			SSL_ERROR("SSL_accept()", ret = NET_ERR_SSL_CONNECT; goto cleanup);
	}

	return NET_ERR_OK;

cleanup:
	net_endpoint_free(endpoint);
	return ret;
}

void net_endpoint_close(net_endpoint_t *endpoint) {
	if (endpoint->ssl != NULL)
		SSL_shutdown(endpoint->ssl);

	if (endpoint->fd > 0) {
		closesocket(endpoint->fd);
		endpoint->fd = 0;
	}

	if (endpoint->pipe.fd[PIPE_READ] != 0) {
		close(endpoint->pipe.fd[PIPE_READ]);
		close(endpoint->pipe.fd[PIPE_WRITE]);
		endpoint->pipe.fd[PIPE_READ] = 0;
	}
}

void net_endpoint_free(net_endpoint_t *endpoint) {
	net_endpoint_close(endpoint);

	if (endpoint->ssl != NULL) {
		SSL_free(endpoint->ssl);
		endpoint->ssl = NULL;
	}

	if (endpoint->ssl_ctx != NULL) {
		SSL_CTX_free(endpoint->ssl_ctx);
		endpoint->ssl_ctx = NULL;
	}

#if WIN32
	if (endpoint->pipe.event != NULL) {
		WSACloseEvent(endpoint->pipe.event);
		endpoint->pipe.event = NULL;
	}
#endif

#if WIN32
	if (endpoint->type <= NET_ENDPOINT_TLS)
		WSACleanup();
#endif
}

net_err_t net_endpoint_recv(net_endpoint_t *endpoint, char *buf, unsigned int *len) {
	int recv_len;
	switch (endpoint->type) {
		case NET_ENDPOINT_TCP:
			recv_len = recv(endpoint->fd, buf, (int)*len, 0);
			break;
		case NET_ENDPOINT_TLS:
			recv_len = SSL_read(endpoint->ssl, buf, (int)*len);
			break;
		case NET_ENDPOINT_PIPE:
			recv_len = read(endpoint->pipe.fd[PIPE_READ], buf, (int)*len);
#if WIN32
			endpoint->pipe.len -= recv_len;
			if (endpoint->pipe.len)
				WSASetEvent(endpoint->pipe.event);
			else
				WSAResetEvent(endpoint->pipe.event);
#endif
			break;
		default:
			return NET_ERR_ENDPOINT_TYPE;
	}

	if (recv_len == 0)
		// connection closed
		return NET_ERR_CLIENT_CLOSED;
	if (recv_len == -1)
		// recv error
		SOCK_ERROR("recv()", return NET_ERR_RECV);

	*len = recv_len;
	return NET_ERR_OK;
}

net_err_t net_endpoint_send(net_endpoint_t *endpoint, const char *buf, unsigned int len) {
	int send_len;
	switch (endpoint->type) {
		case NET_ENDPOINT_TCP:
			send_len = send(endpoint->fd, buf, (int)len, 0);
			break;
		case NET_ENDPOINT_TLS:
			send_len = SSL_write(endpoint->ssl, buf, (int)len);
			break;
		case NET_ENDPOINT_PIPE:
			send_len = write(endpoint->pipe.fd[PIPE_WRITE], buf, (int)len);
#if WIN32
			endpoint->pipe.len += send_len;
			WSASetEvent(endpoint->pipe.event);
#endif
			break;
		default:
			return NET_ERR_ENDPOINT_TYPE;
	}

	if (send_len == -1)
		// send error
		SOCK_ERROR("send()", return NET_ERR_SEND);
	if (send_len != len)
		// send length error
		SOCK_ERROR("send() length", return NET_ERR_SEND);

	return NET_ERR_OK;
}

#if WIN32
net_err_t net_endpoint_select(net_endpoint_t *endpoints, SDL_mutex *mutex, net_select_cb_t cb, void *param) {
	net_endpoint_t *endpoint_list[WSA_MAXIMUM_WAIT_EVENTS];
	void *event_list[WSA_MAXIMUM_WAIT_EVENTS];
	int num_events = 0;
	int mask	   = FD_READ | FD_CLOSE;

	SDL_WITH_MUTEX(mutex) {
		net_endpoint_t *endpoint;
		DL_FOREACH(endpoints, endpoint) {
			if (endpoint->type > NET_ENDPOINT_PIPE)
				continue;
			endpoint_list[num_events] = endpoint;
			event_list[num_events]	  = endpoint->pipe.event;
			if (endpoint->fd != 0)
				// call WSAEventSelect() on sockets only (not on pipes)
				WSAEventSelect(endpoint->fd, endpoint->pipe.event, mask);
			num_events++;
		}
	}

	unsigned int ret = WSAWaitForMultipleEvents(num_events, event_list, false, 5000, true);
	if (ret == WSA_WAIT_FAILED)
		SOCK_ERROR("WSAWaitForMultipleEvents()", return NET_ERR_SELECT);
	if (ret == WSA_WAIT_IO_COMPLETION)
		return NET_ERR_OK;
	if (ret == WSA_WAIT_TIMEOUT)
		return NET_ERR_OK;
	if (cb == NULL)
		return NET_ERR_OK;

	unsigned int index		 = ret - WSA_WAIT_EVENT_0;
	net_endpoint_t *endpoint = endpoint_list[index];

	WSANETWORKEVENTS network_events;
	WSAEnumNetworkEvents(endpoint->fd, event_list[index], &network_events);

	if (network_events.lNetworkEvents & mask)
		return cb(endpoint, param);

	return NET_ERR_OK;
}
#endif
