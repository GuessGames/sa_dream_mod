#pragma once

#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <cstdio>
#include <cmath>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#include "enet/enet.h"
#include "config.h"
#include "semver.h"

#include "CPacketFactory.h"
#include "network/packet.h"
#include "network/packet_types.h"
#include "network/packets/system.h"
#include "network/packets/players.h"
