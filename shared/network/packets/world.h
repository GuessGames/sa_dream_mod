#pragma once
#include "CWeather.h"
#include <CExplosion.h>

namespace Packets::World
{
// TODO: proper weather sync
class GameWeatherTime : public Packet
{
    DEFINE_PACKET_TYPE(GameWeatherTime, ePacketType::GAME_WEATHER_TIME, ePacketChannel::EVENT);

public:
    eWeatherType newWeather;
    eWeatherType oldWeather;
    // eWeatherType forcedWeather;
    uint8_t currentMonth;
    uint8_t currentDay;
    uint8_t currentHour;
    uint8_t currentMinute;
    uint8_t currentSecond = 0;
    // -1 = not forced; set by missions, cheats and the admin menu
    int forcedWeather = -1;
    float interpolation = 0.0f;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        if (Stream::IsWriting)
        {
            newWeather = VCLAMP(newWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);
            oldWeather = VCLAMP(oldWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);
            // forcedWeather = VCLAMP(forcedWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);
            currentMonth = VCLAMP(currentMonth, 1, 12);
            currentDay = VCLAMP(currentDay, 1, 31);
            currentHour = VCLAMP(currentHour, 0, 23);
            currentMinute = VCLAMP(currentMinute, 0, 59);
        }
        serialize_int(stream, (int&)newWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);
        serialize_int(stream, (int&)oldWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);
        // serialize_int(stream, (int&)forcedWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);

        serialize_int(stream, currentMonth, 1, 12);
        serialize_int(stream, currentDay, 1, 31);
        serialize_int(stream, currentHour, 0, 23);
        serialize_int(stream, currentMinute, 0, 59);
        serialize_int(stream, currentSecond, 0, 59);
        serialize_int(stream, forcedWeather, -1, WEATHER_EXTRACOLOURS_2);
        serialize_compressed_float(stream, interpolation, 0.0f, 1.0f, 0.001f);

        return true;
    }
};

// unused for now
class AddExplosion : public Packet
{
    DEFINE_PACKET_TYPE(AddExplosion, ePacketType::ADD_EXPLOSION, ePacketChannel::EVENT);

public:
    eExplosionType type;
    WorldPositionCompressed pos;
    int time;
    bool usesSound;
    float cameraShake;
    bool isVisible;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_int(stream, (int&)type, EXPLOSION_GRENADE, EXPLOSION_RC_VEHICLE);
        serialize_object(stream, pos);
        serialize_int(stream, time, 0, 100);
        serialize_bool(stream, usesSound);
        serialize_compressed_float(stream, cameraShake, -1.0f, 1.0f, 0.01f);
        serialize_bool(stream, isVisible);

        return true;
    }
};

class TagUpdate : public Packet
{
    DEFINE_PACKET_TYPE(TagUpdate, ePacketType::TAG_UPDATE, ePacketChannel::EVENT);

public:
    struct Payload
    {
        int16_t pos_x = 0;
        int16_t pos_y = 0;
        int16_t pos_z = 0;
        bool bFullySprayed = false;
        uint8_t alpha = 0;
    } payload;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_int(stream, payload.pos_x, -3000, 3000);
        serialize_int(stream, payload.pos_y, -3000, 3000);
        serialize_int(stream, payload.pos_z, -3000, 3000);

        serialize_bool(stream, payload.bFullySprayed);
        if (payload.bFullySprayed)
        {
            payload.alpha = 255;
        }
        else
        {
            if (Stream::IsWriting)
            {
                uint8_t temp = payload.alpha / 8;
                serialize_int(stream, temp, 0, 31);
            }
            else if (Stream::IsReading)
            {
                uint8_t temp;
                serialize_int(stream, temp, 0, 31);
                payload.alpha = temp * 8;
            }
        }

        return true;
    }
};

class UpdateAllTags : public Packet
{
    DEFINE_PACKET_TYPE(UpdateAllTags, ePacketType::UPDATE_ALL_TAGS, ePacketChannel::EVENT);

public:
    TagUpdate::Payload tags[100];

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        for (size_t i = 0; i < ARRAY_SIZE(tags); i++)
        {
            serialize_int(stream, tags[i].pos_x, -3000, 3000);
            serialize_int(stream, tags[i].pos_y, -3000, 3000);
            serialize_int(stream, tags[i].pos_z, -3000, 3000);
            bool bIsNull = false;
            bool bFullySprayed = false;
            if (Stream::IsWriting)
            {
                if (tags[i].alpha == 0)
                {
                    bIsNull = true;
                }
                serialize_bool(stream, bIsNull);
                if (bIsNull)
                {
                    continue;
                }

                if (tags[i].alpha == 255)
                {
                    bFullySprayed = true;
                }
                serialize_bool(stream, bFullySprayed);
                if (bFullySprayed)
                {
                    continue;
                }
                uint8_t temp = tags[i].alpha / 8;
                serialize_int(stream, temp, 0, 31);
            }
            else if (Stream::IsReading)
            {
                serialize_bool(stream, bIsNull);
                if (bIsNull)
                {
                    tags[i].alpha = 0;
                    continue;
                }

                serialize_bool(stream, bFullySprayed);
                if (bFullySprayed)
                {
                    tags[i].alpha = 255;
                    continue;
                }

                uint8_t temp;
                serialize_int(stream, temp, 0, 31);
                tags[i].alpha = temp * 8;
            }
        }
        return true;
    }
};

class UpdateMoonSize : public Packet
{
    DEFINE_PACKET_TYPE(UpdateMoonSize, ePacketType::UPDATE_MOON_SIZE, ePacketChannel::EVENT);

public:
    uint8_t moonSize;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_int(stream, moonSize, 0, 7);
        return true;
    }
};

// a pickup dropped in the world by its owner (money/weapons of a dead ped); netId = (creator playerid << 24) | counter
class PickupCreate : public Packet
{
    DEFINE_PACKET_TYPE(PickupCreate, ePacketType::PICKUP_CREATE, ePacketChannel::EVENT);

public:
    uint32_t netId = 0;
    uint16_t modelId = 0;
    uint8_t pickupType = 0;
    uint32_t ammo = 0;
    uint16_t moneyPerDay = 0;
    CVector pos{};

    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_uint32(stream, netId);
        serialize_uint16(stream, modelId);
        serialize_uint8(stream, pickupType);
        serialize_uint32(stream, ammo);
        serialize_uint16(stream, moneyPerDay);
        serialize_float(stream, pos.x);
        serialize_float(stream, pos.y);
        serialize_float(stream, pos.z);
        return true;
    }
};

// the pickup was collected by someone: remove it everywhere
class PickupRemove : public Packet
{
    DEFINE_PACKET_TYPE(PickupRemove, ePacketType::PICKUP_REMOVE, ePacketChannel::EVENT);

public:
    uint32_t netId = 0;

    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_uint32(stream, netId);
        return true;
    }
};
}  // namespace Packets::World
