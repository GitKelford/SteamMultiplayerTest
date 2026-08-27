#include "Items/MTItemPickup.h"

#include "Components/StaticMeshComponent.h"
#include "Inventory/MTInventoryComponent.h"
#include "Interaction/MTInteractionComponent.h"
#include "Items/MTItemData.h"
#include "MTLog.h"
#include "Net/UnrealNetwork.h"
#include "Player/MTCharacter.h"

AMTItemPickup::AMTItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	SetRootComponent(PickupMesh);
	PickupMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void AMTItemPickup::BeginPlay()
{
	Super::BeginPlay();
	ApplyItemVisual();
	UMTInteractionComponent::InitializePrompts(this);
}

void AMTItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMTItemPickup, ItemData);
}

bool AMTItemPickup::CanInteract_Implementation(AActor* Interactor) const
{
	const AMTCharacter* Character = Cast<AMTCharacter>(Interactor);
	const UMTInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
	return !bClaimed && !IsActorBeingDestroyed() && IsValid(ItemData) && Inventory && Inventory->CanAddItem(ItemData);
}

void AMTItemPickup::Interact_Implementation(AActor* Interactor)
{
	if (!HasAuthority() || bClaimed || IsActorBeingDestroyed())
	{
		return;
	}

	AMTCharacter* Character = Cast<AMTCharacter>(Interactor);
	UMTInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
	if (!Inventory || !IsValid(ItemData))
	{
		return;
	}

	bClaimed = true;
	if (Inventory->TryAddItem(ItemData))
	{
		UE_LOG(LogMTInventory, Log, TEXT("Pickup %s acquired by %s"), *GetName(), *GetNameSafe(Character));
		Destroy();
	}
	else
	{
		bClaimed = false;
	}
}

FVector AMTItemPickup::GetInteractionLocation_Implementation() const
{
	return PickupMesh->Bounds.Origin;
}

bool AMTItemPickup::IsInventoryFullFor_Implementation(AActor* Interactor) const
{
	const AMTCharacter* Character = Cast<AMTCharacter>(Interactor);
	const UMTInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
	return !bClaimed && IsValid(ItemData) && Inventory && !Inventory->CanAddItem(ItemData);
}

void AMTItemPickup::InitializeDroppedItem(UMTItemData* Data)
{
	if (!HasAuthority())
	{
		return;
	}
	bClaimed = true;
	ItemData = Data;
	ApplyItemVisual();
}

void AMTItemPickup::ReleaseDropClaim()
{
	if (HasAuthority())
	{
		bClaimed = false;
	}
}

bool AMTItemPickup::HasPickupVisual() const
{
	return PickupMesh->GetStaticMesh() && PickupMesh->IsVisible() && !IsHidden();
}

void AMTItemPickup::ApplyItemVisual()
{
	if (IsValid(ItemData) && !ItemData->WorldMesh.IsNull())
	{
		if (UStaticMesh* Mesh = ItemData->WorldMesh.LoadSynchronous())
		{
			PickupMesh->SetStaticMesh(Mesh);
		}
	}
}

void AMTItemPickup::OnRep_ItemData()
{
	ApplyItemVisual();
}
