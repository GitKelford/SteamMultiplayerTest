#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MTPlayerController.generated.h"

class UUserWidget;

UCLASS()
class MULTIPLAYERTEST_API AMTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMTPlayerController();
	virtual void SetPawn(APawn* InPawn) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Crosshair")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Crosshair")
	bool bShowCrosshair = true;

	UFUNCTION(BlueprintCallable, Category = "UI|Crosshair")
	void RefreshCrosshair();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FString MainMenuMapPath = TEXT("/Game/Maps/L_MainMenu");

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetInventoryOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleInventory();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsInventoryOpen() const
	{
		return InventoryWidget != nullptr;
	}

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_Pawn() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CrosshairWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MainMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> InventoryWidget;

	bool bInventoryInputBlocked = false;

	void RemoveCrosshair();
	void RefreshLocalUI();
};
