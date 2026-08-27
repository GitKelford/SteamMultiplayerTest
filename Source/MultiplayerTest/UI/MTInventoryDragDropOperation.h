#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "MTInventoryDragDropOperation.generated.h"

class UMTInventoryComponent;

UCLASS()
class MULTIPLAYERTEST_API UMTInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	FGuid Handle;
	FVector2D GrabOffset = FVector2D::ZeroVector;
	TWeakObjectPtr<UMTInventoryComponent> Source;
	bool bSubmitted = false;
};
