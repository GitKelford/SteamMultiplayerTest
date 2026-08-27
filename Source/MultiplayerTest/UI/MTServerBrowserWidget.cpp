#include "UI/MTServerBrowserWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Online/MTSessionSubsystem.h"
#include "UI/MTServerRowWidget.h"

void UMTServerBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Sessions = GetGameInstance()->GetSubsystem<UMTSessionSubsystem>();
	RefreshButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Refresh);
	if (Sessions.IsValid())
	{
		Sessions->OnSessionsFound.AddUniqueDynamic(this, &ThisClass::Results);
		Sessions->OnSessionError.AddUniqueDynamic(this, &ThisClass::Error);
		Sessions->OnSessionOperationChanged.AddUniqueDynamic(this, &ThisClass::OperationChanged);
		OperationChanged(Sessions->GetCurrentOperation());
	}
}

void UMTServerBrowserWidget::NativeDestruct()
{
	if (Sessions.IsValid())
	{
		Sessions->OnSessionsFound.RemoveDynamic(this, &ThisClass::Results);
		Sessions->OnSessionError.RemoveDynamic(this, &ThisClass::Error);
		Sessions->OnSessionOperationChanged.RemoveDynamic(this, &ThisClass::OperationChanged);
	}
	Super::NativeDestruct();
}

void UMTServerBrowserWidget::OperationChanged(const EMTSessionOperation Operation)
{
	RefreshButton->SetIsEnabled(Sessions.IsValid() && Operation == EMTSessionOperation::Idle);
}

void UMTServerBrowserWidget::Refresh()
{
	if (!Sessions.IsValid() || Sessions->GetCurrentOperation() != EMTSessionOperation::Idle)
	{
		return;
	}
	ResultsBox->ClearChildren();
	StatusText->SetText(NSLOCTEXT("MultiplayerUI", "SearchingLobbies", "Searching public lobbies..."));
	Sessions->FindSessions();
}

void UMTServerBrowserWidget::Results(const TArray<FMTSessionInfo>& Found)
{
	ResultsBox->ClearChildren();
	StatusText->SetText(
		FText::Format(NSLOCTEXT("MultiplayerUI", "ServersFound", "Servers found: {0}"), FText::AsNumber(Found.Num())));
	for (const FMTSessionInfo& Info : Found)
	{
		UMTServerRowWidget* Row = CreateWidget<UMTServerRowWidget>(GetOwningPlayer(), RowWidgetClass);
		if (Row)
		{
			ResultsBox->AddChildToVerticalBox(Row);
			Row->InitializeRow(Info);
		}
	}
}

void UMTServerBrowserWidget::Error(const FString& Message)
{
	StatusText->SetText(FText::FromString(Message));
}
