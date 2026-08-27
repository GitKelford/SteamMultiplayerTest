#include "Items/MTItemData.h"

FPrimaryAssetId UMTItemData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("MTItem"), ItemId.IsNone() ? GetFName() : ItemId);
}
