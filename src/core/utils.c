// Copyright (c) Kuba Szczodrzyński 2024-12-16.

#include "utils.h"

char *file_read_data(const char *filename) {
	FILE *file;
	FOPEN(file, filename, "rb", return NULL);
	FSEEK(file, 0, SEEK_END, goto error);
	int length;
	FTELL(file, length, goto error);
	FSEEK(file, 0, SEEK_SET, goto error);

	char *data;
	MALLOC(data, length + 1, goto error);
	FREAD(file, data, length, goto free_data);
	data[length] = '\0';

	fclose(file);
	return data;

free_data:
	free(data);
error:
	fclose(file);
	return NULL;
}

cJSON *file_read_json(const char *filename) {
	char *data = file_read_data(filename);
	if (data == NULL)
		return NULL;

	cJSON *json = cJSON_Parse(data);
	if (json == NULL)
		LT_E("JSON parse failed: %s", cJSON_GetErrorPtr());

	free(data);
	return json;
}

void json_read_string(cJSON *json, const char *key, char **value) {
	cJSON *item = cJSON_GetObjectItem(json, key);
	if (!cJSON_IsString(item) || value == NULL)
		return;
	free(*value);
	*value = strdup(item->valuestring);
}

void json_read_uint(cJSON *json, const char *key, unsigned int *value) {
	cJSON *item = cJSON_GetObjectItem(json, key);
	if (!cJSON_IsNumber(item) || value == NULL)
		return;
	*value = item->valueint;
}

void json_read_int(cJSON *json, const char *key, int *value) {
	cJSON *item = cJSON_GetObjectItem(json, key);
	if (!cJSON_IsNumber(item) || value == NULL)
		return;
	*value = item->valueint;
}

void json_read_bool(cJSON *json, const char *key, bool *value) {
	cJSON *item = cJSON_GetObjectItem(json, key);
	if (value == NULL)
		return;
	if (cJSON_IsTrue(item))
		*value = true;
	else if (cJSON_IsFalse(item))
		*value = false;
	else if (cJSON_IsNumber(item))
		*value = item->valueint != 0;
}

void json_read_gfx_size(cJSON *json, const char *key, int *value) {
	cJSON *item = cJSON_GetObjectItem(json, key);
	if (value == NULL)
		return;
	if (cJSON_IsNumber(item)) {
		*value = item->valueint;
	} else if (cJSON_IsString(item)) {
		if (strcmp(item->valuestring, "wrap_content") == 0)
			*value = VIEW_WRAP_CONTENT;
		else if (strcmp(item->valuestring, "match_parent") == 0)
			*value = VIEW_MATCH_PARENT;
	}
}

void json_read_gfx_align(cJSON *json, const char *key, int *value) {
	cJSON *item = cJSON_GetObjectItem(json, key);
	if (!cJSON_IsString(item) || value == NULL)
		return;
	const char *string = item->valuestring;
	do {
		char *next = strchr(string, '|');
		if (next != NULL)
			*next++ = '\0';
		if (strcmp(string, "left") == 0)
			*value |= GFX_ALIGN_LEFT;
		else if (strcmp(string, "right") == 0)
			*value |= GFX_ALIGN_RIGHT;
		else if (strcmp(string, "top") == 0)
			*value |= GFX_ALIGN_TOP;
		else if (strcmp(string, "bottom") == 0)
			*value |= GFX_ALIGN_BOTTOM;
		else if (strcmp(string, "center_horizontal") == 0)
			*value |= GFX_ALIGN_CENTER_HORIZONTAL;
		else if (strcmp(string, "center_vertical") == 0)
			*value |= GFX_ALIGN_CENTER_VERTICAL;
		else if (strcmp(string, "default") == 0)
			*value |= GFX_ALIGN_DEFAULT;
		else if (strcmp(string, "center") == 0)
			*value |= GFX_ALIGN_CENTER;
		string = next;
	} while (string != NULL);
}

void json_read_gfx_color(cJSON *json, const char *key, unsigned int *value) {
	cJSON *item = cJSON_GetObjectItem(json, key);
	if (value == NULL)
		return;
	if (cJSON_IsNumber(item)) {
		*value = item->valueint;
	} else if (cJSON_IsString(item)) {
		if (item->valuestring[0] == '#')
			*value = strtoul(item->valuestring + 1, NULL, 16);
		else
			*value = strtoul(item->valuestring, NULL, 0);
	}
}

void json_read_gfx_view_text(cJSON *json, const char *key, view_text_t *value) {
	cJSON *item = cJSON_GetObjectItem(json, key);
	if (!cJSON_IsObject(item) || value == NULL)
		return;
	json_read_string(item, "text", &value->text);
	json_read_gfx_color(item, "color", &value->color);
	json_read_int(item, "font", &value->font);
	json_read_int(item, "size", &value->size);
	json_read_gfx_align(item, "align", &value->align);
}

void json_read_gfx_view_on_event(cJSON *json, const char *key, void *value, const view_inflate_on_event_t *on_event) {
	cJSON *item = cJSON_GetObjectItem(json, key);
	if (!cJSON_IsString(item) || value == NULL)
		return;
	while (on_event != NULL && on_event->name != NULL) {
		if (strcmp(item->valuestring, on_event->name) == 0) {
			*(view_on_event_t *)value = on_event->func;
			return;
		}
		on_event++;
	}
	if (on_event != NULL && on_event->name == NULL)
		LT_W("Missing view event handler '%s'", item->valuestring);
}
