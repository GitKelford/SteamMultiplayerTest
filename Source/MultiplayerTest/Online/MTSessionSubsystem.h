#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/EngineBaseTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/MTSessionTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MTSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMTSessionCreated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMTSessionsFound, const TArray<FMTSessionInfo>&, Sessions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMTSessionJoined);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMTSessionDestroyed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMTSessionError, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMTSessionOperationChanged, EMTSessionOperation, Operation);

UCLASS(Config = Game)
class MULTIPLAYERTEST_API UMTSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Multiplayer|Session")
	void CreateSession(const FString& GameplayMapPath, int32 MaxPublicConnections = 2, bool bPublicLobby = true);

	UFUNCTION(BlueprintCallable, Category = "Multiplayer|Session")
	void FindSessions(int32 MaxSearchResults = 50);

	UFUNCTION(BlueprintCallable, Category = "Multiplayer|Session")
	void JoinSessionByIndex(int32 SessionIndex);

	UFUNCTION(BlueprintCallable, Category = "Multiplayer|Session")
	void DestroySession();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer|Session")
	void LeaveSession(const FString& MainMenuMapPath);

	UFUNCTION(BlueprintPure, Category = "Multiplayer|Session")
	EMTSessionOperation GetCurrentOperation() const
	{
		return CurrentOperation;
	}

	UFUNCTION(BlueprintPure, Category = "Multiplayer|Session")
	FString GetLastError() const
	{
		return LastError;
	}

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Session")
	FMTSessionCreated OnSessionCreated;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Session")
	FMTSessionsFound OnSessionsFound;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Session")
	FMTSessionJoined OnSessionJoined;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Session")
	FMTSessionDestroyed OnSessionDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Session")
	FMTSessionError OnSessionError;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Session")
	FMTSessionOperationChanged OnSessionOperationChanged;

protected:
	UPROPERTY(Config, EditDefaultsOnly, Category = "Multiplayer|Session")
	FString DefaultGameplayMapPath;

	UPROPERTY(Config, EditDefaultsOnly, Category = "Multiplayer|Session")
	FString RecoveryMenuMapPath = TEXT("/Game/Maps/L_MainMenu");

private:
	bool BeginOperation(EMTSessionOperation NewOperation);
	void SetOperation(EMTSessionOperation NewOperation);
	bool BeginDestroyRequest();
	bool TickSession(float);
	void JoinResult(const FOnlineSessionSearchResult& Result);
	void RecoverFromFailure(const FString& Error);
	void CompleteRecovery();
	void ReportError(const FString& Message);
	void ClearOperationDelegateHandles();

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleInviteAccepted(bool bSuccess, int32 ControllerId, FUniqueNetIdPtr UserId,
							  const FOnlineSessionSearchResult& Result);
	void HandleNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error);
	void HandleMapLoaded(UWorld* World);

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> ActiveSearch;

	FString PendingGameplayMapPath;
	FString PendingReturnMapPath;
	FString LastError;
	EMTSessionOperation CurrentOperation = EMTSessionOperation::Idle;
	double TravelDeadline = 0.0;
	double RecoveryDeadline = 0.0;
	bool bWaitingForTravel = false;
	bool bJoinedMapLoaded = false;
	bool bRecovering = false;

	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	FDelegateHandle MapLoadedHandle;
	FTSTicker::FDelegateHandle TickHandle;

	FDelegateHandle CreateDelegateHandle;
	FDelegateHandle FindDelegateHandle;
	FDelegateHandle JoinDelegateHandle;
	FDelegateHandle DestroyDelegateHandle;
	FDelegateHandle InviteDelegateHandle;
};
