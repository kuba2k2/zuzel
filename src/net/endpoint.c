// Copyright (c) Kuba Szczodrzyński 2024-12-27.

#include "include.h"

net_err_t net_endpoint_listen(net_endpoint_t *endpoint) {
	net_err_t ret;
#if WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 0), &wsa_data);
#endif

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

	return NET_ERR_OK;

cleanup:
	net_endpoint_close(client);
	return ret;
}

void net_endpoint_close(net_endpoint_t *endpoint) {
	closesocket(endpoint->fd);
#if WIN32
	WSACleanup();
#endif
}
