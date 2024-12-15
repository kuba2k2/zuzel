// Copyright (c) Kuba Szczodrzyński 2024-12-7.

#pragma once

#include "include.h"

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
			LT_E("File reading failed, %llu != %llu bytes", read, (unsigned long long)size);                           \
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
