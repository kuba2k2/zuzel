// Copyright (c) Kuba Szczodrzyński 2024-12-7.

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <SDL2/SDL.h>

#ifdef __WIN32__
#include <winsock2.h>
#endif

#include "core/config.h"

#include "core/errmacros.h"
#include "core/logger.h"
#include "core/settings.h"

#include "game/game.h"

#include "ui/gfx/gfx.h"
#include "ui/menu/menu.h"
#include "ui/race/race.h"
