// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#pragma once

#include "include.h"

#include "packet.h"

#define NET_PROTOCOL 1

typedef enum {
	NET_ERR_MIN = -100,
	NET_ERR_SERVER_CLOSED,	 //!< Server connection was closed
	NET_ERR_CLIENT_CLOSED,	 //!< Client connection was closed
	NET_ERR_SOCKET,			 //!< socket() failed
	NET_ERR_SETSOCKOPT,		 //!< setsockopt() failed
	NET_ERR_BIND,			 //!< bind() failed
	NET_ERR_CONNECT,		 //!< connect() failed
	NET_ERR_LISTEN,			 //!< listen() failed
	NET_ERR_ACCEPT,			 //!< accept() failed
	NET_ERR_RECV,			 //!< recv() failed
	NET_ERR_SEND,			 //!< send() failed
	NET_ERR_SELECT,			 //!< select() failed
	NET_ERR_SSL_CTX,		 //!< SSL_CTX_new() failed
	NET_ERR_SSL_CERT,		 //!< SSL_CTX_use_*_file() failed
	NET_ERR_SSL,			 //!< SSL_new() failed
	NET_ERR_SSL_ACCEPT,		 //!< SSL_accept() failed
	NET_ERR_SSL_CONNECT,	 //!< SSL_connect() failed
	NET_ERR_PKT_PROTOCOL,	 //!< Packet protocol invalid
	NET_ERR_PKT_TYPE,		 //!< Packet type invalid
	NET_ERR_PKT_LENGTH,		 //!< Packet length invalid
	NET_ERR_MALLOC,			 //!< Memory allocation failed
	NET_ERR_PIPE,			 //!< Pipe creation failed
	NET_ERR_ENDPOINT_TYPE,	 //!< Endpoint type invalid
	NET_ERR_ENDPOINT_CLOSED, //!< Endpoint already closed
	NET_ERR_OK		  = 0,	 //!< No error
	NET_ERR_OK_PACKET = 1,	 //!< No error, packet is available
} net_err_t;

typedef enum {
	NET_ENDPOINT_TCP,
	NET_ENDPOINT_TLS,
	NET_ENDPOINT_PIPE,
} net_endpoint_type_t;

typedef struct net_endpoint_t {
	SDL_mutex *mutex;		  //!< Mutex locking this endpoint
	net_endpoint_type_t type; //!< Endpoint type

	struct sockaddr_in addr; //!< Endpoint address
	int fd;					 //!< Socket descriptor

	struct {
		int fd[2];	 //!< Pipe descriptor
		long len;	 //!< Pipe sent data length
		void *event; //!< Socket event (Windows only)
		bool no_sdl; //!< Whether net_pkt_send() should avoid sending SDL events here
	} pipe;

	SSL_CTX *ssl_ctx; //!< OpenSSL context (optional)
	SSL *ssl;		  //!< OpenSSL socket (optional)

	struct {
		pkt_t pkt;	 //!< Received packet
		char *start; //!< Receive buffer start (&pkt)
		char *buf;	 //!< Receive buffer write pointer
		char *end;	 //!< Receive buffer end (&pkt + sizeof(pkt_t))
	} recv;

	struct net_endpoint_t *prev, *next;
} net_endpoint_t;

typedef struct net_t {
	net_endpoint_t endpoint; //!< Socket endpoint of the other party
	bool stop;				 //!< Whether the thread should stop gracefully
	game_t *game;			 //!< Joined game (client only)
} net_t;

typedef net_err_t (*net_select_read_cb_t)(net_endpoint_t *endpoint, void *param);
typedef void (*net_select_err_cb_t)(net_endpoint_t *endpoint, void *param, net_err_t err);

// pkt.c
pkt_t *net_pkt_dup(pkt_t *pkt);
net_err_t net_pkt_recv(net_endpoint_t *endpoint);
net_err_t net_pkt_send(net_endpoint_t *endpoint, pkt_t *pkt);
net_err_t net_pkt_send_pipe(net_endpoint_t *endpoint, pkt_t *pkt);
net_err_t net_pkt_broadcast(net_endpoint_t *endpoints, pkt_t *pkt, net_endpoint_t *source);

// endpoint.c
const char *net_endpoint_str(net_endpoint_t *endpoint);
net_endpoint_t *net_endpoint_dup(net_endpoint_t *endpoint);
net_err_t net_endpoint_pipe(net_endpoint_t *endpoint);
net_err_t net_endpoint_listen(net_endpoint_t *endpoint);
net_err_t net_endpoint_accept(const net_endpoint_t *endpoint, net_endpoint_t *client);
net_err_t net_endpoint_connect(net_endpoint_t *endpoint);
void net_endpoint_close(net_endpoint_t *endpoint);
void net_endpoint_free(net_endpoint_t *endpoint);
net_err_t net_endpoint_recv(net_endpoint_t *endpoint, char *buf, unsigned int *len);
net_err_t net_endpoint_send(net_endpoint_t *endpoint, const char *buf, unsigned int len);
net_err_t net_endpoint_select(
	net_endpoint_t *endpoints,
	SDL_mutex *mutex,
	net_select_read_cb_t read_cb,
	net_select_err_cb_t err_cb,
	void *param
);

// errors.c
bool net_error_print();

// server.c
net_t *net_server_start(bool headless);
void net_server_stop();

// client.c
net_endpoint_t *net_client_start(const char *address, bool use_tls);
void net_client_stop();
