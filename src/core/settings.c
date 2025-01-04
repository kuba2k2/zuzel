// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "settings.h"

void settings_load() {
	// load defaults first
	SETTINGS->loglevel				= LT_LEVEL_DEBUG;
	SETTINGS->screen.width			= 640;
	SETTINGS->screen.height			= 480;
	SETTINGS->screen.scale			= 1;
	SETTINGS->player_name			= strdup("Player");
	SETTINGS->public_server_address = strdup("127.0.0.1");
	SETTINGS->server_port			= 1234;
	SETTINGS->tls_cert_file			= strdup("server.crt");
	SETTINGS->tls_key_file			= strdup("server.key");
}

bool settings_save() {
	return false;
}

settings_t *SETTINGS;
