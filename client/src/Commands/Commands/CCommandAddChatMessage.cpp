#include "stdafx.h"
#include "CCommandAddChatMessage.h"

void CCommandAddChatMessage::Process(CRunningScript* script)
{
	char text[128];
	script->ReadTextLabelFromScript(text, 128);
	text[127] = '\0';

	// missions that were not adapted by hand are synced generically now
	if (strstr(text, "currently unsupported"))
	{
		CChat::AddMessage("{ffff00}[SA Dream Mod]{ffffff} This mission is synced in basic mode: texts, cutscenes, blips and markers.");
		return;
	}
	if (strstr(text, "It may cause desyncs"))
	{
		CChat::AddMessage("{ffff00}[SA Dream Mod]{ffffff} The host completes the objectives, other players help. Some parts may still desync.");
		return;
	}

	CChat::AddMessage(text);
}