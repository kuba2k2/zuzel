// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#pragma once

#include "include.h"

typedef struct {
	int loglevel;

	struct {
		int width;
		int height;
		int scale;
	} screen;

	char *player_name;
	char *game_name;
	int game_speed;
	char *public_server_address;
	int server_port;

	char *tls_cert_file;
	X509 *tls_cert;
	char *tls_key_file;
	RSA *tls_key;

	bool net_slowdown;
} settings_t;

void settings_load();
bool settings_save();

extern settings_t *SETTINGS;
