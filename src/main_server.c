// Copyright (c) Kuba Szczodrzyński 2025-1-4.

#include "include.h"

int SDL_main(int argc, char *argv[]) {
	MALLOC(SETTINGS, sizeof(*SETTINGS), return 1);
	settings_load();

	net_server_start(true);
	return 0;
}
