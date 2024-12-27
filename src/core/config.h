// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#pragma once

// Logger format options
#ifndef LT_LOGGER_TIMESTAMP
#define LT_LOGGER_TIMESTAMP 1
#endif

#ifndef LT_LOGGER_CALLER
#define LT_LOGGER_CALLER 1
#endif

#ifndef LT_LOGGER_TASK
#define LT_LOGGER_TASK 0
#endif

#ifndef LT_LOGGER_COLOR
#define LT_LOGGER_COLOR 1
#endif

// Settings storage
#ifndef SETTINGS_FILE
#define SETTINGS_FILE "settings.json"
#endif

// Constant game settings

#define GFX_MAX_FONTS 10

#define GAME_NAME_LEN	32
#define GAME_KEY_LEN	6
#define PLAYER_NAME_LEN 32
