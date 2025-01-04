// Copyright (c) Kuba Szczodrzyński 2025-1-4.

#include "fragment.h"

#define PER_PAGE 5

static view_t *list			= NULL; // UI-owned
static view_t *row			= NULL; // fragment-owned ("freed" in on_hide)
static view_t *btn_prev		= NULL;
static view_t *btn_next		= NULL;
static view_t *btn_join		= NULL;
static view_t *text_summary = NULL;
static int current_page		= 0;

static void on_error(ui_t *ui);

static void get_game_list(ui_t *ui, int page) {
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
	if (net_pkt_send(ui->client, (pkt_t *)&pkt) != NET_ERR_OK)
		on_error(ui);
}

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	// find necessary views
	list		 = gfx_view_find_by_id(fragment->views, "list");
	btn_prev	 = gfx_view_find_by_id(fragment->views, "btn_prev");
	btn_next	 = gfx_view_find_by_id(fragment->views, "btn_next");
	btn_join	 = gfx_view_find_by_id(fragment->views, "btn_join");
	text_summary = gfx_view_find_by_id(fragment->views, "text_summary");

	// clone the list row and keep statically
	row = gfx_view_clone(list->children, list);

	// request a game list from the server (also clear the list box rows)
	get_game_list(ui, 0);
	return true;
}

static bool on_hide(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	// free the current list box contents
	gfx_view_free(list->children);
	list->children = NULL;
	// put the cloned row back into the list
	DL_APPEND(list->children, row);
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
			current_page		  = pkt->game_list.page;
			btn_prev->is_disabled = current_page == 0;
			int total_pages		  = (pkt->game_list.total_count - 1) / pkt->game_list.per_page + 1;
			btn_next->is_disabled = current_page >= total_pages - 1;
			btn_join->is_disabled = true;
			snprintf(
				buf,
				sizeof(buf) - 1,
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
				view_t *row_divider	 = gfx_view_find_by_id(list->children->prev, "row_divider");
				row_divider->is_gone = false;
			}
			// create a new row
			view_t *clone = gfx_view_clone(row, list);
			DL_APPEND(list->children, clone);
			// find all views
			view_t *row_bg		= gfx_view_find_by_id(clone, "row_bg");
			view_t *row_name	= gfx_view_find_by_id(clone, "row_name");
			view_t *row_key		= gfx_view_find_by_id(clone, "row_key");
			view_t *row_line1	= gfx_view_find_by_id(clone, "row_line1");
			view_t *row_line2	= gfx_view_find_by_id(clone, "row_line2");
			view_t *row_divider = gfx_view_find_by_id(clone, "row_divider");
			// update texts
			gfx_view_set_text(row_name, pkt->game_data.name);
			gfx_view_set_text(row_key, pkt->game_data.key);
			snprintf(buf, sizeof(buf) - 1, "In Lobby \x07 Players: %d", 0);
			gfx_view_set_text(row_line1, buf);
			snprintf(buf, sizeof(buf) - 1, "Game Speed: %d", pkt->game_data.speed);
			gfx_view_set_text(row_line2, buf);
			row_bg->is_gone		 = true;
			row_divider->is_gone = true;
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
	ui_state_prev(ui);
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
	}
	return false;
}

static const view_inflate_on_event_t inflate_on_event[] = {
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
