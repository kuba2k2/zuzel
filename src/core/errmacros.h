// Copyright (c) Kuba Szczodrzyński 2024-12-7.

#pragma once

#include "include.h"

#if __APPLE__
#undef FREAD
#undef FWRITE
#endif

#define MALLOC(ptr, size, err)                                                                                         \
	do {                                                                                                               \
		ptr = malloc(size);                                                                                            \
		if (ptr == NULL) {                                                                                             \
			LT_E("Memory allocation failed for '" #ptr "' (%llu bytes)", (unsigned long long)size);                    \
			err;                                                                                                       \
		}                                                                                                              \
		memset(ptr, 0, size);                                                                                          \
	} while (0)

#define FOPEN(file, filename, mode, err)                                                                               \
	do {                                                                                                               \
		file = fopen(filename, mode);                                                                                  \
		if (file == NULL) {                                                                                            \
			LT_E("Can't open file '%s'", filename);                                                                    \
			err;                                                                                                       \
		}                                                                                                              \
	} while (0)
#define FREAD(file, buf, size, err)                                                                                    \
	do {                                                                                                               \
		size_t read = fread(buf, 1, size, file);                                                                       \
		if (read != size) {                                                                                            \
			LT_E("File reading failed, %llu != %llu bytes", (unsigned long long)read, (unsigned long long)size);       \
			err;                                                                                                       \
		}                                                                                                              \
	} while (0)
#define FWRITE(file, buf, size, err)                                                                                   \
	do {                                                                                                               \
		size_t written = fwrite(buf, 1, size, file);                                                                   \
		if (written != size) {                                                                                         \
			LT_E("File writing failed, %llu != %llu bytes", (unsigned long long)written, (unsigned long long)size);    \
			err;                                                                                                       \
		}                                                                                                              \
	} while (0)
#define FSEEK(file, offset, origin, err)                                                                               \
	do {                                                                                                               \
		if (fseek(file, offset, origin) != 0) {                                                                        \
			LT_E("File seeking failed");                                                                               \
			err;                                                                                                       \
		}                                                                                                              \
	} while (0)
#define FTELL(file, into, err)                                                                                         \
	do {                                                                                                               \
		if ((into = ftell(file)) < 0) {                                                                                \
			LT_E("File tell failed");                                                                                  \
			err;                                                                                                       \
		}                                                                                                              \
	} while (0)

#define SDL_ERROR(func, err)                                                                                           \
	do {                                                                                                               \
		LT_F(func " failed; SDL_Error: %s", SDL_GetError());                                                           \
		err;                                                                                                           \
	} while (0)

#define LT_ERR(level, err, ...)                                                                                        \
	do {                                                                                                               \
		LT_##level(__VA_ARGS__);                                                                                       \
		err;                                                                                                           \
	} while (0)

#if WIN32
#define SOCK_ERROR(func, err)                                                                                          \
	do {                                                                                                               \
		if (!net_error_print())                                                                                        \
			LT_F(func " failed; WSAGetLastError: %d", WSAGetLastError());                                              \
		err;                                                                                                           \
	} while (0)
#else
#define SOCK_ERROR(func, err)                                                                                          \
	do {                                                                                                               \
		if (!net_error_print())                                                                                        \
			LT_F(func " failed; errno: %s", strerror(errno));                                                          \
		err;                                                                                                           \
	} while (0)
#endif

#define SSL_ERROR(func, err)                                                                                           \
	do {                                                                                                               \
		LT_F(func " failed; SSL error: %s", ERR_error_string(ERR_get_error(), NULL));                                  \
		err;                                                                                                           \
	} while (0)
