// Copyright (c) Kuba Szczodrzyński 2024-12-16.

#pragma once

#include "include.h"

typedef struct view_text_t view_text_t;
typedef struct view_inflate_on_event_t view_inflate_on_event_t;

char *file_read_data(const char *filename);
cJSON *file_read_json(const char *filename);
void json_read_string(cJSON *json, const char *key, char **value);
void json_read_uint(cJSON *json, const char *key, unsigned int *value);
void json_read_int(cJSON *json, const char *key, int *value);
void json_read_bool(cJSON *json, const char *key, bool *value);
void json_read_gfx_size(cJSON *json, const char *key, int *value);
void json_read_gfx_align(cJSON *json, const char *key, int *value);
void json_read_gfx_color(cJSON *json, const char *key, unsigned int *value);
void json_read_gfx_view_text(cJSON *json, const char *key, view_text_t *value);
void json_read_gfx_view_on_event(cJSON *json, const char *key, void *value, const view_inflate_on_event_t *on_event);
