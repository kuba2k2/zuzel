// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "game.h"

void game_print_error(game_err_t error) {
	switch (error) {
		case GAME_ERR_OK:
			LT_E("Received error packet with unspecified reason");
			break;
		case GAME_ERR_INVALID_STATE:
			LT_E("Operation invalid in the current game state");
			break;
		case GAME_ERR_NOT_FOUND:
			LT_E("Game not found by the specified key");
			break;
		case GAME_ERR_NO_PLAYER:
			LT_E("Player not found by the specified ID");
			break;
		case GAME_ERR_SERVER_ERROR:
			LT_E("Internal server error");
			break;
	}
}

bool game_send_error(net_endpoint_t *endpoint, game_err_t error) {
	pkt_error_t pkt = {
		.hdr.type = PKT_ERROR,
		.error	  = error,
	};
	net_pkt_send(endpoint, (pkt_t *)&pkt);
	return false;
}

bool send_err_invalid_state(game_t *game, pkt_t *recv_pkt, net_endpoint_t *source) {
	if (!game->is_server)
		// do not send from client
		return false;
	pkt_error_t pkt = {
		.hdr.type = PKT_ERROR,
		.error	  = GAME_ERR_INVALID_STATE,
	};
	net_pkt_send(source, (pkt_t *)&pkt);
	return false;
}
