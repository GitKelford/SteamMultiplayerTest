#include "Online/MTSessionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "MTLog.h"
#include "Online/MTSessionFilter.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "UObject/UObjectGlobals.h"

namespace MTSession
{
	constexpr double TravelTimeout = 90.0;
	constexpr double RecoveryTimeout = 15.0;

	bool IsSteamDriver(const UNetDriver* Driver)
	{
		if (!Driver)
		{
			return false;
		}

		const FString DriverName = Driver->GetClass()->GetName();
		return DriverName == TEXT("SteamNetDriver") || DriverName == TEXT("SteamSocketsNetDriver");
	}

	bool HasSteamDriver()
	{
		if (!GEngine)
		{
			return false;
		}

		const FNetDriverDefinition* Definition = GEngine->NetDriverDefinitions.FindByPredicate(
			[](const FNetDriverDefinition& Entry) { return Entry.DefName == NAME_GameNetDriver; });
		if (!Definition)
		{
			return false;
		}

		UClass* DriverClass = LoadClass<UNetDriver>(nullptr, *Definition->DriverClassName.ToString());
		const UNetDriver* Driver = DriverClass ? DriverClass->GetDefaultObject<UNetDriver>() : nullptr;
		return IsSteamDriver(Driver) && Driver->IsAvailable();
	}
}

void UMTSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
	}

	MapLoadedHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandleMapLoaded);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::TickSession));

	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!OnlineSubsystem || OnlineSubsystem->GetSubsystemName() != FName(TEXT("STEAM")))
	{
		UE_LOG(LogMTOnline, Warning,
			   TEXT("Steam unavailable: session API disabled (offline gameplay is still allowed)."));
		return;
	}

	SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		ReportError(TEXT("Online session interface is unavailable."));
		return;
	}

	InviteDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
		FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::HandleInviteAccepted));
	UE_LOG(LogMTOnline, Log, TEXT("Online subsystem initialized: %s"), *OnlineSubsystem->GetSubsystemName().ToString());
}

void UMTSessionSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(MapLoadedHandle);
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	ClearOperationDelegateHandles();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteDelegateHandle);
	}

	ActiveSearch.Reset();
	SessionInterface.Reset();
	Super::Deinitialize();
}

void UMTSessionSubsystem::SetOperation(const EMTSessionOperation NewOperation)
{
	if (CurrentOperation == NewOperation)
	{
		return;
	}
	CurrentOperation = NewOperation;
	OnSessionOperationChanged.Broadcast(CurrentOperation);
}

bool UMTSessionSubsystem::BeginOperation(const EMTSessionOperation NewOperation)
{
	if (CurrentOperation != EMTSessionOperation::Idle)
	{
		ReportError(TEXT("Another session operation is already in progress."));
		return false;
	}

	if (!SessionInterface.IsValid())
	{
		ReportError(TEXT("Session interface is unavailable."));
		return false;
	}

	SetOperation(NewOperation);
	LastError.Reset();
	return true;
}

void UMTSessionSubsystem::CreateSession(const FString& GameplayMapPath, const int32 MaxPublicConnections,
										const bool bPublicLobby)
{
	if (!MTSession::HasSteamDriver())
	{
		ReportError(
			TEXT("Steam transport is unavailable. Enable the configured Steam socket plugin and restart the game."));
		return;
	}

	if (!BeginOperation(EMTSessionOperation::Creating))
	{
		return;
	}

	PendingGameplayMapPath = GameplayMapPath.IsEmpty() ? DefaultGameplayMapPath : GameplayMapPath;
	if (!FPackageName::IsValidLongPackageName(PendingGameplayMapPath) ||
		!FPackageName::DoesPackageExist(PendingGameplayMapPath))
	{
		SetOperation(EMTSessionOperation::Idle);
		ReportError(FString::Printf(TEXT("CreateSession: map '%s' is invalid or missing from this build. Check the map "
										 "path and Packaging > Maps to Cook."),
									*PendingGameplayMapPath));
		return;
	}

	if (MaxPublicConnections < 1)
	{
		SetOperation(EMTSessionOperation::Idle);
		ReportError(TEXT("MaxPublicConnections must be at least one."));
		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SetOperation(EMTSessionOperation::Idle);
		ReportError(TEXT("A game session already exists; destroy it before creating another."));
		return;
	}

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = FMath::Min(MaxPublicConnections, 2);
	Settings.bIsLANMatch = false;
	Settings.bShouldAdvertise = bPublicLobby;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = bPublicLobby;
	Settings.bAllowInvites = true;
	Settings.bAllowJoinViaPresenceFriendsOnly = false;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bUseLobbiesVoiceChatIfAvailable = false;
	Settings.Set(SETTING_MAPNAME, PendingGameplayMapPath, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(MTSessionFilter::TagKey, MTSessionFilter::TagValue, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(MTSessionFilter::PublicKey, bPublicLobby ? 1 : 0, EOnlineDataAdvertisementType::ViaOnlineService);

	CreateDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));

	UE_LOG(LogMTOnline, Log, TEXT("CreateSession requested for map %s with %d public connections"),
		   *PendingGameplayMapPath, Settings.NumPublicConnections);

	if (!SessionInterface->CreateSession(0, NAME_GameSession, Settings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
		CreateDelegateHandle.Reset();
		SetOperation(EMTSessionOperation::Idle);
		ReportError(TEXT("CreateSession request could not be started."));
	}
}

void UMTSessionSubsystem::HandleCreateSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
	CreateDelegateHandle.Reset();

	UE_LOG(LogMTOnline, Log, TEXT("CreateSession result for %s: %s"), *SessionName.ToString(),
		   bWasSuccessful ? TEXT("success") : TEXT("failure"));

	if (!bWasSuccessful)
	{
		SetOperation(EMTSessionOperation::Idle);
		ReportError(TEXT("Steam session creation failed."));
		return;
	}

	bWaitingForTravel = true;
	TravelDeadline = FPlatformTime::Seconds() + MTSession::TravelTimeout;
	UGameplayStatics::OpenLevel(this, FName(*PendingGameplayMapPath), true, TEXT("listen"));
	OnSessionCreated.Broadcast();
}

void UMTSessionSubsystem::FindSessions(const int32 MaxSearchResults)
{
	if (!BeginOperation(EMTSessionOperation::Searching))
	{
		return;
	}

	ActiveSearch = MakeShared<FOnlineSessionSearch>();
	ActiveSearch->MaxSearchResults = FMath::Clamp(MaxSearchResults, 1, 200);
	ActiveSearch->bIsLanQuery = false;
	ActiveSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	ActiveSearch->QuerySettings.Set(MTSessionFilter::TagKey, MTSessionFilter::TagValue, EOnlineComparisonOp::Equals);
	ActiveSearch->QuerySettings.Set(MTSessionFilter::PublicKey, 1, EOnlineComparisonOp::Equals);

	FindDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));
	UE_LOG(LogMTOnline, Log, TEXT("FindSessions requested (max %d)"), ActiveSearch->MaxSearchResults);

	if (!SessionInterface->FindSessions(0, ActiveSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindDelegateHandle);
		FindDelegateHandle.Reset();
		ActiveSearch.Reset();
		SetOperation(EMTSessionOperation::Idle);
		ReportError(TEXT("FindSessions request could not be started."));
	}
}

void UMTSessionSubsystem::HandleFindSessionsComplete(const bool bWasSuccessful)
{
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindDelegateHandle);
	FindDelegateHandle.Reset();
	SetOperation(EMTSessionOperation::Idle);

	if (!bWasSuccessful || !ActiveSearch.IsValid())
	{
		ActiveSearch.Reset();
		ReportError(TEXT("Steam session search failed."));
		return;
	}

	ActiveSearch->SearchResults.RemoveAll(
		[](const FOnlineSessionSearchResult& Result)
		{ return !Result.IsValid() || !MTSessionFilter::IsPublicLobby(Result.Session.SessionSettings); });

	TArray<FMTSessionInfo> PublicResults;
	PublicResults.Reserve(ActiveSearch->SearchResults.Num());
	for (int32 Index = 0; Index < ActiveSearch->SearchResults.Num(); ++Index)
	{
		const FOnlineSessionSearchResult& Result = ActiveSearch->SearchResults[Index];
		FMTSessionInfo& Info = PublicResults.AddDefaulted_GetRef();
		Info.SessionIndex = Index;
		Info.OwningUserName = Result.Session.OwningUserName;
		Info.ServerName = Info.OwningUserName + TEXT(" - MultiplayerTest");
		Info.bPublic = true;
		Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Info.CurrentPlayers = FMath::Max(0, Info.MaxPlayers - Result.Session.NumOpenPublicConnections);
		Info.PingMilliseconds = Result.PingInMs >= 0 ? Result.PingInMs : INDEX_NONE;
	}

	UE_LOG(LogMTOnline, Log, TEXT("FindSessions completed: %d session(s) found"), PublicResults.Num());
	OnSessionsFound.Broadcast(PublicResults);
}

void UMTSessionSubsystem::JoinSessionByIndex(const int32 SessionIndex)
{
	if (CurrentOperation != EMTSessionOperation::Idle)
	{
		ReportError(TEXT("Another session operation is already in progress."));
		return;
	}

	if (!ActiveSearch.IsValid() || !ActiveSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		ReportError(TEXT("Invalid session result index."));
		return;
	}

	const FOnlineSessionSearchResult& Result = ActiveSearch->SearchResults[SessionIndex];
	if (!MTSessionFilter::IsPublicLobby(Result.Session.SessionSettings))
	{
		ReportError(TEXT("The selected lobby is no longer available."));
		return;
	}

	JoinResult(Result);
}

void UMTSessionSubsystem::HandleInviteAccepted(const bool bSuccess, const int32 ControllerId, FUniqueNetIdPtr UserId,
											   const FOnlineSessionSearchResult& Result)
{
	if (!bSuccess || ControllerId != 0 || !UserId.IsValid())
	{
		return;
	}

	JoinResult(Result);
}

void UMTSessionSubsystem::JoinResult(const FOnlineSessionSearchResult& Result)
{
	if (!Result.IsValid() || !MTSessionFilter::IsProjectLobby(Result.Session.SessionSettings))
	{
		ReportError(TEXT("The selected lobby does not belong to this project."));
		return;
	}

	if (!MTSession::HasSteamDriver())
	{
		ReportError(TEXT("Steam transport is unavailable. IP fallback is not allowed for Steam sessions."));
		return;
	}

	if (!BeginOperation(EMTSessionOperation::Joining))
	{
		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SetOperation(EMTSessionOperation::Idle);
		ReportError(TEXT("Leave the current session before joining another."));
		return;
	}

	JoinDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));
	UE_LOG(LogMTOnline, Log, TEXT("JoinSession requested for project lobby"));

	if (!SessionInterface->JoinSession(0, NAME_GameSession, Result))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinDelegateHandle);
		JoinDelegateHandle.Reset();
		SetOperation(EMTSessionOperation::Idle);
		ReportError(TEXT("JoinSession request could not be started."));
	}
}

void UMTSessionSubsystem::HandleJoinSessionComplete(const FName SessionName,
													const EOnJoinSessionCompleteResult::Type Result)
{
	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinDelegateHandle);
	JoinDelegateHandle.Reset();

	UE_LOG(LogMTOnline, Log, TEXT("JoinSession result for %s: %d"), *SessionName.ToString(),
		   static_cast<int32>(Result));

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		RecoverFromFailure(
			FString::Printf(TEXT("Joining the Steam session failed (result %d)."), static_cast<int32>(Result)));
		return;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString) || ConnectString.IsEmpty())
	{
		RecoverFromFailure(TEXT("Steam session connect string could not be resolved."));
		return;
	}

	if (!ConnectString.StartsWith(TEXT("steam.")))
	{
		RecoverFromFailure(TEXT("The session returned a non-Steam address. Direct IP travel was rejected."));
		return;
	}

	APlayerController* LocalController = GetGameInstance()->GetFirstLocalPlayerController();
	if (!LocalController)
	{
		RecoverFromFailure(TEXT("No local PlayerController is available for ClientTravel."));
		return;
	}

	UE_LOG(LogMTOnline, Log, TEXT("Connect string resolved; starting ClientTravel"));
	bWaitingForTravel = true;
	bJoinedMapLoaded = false;
	TravelDeadline = FPlatformTime::Seconds() + MTSession::TravelTimeout;
	LocalController->ClientTravel(ConnectString, TRAVEL_Absolute);
}

void UMTSessionSubsystem::LeaveSession(const FString& MainMenuMapPath)
{
	if (CurrentOperation != EMTSessionOperation::Idle)
	{
		ReportError(TEXT("Session operation in progress."));
		return;
	}

	if (!FPackageName::IsValidLongPackageName(MainMenuMapPath) || !FPackageName::DoesPackageExist(MainMenuMapPath))
	{
		ReportError(FString::Printf(TEXT("LeaveSession: map '%s' is invalid or missing from this build. Check the map "
										 "path and Packaging > Maps to Cook."),
									*MainMenuMapPath));
		return;
	}

	if (!SessionInterface.IsValid() || !SessionInterface->GetNamedSession(NAME_GameSession))
	{
		UGameplayStatics::OpenLevel(this, FName(*MainMenuMapPath));
		return;
	}

	PendingReturnMapPath = MainMenuMapPath;
	DestroySession();
}

void UMTSessionSubsystem::DestroySession()
{
	if (!BeginOperation(EMTSessionOperation::Destroying))
	{
		return;
	}

	if (!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SetOperation(EMTSessionOperation::Idle);
		PendingReturnMapPath.Reset();
		ReportError(TEXT("There is no game session to destroy."));
		return;
	}

	if (!BeginDestroyRequest())
	{
		SetOperation(EMTSessionOperation::Idle);
		PendingReturnMapPath.Reset();
		ReportError(TEXT("DestroySession request could not be started."));
	}
}

bool UMTSessionSubsystem::BeginDestroyRequest()
{
	DestroyDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));
	if (SessionInterface->DestroySession(NAME_GameSession))
	{
		return true;
	}

	SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
	DestroyDelegateHandle.Reset();
	return false;
}

void UMTSessionSubsystem::HandleDestroySessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
	DestroyDelegateHandle.Reset();

	UE_LOG(LogMTOnline, Log, TEXT("DestroySession result for %s: %s"), *SessionName.ToString(),
		   bWasSuccessful ? TEXT("success") : TEXT("failure"));

	if (bRecovering)
	{
		CompleteRecovery();
		return;
	}

	SetOperation(EMTSessionOperation::Idle);
	if (!bWasSuccessful)
	{
		PendingReturnMapPath.Reset();
		ReportError(TEXT("Destroying the Steam session failed."));
		return;
	}

	ActiveSearch.Reset();
	const FString ReturnMap = MoveTemp(PendingReturnMapPath);
	PendingReturnMapPath.Reset();
	if (!ReturnMap.IsEmpty())
	{
		UGameplayStatics::OpenLevel(this, FName(*ReturnMap));
	}
	OnSessionDestroyed.Broadcast();
}

void UMTSessionSubsystem::ReportError(const FString& Message)
{
	LastError = Message;
	UE_LOG(LogMTOnline, Error, TEXT("%s"), *Message);
	OnSessionError.Broadcast(Message);
}

void UMTSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* Driver, const ENetworkFailure::Type Type,
											   const FString& Error)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	const bool bGameDriver = !Driver || Driver->NetDriverName == NAME_GameNetDriver ||
							 Driver->NetDriverName == FName(TEXT("PendingNetDriver"));
	if (!bGameDriver)
	{
		return;
	}

	const bool bDepartingClient =
		World->GetNetMode() != NM_Client && !bWaitingForTravel &&
		(Type == ENetworkFailure::ConnectionLost || Type == ENetworkFailure::ConnectionTimeout);
	if (bDepartingClient)
	{
		return;
	}

	RecoverFromFailure(FString::Printf(TEXT("Connection failed (%d): %s"), static_cast<int32>(Type), *Error));
}

void UMTSessionSubsystem::HandleTravelFailure(UWorld* World, const ETravelFailure::Type Type, const FString& Error)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	RecoverFromFailure(FString::Printf(TEXT("Map travel failed (%d): %s"), static_cast<int32>(Type), *Error));
}

void UMTSessionSubsystem::HandleMapLoaded(UWorld* World)
{
	if (!World || World->GetGameInstance() != GetGameInstance() || !bWaitingForTravel || bRecovering)
	{
		return;
	}

	if (CurrentOperation == EMTSessionOperation::Joining && World->GetNetMode() == NM_Client)
	{
		bJoinedMapLoaded = true;
		return;
	}

	if (CurrentOperation != EMTSessionOperation::Creating || World->GetNetMode() != NM_ListenServer)
	{
		return;
	}

	if (!MTSession::IsSteamDriver(World->GetNetDriver()))
	{
		RecoverFromFailure(TEXT("Steam listen driver failed and IP fallback was activated. The lobby was closed."));
		return;
	}

	bWaitingForTravel = false;
	SetOperation(EMTSessionOperation::Idle);
}

void UMTSessionSubsystem::RecoverFromFailure(const FString& Error)
{
	if (bRecovering)
	{
		return;
	}

	bRecovering = true;
	bWaitingForTravel = false;
	bJoinedMapLoaded = false;
	SetOperation(EMTSessionOperation::Destroying);
	PendingReturnMapPath.Reset();
	ReportError(Error);

	ClearOperationDelegateHandles();
	if (ActiveSearch.IsValid())
	{
		SessionInterface->CancelFindSessions();
		ActiveSearch.Reset();
	}

	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession) && BeginDestroyRequest())
	{
		RecoveryDeadline = FPlatformTime::Seconds() + MTSession::RecoveryTimeout;
		return;
	}

	CompleteRecovery();
}

void UMTSessionSubsystem::CompleteRecovery()
{
	if (DestroyDelegateHandle.IsValid() && SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
		DestroyDelegateHandle.Reset();
	}

	RecoveryDeadline = 0.0;
	SetOperation(EMTSessionOperation::Idle);
	bRecovering = false;

	if (!FPackageName::IsValidLongPackageName(RecoveryMenuMapPath) ||
		!FPackageName::DoesPackageExist(RecoveryMenuMapPath))
	{
		ReportError(LastError + TEXT(" Recovery menu map is missing from this build."));
		return;
	}

	UGameplayStatics::OpenLevel(this, FName(*RecoveryMenuMapPath));
}

bool UMTSessionSubsystem::TickSession(float)
{
	const double Now = FPlatformTime::Seconds();
	if (bRecovering)
	{
		if (RecoveryDeadline > 0.0 && Now > RecoveryDeadline)
		{
			CompleteRecovery();
		}
		return true;
	}

	if (!bWaitingForTravel)
	{
		return true;
	}

	APlayerController* LocalController = GetGameInstance()->GetFirstLocalPlayerController();
	if (bJoinedMapLoaded && LocalController && LocalController->GetPawn())
	{
		bWaitingForTravel = false;
		bJoinedMapLoaded = false;
		SetOperation(EMTSessionOperation::Idle);
		UE_LOG(LogMTOnline, Log, TEXT("Steam world joined and local pawn possessed"));
		OnSessionJoined.Broadcast();
		return true;
	}

	if (Now > TravelDeadline)
	{
		RecoverFromFailure(TEXT("Timed out waiting for the server world/player. Returning to menu."));
	}

	return true;
}

void UMTSessionSubsystem::ClearOperationDelegateHandles()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinDelegateHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
	}

	CreateDelegateHandle.Reset();
	FindDelegateHandle.Reset();
	JoinDelegateHandle.Reset();
	DestroyDelegateHandle.Reset();
}
