// Copyright (c) Kuba Szczodrzyński 2024-12-16.

#include "utils.h"

void hexdump(const void *buf, size_t len) {
	SDL_LockMutex(log_mutex);
	uint16_t pos = 0;
	while (pos < len) {
		// print hex offset
		printf("%06x ", pos);
		// calculate current line width
		uint8_t lineWidth = min(16, len - pos);
		// print hexadecimal representation
		for (uint8_t i = 0; i < lineWidth; i++) {
			if (i % 8 == 0) {
				putchar(' ');
			}
			printf("%02x ", ((const uint8_t *)buf)[pos + i]);
		}
		// print ascii representation
		putchar(' ');
		putchar('|');
		for (uint8_t i = 0; i < lineWidth; i++) {
			uint8_t c = ((const uint8_t *)buf)[pos + i];
			putchar((c >= 0x20 && c <= 0x7f) ? c : '.');
		}
		puts("|\r");
		pos += lineWidth;
	}
	fflush(stdout);
	SDL_UnlockMutex(log_mutex);
}

unsigned long long millis() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void SDL_SemReset(SDL_sem *sem) {
	while (SDL_SemTryWait(sem) == 0) {
		/* reset the semaphore */
	}
}

char *strncpy2(char *dest, const char *src, size_t count) {
	strncpy(dest, src, count);
	dest[count] = '\0';
	return dest;
}

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

bool file_write_data(const char *filename, const char *data, int length) {
	FILE *file;
	FOPEN(file, filename, "wb", return false);
	FWRITE(file, data, length, goto error);
	fclose(file);
	return true;

error:
	fclose(file);
	return false;
}

bool file_write_json(const char *filename, cJSON *json) {
	char *data = cJSON_Print(json);
	if (data == NULL)
		return false;
	bool ret = file_write_data(filename, data, (int)strlen(data));
	free(data);
	return ret;
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
