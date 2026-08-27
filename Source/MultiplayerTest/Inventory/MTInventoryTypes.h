#pragma once
#include "CoreMinimal.h"
#include "MTInventoryTypes.generated.h"
class UMTItemData;

USTRUCT(BlueprintType)

struct FMTInventoryEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid Handle;
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UMTItemData> ItemData = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint Position = FIntPoint::ZeroValue;
};
