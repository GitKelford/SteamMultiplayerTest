#include "Inventory/MTInventoryGrid.h"

#include "Items/MTItemData.h"

bool MTInventoryGrid::IsValidSize(const FIntPoint Size)
{
	return Size.X > 0 && Size.Y > 0 && Size.X <= Width && Size.Y <= Height;
}

bool MTInventoryGrid::IsAreaFree(const TArray<FMTInventoryEntry>& Items, const FIntPoint Position, const FIntPoint Size,
								 const FGuid IgnoreHandle)
{
	if (!IsValidSize(Size) || Position.X < 0 || Position.Y < 0 || Position.X > Width - Size.X ||
		Position.Y > Height - Size.Y)
	{
		return false;
	}

	for (const FMTInventoryEntry& Entry : Items)
	{
		if (IgnoreHandle.IsValid() && Entry.Handle == IgnoreHandle)
		{
			continue;
		}
		if (!IsValid(Entry.ItemData) || !IsValidSize(Entry.ItemData->GridSize))
		{
			return false;
		}

		const FIntPoint OtherSize = Entry.ItemData->GridSize;
		if (Position.X < Entry.Position.X + OtherSize.X && Position.X + Size.X > Entry.Position.X &&
			Position.Y < Entry.Position.Y + OtherSize.Y && Position.Y + Size.Y > Entry.Position.Y)
		{
			return false;
		}
	}

	return true;
}

bool MTInventoryGrid::FindFirstAvailablePosition(const TArray<FMTInventoryEntry>& Items, const FIntPoint Size,
												 FIntPoint& OutPosition)
{
	OutPosition = FIntPoint(INDEX_NONE, INDEX_NONE);
	if (!IsValidSize(Size))
	{
		return false;
	}

	for (int32 Y = 0; Y <= Height - Size.Y; ++Y)
	{
		for (int32 X = 0; X <= Width - Size.X; ++X)
		{
			if (IsAreaFree(Items, FIntPoint(X, Y), Size))
			{
				OutPosition = FIntPoint(X, Y);
				return true;
			}
		}
	}

	return false;
}

bool MTInventoryGrid::TryAddItem(TArray<FMTInventoryEntry>& Items, UMTItemData* ItemData)
{
	FIntPoint Position;
	if (!IsValid(ItemData) || !FindFirstAvailablePosition(Items, ItemData->GridSize, Position))
	{
		return false;
	}

	FMTInventoryEntry Entry;
	Entry.Handle = FGuid::NewGuid();
	Entry.ItemData = ItemData;
	Entry.Position = Position;
	Items.Add(Entry);
	return true;
}

bool MTInventoryGrid::TryMoveItem(TArray<FMTInventoryEntry>& Items, const FGuid Handle, const FIntPoint Position)
{
	FMTInventoryEntry* Entry =
		Items.FindByPredicate([Handle](const FMTInventoryEntry& Item) { return Item.Handle == Handle; });
	if (!Entry || !IsValid(Entry->ItemData) || !IsAreaFree(Items, Position, Entry->ItemData->GridSize, Handle))
	{
		return false;
	}

	Entry->Position = Position;
	return true;
}

bool MTInventoryGrid::RemoveItem(TArray<FMTInventoryEntry>& Items, const FGuid Handle)
{
	const int32 Index =
		Items.IndexOfByPredicate([Handle](const FMTInventoryEntry& Entry) { return Entry.Handle == Handle; });
	if (Index == INDEX_NONE)
	{
		return false;
	}

	Items.RemoveAt(Index);
	return true;
}
