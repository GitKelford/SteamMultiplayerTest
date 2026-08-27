#include "World/MTChestActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Items/MTItemPickup.h"
#include "Interaction/MTInteractionComponent.h"
#include "MTLog.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

AMTChestActor::AMTChestActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ChestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestMesh"));
	SetRootComponent(ChestMesh);
	ChestMesh->SetCollisionProfileName(TEXT("BlockAll"));

	ItemSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ItemSpawnPoint"));
	ItemSpawnPoint->SetupAttachment(ChestMesh);
}

void AMTChestActor::BeginPlay()
{
	Super::BeginPlay();
	UMTInteractionComponent::InitializePrompts(this);
}

void AMTChestActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMTChestActor, bIsOpen);
}

bool AMTChestActor::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor) && !bIsOpen && !bIsOpening && !IsActorBeingDestroyed();
}

void AMTChestActor::Interact_Implementation(AActor* Interactor)
{
	if (!HasAuthority() || bIsOpen || bIsOpening)
	{
		return;
	}

	TGuardValue<bool> OpeningGuard(bIsOpening, true);
	if (!SpawnPickup())
	{
		UE_LOG(LogMTInteraction, Error, TEXT("Chest %s remains closed because its pickup failed to spawn"), *GetName());
		return;
	}

	bIsOpen = true;
	ForceNetUpdate();
	OnChestVisualStateChanged(true);
	UE_LOG(LogMTInteraction, Log, TEXT("Chest %s opened by %s"), *GetName(), *GetNameSafe(Interactor));
	MulticastPlayOpenEffect(ItemSpawnPoint->GetComponentTransform());
}

void AMTChestActor::MulticastPlayOpenEffect_Implementation(FTransform SpawnTransform)
{
	if (GetNetMode() == NM_DedicatedServer || !OpenChestEffect)
	{
		return;
	}
	StopOpenEffect();
	ActiveOpenEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this, OpenChestEffect, SpawnTransform.GetLocation(), SpawnTransform.Rotator(), SpawnTransform.GetScale3D(),
		false, true);
	if (ActiveOpenEffect)
	{
		GetWorldTimerManager().SetTimer(OpenEffectTimer, this, &ThisClass::StopOpenEffect,
										FMath::Max(0.1f, OpenEffectDuration));
	}
}

void AMTChestActor::StopOpenEffect()
{
	if (IsValid(ActiveOpenEffect))
	{
		ActiveOpenEffect->DeactivateImmediate();
		ActiveOpenEffect->DestroyComponent();
	}
	ActiveOpenEffect = nullptr;
}

FVector AMTChestActor::GetInteractionLocation_Implementation() const
{
	return ChestMesh->Bounds.Origin;
}

void AMTChestActor::OnRep_IsOpen()
{
	OnChestVisualStateChanged(bIsOpen);
}

bool AMTChestActor::SpawnPickup()
{
	if (!PickupClass)
	{
		UE_LOG(LogMTInteraction, Warning, TEXT("Chest %s opened without a configured PickupClass"), *GetName());
		return false;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AMTItemPickup* Pickup =
		World->SpawnActor<AMTItemPickup>(PickupClass, ItemSpawnPoint->GetComponentTransform(), Params);
	if (Pickup)
	{
		UE_LOG(LogMTInteraction, Log, TEXT("Chest %s spawned pickup %s"), *GetName(), *Pickup->GetName());
	}
	else
	{
		UE_LOG(LogMTInteraction, Error, TEXT("Chest %s failed to spawn its pickup"), *GetName());
	}
	return Pickup != nullptr;
}
