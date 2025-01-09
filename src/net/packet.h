// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#pragma once

#include "include.h"

#include "game/structs.h"

typedef enum {
	PKT_PING = 1,		 //!< Ping/time sync
	PKT_SUCCESS,		 //!< Success response
	PKT_ERROR,			 //!< Error response
	PKT_GAME_LIST,		 //!< List games request/response
	PKT_GAME_NEW,		 //!< New game request
	PKT_GAME_JOIN,		 //!< Join game request
	PKT_GAME_DATA,		 //!< Game data
	PKT_GAME_STATE,		 //!< Game state change
	PKT_PLAYER_LIST,	 //!< List players request/response
	PKT_PLAYER_NEW,		 //!< New player request
	PKT_PLAYER_DATA,	 //!< Player data
	PKT_PLAYER_STATE,	 //!< Player state change
	PKT_PLAYER_KEYPRESS, //!< Player keypress information
	PKT_PLAYER_UPDATE,	 //!< Player generic update
	PKT_PLAYER_LEAVE,	 //!< Player leave event
	PKT_SEND_GAME_DATA,	 //!< Request to broadcast game data
	PKT_MAX,
} pkt_type_t;

typedef struct __attribute__((packed)) {
	uint8_t protocol;
	STRUCT_PADDING(protocol, sizeof(uint8_t));
	pkt_type_t type : 32;
	uint32_t len;
	uint32_t reserved;
} pkt_hdr_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	uint32_t seq;
	uint32_t is_response;
} pkt_ping_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
} pkt_success_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	game_err_t error : 32;
} pkt_error_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	uint32_t page;
	uint32_t per_page;
	uint32_t total_count;
} pkt_game_list_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	uint32_t is_public;
} pkt_game_new_t;

typedef struct __attribute__((packed)) {
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

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	game_state_t state : 32;
} pkt_game_state_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	uint32_t total_count;
} pkt_player_list_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	char name[PLAYER_NAME_LEN + 1];
} pkt_player_new_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	uint32_t id;
	char name[PLAYER_NAME_LEN + 1];
	STRUCT_PADDING(name, PLAYER_NAME_LEN + 1);
	uint32_t color;
	uint32_t is_local;
	player_state_t state : 32;
} pkt_player_data_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	uint32_t id;
	player_state_t state : 32;
} pkt_player_state_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	uint32_t id;
	uint32_t time;
	player_pos_state_t pos_state : 32;
} pkt_player_keypress_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
	uint32_t id;
} pkt_player_leave_t;

typedef struct __attribute__((packed)) {
	pkt_hdr_t hdr;
} pkt_send_game_data_t;

typedef union __attribute__((packed)) pkt_t {
	pkt_hdr_t hdr;
	pkt_ping_t ping;
	pkt_success_t success;
	pkt_error_t error;
	pkt_game_list_t game_list;
	pkt_game_new_t game_new;
	pkt_game_join_t game_join;
	pkt_game_data_t game_data;
	pkt_game_state_t game_state;
	pkt_player_data_t player_data;
	pkt_player_state_t player_state;
	pkt_player_keypress_t player_keypress;
	// pkt_player_update_t player_update;
	pkt_player_leave_t player_leave;
	pkt_send_game_data_t send_game_data;
} pkt_t;
