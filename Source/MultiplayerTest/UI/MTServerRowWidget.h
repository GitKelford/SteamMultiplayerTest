#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Online/MTSessionTypes.h"
#include "MTServerRowWidget.generated.h"

class UButton;
class UMTSessionSubsystem;
class UTextBlock;

UCLASS()
class MULTIPLAYERTEST_API UMTServerRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeRow(const FMTSessionInfo& Info);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> JoinButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ServerText;

private:
	UFUNCTION()
	void OperationChanged(EMTSessionOperation Operation);

	UFUNCTION()
	void Join();

	void RefreshLabel();

	FMTSessionInfo ServerInfo;
	int32 SessionIndex = INDEX_NONE;
	TWeakObjectPtr<UMTSessionSubsystem> Sessions;
};
