#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MTInteractableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)

class UMTInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class MULTIPLAYERTEST_API IMTInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FVector GetInteractionLocation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool IsInventoryStorable() const;
	virtual bool IsInventoryStorable_Implementation() const
	{
		return false;
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool IsInventoryFullFor(AActor* Interactor) const;
	virtual bool IsInventoryFullFor_Implementation(AActor* Interactor) const
	{
		return false;
	}
};
