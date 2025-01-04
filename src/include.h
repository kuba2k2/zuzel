// Copyright (c) Kuba Szczodrzyński 2024-12-7.

#pragma once

#include <fcntl.h>
#include <io.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <cJSON.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <utlist.h>

#ifdef __WIN32__
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "core/config.h"

#include "core/errmacros.h"
#include "core/logger.h"
#include "core/settings.h"
#include "core/utils.h"

#include "game/game.h"
#include "net/net.h"

#include "ui/fragment/fragment.h"
#include "ui/gfx/gfx.h"
#include "ui/ui.h"
