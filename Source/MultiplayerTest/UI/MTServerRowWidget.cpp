#include "UI/MTServerRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Online/MTSessionSubsystem.h"

void UMTServerRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	JoinButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Join);
	Sessions = GetGameInstance()->GetSubsystem<UMTSessionSubsystem>();
	if (Sessions.IsValid())
	{
		Sessions->OnSessionOperationChanged.AddUniqueDynamic(this, &ThisClass::OperationChanged);
		OperationChanged(Sessions->GetCurrentOperation());
	}
}

void UMTServerRowWidget::NativeDestruct()
{
	if (Sessions.IsValid())
	{
		Sessions->OnSessionOperationChanged.RemoveDynamic(this, &ThisClass::OperationChanged);
	}
	Super::NativeDestruct();
}

void UMTServerRowWidget::InitializeRow(const FMTSessionInfo& Info)
{
	SessionIndex = Info.SessionIndex;
	ServerInfo = Info;
	RefreshLabel();
}

void UMTServerRowWidget::OperationChanged(const EMTSessionOperation Operation)
{
	JoinButton->SetIsEnabled(Sessions.IsValid() && Operation == EMTSessionOperation::Idle);
}

void UMTServerRowWidget::RefreshLabel()
{
	const FString Ping = ServerInfo.PingMilliseconds >= 0
							 ? FString::Printf(TEXT("%d ms RTT"), ServerInfo.PingMilliseconds)
							 : TEXT("Ping: N/A");
	ServerText->SetText(FText::FromString(FString::Printf(TEXT("%s   %d/%d   %s"), *ServerInfo.ServerName,
														  ServerInfo.CurrentPlayers, ServerInfo.MaxPlayers, *Ping)));
}

void UMTServerRowWidget::Join()
{
	if (Sessions.IsValid() && Sessions->GetCurrentOperation() == EMTSessionOperation::Idle)
	{
		Sessions->JoinSessionByIndex(SessionIndex);
	}
}
