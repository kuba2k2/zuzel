# Żużel

*(motorcycle speedway)*

This is a university project for the Computer Networks 2024/2025 course on Poznan University of Technology:

*"A modern remake of a classic MS-DOS racing arcade game from 1994".*

## Introduction

This is a recreation of an MS-DOS video game, which has been popular in schools and universities in Poland, in the
1990s.

It is probably one of the simplest racing games ever made, where each of the (maximum 4) players press a key, in order
to turn left.

In the original game, every player has a different key assigned on the keyboard connected to a single computer. In the
remake, however, players are connected via network. This makes it possible to play on multiple devices, while not even
having to be in the same room (which is highly recommended though)!

## Rules & game objective

When the game starts, every player's motorcycle is visible as a colored line. When it starts moving, it will gradually
become faster.

Pressing a key makes it turn left. During that time, the motorcycle slightly slows down.

Crashing into a wall makes a player lose. Completing four (4) laps makes them finish the race.

A single match in the game equals to 15 rounds. Each round is 4 laps.

Players get an appropriate amount of points, based on how quickly they were able to finish the race.

## Building

The game is a CMake project. It can be compiled using GCC or MSVC.

The repository uses submodules - make sure to initialize them too!

On Windows, using MinGW64 or MSVC is recommended (Cygwin and MSYS2 can make SDL2 compilation impossible).

Assuming you have CMake, Make and a C compiler installed:

```shell
git clone https://github.com/kuba2k2/zuzel
cd zuzel
git submodule update --init --recursive
mkdir cmake-build && cd cmake-build
cmake ..
make
```

On some versions of Linux, it might be necessary to install `libxext-dev` or `libXext-devel` (depending on the
distribution).

## Running

After CMake builds the project, run `cmake-build/src/zuzel` to start the game client.

The game server can also be started using `cmake-build/src/zuzel-server`. This requires an SSL certificate and private
key in the current working directory. The following command can generate the two needed files:

```shell
openssl req -newkey rsa:2048 -nodes -keyout server.key -x509 -days 365 -out server.crt
```

## Settings

Game settings can be configured using `settings.json` (in the current working directory).

```shell
# cat settings.json
{
    # logger level
    "loglevel": 2,
    # game window configuration
    "screen": {
        "width": 640,
        "height": 480,
        "scale": 1
    },
    # last used player name
    "player_name": "Player",
    # last used game name
    "game_name": "Player's Game",
    # last used game speed
    "game_speed": 3,
    # last used public server address, also port number for zuzel-server
    "public_server_address": "127.0.0.1:5678",
    # last used local server address
    "last_join_address": "192.168.0.160",
    # port number for the local server
    "server_port": 1234,
    # TLS certificate and key for zuzel-server
    "tls_cert_file": "server.crt",
    "tls_key_file": "server.key",
    # debugging option: 100 ms slowdown of network responses
    "net_slowdown": false
}
```

## Game implementation

The project is split over several different modules:

- `core/` - logger and various utilities,
- `game/` - game thread, player position calculation, etc.,
- `game/match/` - match thread,
- `net/server.c` - server (room discovery),
- `net/client.c` - client (room connection),
- `net/endpoint.c` - cross-platform implementation of network-related functions,
- `ui/gfx/` - graphics modules (fonts, views),
- `ui/fragment/` - UI fragments (pages).

The *server* is responsible accepting connections from *clients* and providing a list of public *game rooms*.

When the *server* and *client* both decide which *room* to join, a *game thread* is created (on both ends) and the
network sockets are handed over to that thread.

The *game thread* is then responsible for changing game options (speed, etc.), waiting for other players, and starting
the *match thread* when all players become ready.

The *match thread* calculates player positions and waits for keypress events from all players. It also notifies the UI
that players have moved, so that they could be redrawn.

## Game protocol

The *server*, *client* and *game thread* communicate by sending and receiving packets. The packet structure is as
follows:

| &nbsp;     | Type              | Description                  |
|------------|-------------------|------------------------------|
| **HEADER** |                   |                              |
| `protocol` | `uint8_t`         | Protocol version (always 1)  |
|            | Padding (3 bytes) |                              |
| `type`     | `uint32_t`        | Packet type                  |
| `len`      | `uint32_t`        | Packet length (incl. header) |
| `reserved` | `uint32_t`        | Unused                       |
| **DATA**   |                   |                              |
|            | `pkt_<*>_t`       | Packet data structure        |

Packet data structures can be viewed in [`src/net/packet.h`](src/net/packet.h). There are several defined packet types:

| Name                 | Type | Length | Description                    |
|----------------------|------|--------|--------------------------------|
| `PING`               | 1    | 32 B   | Ping/time sync                 |
| `ERROR`              | 2    | 20 B   | Error response                 |
| `GAME_LIST`          | 3    | 28 B   | List games request/response    |
| `GAME_NEW`           | 4    | 20 B   | New game request               |
| `GAME_JOIN`          | 5    | 24 B   | Join game request              |
| `GAME_DATA`          | 6    | 84 B   | Game data                      |
| `GAME_START`         | 7    | 16 B   | Server match thread started    |
| `GAME_STOP`          | 8    | 16 B   | Server match thread stopped    |
| `GAME_START_ROUND`   | 9    | 32 B   | Round start timestamp          |
| `PLAYER_NEW`         | 10   | 44 B   | New player request             |
| `PLAYER_DATA`        | 11   | 64 B   | Player data                    |
| `PLAYER_KEYPRESS`    | 12   | 28 B   | Player keypress information    |
| `PLAYER_LEAVE`       | 13   | 20 B   | Player leave event             |
| `REQUEST_SEND_DATA`* | 14   | 32 B   | Request to broadcast game data |
| `REQUEST_TIME_SYNC`* | 15   | 16 B   | Request to ping all endpoints  |

\* These packets are local-only (for inter-thread communication), they are not sent over the network.

All fields in all packets are little-endian. Character arrays (strings) are NULL-terminated.

Each packet's `len` must correspond to the length of the structure indicated by `type`. Otherwise, such an invalid
packet is ignored by the server.

## Acknowledgements

Credits for the game idea and graphics go to the original author, Piotr Kamiński.

## License

```
MIT License

Copyright (c) 2025 Kuba Szczodrzyński

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
