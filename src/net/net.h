// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#pragma once

#include "include.h"

#include "packet.h"

#define NET_PROTOCOL 1

typedef enum {
	NET_ERR_MIN = -100,
	NET_ERR_SERVER_CLOSED, //!< Server connection was closed
	NET_ERR_CLIENT_CLOSED, //!< Client connection was closed
	NET_ERR_SOCKET,		   //!< socket() failed
	NET_ERR_SETSOCKOPT,	   //!< setsockopt() failed
	NET_ERR_BIND,		   //!< bind() failed
	NET_ERR_LISTEN,		   //!< listen() failed
	NET_ERR_ACCEPT,		   //!< accept() failed
	NET_ERR_RECV,		   //!< recv() failed
	NET_ERR_SEND,		   //!< send() failed
	NET_ERR_SSL_CTX,	   //!< SSL_CTX_new() failed
	NET_ERR_SSL_CERT,	   //!< SSL_CTX_use_*_file() failed
	NET_ERR_SSL,		   //!< SSL_new() failed
	NET_ERR_SSL_ACCEPT,	   //!< SSL_accept() failed
	NET_ERR_PKT_PROTOCOL,  //!< Packet protocol invalid
	NET_ERR_PKT_TYPE,	   //!< Packet type invalid
	NET_ERR_PKT_LENGTH,	   //!< Packet length invalid
	NET_ERR_OK		  = 0, //!< No error
	NET_ERR_OK_PACKET = 1, //!< No error, packet is available
} net_err_t;

typedef struct net_endpoint_t {
	struct sockaddr_in addr; //!< Endpoint address
	int fd;					 //!< Socket descriptor

	bool use_ssl;	  //!< Whether to use SSL for this connection
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
} net_t;

// pkt.c
net_err_t net_pkt_recv(net_endpoint_t *endpoint);
net_err_t net_pkt_send(net_endpoint_t *endpoint, pkt_t *pkt);

// endpoint.c
net_err_t net_endpoint_listen(net_endpoint_t *endpoint);
net_err_t net_endpoint_accept(const net_endpoint_t *endpoint, net_endpoint_t *client);
void net_endpoint_close(net_endpoint_t *endpoint);
net_err_t net_endpoint_recv(net_endpoint_t *endpoint, char *buf, unsigned int *len);
net_err_t net_endpoint_send(net_endpoint_t *endpoint, const char *buf, unsigned int len);

// server.c
net_t *net_server_start();
void net_server_stop();
