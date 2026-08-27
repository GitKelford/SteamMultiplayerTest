#include "Inventory/MTInventoryComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/MTInventoryGrid.h"
#include "Items/MTItemData.h"
#include "Items/MTItemPickup.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UMTInventoryComponent::UMTInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMTInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UMTInventoryComponent, Items, COND_OwnerOnly);
}

bool UMTInventoryComponent::IsAreaFree(FIntPoint Position, FIntPoint Size, FGuid IgnoreHandle) const
{
	return MTInventoryGrid::IsAreaFree(Items, Position, Size, IgnoreHandle);
}

bool UMTInventoryComponent::CanPlaceItem(UMTItemData* ItemData, FIntPoint Position, FGuid IgnoreHandle) const
{
	return IsValid(ItemData) && IsAreaFree(Position, ItemData->GridSize, IgnoreHandle);
}

bool UMTInventoryComponent::FindFirstAvailablePosition(FIntPoint Size, FIntPoint& OutPosition) const
{
	return MTInventoryGrid::FindFirstAvailablePosition(Items, Size, OutPosition);
}

bool UMTInventoryComponent::CanAddItem(UMTItemData* ItemData) const
{
	FIntPoint Position;
	return IsValid(ItemData) && FindFirstAvailablePosition(ItemData->GridSize, Position);
}

bool UMTInventoryComponent::TryAddItem(UMTItemData* ItemData)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || bMutationInProgress)
	{
		return false;
	}
	TGuardValue<bool> Guard(bMutationInProgress, true);
	if (!MTInventoryGrid::TryAddItem(Items, ItemData))
	{
		return false;
	}
	NotifyInventoryChanged();
	return true;
}

bool UMTInventoryComponent::TryMoveItem(FGuid Handle, FIntPoint Position)
{
	if (!HasOwningPlayerAuthority() || bMutationInProgress)
	{
		return false;
	}
	TGuardValue<bool> Guard(bMutationInProgress, true);
	if (!MTInventoryGrid::TryMoveItem(Items, Handle, Position))
	{
		return false;
	}
	NotifyInventoryChanged();
	return true;
}

bool UMTInventoryComponent::RemoveItem(FGuid Handle)
{
	if (!HasOwningPlayerAuthority() || bMutationInProgress)
	{
		return false;
	}
	TGuardValue<bool> Guard(bMutationInProgress, true);
	if (!MTInventoryGrid::RemoveItem(Items, Handle))
	{
		return false;
	}
	NotifyInventoryChanged();
	return true;
}

bool UMTInventoryComponent::HasOwningPlayerAuthority() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	return Pawn && Pawn->HasAuthority() && PlayerController && PlayerController->GetPawn() == Pawn &&
		   Pawn->GetOwner() == PlayerController;
}

void UMTInventoryComponent::RequestMoveItem(FGuid Handle, FIntPoint Position)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled())
	{
		ServerMoveItem(Handle, Position);
	}
}

void UMTInventoryComponent::RequestDropItem(FGuid Handle)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled())
	{
		ServerDropItem(Handle);
	}
}

void UMTInventoryComponent::ServerMoveItem_Implementation(FGuid Handle, FIntPoint Position)
{
	TryMoveItem(Handle, Position);
}

void UMTInventoryComponent::ServerDropItem_Implementation(FGuid Handle)
{
	if (!HasOwningPlayerAuthority() || bMutationInProgress)
	{
		return;
	}
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	const FMTInventoryEntry* Entry =
		Items.FindByPredicate([Handle](const FMTInventoryEntry& Item) { return Item.Handle == Handle; });
	if (!Character || !Entry || !IsValid(Entry->ItemData))
	{
		return;
	}
	UMTItemData* Item = Entry->ItemData;
	TSubclassOf<AMTItemPickup> Class = Item->PickupClass ? Item->PickupClass : DroppedPickupClass;
	if (!Class || Class->HasAnyClassFlags(CLASS_Abstract))
	{
		return;
	}
	TGuardValue<bool> Guard(bMutationInProgress, true);

	const float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	const FVector Location = Character->GetActorLocation() + Forward * FMath::Max(DropDistance, Radius + 60.0f) +
							 FVector(0, 0, DropHeightOffset);
	FHitResult Obstacle;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(MTDrop), false, Character);
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (World->LineTraceSingleByChannel(Obstacle, Character->GetActorLocation(), Location, ECC_Visibility, Query))
	{
		return;
	}

	const FTransform Transform(Character->GetActorRotation(), Location);
	AMTItemPickup* Pickup = World->SpawnActorDeferred<AMTItemPickup>(
		Class, Transform, nullptr, Character,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (!Pickup)
	{
		return;
	}
	Pickup->InitializeDroppedItem(Item);
	UGameplayStatics::FinishSpawningActor(Pickup, Transform);
	if (!IsValid(Pickup) || Pickup->IsActorBeingDestroyed())
	{
		return;
	}

	FVector Origin, Extent;
	Pickup->GetActorBounds(true, Origin, Extent);
	const FVector Delta = Origin - Character->GetActorLocation();
	const bool bInsideCapsule =
		Delta.SizeSquared2D() < FMath::Square(Radius + FVector2D(Extent.X, Extent.Y).Size()) &&
		FMath::Abs(Delta.Z) < Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + Extent.Z;
	if (bInsideCapsule || Pickup->GetItemData() != Item || !Pickup->HasPickupVisual() ||
		!MTInventoryGrid::RemoveItem(Items, Handle))
	{
		Pickup->Destroy();
		return;
	}
	Pickup->ReleaseDropClaim();
	Pickup->ForceNetUpdate();
	NotifyInventoryChanged();
}

void UMTInventoryComponent::OnRep_Items()
{
	OnInventoryChanged.Broadcast();
}

void UMTInventoryComponent::NotifyInventoryChanged()
{
	GetOwner()->ForceNetUpdate();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled())
	{
		OnInventoryChanged.Broadcast();
	}
}
