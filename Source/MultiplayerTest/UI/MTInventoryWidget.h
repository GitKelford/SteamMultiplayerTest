#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MTInventoryWidget.generated.h"

class UButton;
class UCanvasPanel;
class UImage;
class UMTInventoryComponent;
class UMTInventoryDragDropOperation;
class UMTInventoryItemWidget;
class UMTSessionSubsystem;
class USizeBox;
class UTextBlock;

UCLASS()
class MULTIPLAYERTEST_API UMTInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMTInventoryItemWidget> ItemWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI", meta = (ClampMin = "24", ClampMax = "100"))
	float CellSize = 56.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FLinearColor ValidDropColor = FLinearColor(0.1f, 0.8f, 0.5f, 0.45f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FLinearColor InvalidDropColor = FLinearColor(0.9f, 0.15f, 0.15f, 0.45f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FVector2D ItemVisualInset = FVector2D(1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Session")
	FString MainMenuMapPath = TEXT("/Game/Maps/L_MainMenu");

	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshItems();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event,
							  UDragDropOperation* Operation) override;
	virtual bool NativeOnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event,
								  UDragDropOperation* Operation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& Event, UDragDropOperation* Operation) override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> InventoryCanvas;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> GridSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LeaveButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> DropPreview;

private:
	UFUNCTION()
	void Close();

	UFUNCTION()
	void LeaveSession();

	bool ResolveDrop(const FDragDropEvent& Event, UMTInventoryDragDropOperation* Drag, FIntPoint& Position,
					 bool& bInside) const;

	TWeakObjectPtr<UMTInventoryComponent> Inventory;
};
