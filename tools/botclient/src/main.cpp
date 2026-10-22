// SA Dream Mod bot client: connects N fake players to a server, without the game.
// Every bot does the real handshake and then runs in a circle sending on-foot updates like a player, so the
// server and the real game windows see N players. It measures how many get in, the server's traffic and the RTT.
//
//   botclient.exe --ip 127.0.0.1 --port 6767 --bots 7 --seconds 120 --x 2495 --y -1670 --z 13.4 --radius 6
#include "stdafx.h"
#include <chrono>
#include <thread>

namespace
{
struct SBot
{
    int index = 0;
    ENetHost* host = nullptr;
    ENetPeer* peer = nullptr;
    bool connected = false;  // enet connection
    bool joined = false;     // the server's handshake arrived
    int playerid = -1;
    std::string refused;  // why the server did not take it
    uint64_t bytesReceived = 0;
    uint32_t packetsReceived = 0;
};

struct SOptions
{
    std::string ip = "127.0.0.1";
    int port = Config::DEFAULT_PORT;
    int bots = 7;
    int seconds = 60;
    float x = 2495.0f, y = -1670.0f, z = 13.4f;  // Grove Street
    float radius = 6.0f;
    int rateMs = 100;  // on-foot updates per bot, the game sends about the same
    std::string name = "Bot";
};

uint32_t g_packedVersion = 0;

void Send(SBot& bot, Packet& packet)
{
    static uint8_t buffer[10 * 1024];
    memset(buffer, 0, sizeof(buffer));
    serialize::WriteStream stream(buffer, sizeof(buffer));

    uint32_t type = static_cast<uint16_t>(packet.GetType());
    stream.SerializeBits(type, 16);
    if (packet.GetChannel() != ePacketChannel::SYSTEM)
    {
        uint32_t serverTime = 0;  // the server stamps its own time
        stream.SerializeBits(serverTime, 32);
    }
    if (!packet.SerializeWrite(stream))
    {
        printf("[bot %d] could not serialize packet %d\n", bot.index, type);
        return;
    }
    stream.Flush();

    uint32_t flags = GetChannelReliability(packet.GetChannel()) == ePacketReliability::RELIABLE ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* enetPacket = enet_packet_create(stream.GetData(), stream.GetBytesProcessed(), flags);
    if (enetPacket)
        enet_peer_send(bot.peer, static_cast<uint8_t>(packet.GetChannel()), enetPacket);
}

void OnReceive(SBot& bot, const uint8_t* data, size_t size)
{
    bot.bytesReceived += size;
    bot.packetsReceived++;

    serialize::ReadStream stream(data, (int)size);
    uint32_t type = 0;
    if (!stream.SerializeBits(type, 16))
        return;

    auto it = GetPacketFactory().m_map.find(static_cast<ePacketType>(type));
    if (it == GetPacketFactory().m_map.end())
        return;  // only the system packets are of interest

    if (it->second->GetChannel() != ePacketChannel::SYSTEM)
    {
        uint32_t serverTime = 0;
        stream.SerializeBits(serverTime, 32);
    }

    if (type == (uint32_t)ePacketType::PLAYER_HANDSHAKE)
    {
        Packets::System::PlayerHandshake packet{};
        if (static_cast<Packet&>(packet).SerializeRead(stream))
        {
            bot.joined = true;
            bot.playerid = packet.yourid;
            printf("[bot %d] joined as player id %d\n", bot.index, bot.playerid);
        }
    }
    else if (type == (uint32_t)ePacketType::PLAYER_DISCONNECTED)
    {
        Packets::System::PlayerDisconnected packet{};
        if (static_cast<Packet&>(packet).SerializeRead(stream) && packet.payload.playerid == -1)
        {
            // addressed to us: the server refuses this connection
            static const char* reasons[] = {"no reason given", "version mismatch", "name taken"};
            bot.refused = packet.payload.reason < 3 ? reasons[packet.payload.reason] : "unknown reason";
        }
    }
}

void Service(SBot& bot)
{
    ENetEvent event;
    while (bot.host && enet_host_service(bot.host, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            bot.connected = true;
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            OnReceive(bot, event.packet->data, event.packet->dataLength);
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            if (bot.connected || bot.joined)
                printf("[bot %d] disconnected by the server%s%s\n", bot.index, bot.refused.empty() ? "" : ": ",
                    bot.refused.c_str());
            else
                printf("[bot %d] the server did not accept the connection (full?)\n", bot.index);
            bot.connected = bot.joined = false;
            bot.peer = nullptr;
            break;
        default:
            break;
        }
    }
}

bool Connect(SBot& bot, const SOptions& options)
{
    bot.host = enet_host_create(nullptr, 1, (int)ePacketChannel::COUNT, 0, 0);
    if (!bot.host)
        return false;

    ENetAddress address{};
    enet_address_set_host(&address, options.ip.c_str());
    address.port = (enet_uint16)options.port;
    bot.peer = enet_host_connect(bot.host, &address, (int)ePacketChannel::COUNT, g_packedVersion);
    if (!bot.peer)
        return false;

    // up to 5 s for the connection
    for (int i = 0; i < 100 && bot.peer && !bot.connected; i++)
    {
        Service(bot);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    if (!bot.connected)
        return false;

    Packets::System::PlayerConnected packet{};
    packet.payload.isAlreadyConnected = false;
    packet.payload.playerid = -1;
    packet.payload.version = g_packedVersion;
    snprintf(packet.payload.name, sizeof(packet.payload.name), "%s%d", options.name.c_str(), bot.index + 1);
    Send(bot, packet);

    // up to 5 s for the handshake
    for (int i = 0; i < 100 && bot.peer && !bot.joined; i++)
    {
        Service(bot);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return bot.joined;
}

void SendOnFoot(SBot& bot, const SOptions& options, float t)
{
    // each bot on its own spot of the circle, running counter-clockwise (about 6 m/s)
    float speed = 6.0f / std::max(options.radius, 1.0f);
    float angle = t * speed + bot.index * 6.2831853f / std::max(options.bots, 1);
    CVector pos(options.x + cosf(angle) * options.radius, options.y + sinf(angle) * options.radius, options.z);
    CVector velocity(-sinf(angle) * 6.0f / 50.0f, cosf(angle) * 6.0f / 50.0f, 0.0f);  // game units per 1/50 s
    float heading = angle;  // facing along the circle (heading 0 = north, the tangent of angle points there)

    Packets::Players::OnFootUpdate packet{};
    packet.vecPos = pos;
    packet.vecMoveSpeed = velocity;
    packet.currentRotation = heading;
    packet.aimingRotation = heading;
    packet.healthSnapshot.iHealth = 100;
    Send(bot, packet);
}

bool ParseArgs(int argc, char** argv, SOptions& options)
{
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        auto next = [&]() -> const char* { return i + 1 < argc ? argv[++i] : ""; };
        if (arg == "--ip") options.ip = next();
        else if (arg == "--port") options.port = atoi(next());
        else if (arg == "--bots") options.bots = atoi(next());
        else if (arg == "--seconds") options.seconds = atoi(next());
        else if (arg == "--x") options.x = (float)atof(next());
        else if (arg == "--y") options.y = (float)atof(next());
        else if (arg == "--z") options.z = (float)atof(next());
        else if (arg == "--radius") options.radius = (float)atof(next());
        else if (arg == "--rate") options.rateMs = std::max(10, atoi(next()));
        else if (arg == "--name") options.name = next();
        else
        {
            printf("usage: botclient [--ip 127.0.0.1] [--port %d] [--bots 7] [--seconds 60] [--x X --y Y --z Z]\n"
                   "                 [--radius 6] [--rate 100 (ms between updates)] [--name Bot]\n",
                Config::DEFAULT_PORT);
            return false;
        }
    }
    return true;
}
}  // namespace

int main(int argc, char** argv)
{
    // the test script kills it at the end and reads the redirected output: no buffering
    setvbuf(stdout, nullptr, _IONBF, 0);

    SOptions options;
    if (!ParseArgs(argc, argv, options))
        return 1;

    if (enet_initialize() != 0)
    {
        printf("enet_initialize failed\n");
        return 1;
    }
    g_packedVersion = semver_parse(COOPANDREAS_VERSION, nullptr);

    printf("connecting %d bots to %s:%d (server limit Config::MAX_SERVER_PLAYERS = %d, real players count too)\n",
        options.bots, options.ip.c_str(), options.port, Config::MAX_SERVER_PLAYERS);

    std::vector<SBot> bots(options.bots);
    int joined = 0;
    for (int i = 0; i < options.bots; i++)
    {
        bots[i].index = i;
        if (Connect(bots[i], options))
            joined++;
        else
            printf("[bot %d] could not join%s%s\n", i, bots[i].refused.empty() ? "" : ": ", bots[i].refused.c_str());
    }
    printf("joined: %d of %d\n", joined, options.bots);

    auto start = std::chrono::steady_clock::now();
    auto lastReport = start;
    auto lastSend = start;
    uint64_t lastBytes = 0;
    while (true)
    {
        auto now = std::chrono::steady_clock::now();
        float t = std::chrono::duration<float>(now - start).count();
        if (t > options.seconds)
            break;

        // the network is serviced every 10 ms (a slower loop shows up as RTT), the updates go at --rate
        bool bSend = now - lastSend >= std::chrono::milliseconds(options.rateMs);
        if (bSend)
            lastSend = now;
        for (auto& bot : bots)
        {
            Service(bot);
            if (bSend && bot.joined && bot.peer)
                SendOnFoot(bot, options, t);
            if (bot.host)
                enet_host_flush(bot.host);
        }

        if (now - lastReport > std::chrono::seconds(10))
        {
            uint64_t bytes = 0;
            int alive = 0;
            uint32_t rttSum = 0;
            uint32_t rttMax = 0;
            for (auto& bot : bots)
            {
                bytes += bot.bytesReceived;
                if (bot.joined && bot.peer)
                {
                    alive++;
                    rttSum += bot.peer->roundTripTime;
                    rttMax = std::max(rttMax, bot.peer->roundTripTime);
                }
            }
            float secs = std::chrono::duration<float>(now - lastReport).count();
            printf("[%3.0f s] bots online %d, server -> each bot %.1f KB/s, RTT avg %u ms max %u ms\n", t, alive,
                alive ? (bytes - lastBytes) / 1024.0f / secs / alive : 0.0f, alive ? rttSum / alive : 0, rttMax);
            lastBytes = bytes;
            lastReport = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (auto& bot : bots)
    {
        if (bot.peer)
        {
            enet_peer_disconnect_now(bot.peer, 0);
        }
        if (bot.host)
        {
            enet_host_flush(bot.host);
            enet_host_destroy(bot.host);
        }
    }
    enet_deinitialize();
    printf("done\n");
    return 0;
}
