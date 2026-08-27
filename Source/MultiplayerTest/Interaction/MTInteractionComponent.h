#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "MTInteractionComponent.generated.h"

class UWidgetComponent;

struct FMTOutlinePrimitiveState
{
	TWeakObjectPtr<UPrimitiveComponent> Primitive;
	bool bRenderCustomDepth = false;
	int32 StencilValue = 0;
	ERendererStencilMask StencilWriteMask = ERendererStencilMask::ERSM_Default;
};

UCLASS(ClassGroup = (MultiplayerTest), meta = (BlueprintSpawnableComponent))
class MULTIPLAYERTEST_API UMTInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMTInteractionComponent();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedActor() const
	{
		return FocusedActor.Get();
	}

	void StartLocalFocus();
	void StopLocalFocus();

	static void InitializePrompts(AActor* Actor);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "50.0", Units = "cm"))
	float InteractionDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

private:
	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* TargetActor);

	void UpdateLocalFocus();
	void SetFocusedActor(AActor* NewFocus, bool bInventoryFull = false);
	void HideFocusedPrompts();
	void ShowFocusedPrompts(AActor* Target);
	AActor* TraceForInteractable(FHitResult& Hit, FVector& Start, FVector& End) const;
	bool CanReachTarget(AActor* TargetActor, FString& Reason, bool bDrawValidation = false) const;
	bool ValidateServerInteraction(AActor* TargetActor, FString& Reason) const;
	bool IsDebugEnabled() const;
	void DrawTraceDebug(const FHitResult& Hit, const FVector& Start, const FVector& End, AActor* Candidate,
						const FString& Reason, float Lifetime) const;

	FTimerHandle FocusTimer;
	TWeakObjectPtr<AActor> FocusedActor;
	bool bFocusedInventoryFull = false;
	TArray<FMTOutlinePrimitiveState> OutlinedPrimitives;

	TArray<TWeakObjectPtr<UWidgetComponent>> FocusedPrompts;
};
