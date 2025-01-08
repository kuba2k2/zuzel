// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "settings.h"

void settings_load() {
	// load defaults first
	SETTINGS->loglevel				= LT_LEVEL_DEBUG;
	SETTINGS->screen.width			= 640;
	SETTINGS->screen.height			= 480;
	SETTINGS->screen.scale			= 1;
	SETTINGS->player_name			= strdup("Player");
	SETTINGS->game_name				= NULL;
	SETTINGS->public_server_address = strdup("127.0.0.1:5678");
	SETTINGS->server_port			= 1234;
	SETTINGS->tls_cert_file			= strdup("server.crt");
	SETTINGS->tls_key_file			= strdup("server.key");

	cJSON *json = file_read_json("settings.json");
	if (json == NULL)
		return;

	json_read_int(json, "loglevel", &SETTINGS->loglevel);
	cJSON *screen = cJSON_GetObjectItem(json, "screen");
	json_read_int(screen, "width", &SETTINGS->screen.width);
	json_read_int(screen, "height", &SETTINGS->screen.height);
	json_read_int(screen, "scale", &SETTINGS->screen.scale);
	json_read_string(json, "player_name", &SETTINGS->player_name);
	json_read_string(json, "game_name", &SETTINGS->game_name);
	json_read_string(json, "public_server_address", &SETTINGS->public_server_address);
	json_read_int(json, "server_port", &SETTINGS->server_port);
	json_read_string(json, "tls_cert_file", &SETTINGS->tls_cert_file);
	json_read_string(json, "tls_key_file", &SETTINGS->tls_key_file);

	LT_I("Loaded settings:");
	LT_I(" - loglevel: %d", SETTINGS->loglevel);
	LT_I(" - screen.width: %d", SETTINGS->screen.width);
	LT_I(" - screen.height: %d", SETTINGS->screen.height);
	LT_I(" - screen.scale: %d", SETTINGS->screen.scale);
	LT_I(" - player_name: \"%s\"", SETTINGS->player_name);
	LT_I(" - game_name: \"%s\"", SETTINGS->game_name);
	LT_I(" - public_server_address: \"%s\"", SETTINGS->public_server_address);
	LT_I(" - server_port: %d", SETTINGS->server_port);
	LT_I(" - tls_cert_file: \"%s\"", SETTINGS->tls_cert_file);
	LT_I(" - tls_key_file: \"%s\"", SETTINGS->tls_key_file);

	cJSON_Delete(json);
}

bool settings_save() {
	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "loglevel", SETTINGS->loglevel);
	cJSON *screen = cJSON_AddObjectToObject(json, "screen");
	cJSON_AddNumberToObject(screen, "width", SETTINGS->screen.width);
	cJSON_AddNumberToObject(screen, "height", SETTINGS->screen.height);
	cJSON_AddNumberToObject(screen, "scale", SETTINGS->screen.scale);
	cJSON_AddStringToObject(json, "player_name", SETTINGS->player_name);
	cJSON_AddStringToObject(json, "game_name", SETTINGS->game_name);
	cJSON_AddStringToObject(json, "public_server_address", SETTINGS->public_server_address);
	cJSON_AddNumberToObject(json, "server_port", SETTINGS->server_port);
	cJSON_AddStringToObject(json, "tls_cert_file", SETTINGS->tls_cert_file);
	cJSON_AddStringToObject(json, "tls_key_file", SETTINGS->tls_key_file);

	bool ret = file_write_json("settings.json", json);
	cJSON_Delete(json);
	return ret;
}

settings_t *SETTINGS;
