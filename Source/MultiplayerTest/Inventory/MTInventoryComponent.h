#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/MTInventoryTypes.h"
#include "MTInventoryComponent.generated.h"
class UMTItemData;
class AMTItemPickup;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMTInventoryChanged);

UCLASS(ClassGroup = (MultiplayerTest), meta = (BlueprintSpawnableComponent))
class MULTIPLAYERTEST_API UMTInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UMTInventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool TryAddItem(UMTItemData* ItemData);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool TryMoveItem(FGuid Handle, FIntPoint Position);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool RemoveItem(FGuid Handle);
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FMTInventoryEntry> GetInventoryItems() const
	{
		return Items;
	}
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetGridWidth() const
	{
		return 8;
	}
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetGridHeight() const
	{
		return 8;
	}
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsEmpty() const
	{
		return Items.IsEmpty();
	}
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsAreaFree(FIntPoint Position, FIntPoint Size, FGuid IgnoreHandle) const;
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanPlaceItem(UMTItemData* ItemData, FIntPoint Position, FGuid IgnoreHandle) const;
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool FindFirstAvailablePosition(FIntPoint Size, FIntPoint& OutPosition) const;
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItem(UMTItemData* ItemData) const;
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RequestMoveItem(FGuid Handle, FIntPoint Position);
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RequestDropItem(FGuid Handle);
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FMTInventoryChanged OnInventoryChanged;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Drop")
	TSubclassOf<AMTItemPickup> DroppedPickupClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Drop", meta = (ClampMin = "0", Units = "cm"))
	float DropDistance = 150.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Drop", meta = (Units = "cm"))
	float DropHeightOffset = 20.0f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FMTInventoryEntry> Items;
	bool bMutationInProgress = false;
	UFUNCTION()
	void OnRep_Items();
	UFUNCTION(Server, Reliable)
	void ServerMoveItem(FGuid Handle, FIntPoint Position);
	UFUNCTION(Server, Reliable)
	void ServerDropItem(FGuid Handle);
	bool HasOwningPlayerAuthority() const;
	void NotifyInventoryChanged();
};
