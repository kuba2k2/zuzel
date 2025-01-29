// Copyright (c) Kuba Szczodrzyński 2025-1-29.

#include "version.h"

#include "version_res.h"

#ifndef GIT_VERSION
#define GIT_VERSION "vunknown"
#endif

const char *version_get_banner() {
	char compiler[32];
#if defined(__GNUC__)
	snprintf(compiler, sizeof(compiler), "GCC %u.%u.%u", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
	snprintf(compiler, sizeof(compiler), "MSVC %u.%u", _MSC_VER / 100, _MSC_VER % 100);
#else
	strcpy(compiler, "unknown compiler");
#endif

#if defined(__CYGWIN__)
	strcat(compiler, " - Cygwin");
#elif defined(__MINGW64__)
	strcat(compiler, " - MinGW "__MINGW64_VERSION_STR);
#endif

	static char banner[128];
	snprintf(banner, sizeof(banner), GIT_VERSION " on %s (%s)", SDL_GetPlatform(), compiler);
	return banner;
}

void version_print() {
	LT_I("Zuzel %s", version_get_banner());
}
