// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#include "fragment.h"

#include "fragment_res.h"

bool fragment_init_all(fragment_t **fragments, void *param) {
	fragment_main.json		  = FRAGMENT_MAIN_JSON;
	fragment_server_new.json  = FRAGMENT_SERVER_NEW_JSON;
	fragment_server_join.json = FRAGMENT_SERVER_JOIN_JSON;
	fragment_connecting.json  = FRAGMENT_CONNECTING_JSON;
	fragment_error.json		  = FRAGMENT_ERROR_JSON;

	fragments[UI_STATE_MAIN]		= &fragment_main;
	fragments[UI_STATE_SERVER_JOIN] = &fragment_server_join;
	fragments[UI_STATE_SERVER_NEW]	= &fragment_server_new;
	fragments[UI_STATE_CONNECTING]	= &fragment_connecting;
	fragments[UI_STATE_ERROR]		= &fragment_error;

	for (ui_state_t state = UI_STATE_MAIN; state < UI_STATE_MAX; state++) {
		if (fragments[state] == NULL)
			continue;
		cJSON *json = cJSON_Parse((const char *)fragments[state]->json);
		if (json == NULL)
			LT_E("JSON parse failed: %s", cJSON_GetErrorPtr());
		fragments[state]->views = gfx_view_inflate(json, NULL, fragments[state]->inflate_on_event);
		cJSON_Delete(json);
		if (fragments[state]->views == NULL)
			LT_ERR(E, , "View inflate failed for state %d", state);
		gfx_view_set_event_param(fragments[state]->views, NULL, param);
	}

	return true;
}
