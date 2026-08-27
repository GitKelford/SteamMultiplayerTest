#include "UI/MTInventoryWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Inventory/MTInventoryComponent.h"
#include "Items/MTItemData.h"
#include "Online/MTSessionSubsystem.h"
#include "Player/MTCharacter.h"
#include "Player/MTPlayerController.h"
#include "UI/MTInventoryDragDropOperation.h"
#include "UI/MTInventoryItemWidget.h"

void UMTInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	AMTCharacter* Character = Cast<AMTCharacter>(GetOwningPlayerPawn());
	Inventory = Character ? Character->GetInventoryComponent() : nullptr;
	if (Inventory.IsValid())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshItems);
	}
	CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Close);
	LeaveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::LeaveSession);
	RefreshItems();
}

void UMTInventoryWidget::NativeDestruct()
{
	if (Inventory.IsValid())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshItems);
	}
	Super::NativeDestruct();
}

void UMTInventoryWidget::RefreshItems()
{
	if (!Inventory.IsValid() || !ItemWidgetClass)
	{
		return;
	}
	CellSize = FMath::Clamp(CellSize, 24.0f, 100.0f);
	GridSizeBox->SetWidthOverride(CellSize * 8);
	GridSizeBox->SetHeightOverride(CellSize * 8);
	InventoryCanvas->ClearChildren();
	for (const FMTInventoryEntry& Entry : Inventory->GetInventoryItems())
	{
		if (!IsValid(Entry.ItemData))
		{
			continue;
		}
		UMTInventoryItemWidget* Item = CreateWidget<UMTInventoryItemWidget>(GetOwningPlayer(), ItemWidgetClass);
		if (!Item)
		{
			continue;
		}
		UCanvasPanelSlot* ItemSlot = InventoryCanvas->AddChildToCanvas(Item);
		ItemSlot->SetAutoSize(false);
		ItemSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		ItemSlot->SetAlignment(FVector2D::ZeroVector);

		const FVector2D BasePosition(Entry.Position.X * CellSize, Entry.Position.Y * CellSize);

		const FVector2D FullSize = FVector2D(Entry.ItemData->GridSize) * CellSize;

		const FVector2D VisualPosition = BasePosition + ItemVisualInset;

		const FVector2D VisualSize = FullSize - ItemVisualInset * 2.0f;

		ItemSlot->SetPosition(VisualPosition);
		ItemSlot->SetSize(VisualSize);
		ItemSlot->SetZOrder(1);

		Item->SetDragVisualSize(VisualSize);
		Item->InitializeItem(Entry, Inventory.Get());
	}
	DropPreview->SetVisibility(ESlateVisibility::Collapsed);
	StatusText->SetText(
		FText::Format(NSLOCTEXT("MultiplayerUI", "InventoryStatus", "Items: {0} | Drag: LMB | Drop: outside grid"),
					  FText::AsNumber(Inventory->GetInventoryItems().Num())));
}

bool UMTInventoryWidget::ResolveDrop(const FDragDropEvent& Event, UMTInventoryDragDropOperation* Drag,
									 FIntPoint& Position, bool& bInside) const
{
	if (!Drag || Drag->bSubmitted || !Inventory.IsValid() || Drag->Source != Inventory)
	{
		return false;
	}
	const FGeometry& CanvasGeometry = InventoryCanvas->GetCachedGeometry();
	const FVector2D Mouse = CanvasGeometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	bInside = Mouse.X >= 0 && Mouse.Y >= 0 && Mouse.X < CanvasGeometry.GetLocalSize().X &&
			  Mouse.Y < CanvasGeometry.GetLocalSize().Y;
	const FVector2D TopLeft = (Mouse - Drag->GrabOffset) / CellSize;
	Position = FIntPoint(FMath::FloorToInt(TopLeft.X), FMath::FloorToInt(TopLeft.Y));
	return true;
}

bool UMTInventoryWidget::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event,
									  UDragDropOperation* Operation)
{
	UMTInventoryDragDropOperation* Drag = Cast<UMTInventoryDragDropOperation>(Operation);
	FIntPoint Position;
	bool bInside = false;
	if (!ResolveDrop(Event, Drag, Position, bInside))
	{
		return false;
	}
	Drag->bSubmitted = true;
	if (bInside)
	{
		Inventory->RequestMoveItem(Drag->Handle, Position);
	}
	else
	{
		Inventory->RequestDropItem(Drag->Handle);
	}
	DropPreview->SetVisibility(ESlateVisibility::Collapsed);
	return true;
}

bool UMTInventoryWidget::NativeOnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event,
										  UDragDropOperation* Operation)
{
	UMTInventoryDragDropOperation* Drag = Cast<UMTInventoryDragDropOperation>(Operation);
	FIntPoint Position;
	bool bInside = false;
	if (!ResolveDrop(Event, Drag, Position, bInside))
	{
		return false;
	}
	const TArray<FMTInventoryEntry> Items = Inventory->GetInventoryItems();
	const FMTInventoryEntry* Entry =
		Items.FindByPredicate([Drag](const FMTInventoryEntry& Item) { return Item.Handle == Drag->Handle; });
	if (!bInside || !Entry || !IsValid(Entry->ItemData))
	{
		DropPreview->SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}
	const bool bValid = Inventory->CanPlaceItem(Entry->ItemData, Position, Drag->Handle);
	UCanvasPanelSlot* PreviewSlot = CastChecked<UCanvasPanelSlot>(DropPreview->Slot);

	const FVector2D PreviewBasePosition = FVector2D(Position) * CellSize;

	const FVector2D PreviewFullSize = FVector2D(Entry->ItemData->GridSize) * CellSize;

	PreviewSlot->SetPosition(PreviewBasePosition + ItemVisualInset);

	PreviewSlot->SetSize(PreviewFullSize - ItemVisualInset * 2.0f);

	DropPreview->SetColorAndOpacity(bValid ? ValidDropColor : InvalidDropColor);
	DropPreview->SetVisibility(ESlateVisibility::HitTestInvisible);
	return true;
}

void UMTInventoryWidget::NativeOnDragLeave(const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	DropPreview->SetVisibility(ESlateVisibility::Collapsed);
	Super::NativeOnDragLeave(Event, Operation);
}

FReply UMTInventoryWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::F)
	{
		UWidgetBlueprintLibrary::CancelDragDrop();
		Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

void UMTInventoryWidget::Close()
{
	if (AMTPlayerController* PlayerController = Cast<AMTPlayerController>(GetOwningPlayer()))
	{
		PlayerController->SetInventoryOpen(false);
	}
}

void UMTInventoryWidget::LeaveSession()
{
	UWidgetBlueprintLibrary::CancelDragDrop();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMTSessionSubsystem* Sessions = GameInstance->GetSubsystem<UMTSessionSubsystem>())
		{
			LeaveButton->SetIsEnabled(false);
			Sessions->LeaveSession(MainMenuMapPath);
		}
	}
}
