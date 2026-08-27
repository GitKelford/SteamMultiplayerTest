#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MTInteractableInterface.h"
#include "MTItemPickup.generated.h"

class UMTItemData;
class UStaticMeshComponent;

UCLASS()
class MULTIPLAYERTEST_API AMTItemPickup : public AActor, public IMTInteractableInterface
{
	GENERATED_BODY()

public:
	AMTItemPickup();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FVector GetInteractionLocation_Implementation() const override;
	virtual bool IsInventoryStorable_Implementation() const override
	{
		return true;
	}
	virtual bool IsInventoryFullFor_Implementation(AActor* Interactor) const override;

	void InitializeDroppedItem(UMTItemData* Data);
	void ReleaseDropClaim();
	bool HasPickupVisual() const;
	UFUNCTION(BlueprintPure, Category = "Item")
	UMTItemData* GetItemData() const
	{
		return ItemData;
	}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ItemData, Category = "Item")
	TObjectPtr<UMTItemData> ItemData;

private:
	UFUNCTION()
	void OnRep_ItemData();
	void ApplyItemVisual();
	bool bClaimed = false;
};
