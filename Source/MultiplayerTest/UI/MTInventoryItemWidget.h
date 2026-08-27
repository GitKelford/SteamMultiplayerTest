#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/MTInventoryTypes.h"
#include "MTInventoryItemWidget.generated.h"

class UImage;
class UMTInventoryComponent;
class UTextBlock;

UCLASS()
class MULTIPLAYERTEST_API UMTInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeItem(const FMTInventoryEntry& InEntry, UMTInventoryComponent* InInventory);
	void SetDragVisualSize(FVector2D InSize) { DragVisualSize = InSize; }

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event,
		UDragDropOperation*& Operation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

private:
	UPROPERTY()
	FMTInventoryEntry Entry;

	TWeakObjectPtr<UMTInventoryComponent> Inventory;
	FVector2D GrabOffset;
	FVector2D DragVisualSize = FVector2D::ZeroVector;
};
