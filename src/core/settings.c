// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "settings.h"

void settings_load() {
	// load defaults first
	SETTINGS->loglevel		= LT_LEVEL_DEBUG;
	SETTINGS->screen.width	= 640;
	SETTINGS->screen.height = 480;
	SETTINGS->screen.scale	= 1;
}

bool settings_save() {
	return false;
}

settings_t *SETTINGS;
