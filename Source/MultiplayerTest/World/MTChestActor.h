#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MTInteractableInterface.h"
#include "MTChestActor.generated.h"

class AMTItemPickup;
class USceneComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class MULTIPLAYERTEST_API AMTChestActor : public AActor, public IMTInteractableInterface
{
	GENERATED_BODY()

public:
	AMTChestActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FVector GetInteractionLocation_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "Chest")
	bool IsOpen() const
	{
		return bIsOpen;
	}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ChestMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ItemSpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest")
	TSubclassOf<AMTItemPickup> PickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Effects")
	TObjectPtr<UNiagaraSystem> OpenChestEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Effects", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float OpenEffectDuration = 1.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Chest", meta = (DisplayName = "On Chest Visual State Changed"))
	void OnChestVisualStateChanged(bool bOpen);

private:
	UPROPERTY(ReplicatedUsing = OnRep_IsOpen)
	bool bIsOpen = false;

	UFUNCTION()
	void OnRep_IsOpen();

	bool SpawnPickup();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayOpenEffect(FTransform SpawnTransform);

	void StopOpenEffect();

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveOpenEffect;

	FTimerHandle OpenEffectTimer;
	bool bIsOpening = false;
};
