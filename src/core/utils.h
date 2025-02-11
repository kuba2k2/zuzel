// Copyright (c) Kuba Szczodrzyński 2024-12-16.

#pragma once

#include <stdlib.h>

#define STRINGIFY(x)	   #x
#define STRINGIFY_MACRO(x) STRINGIFY(x)

#define STRUCT_PADDING(field, len) char _padding_##field[4 - (len) % 4]
#define BUILD_BUG_ON(condition)	   ((void)sizeof(char[1 - 2 * !!(condition)]))

#define CONCAT_(prefix, suffix) prefix##suffix
#define CONCAT(prefix, suffix)	CONCAT_(prefix, suffix)
#define UNIQ(name)				CONCAT(name, __LINE__)

#define SDL_WITH_MUTEX(m)                                                                                              \
	for (volatile int UNIQ(loop) = SDL_LockMutex((m) = (m) ? (m) : SDL_CreateMutex()) * 0; UNIQ(loop) < 1;             \
		 SDL_UnlockMutex(m), UNIQ(loop)++)
#define SDL_WITH_MUTEX_OPTIONAL(m) for (volatile int i = SDL_LockMutex(m) * 0; i < 1; SDL_UnlockMutex(m), i++)

#define SDL_LOCK_MUTEX(m)                                                                                              \
	do {                                                                                                               \
		SDL_LockMutex((m) = (m) ? (m) : SDL_CreateMutex());                                                            \
	} while (0)
#define SDL_UNLOCK_MUTEX(m)                                                                                            \
	do {                                                                                                               \
		SDL_UnlockMutex(m);                                                                                            \
	} while (0)

#define FREE_NULL(var)                                                                                                 \
	do {                                                                                                               \
		free(var);                                                                                                     \
		var = NULL;                                                                                                    \
	} while (0)

#define PIPE_READ  0
#define PIPE_WRITE 1
#if WIN32
#define pipe(pipefd) _pipe(pipefd, 256, O_BINARY)
#endif

#ifndef min
#define min(a, b)                                                                                                      \
	({                                                                                                                 \
		__typeof__(a) _a = (a);                                                                                        \
		__typeof__(b) _b = (b);                                                                                        \
		_a < _b ? _a : _b;                                                                                             \
	})
#endif

#ifndef max
#define max(a, b)                                                                                                      \
	({                                                                                                                 \
		__typeof__(a) _a = (a);                                                                                        \
		__typeof__(b) _b = (b);                                                                                        \
		_a > _b ? _a : _b;                                                                                             \
	})
#endif

#ifdef __GNUC__
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif

#ifdef MSVC
#define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#endif

#include "include.h"

#if MSVC
#define gettimeofday(tp, tzp) posix_gettimeofday(tp, tzp)
#endif

typedef struct view_text_t view_text_t;
typedef struct view_inflate_on_event_t view_inflate_on_event_t;

void hexdump(const void *buf, size_t len);
#if MSVC
int posix_gettimeofday(struct timeval *tp, void *tzp);
#endif
unsigned long long millis();
void SDL_SemReset(SDL_sem *sem);
char *strncpy2(char *dest, const char *src, size_t count);
char *file_read_data(const char *filename);
cJSON *file_read_json(const char *filename);
bool file_write_data(const char *filename, const char *data, size_t length);
bool file_write_json(const char *filename, cJSON *json);
void json_read_string(cJSON *json, const char *key, char **value);
void json_read_uint(cJSON *json, const char *key, unsigned int *value);
void json_read_int(cJSON *json, const char *key, int *value);
void json_read_bool(cJSON *json, const char *key, bool *value);
void json_read_gfx_size(cJSON *json, const char *key, int *value);
void json_read_gfx_align(cJSON *json, const char *key, int *value);
void json_read_gfx_color(cJSON *json, const char *key, unsigned int *value);
void json_read_gfx_view_text(cJSON *json, const char *key, view_text_t *value);
void json_read_gfx_view_on_event(cJSON *json, const char *key, void *value, const view_inflate_on_event_t *on_event);
