// Copyright (c) Kuba Szczodrzyński 2025-1-4.

#include "fragment.h"

#define PER_PAGE 5

static view_t *list			= NULL; // UI-owned
static view_t *cloned_row	= NULL; // fragment-owned ("freed" in on_hide)
static view_t *btn_prev		= NULL;
static view_t *btn_next		= NULL;
static view_t *btn_join		= NULL;
static view_t *text_summary = NULL;
static int current_page		= 0;
static bool is_joining		= false;

static void on_error(ui_t *ui);

static void get_game_list(ui_t *ui, int page) {
	if (ui->client == NULL)
		return;

	// free the entire list box
	gfx_view_free(list->children);
	list->children = NULL;

	// reset the views
	btn_prev->is_disabled = true;
	btn_next->is_disabled = true;
	btn_join->is_disabled = true;
	gfx_view_set_text(text_summary, "Loading...");
	ui->force_layout = true;

	// send a request packet
	pkt_game_list_t pkt = {
		.hdr.type	 = PKT_GAME_LIST,
		.page		 = page,
		.per_page	 = PER_PAGE,
		.total_count = 0,
	};
	is_joining = false;
	if (net_pkt_send_pipe(ui->client, (pkt_t *)&pkt) != NET_ERR_OK)
		on_error(ui);
}

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	// find necessary views
	GFX_VIEW_BIND(fragment->views, list, goto error);
	GFX_VIEW_BIND(fragment->views, btn_prev, goto error);
	GFX_VIEW_BIND(fragment->views, btn_next, goto error);
	GFX_VIEW_BIND(fragment->views, btn_join, goto error);
	GFX_VIEW_BIND(fragment->views, text_summary, goto error);

	// clone the list row and keep statically
	cloned_row = gfx_view_clone(list->children, list);

	// request a game list from the server (also clear the list box rows)
	get_game_list(ui, 0);
	return true;

error:
	on_error(ui);
	return false;
}

static bool on_hide(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	if (list == NULL || cloned_row == NULL)
		return false;
	// free the current list box contents
	gfx_view_free(list->children);
	list->children = NULL;
	// put the cloned row back into the list
	DL_APPEND(list->children, cloned_row);
	return true;
}

static bool on_btn_row(view_t *view, SDL_Event *e, ui_t *ui) {
	if (is_joining)
		return false;

	// clear all highlighted rows
	view_t *row;
	DL_FOREACH(list->children, row) {
		row->children->is_gone = true;
	}
	// highlight the pressed row
	view_t *row_bg	= view->parent->children;
	row_bg->is_gone = false;
	// enable the join button
	btn_join->is_disabled = false;
	ui->force_layout	  = true;
	return true;
}

static bool on_btn_join(view_t *view, SDL_Event *e, ui_t *ui) {
	if (ui->client == NULL)
		return false;

	// find the highlighted row
	view_t *row;
	DL_SEARCH_SCALAR(list->children, row, children->is_gone, false);
	if (row == NULL)
		return false;
	// get the game key view
	view_t *row_key;
	GFX_VIEW_BIND(row, row_key, return false);

	btn_prev->is_disabled = true;
	btn_next->is_disabled = true;
	btn_join->is_disabled = true;
	gfx_view_set_text(text_summary, "Joining...");
	ui->force_layout = true;

	// send a join request
	pkt_game_join_t pkt = {
		.hdr.type = PKT_GAME_JOIN,
	};
	strncpy2(pkt.key, row_key->data.text.text, GAME_KEY_LEN);
	is_joining = true;
	if (net_pkt_send_pipe(ui->client, (pkt_t *)&pkt) != NET_ERR_OK)
		on_error(ui);
	return true;
}

static bool on_btn_prev(view_t *view, SDL_Event *e, ui_t *ui) {
	get_game_list(ui, current_page - 1);
	return true;
}

static bool on_btn_next(view_t *view, SDL_Event *e, ui_t *ui) {
	get_game_list(ui, current_page + 1);
	return true;
}

static void on_packet(ui_t *ui, pkt_t *pkt) {
	char buf[64];

	switch (pkt->hdr.type) {
		case PKT_GAME_LIST:
			ui->force_layout	  = true;
			current_page		  = (int)pkt->game_list.page;
			btn_prev->is_disabled = current_page == 0;
			int total_pages =
				pkt->game_list.total_count ? (int)((pkt->game_list.total_count - 1) / pkt->game_list.per_page + 1) : 1;
			btn_next->is_disabled = current_page >= total_pages - 1;
			btn_join->is_disabled = true;
			snprintf(
				buf,
				sizeof(buf),
				"Page %d of %d\n%d Games Found",
				current_page + 1,
				total_pages,
				pkt->game_list.total_count
			);
			gfx_view_set_text(text_summary, buf);
			return;

		case PKT_GAME_DATA:
			ui->force_layout = true;
			// add a divider to the last item
			if (list->children != NULL) {
				view_t *row_divider;
				GFX_VIEW_BIND(list->children->prev, row_divider, return);
				row_divider->is_gone = false;
			}
			// create a new row
			view_t *clone = gfx_view_clone(cloned_row, list);
			DL_APPEND(list->children, clone);
			// find all views
			view_t *row_bg, *row_name, *row_key, *row_line1, *row_line2, *row_divider;
			GFX_VIEW_BIND(clone, row_bg, return);
			GFX_VIEW_BIND(clone, row_name, return);
			GFX_VIEW_BIND(clone, row_key, return);
			GFX_VIEW_BIND(clone, row_line1, return);
			GFX_VIEW_BIND(clone, row_line2, return);
			GFX_VIEW_BIND(clone, row_divider, return);
			// update texts
			gfx_view_set_text(row_name, pkt->game_data.name);
			gfx_view_set_text(row_key, pkt->game_data.key);
			if (pkt->game_data.state == GAME_IDLE)
				snprintf(buf, sizeof(buf), "In Lobby \x07 Players: %d", pkt->game_data.players);
			else
				snprintf(
					buf,
					sizeof(buf),
					"In Game \x07 Players: %d \x07 Round: %u of %u",
					pkt->game_data.players,
					pkt->game_data.round,
					pkt->game_data.rounds
				);
			gfx_view_set_text(row_line1, buf);
			snprintf(buf, sizeof(buf), "Game Speed: %d", pkt->game_data.speed);
			gfx_view_set_text(row_line2, buf);
			row_bg->is_gone		 = true;
			row_divider->is_gone = true;
			return;

		case PKT_ERROR:
			game_print_error(pkt->error.error);
			// on game error, go back to the browse screen
			ui_state_set(ui, UI_STATE_ERROR);
			return;

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

static bool on_btn_back(view_t *view, SDL_Event *e, ui_t *ui) {
	ui->client = NULL;
	net_client_stop();
	net_server_stop();
	ui_state_set(ui, UI_STATE_SERVER_JOIN);
	return true;
}

static bool on_event(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	switch (e->type) {
		case SDL_USEREVENT_CLIENT:
			if (e->user.code == false)
				on_error(ui);
			return true;

		case SDL_USEREVENT_PACKET:
			on_packet(ui, e->user.data1);
			return true;

		case SDL_USEREVENT_GAME:
			ui->client = NULL;
			ui->game   = e->user.data1;
			ui_state_set(ui, UI_STATE_LOBBY);
			return true;
	}
	return false;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_row),
	GFX_VIEW_ON_EVENT(on_btn_join),
	GFX_VIEW_ON_EVENT(on_btn_prev),
	GFX_VIEW_ON_EVENT(on_btn_next),
	GFX_VIEW_ON_EVENT(on_btn_back),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_browse = {
	.on_show		  = on_show,
	.on_hide		  = on_hide,
	.on_event		  = on_event,
	.inflate_on_event = inflate_on_event,
};
