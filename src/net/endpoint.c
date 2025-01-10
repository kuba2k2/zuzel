// Copyright (c) Kuba Szczodrzyński 2024-12-27.

#include "include.h"

static char str_buf[128] = {0};

const char *net_endpoint_str(net_endpoint_t *endpoint) {
	if (endpoint == NULL)
		return "(null)";
	const char *type = "TLS";
	switch (endpoint->type) {
		case NET_ENDPOINT_TCP:
			type = "TCP";
		case NET_ENDPOINT_TLS:
			snprintf(
				str_buf,
				sizeof(str_buf),
				"%s(%s:%d)",
				type,
				inet_ntoa(endpoint->addr.sin_addr),
				ntohs(endpoint->addr.sin_port)
			);
			break;
		case NET_ENDPOINT_PIPE:
			snprintf(str_buf, sizeof(str_buf), "PIPE(fds=[%d, %d])", endpoint->pipe.fd[0], endpoint->pipe.fd[1]);
			break;
		default:
			return "(invalid)";
	}
	return str_buf;
}

net_endpoint_t *net_endpoint_dup(net_endpoint_t *endpoint) {
	// allocate memory
	net_endpoint_t *item;
	MALLOC(item, sizeof(*item), return NULL);
	*item		= *endpoint;
	item->mutex = NULL;
#if WIN32
	item->pipe.event = WSACreateEvent();
#endif
	// adjust the recv buffer pointers
	if (item->recv.buf != NULL) {
		uintptr_t buf_pos = endpoint->recv.buf - endpoint->recv.start;
		item->recv.start  = (char *)&item->recv.pkt;
		item->recv.end	  = item->recv.start + sizeof(item->recv.pkt);
		item->recv.buf	  = item->recv.start + buf_pos;
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
		if (SSL_CTX_use_certificate(endpoint->ssl_ctx, SETTINGS->tls_cert) != 1)
			SSL_ERROR("SSL_CTX_use_certificate()", ret = NET_ERR_SSL_CERT; goto cleanup);
		if (SSL_CTX_use_RSAPrivateKey(endpoint->ssl_ctx, SETTINGS->tls_key) != 1)
			SSL_ERROR("SSL_CTX_use_RSAPrivateKey()", ret = NET_ERR_SSL_CERT; goto cleanup);
	}

	int sfd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sfd == -1)
		SOCK_ERROR("socket()", ret = NET_ERR_SOCKET; goto cleanup);

	// enable address reuse
	int on = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on)) != 0)
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
	net_endpoint_close(endpoint);
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

	socklen_t addrlen = sizeof(client->addr);

	if ((client->fd = (int)accept(endpoint->fd, (struct sockaddr *)&client->addr, &addrlen)) < 0) {
		ret = NET_ERR_ACCEPT;
#if WIN32
		if (WSAGetLastError() == WSAECONNRESET)
			ret = NET_ERR_CLIENT_CLOSED;
		else if (WSAGetLastError() == WSAEINTR)
			ret = NET_ERR_SERVER_CLOSED;
#else
		if (errno == ECONNABORTED)
			ret = NET_ERR_CLIENT_CLOSED;
		if (errno == ECONNRESET)
			ret = NET_ERR_CLIENT_CLOSED;
		if (errno == EINTR)
			ret = NET_ERR_SERVER_CLOSED;
		if (errno == EBADF)
			ret = NET_ERR_SERVER_CLOSED;
		if (errno == EINVAL)
			ret = NET_ERR_SERVER_CLOSED;
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
	net_endpoint_close(client);
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
	if (cfd == -1)
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
	net_endpoint_close(endpoint);
	return ret;
}

void net_endpoint_close(net_endpoint_t *endpoint) {
	SDL_WITH_MUTEX(endpoint->mutex) {
		if (endpoint->ssl != NULL)
			SSL_shutdown(endpoint->ssl);

		if (endpoint->fd > 0) {
#if WIN32
			closesocket(endpoint->fd);
#else
			shutdown(endpoint->fd, SHUT_RDWR);
			close(endpoint->fd);
#endif
			endpoint->fd = 0;
		}

		if (endpoint->pipe.fd[PIPE_READ] != 0) {
			close(endpoint->pipe.fd[PIPE_READ]);
			close(endpoint->pipe.fd[PIPE_WRITE]);
			endpoint->pipe.fd[PIPE_READ] = 0;
		}

#if WIN32
		// signal the pipe's event so that WSAWaitForMultipleEvents() returns
		if (endpoint->pipe.event != NULL)
			WSASetEvent(endpoint->pipe.event);
#endif
	}
}

void net_endpoint_free(net_endpoint_t *endpoint) {
	net_endpoint_close(endpoint);

	SDL_WITH_MUTEX(endpoint->mutex) {
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
}

net_err_t net_endpoint_recv(net_endpoint_t *endpoint, char *buf, unsigned int *len) {
	long recv_len;
	switch (endpoint->type) {
		case NET_ENDPOINT_TCP:
			recv_len = recv(endpoint->fd, buf, (int)*len, 0);
			break;
		case NET_ENDPOINT_TLS:
			recv_len = SSL_read(endpoint->ssl, buf, (int)*len);
			break;
		case NET_ENDPOINT_PIPE:
			if (endpoint->pipe.len == 0)
				// nothing to receive - e.g. signaled by net_endpoint_close() on Windows
				goto empty;
			recv_len = read(endpoint->pipe.fd[PIPE_READ], buf, (int)*len);
			endpoint->pipe.len -= recv_len;
#if WIN32
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

	if (recv_len == -1) {
#if WIN32
		switch (WSAGetLastError()) {
			case WSAEWOULDBLOCK:
				goto empty;
			case WSAECONNABORTED:
			case WSAECONNRESET:
				return NET_ERR_CLIENT_CLOSED;
			default:
				break;
		}
#else
		switch (errno) {
			case EWOULDBLOCK:
				goto empty;
			case ECONNABORTED:
			case ECONNRESET:
				return NET_ERR_CLIENT_CLOSED;
			default:
				break;
		}
#endif
		// recv error
		SOCK_ERROR("recv()", return NET_ERR_RECV);
	}

	*len = recv_len;
	return NET_ERR_OK;

empty:
	*len = 0;
	return NET_ERR_OK;
}

net_err_t net_endpoint_send(net_endpoint_t *endpoint, const char *buf, unsigned int len) {
	long send_len;
	switch (endpoint->type) {
		case NET_ENDPOINT_TCP:
			send_len = send(endpoint->fd, buf, (int)len, 0);
			break;
		case NET_ENDPOINT_TLS:
			send_len = SSL_write(endpoint->ssl, buf, (int)len);
			break;
		case NET_ENDPOINT_PIPE:
			send_len = write(endpoint->pipe.fd[PIPE_WRITE], buf, (int)len);
			endpoint->pipe.len += send_len;
#if WIN32
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
#define SELECT_MAX_FDS WSA_MAXIMUM_WAIT_EVENTS
#else
#define SELECT_MAX_FDS 32
#endif

net_err_t net_endpoint_select(
	net_endpoint_t *endpoints,
	SDL_mutex *mutex,
	net_select_read_cb_t read_cb,
	net_select_err_cb_t error_cb,
	void *param
) {
	net_endpoint_t *endpoint_list[SELECT_MAX_FDS];
	int num_endpoints = 0;
#if WIN32
	void *event_list[SELECT_MAX_FDS];
#else
	struct pollfd pollfd_list[SELECT_MAX_FDS];
#endif

	SDL_WITH_MUTEX_OPTIONAL(mutex) {
		net_endpoint_t *endpoint;
		DL_FOREACH(endpoints, endpoint) {
			if (endpoint->type > NET_ENDPOINT_PIPE)
				continue;
			if (endpoint->type == NET_ENDPOINT_TLS && SSL_pending(endpoint->ssl)) {
				// immediately allow reading any previously-buffered TLS data
				net_err_t err;
				if (read_cb != NULL && (err = read_cb(endpoint, param)) != NET_ERR_OK && error_cb != NULL)
					error_cb(endpoint, param, err);
				SDL_UnlockMutex(mutex);
				return NET_ERR_OK;
			}
#if WIN32
			if (endpoint->type != NET_ENDPOINT_PIPE) {
				// call WSAEventSelect() on sockets only (not on pipes)
				WSAEventSelect(endpoint->fd, endpoint->pipe.event, FD_READ | FD_CLOSE);
			}
			event_list[num_endpoints] = endpoint->pipe.event;
#else
			// pass a descriptor to poll(), depending on the endpoint type
			pollfd_list[num_endpoints].fd =
				endpoint->type == NET_ENDPOINT_PIPE ? endpoint->pipe.fd[PIPE_READ] : endpoint->fd;
			pollfd_list[num_endpoints].events = POLLIN; // implicitly: POLLERR | POLLHUP | POLLNVAL
#endif
			endpoint_list[num_endpoints] = endpoint;
			num_endpoints++;
		}
	}

#if WIN32
	unsigned int ret = WSAWaitForMultipleEvents(num_endpoints, event_list, false, 5000, true);
	if (ret == WSA_WAIT_FAILED)
		SOCK_ERROR("WSAWaitForMultipleEvents()", return NET_ERR_SELECT);
	if (ret == WSA_WAIT_IO_COMPLETION)
		return NET_ERR_OK;
	if (ret == WSA_WAIT_TIMEOUT)
		return NET_ERR_OK;
#else
	int ret = poll(pollfd_list, num_endpoints, 5000);
	if (ret == 0) // timeout
		return NET_ERR_OK;
	if (ret == -1)
		SOCK_ERROR("select()", return NET_ERR_SELECT);
#endif

	if (read_cb == NULL)
		return NET_ERR_OK;

	// on Windows, only check the first available event
	// on Linux, check all endpoints for events
#if WIN32
	for (unsigned int index = ret - WSA_WAIT_EVENT_0, i = 0; i < 1; i++)
#else
	for (unsigned int index = 0; index < num_endpoints; index++)
#endif
	{
		net_endpoint_t *endpoint = endpoint_list[index];

#if WIN32
		if (endpoint->type == NET_ENDPOINT_PIPE) {
			// pipe event signaled
			net_err_t err;
			if ((err = read_cb(endpoint, param)) != NET_ERR_OK && error_cb != NULL)
				error_cb(endpoint, param, err);
			continue;
		}
#endif

#if WIN32
		WSANETWORKEVENTS network_events;
		if (WSAEnumNetworkEvents(endpoint->fd, event_list[index], &network_events) != 0) {
			net_err_t err;
			if (WSAGetLastError() == WSAENOTSOCK)
				err = NET_ERR_CLIENT_CLOSED;
			else
				SOCK_ERROR("WSAEnumNetworkEvents()", err = NET_ERR_SELECT);
			if (error_cb != NULL)
				error_cb(endpoint, param, err);
			continue;
		}
#else
		short events = pollfd_list[index].revents;
#endif

#if WIN32
		if (network_events.lNetworkEvents & FD_READ)
#else
		if (events & POLLIN)
#endif
		{
			LT_V("select()=READ, index=%u, endpoint=%s", index, net_endpoint_str(endpoint));
			net_err_t err;
			if ((err = read_cb(endpoint, param)) != NET_ERR_OK && error_cb != NULL)
				error_cb(endpoint, param, err);
			continue;
		}

#if WIN32
		if (network_events.lNetworkEvents & FD_CLOSE)
#else
		if (events & (POLLHUP | POLLERR | POLLNVAL))
#endif
		{
			LT_V("select()=CLOSE, index=%u, endpoint=%s", index, net_endpoint_str(endpoint));
			if (error_cb != NULL)
				error_cb(endpoint, param, NET_ERR_CLIENT_CLOSED);
		}
	}

	return NET_ERR_OK;
}
