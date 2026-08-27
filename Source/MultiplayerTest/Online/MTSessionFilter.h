#pragma once
#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"

namespace MTSessionFilter
{
	inline const FName TagKey(TEXT("MPSteamTag"));
	inline const FName PublicKey(TEXT("MPSteamPublic"));
	inline const FString TagValue(TEXT("UE5.8_MPSteamTest"));

	inline bool IsProjectLobby(const FOnlineSessionSettings& Settings)
	{
		FString Tag;
		return Settings.Get(TagKey, Tag) && Tag.Equals(TagValue, ESearchCase::CaseSensitive);
	}

	inline bool IsPublicLobby(const FOnlineSessionSettings& Settings)
	{
		int32 Public = 0;
		return IsProjectLobby(Settings) && Settings.bShouldAdvertise && Settings.Get(PublicKey, Public) && Public == 1;
	}
}
