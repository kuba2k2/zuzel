// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#pragma once

typedef enum {
	UI_EVENT_ERROR			   = 1, //!< An error has occurred
	UI_EVENT_CLIENT_CONNECTION = 2, //!< Client connection was successful
} ui_event_type_t;
