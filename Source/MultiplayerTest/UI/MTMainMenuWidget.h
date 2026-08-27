#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Online/MTSessionTypes.h"
#include "MTMainMenuWidget.generated.h"

class UButton;
class UCheckBox;
class UMTServerBrowserWidget;
class UMTSessionSubsystem;
class UTextBlock;

UCLASS()
class MULTIPLAYERTEST_API UMTMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FString GameplayMapPath = TEXT("/Game/Maps/L_Game");

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HostButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> PublicCheck;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMTServerBrowserWidget> ServerBrowser;

private:
	UFUNCTION()
	void Host();

	UFUNCTION()
	void Quit();

	UFUNCTION()
	void Error(const FString& Message);

	UFUNCTION()
	void OperationChanged(EMTSessionOperation Operation);

	TWeakObjectPtr<UMTSessionSubsystem> Sessions;
};
