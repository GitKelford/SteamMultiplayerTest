#include "Player/MTPlayerController.h"

#include "Player/MTCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Player/MTPlayerCameraManager.h"

AMTPlayerController::AMTPlayerController()
{
	PlayerCameraManagerClass = AMTPlayerCameraManager::StaticClass();
}

void AMTPlayerController::BeginPlay()
{
	Super::BeginPlay();
	RefreshLocalUI();
	RefreshCrosshair();
}

void AMTPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	if (HasActorBegunPlay())
	{
		RefreshLocalUI();
		RefreshCrosshair();
	}
}

void AMTPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	RefreshLocalUI();
	RefreshCrosshair();
}

void AMTPlayerController::RefreshCrosshair()
{
	if (!IsLocalController() || !Cast<AMTCharacter>(GetPawn()) || !bShowCrosshair || !CrosshairWidgetClass ||
		IsInventoryOpen())
	{
		RemoveCrosshair();
		return;
	}
	if (CrosshairWidget && CrosshairWidget->GetClass() != CrosshairWidgetClass)
	{
		RemoveCrosshair();
	}
	if (!CrosshairWidget)
	{
		CrosshairWidget = CreateWidget<UUserWidget>(this, CrosshairWidgetClass);
		if (CrosshairWidget)
		{
			CrosshairWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			CrosshairWidget->AddToPlayerScreen();
		}
	}
}

void AMTPlayerController::RemoveCrosshair()
{
	if (CrosshairWidget)
	{
		CrosshairWidget->RemoveFromParent();
	}
	CrosshairWidget = nullptr;
}

void AMTPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetInventoryOpen(false);
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
	}
	MainMenuWidget = nullptr;
	RemoveCrosshair();
	Super::EndPlay(EndPlayReason);
}

void AMTPlayerController::RefreshLocalUI()
{
	if (!IsLocalController())
	{
		return;
	}
	const bool bMenu = UGameplayStatics::GetCurrentLevelName(this, true) == FPackageName::GetShortName(MainMenuMapPath);
	if (bMenu && MainMenuWidgetClass && !MainMenuWidget)
	{
		MainMenuWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToPlayerScreen();
			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
			SetInputMode(Mode);
			bShowMouseCursor = true;
		}
	}
	if (!Cast<AMTCharacter>(GetPawn()))
	{
		SetInventoryOpen(false);
	}
	if (!bMenu)
	{
		if (MainMenuWidget)
		{
			MainMenuWidget->RemoveFromParent();
			MainMenuWidget = nullptr;
		}

		if (!IsInventoryOpen())
		{
			SetInputMode(FInputModeGameOnly());
			bShowMouseCursor = false;
		}
	}
}

void AMTPlayerController::ToggleInventory()
{
	SetInventoryOpen(!IsInventoryOpen());
}

void AMTPlayerController::SetInventoryOpen(bool bOpen)
{
	if (!IsLocalController())
	{
		return;
	}
	if (bOpen && !InventoryWidget && Cast<AMTCharacter>(GetPawn()) && InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
		if (!InventoryWidget)
		{
			return;
		}
		InventoryWidget->AddToPlayerScreen(10);
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		bShowMouseCursor = true;
		if (!bInventoryInputBlocked)
		{
			SetIgnoreMoveInput(true);
			SetIgnoreLookInput(true);
			bInventoryInputBlocked = true;
		}
	}
	else if (!bOpen && InventoryWidget)
	{
		UWidgetBlueprintLibrary::CancelDragDrop();
		InventoryWidget->RemoveFromParent();
		InventoryWidget = nullptr;
		if (bInventoryInputBlocked)
		{
			SetIgnoreMoveInput(false);
			SetIgnoreLookInput(false);
			bInventoryInputBlocked = false;
		}
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
	RefreshCrosshair();
}
