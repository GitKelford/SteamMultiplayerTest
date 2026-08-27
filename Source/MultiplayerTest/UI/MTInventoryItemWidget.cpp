#include "UI/MTInventoryItemWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Inventory/MTInventoryComponent.h"
#include "Items/MTItemData.h"
#include "UI/MTInventoryDragDropOperation.h"

void UMTInventoryItemWidget::InitializeItem(const FMTInventoryEntry& InEntry, UMTInventoryComponent* InInventory)
{
	Entry = InEntry;
	Inventory = InInventory;
	if (!IsValid(Entry.ItemData))
	{
		return;
	}
	SetToolTipText(Entry.ItemData->DisplayName);
	ItemText->SetText(Entry.ItemData->DisplayName);
	UTexture2D* Icon = Entry.ItemData->Icon.LoadSynchronous();
	ItemText->SetVisibility(Icon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	ItemIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Icon)
	{
		ItemIcon->SetBrushFromTexture(Icon);
	}
}

FReply UMTInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	GrabOffset = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	return UWidgetBlueprintLibrary::DetectDragIfPressed(Event, this, EKeys::LeftMouseButton).NativeReply;
}

void UMTInventoryItemWidget::NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event,
												  UDragDropOperation*& Operation)
{
	if (!Inventory.IsValid() || !Entry.Handle.IsValid())
	{
		return;
	}
	UMTInventoryDragDropOperation* Drag = NewObject<UMTInventoryDragDropOperation>();
	Drag->Handle = Entry.Handle;
	Drag->Source = Inventory;
	Drag->GrabOffset = GrabOffset;
	UMTInventoryItemWidget* Visual = CreateWidget<UMTInventoryItemWidget>(GetOwningPlayer(), GetClass());
	if (Visual)
	{
		Visual->SetDragVisualSize(DragVisualSize);
		Visual->InitializeItem(Entry, Inventory.Get());
		Visual->SetVisibility(ESlateVisibility::HitTestInvisible);

		USizeBox* DragSizeBox = NewObject<USizeBox>(Drag);
		const FVector2D Size = DragVisualSize.IsNearlyZero() ? Geometry.GetLocalSize() : DragVisualSize;
		DragSizeBox->SetWidthOverride(Size.X);
		DragSizeBox->SetHeightOverride(Size.Y);
		DragSizeBox->AddChild(Visual);
		Drag->DefaultDragVisual = DragSizeBox;
	}
	Drag->Pivot = EDragPivot::MouseDown;
	Operation = Drag;
}
