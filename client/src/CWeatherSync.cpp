#include "CWeatherSync.h"

void CWeatherSync::SyncCurrentState()
{
	if (!CLocalPlayer::m_bIsHost)
	{
		return;
	}

	Packets::World::GameWeatherTime packet{};
	packet.newWeather = static_cast<eWeatherType>(CWeather::NewWeatherType);
	packet.oldWeather = static_cast<eWeatherType>(CWeather::OldWeatherType);
	packet.forcedWeather = CWeather::ForcedWeatherType;
	packet.interpolation = std::clamp(CWeather::InterpolationValue, 0.0f, 1.0f);
	packet.currentMonth = CClock::ms_nGameClockMonth;
	packet.currentDay = CClock::CurrentDay;
	packet.currentHour = CClock::ms_nGameClockHours;
	packet.currentMinute = CClock::ms_nGameClockMinutes;
	packet.currentSecond = static_cast<uint8_t>(std::min<unsigned short>(CClock::ms_nGameClockSeconds, 59));
	GetPacketFactory().Send(packet);
}

void CWeatherSync::Process()
{
	if (!CLocalPlayer::m_bIsHost)
		return;

	static short lastOld = -2, lastNew = -2, lastForced = -2;
	static uint32_t lastSent = 0;
	bool changed = CWeather::OldWeatherType != lastOld || CWeather::NewWeatherType != lastNew || CWeather::ForcedWeatherType != lastForced;
	if (!changed && GetTickCount() - lastSent < 2000)
		return;

	lastOld = CWeather::OldWeatherType;
	lastNew = CWeather::NewWeatherType;
	lastForced = CWeather::ForcedWeatherType;
	lastSent = GetTickCount();
	SyncCurrentState();
}

void CWeatherSync::HandlePacket(Packets::World::GameWeatherTime* pGameWeatherTime)
{
	ms_bHasHostState = true;
	ms_nHostOld = pGameWeatherTime->oldWeather;
	ms_nHostNew = pGameWeatherTime->newWeather;
	ms_nHostForced = static_cast<short>(pGameWeatherTime->forcedWeather);
	ApplyClient();
	CWeather::InterpolationValue = pGameWeatherTime->interpolation;

	CClock::ms_nGameClockMonth = pGameWeatherTime->currentMonth;
	CClock::CurrentDay = pGameWeatherTime->currentDay;

	// the clocks run at the same speed: only correct a real difference, small corrections made the light jump
	int host = pGameWeatherTime->currentHour * 3600 + pGameWeatherTime->currentMinute * 60 + pGameWeatherTime->currentSecond;
	int local = CClock::ms_nGameClockHours * 3600 + CClock::ms_nGameClockMinutes * 60 + CClock::ms_nGameClockSeconds;
	int diff = abs(host - local);
	diff = std::min(diff, 24 * 3600 - diff);
	if (diff > 90)
	{
		CClock::ms_nGameClockHours = pGameWeatherTime->currentHour;
		CClock::ms_nGameClockMinutes = pGameWeatherTime->currentMinute;
		CClock::ms_nGameClockSeconds = pGameWeatherTime->currentSecond;
	}
}

void CWeatherSync::ApplyClient()
{
	if (!ms_bHasHostState || CLocalPlayer::m_bIsHost)
		return;

	CWeather::OldWeatherType = ms_nHostOld;
	CWeather::NewWeatherType = ms_nHostNew;
	CWeather::ForcedWeatherType = ms_nHostForced;
}

void CWeatherSync::ServerTimeRecalculated(server_time_t serverTime)
{
	CWaterLevel::m_nWaterTimeOffset = CTimer::m_snTimeInMilliseconds - serverTime;
}
