// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#include "fragment.h"

static void on_error(ui_t *ui);

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	void *result = NULL;
	char message[128];

	lt_log_clear_errors();

	// start the network stack and update UI message
	if (ui->connection.type == UI_CONNECT_NEW_LOCAL) {
		strcpy(message, "Connecting to local game server...");
		result = net_server_start(false);
	} else if (ui->connection.type == UI_CONNECT_JOIN_ADDRESS) {
		sprintf(message, "Connecting to %s...", ui->connection.address);
		result = ui->client = net_client_start(ui->connection.address, ui->connection.use_tls);
	} else {
		strcpy(message, "Connecting to public game server...");
		result = ui->client = net_client_start(ui->connection.address, ui->connection.use_tls);
	}

	if (result == NULL) {
		on_error(ui);
		return false;
	}

	gfx_view_set_text(gfx_view_find_by_id(fragment->views, "message"), message);
	return true;
}

static void on_connected(ui_t *ui) {
	if (ui->client == NULL)
		return;

	net_err_t send_err;
	switch (ui->connection.type) {
		case UI_CONNECT_NEW_PUBLIC:
		case UI_CONNECT_NEW_PRIVATE:
		case UI_CONNECT_NEW_LOCAL: {
			pkt_game_new_t pkt = {
				.hdr.type = PKT_GAME_NEW,
			};
			send_err = net_pkt_send(ui->client, (pkt_t *)&pkt);
			break;
		}

		case UI_CONNECT_JOIN_BROWSE:
			ui_state_next(ui);
			return;

		case UI_CONNECT_JOIN_KEY: {
			pkt_game_join_t pkt = {
				.hdr.type = PKT_GAME_JOIN,
			};
			strncpy(pkt.key, ui->connection.key, GAME_KEY_LEN);
			send_err = net_pkt_send(ui->client, (pkt_t *)&pkt);
			break;
		}

		case UI_CONNECT_JOIN_ADDRESS: {
			pkt_game_join_t pkt = {
				.hdr.type = PKT_GAME_JOIN,
				.key	  = {0},
			};
			send_err = net_pkt_send(ui->client, (pkt_t *)&pkt);
			break;
		}
	}

	if (send_err != NET_ERR_OK)
		on_error(ui);
}

static void on_packet(ui_t *ui, pkt_t *pkt) {
	if (ui->client == NULL)
		return;

	// only allow one of two possible responses:
	// - PKT_GAME_DATA
	// - PKT_ERROR
	switch (pkt->hdr.type) {
		case PKT_GAME_DATA:
			FREE_NULL(ui->connection.game_data);
			ui->connection.game_data = net_pkt_dup(pkt);
			ui_state_next(ui);
			return;

		case PKT_ERROR:
			switch (pkt->error.error) {
				case GAME_ERR_OK:
					LT_E("Received error packet with unspecified reason");
					goto error;
				case GAME_ERR_INVALID_STATE:
					LT_E("Operation invalid in the current game state");
					goto error;
				case GAME_ERR_NOT_FOUND:
					LT_E("Game not found by the specified key");
					goto error;
			}
			goto error;

		default:
			LT_E("Unsupported packet received from the server");
			goto error;
	}

error:
	on_error(ui);
}

static void on_error(ui_t *ui) {
	ui->client = NULL;
	net_client_stop();
	net_server_stop();
	ui_state_error(ui);
}

static bool on_btn_cancel(view_t *view, SDL_Event *e, ui_t *ui) {
	ui->client = NULL;
	net_client_stop();
	net_server_stop();
	ui_state_prev(ui);
	return true;
}

static bool on_event(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	switch (e->type) {
		case SDL_USEREVENT_SERVER:
			if (e->user.code == false) {
				on_error(ui);
				return true;
			}
			ui->client = net_client_start(ui->connection.address, ui->connection.use_tls);
			if (ui->client == NULL)
				on_error(ui);
			return true;

		case SDL_USEREVENT_CLIENT:
			if (e->user.code == false)
				on_error(ui);
			else
				on_connected(ui);
			return true;

		case SDL_USEREVENT_PACKET:
			on_packet(ui, e->user.data1);
			return true;
	}
	return false;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_cancel),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_connecting = {
	.on_show		  = on_show,
	.on_event		  = on_event,
	.inflate_on_event = inflate_on_event,
};
