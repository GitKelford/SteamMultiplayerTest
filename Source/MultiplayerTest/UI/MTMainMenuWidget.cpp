#include "UI/MTMainMenuWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Online/MTSessionSubsystem.h"

void UMTMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	Sessions = GetGameInstance()->GetSubsystem<UMTSessionSubsystem>();
	HostButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Host);
	QuitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Quit);
	if (Sessions.IsValid())
	{
		Sessions->OnSessionError.AddUniqueDynamic(this, &ThisClass::Error);
		Sessions->OnSessionOperationChanged.AddUniqueDynamic(this, &ThisClass::OperationChanged);
		OperationChanged(Sessions->GetCurrentOperation());
		const FString PreviousError = Sessions->GetLastError();
		if (!PreviousError.IsEmpty())
		{
			Error(PreviousError);
		}
	}
}

void UMTMainMenuWidget::NativeDestruct()
{
	if (Sessions.IsValid())
	{
		Sessions->OnSessionError.RemoveDynamic(this, &ThisClass::Error);
		Sessions->OnSessionOperationChanged.RemoveDynamic(this, &ThisClass::OperationChanged);
	}
	Super::NativeDestruct();
}

void UMTMainMenuWidget::OperationChanged(const EMTSessionOperation Operation)
{
	const bool bSessionIdle = Sessions.IsValid() && Operation == EMTSessionOperation::Idle;
	HostButton->SetIsEnabled(bSessionIdle);
	PublicCheck->SetIsEnabled(bSessionIdle);
	if (Operation == EMTSessionOperation::Joining)
	{
		StatusText->SetText(NSLOCTEXT("MultiplayerUI", "ConnectingToServer", "Connecting to server..."));
	}
	else if (Operation == EMTSessionOperation::Creating)
	{
		StatusText->SetText(NSLOCTEXT("MultiplayerUI", "CreatingLobby", "Creating lobby..."));
	}
}

void UMTMainMenuWidget::Host()
{
	if (!Sessions.IsValid() || Sessions->GetCurrentOperation() != EMTSessionOperation::Idle)
	{
		return;
	}
	StatusText->SetText(NSLOCTEXT("MultiplayerUI", "CreatingLobby", "Creating lobby..."));
	Sessions->CreateSession(GameplayMapPath, 2, PublicCheck->IsChecked());
}

void UMTMainMenuWidget::Quit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UMTMainMenuWidget::Error(const FString& Message)
{
	StatusText->SetText(FText::FromString(Message));
}
