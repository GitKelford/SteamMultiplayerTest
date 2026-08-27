#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Online/MTSessionTypes.h"
#include "MTServerBrowserWidget.generated.h"

class UButton;
class UMTServerRowWidget;
class UMTSessionSubsystem;
class UTextBlock;
class UVerticalBox;

UCLASS()
class MULTIPLAYERTEST_API UMTServerBrowserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMTServerRowWidget> RowWidgetClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ResultsBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

private:
	UFUNCTION()
	void Refresh();

	UFUNCTION()
	void Results(const TArray<FMTSessionInfo>& Found);

	UFUNCTION()
	void Error(const FString& Message);

	UFUNCTION()
	void OperationChanged(EMTSessionOperation Operation);

	TWeakObjectPtr<UMTSessionSubsystem> Sessions;
};
