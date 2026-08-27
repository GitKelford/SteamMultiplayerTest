#pragma once

#include "CoreMinimal.h"
#include "MTSessionTypes.generated.h"

UENUM(BlueprintType)
enum class EMTSessionOperation : uint8
{
	Idle,
	Creating,
	Searching,
	Joining,
	Destroying
};

USTRUCT(BlueprintType)

struct FMTSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 SessionIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString OwningUserName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString ServerName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bPublic = false;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 PingMilliseconds = 0;
};
