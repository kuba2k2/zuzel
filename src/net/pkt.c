// Copyright (c) Kuba Szczodrzyński 2024-12-24.

#include "net.h"

static const int pkt_len_list[] = {
	0,
	sizeof(pkt_ping_t),
	sizeof(pkt_success_t),
	sizeof(pkt_error_t),
	sizeof(pkt_game_list_t),
	sizeof(pkt_game_new_t),
	sizeof(pkt_game_join_t),
	sizeof(pkt_game_data_t),
	sizeof(pkt_game_state_t),
	sizeof(pkt_player_data_t),
	sizeof(pkt_player_state_t),
	sizeof(pkt_player_keypress_t),
	0,
	sizeof(pkt_player_leave_t),
	sizeof(pkt_send_update_t),
};

static const char *pkt_name_list[] = {
	"",
	"PKT_PING",
	"PKT_SUCCESS",
	"PKT_ERROR",
	"PKT_GAME_LIST",
	"PKT_GAME_NEW",
	"PKT_GAME_JOIN",
	"PKT_GAME_DATA",
	"PKT_GAME_STATE",
	"PKT_PLAYER_DATA",
	"PKT_PLAYER_STATE",
	"PKT_PLAYER_KEYPRESS",
	"PKT_PLAYER_UPDATE",
	"PKT_PLAYER_LEAVE",
	"PKT_SEND_UPDATE",
};

/**
 * Receive a single pkt_t from the socket.
 *
 * @param endpoint where to receive the packet from
 * @return net_err_t
 */
net_err_t net_pkt_recv(net_endpoint_t *endpoint) {
	BUILD_BUG_ON(sizeof(pkt_len_list) != sizeof(*pkt_len_list) * PKT_MAX);
	BUILD_BUG_ON(sizeof(pkt_name_list) != sizeof(*pkt_name_list) * PKT_MAX);

	pkt_t *pkt = &endpoint->recv.pkt;
	if (endpoint->recv.buf == NULL) {
		endpoint->recv.buf	 = (char *)&endpoint->recv.pkt;
		endpoint->recv.start = endpoint->recv.buf;
		endpoint->recv.end	 = endpoint->recv.buf + sizeof(endpoint->recv.pkt);
	}

	// calculate total received length
	unsigned int total_len = endpoint->recv.buf - endpoint->recv.start;
	// calculate bytes remaining to receive
	unsigned int recv_len;
	if (total_len < sizeof(pkt->hdr))
		// finish receiving the header first
		recv_len = sizeof(pkt->hdr) - total_len;
	else
		// receive until the end of the packet
		recv_len = pkt->hdr.len - total_len;

	net_err_t err;
	if ((err = net_endpoint_recv(endpoint, endpoint->recv.buf, &recv_len)) != NET_ERR_OK)
		return err;

	// recv successful
	endpoint->recv.buf += recv_len;
	total_len += recv_len;
	LT_V("Data received (%d bytes - %u total)", recv_len, total_len);

	// return if packet header not received yet
	if (total_len < sizeof(pkt_hdr_t))
		return NET_ERR_OK;

	// check if the protocol version is correct
	if (pkt->hdr.protocol != NET_PROTOCOL) {
		LT_E("Packet protocol invalid (%d != %d)", pkt->hdr.protocol, NET_PROTOCOL);
		endpoint->recv.buf = endpoint->recv.start;
		return NET_ERR_PKT_PROTOCOL;
	}
	// check if the type is valid
	if (pkt->hdr.type < PKT_PING || pkt->hdr.type >= PKT_MAX) {
		LT_E("Packet type invalid (%d)", pkt->hdr.type);
		endpoint->recv.buf = endpoint->recv.start;
		return NET_ERR_PKT_TYPE;
	}
	// check if the length matches
	if (pkt->hdr.len != pkt_len_list[pkt->hdr.type]) {
		LT_E("Packet length invalid (%d != %d)", pkt->hdr.len, pkt_len_list[pkt->hdr.type]);
		endpoint->recv.buf = endpoint->recv.start;
		return NET_ERR_PKT_LENGTH;
	}

	// return if the packet is not completely received yet
	if (total_len < pkt->hdr.len)
		return NET_ERR_OK;

	LT_D("Packet %s received (%d bytes) <- %s", pkt_name_list[pkt->hdr.type], pkt->hdr.len, net_endpoint_str(endpoint));
	hexdump((uint8_t *)pkt + sizeof(pkt_hdr_t), pkt->hdr.len - sizeof(pkt_hdr_t));

	// indicate that a complete packet is available; also reset the write buffer
	endpoint->recv.buf = endpoint->recv.start;
	return NET_ERR_OK_PACKET;
}

/**
 * Send a single pkt_t on the socket.
 *
 * @param endpoint where to send the packet to
 * @param pkt packet to send
 * @return net_err_t
 */
net_err_t net_pkt_send(net_endpoint_t *endpoint, pkt_t *pkt) {
	pkt->hdr.protocol = NET_PROTOCOL;
	pkt->hdr.len	  = pkt_len_list[pkt->hdr.type];

	net_err_t err;
	if ((err = net_endpoint_send(endpoint, (const char *)pkt, pkt->hdr.len)) != NET_ERR_OK)
		return err;

	LT_D("Packet %s sent (%d bytes) -> %s", pkt_name_list[pkt->hdr.type], pkt->hdr.len, net_endpoint_str(endpoint));
	hexdump((uint8_t *)pkt + sizeof(pkt_hdr_t), pkt->hdr.len - sizeof(pkt_hdr_t));

	return NET_ERR_OK;
}

/**
 * Send a single pkt_t to all endpoints.
 *
 * @param endpoints where to send the packet to (DL list)
 * @param pkt packet to send
 * @param source endpoint to skip during sending
 * @return net_err_t
 */
net_err_t net_pkt_broadcast(net_endpoint_t *endpoints, pkt_t *pkt, net_endpoint_t *source) {
	net_err_t ret = NET_ERR_OK;
	net_endpoint_t *endpoint;
	DL_FOREACH(endpoints, endpoint) {
		if (endpoint == source)
			continue;
		ret = net_pkt_send(endpoint, pkt);
		if (ret != NET_ERR_OK)
			return ret;
	}
	return ret;
}
