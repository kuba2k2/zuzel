// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#pragma once

#include "include.h"

#include "game/game_t.h"
#include "game/player/player_t.h"

typedef enum {
	PKT_PING = 1,		   //!< Ping/time sync
	PKT_SUCCESS,		   //!< Success response
	PKT_ERROR,			   //!< Error response
	PKT_GAME_LIST,		   //!< List games request/response
	PKT_GAME_NEW,		   //!< New game request
	PKT_GAME_JOIN,		   //!< Join game request
	PKT_GAME_DATA,		   //!< Game data
	PKT_GAME_START,		   //!< Server match thread started
	PKT_GAME_STOP,		   //!< Server match thread stopped
	PKT_GAME_START_ROUND,  //!< Round start timestamp
	PKT_PLAYER_LIST,	   //!< List players request/response
	PKT_PLAYER_NEW,		   //!< New player request
	PKT_PLAYER_DATA,	   //!< Player data
	PKT_PLAYER_KEYPRESS,   //!< Player keypress information
	PKT_PLAYER_UPDATE,	   //!< Player generic update
	PKT_PLAYER_LEAVE,	   //!< Player leave event
	PKT_REQUEST_SEND_DATA, //!< Request to broadcast game data
	PKT_REQUEST_TIME_SYNC, //!< Request to ping all endpoints
	PKT_MAX,
} pkt_type_t;

typedef struct __attribute__((packed)) pkt_hdr_t {
	uint8_t protocol;
	STRUCT_PADDING(protocol, sizeof(uint8_t));
	pkt_type_t type : 32;
	uint32_t len;
	uint32_t reserved;
} pkt_hdr_t;

typedef struct __attribute__((packed)) pkt_ping_t {
	pkt_hdr_t hdr;
	uint64_t send_time;
	uint64_t recv_time;
} pkt_ping_t;

typedef struct __attribute__((packed)) pkt_success_t {
	pkt_hdr_t hdr;
} pkt_success_t;

typedef struct __attribute__((packed)) pkt_error_t {
	pkt_hdr_t hdr;
	game_err_t error : 32;
} pkt_error_t;

typedef struct __attribute__((packed)) pkt_game_list_t {
	pkt_hdr_t hdr;
	uint32_t page;
	uint32_t per_page;
	uint32_t total_count;
} pkt_game_list_t;

typedef struct __attribute__((packed)) pkt_game_new_t {
	pkt_hdr_t hdr;
	uint32_t is_public;
} pkt_game_new_t;

typedef struct __attribute__((packed)) pkt_game_join_t {
	pkt_hdr_t hdr;
	char key[GAME_KEY_LEN + 1];
} pkt_game_join_t;

typedef struct __attribute__((packed)) pkt_game_data_t {
	pkt_hdr_t hdr;
	uint32_t is_list;
	char key[GAME_KEY_LEN + 1];
	STRUCT_PADDING(key, GAME_KEY_LEN + 1);
	char name[GAME_NAME_LEN + 1];
	STRUCT_PADDING(name, GAME_NAME_LEN + 1);
	uint32_t is_public;
	uint32_t is_local;
	uint32_t speed;
	game_state_t state : 32;
	uint32_t players;
} pkt_game_data_t;

typedef struct __attribute__((packed)) pkt_game_start_t {
	pkt_hdr_t hdr;
} pkt_game_start_t;

typedef struct __attribute__((packed)) pkt_game_stop_t {
	pkt_hdr_t hdr;
} pkt_game_stop_t;

typedef struct __attribute__((packed)) pkt_game_start_round_t {
	pkt_hdr_t hdr;
	uint64_t count_at;
	uint64_t start_at;
} pkt_game_start_round_t;

typedef struct __attribute__((packed)) pkt_player_list_t {
	pkt_hdr_t hdr;
	uint32_t total_count;
} pkt_player_list_t;

typedef struct __attribute__((packed)) pkt_player_new_t {
	pkt_hdr_t hdr;
	char name[PLAYER_NAME_LEN + 1];
} pkt_player_new_t;

typedef struct __attribute__((packed)) pkt_player_data_t {
	pkt_hdr_t hdr;
	uint32_t id;
	char name[PLAYER_NAME_LEN + 1];
	STRUCT_PADDING(name, PLAYER_NAME_LEN + 1);
	uint32_t color;
	uint32_t is_local;
	player_state_t state : 32;
} pkt_player_data_t;

typedef struct __attribute__((packed)) pkt_player_keypress_t {
	pkt_hdr_t hdr;
	uint32_t id;
	uint32_t time;
	player_pos_dir_t direction : 32;
} pkt_player_keypress_t;

typedef struct __attribute__((packed)) pkt_player_leave_t {
	pkt_hdr_t hdr;
	uint32_t id;
} pkt_player_leave_t;

typedef struct __attribute__((packed)) pkt_request_send_data_t {
	pkt_hdr_t hdr;
	uintptr_t join_endpoint; //!< Pointer to the endpoint that just joined
	uint32_t updated_game;	 //!< Whether the game data was updated (0/1)
	uint32_t updated_player; //!< ID of the player that was updated (or just left)
} pkt_request_send_data_t;

typedef struct __attribute__((packed)) pkt_request_time_sync_t {
	pkt_hdr_t hdr;
} pkt_request_time_sync_t;

typedef union __attribute__((packed)) pkt_t {
	pkt_hdr_t hdr;
	pkt_ping_t ping;
	pkt_success_t success;
	pkt_error_t error;
	pkt_game_list_t game_list;
	pkt_game_new_t game_new;
	pkt_game_join_t game_join;
	pkt_game_data_t game_data;
	pkt_game_start_t game_start;
	pkt_game_stop_t game_stop;
	pkt_game_start_round_t game_start_round;
	pkt_player_data_t player_data;
	pkt_player_keypress_t player_keypress;
	// pkt_player_update_t player_update;
	pkt_player_leave_t player_leave;
	pkt_request_send_data_t request_send_data;
	pkt_request_time_sync_t request_time_sync;
} pkt_t;
