#pragma once
#include "Inventory/MTInventoryTypes.h"

namespace MTInventoryGrid
{
	constexpr int32 Width = 8;
	constexpr int32 Height = 8;
	bool IsValidSize(FIntPoint Size);
	bool IsAreaFree(const TArray<FMTInventoryEntry>& Items, FIntPoint Position, FIntPoint Size,
					FGuid IgnoreHandle = FGuid());
	bool FindFirstAvailablePosition(const TArray<FMTInventoryEntry>& Items, FIntPoint Size, FIntPoint& OutPosition);
	bool TryAddItem(TArray<FMTInventoryEntry>& Items, UMTItemData* ItemData);
	bool TryMoveItem(TArray<FMTInventoryEntry>& Items, FGuid Handle, FIntPoint Position);
	bool RemoveItem(TArray<FMTInventoryEntry>& Items, FGuid Handle);
}
