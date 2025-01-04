// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#include "fragment.h"

#include "fragment_res.h"

#define DEFINE_FRAGMENT(lower, upper)                                                                                  \
	do {                                                                                                               \
		fragments[UI_STATE_##upper] = &fragment_##lower;                                                               \
		fragment_##lower.file		= "../../res/ui_" #lower ".json";                                                  \
		fragment_##lower.json		= (const char *)FRAGMENT_##upper##_JSON;                                           \
		fragment_inflate(&fragment_##lower, param);                                                                    \
	} while (0)

static void fragment_inflate(fragment_t *fragment, void *param) {
	cJSON *json = cJSON_Parse((const char *)fragment->json);
	if (json == NULL)
		LT_E("JSON parse failed: %s", cJSON_GetErrorPtr());
	fragment->views = gfx_view_inflate(json, NULL, fragment->inflate_on_event);
	cJSON_Delete(json);
	if (fragment->views == NULL)
		LT_ERR(E, , "View inflate failed for file '%s'", fragment->file);
	gfx_view_set_event_param(fragment->views, NULL, param);
}

bool fragment_init_all(fragment_t **fragments, void *param) {
	DEFINE_FRAGMENT(main, MAIN);
	DEFINE_FRAGMENT(server_new, SERVER_NEW);
	DEFINE_FRAGMENT(server_join, SERVER_JOIN);
	DEFINE_FRAGMENT(connecting, CONNECTING);
	DEFINE_FRAGMENT(error, ERROR);

	return true;
}

void fragment_reload(fragment_t *fragment, void *param) {
	gfx_view_free(fragment->views);

	char *file_data;
	fragment->json = file_data = file_read_data(fragment->file);
	if (fragment->json == NULL)
		LT_ERR(E, return, "File read failed '%s'", fragment->file);

	fragment_inflate(fragment, param);
	// free the read data to prevent memory leaks
	free(file_data);
}
