// Copyright (c) Kuba Szczodrzyński 2024-12-27.

#include "include.h"

net_err_t net_endpoint_listen(net_endpoint_t *endpoint) {
	net_err_t ret;
#if WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

	if (endpoint->use_ssl) {
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

	return NET_ERR_OK;

cleanup:
	net_endpoint_close(endpoint);
	return ret;
}

net_err_t net_endpoint_accept(const net_endpoint_t *endpoint, net_endpoint_t *client) {
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

	if (endpoint->use_ssl) {
		client->use_ssl = true;
		if ((client->ssl = SSL_new(endpoint->ssl_ctx)) == NULL)
			SSL_ERROR("SSL_new()", ret = NET_ERR_SSL; goto cleanup);
		if (SSL_set_fd(client->ssl, client->fd) != 1)
			SSL_ERROR("SSL_set_fd()", ret = NET_ERR_SSL; goto cleanup);
		if (SSL_accept(client->ssl) != 1)
			SSL_ERROR("SSL_accept()", ret = NET_ERR_SSL_ACCEPT; goto cleanup);
	}

	return NET_ERR_OK;

cleanup:
	net_endpoint_close(client);
	return ret;
}

void net_endpoint_close(net_endpoint_t *endpoint) {
	if (endpoint->ssl != NULL) {
		SSL_shutdown(endpoint->ssl);
		SSL_free(endpoint->ssl);
		endpoint->ssl = NULL;
	}

	if (endpoint->ssl_ctx != NULL) {
		SSL_CTX_free(endpoint->ssl_ctx);
		endpoint->ssl_ctx = NULL;
	}

	if (endpoint->fd > 0) {
		closesocket(endpoint->fd);
		endpoint->fd = 0;
	}

#if WIN32
	WSACleanup();
#endif
}

net_err_t net_endpoint_recv(net_endpoint_t *endpoint, char *buf, unsigned int *len) {
	int recv_len;
	if (!endpoint->use_ssl) {
		recv_len = recv(endpoint->fd, buf, (int)*len, 0);
	} else {
		recv_len = SSL_read(endpoint->ssl, buf, (int)*len);
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
	if (!endpoint->use_ssl) {
		send_len = send(endpoint->fd, buf, (int)len, 0);
	} else {
		send_len = SSL_write(endpoint->ssl, buf, (int)len);
	}

	if (send_len == -1)
		// send error
		SOCK_ERROR("send()", return NET_ERR_SEND);
	if (send_len != len)
		// send length error
		SOCK_ERROR("send() length", return NET_ERR_SEND);

	return NET_ERR_OK;
}
