#include "Interaction/MTInteractionComponent.h"

#include "DrawDebugHelpers.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/MTInteractableInterface.h"
#include "MTLog.h"
#include "Player/MTCharacter.h"
#include "TimerManager.h"

namespace MTInteraction
{
	constexpr int32 OutlineStencil = 252;
	constexpr int32 InventoryFullStencil = 253;
	constexpr float FocusInterval = 0.05f;
	const FName PromptTag(TEXT("InteractionPrompt"));
}

UMTInteractionComponent::UMTInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMTInteractionComponent::StartLocalFocus()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled() && GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(FocusTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(FocusTimer, this, &ThisClass::UpdateLocalFocus,
											   MTInteraction::FocusInterval, true);
	}
}

void UMTInteractionComponent::StopLocalFocus()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FocusTimer);
	}
	SetFocusedActor(nullptr);
}

void UMTInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopLocalFocus();
	Super::EndPlay(EndPlayReason);
}

void UMTInteractionComponent::UpdateLocalFocus()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		StopLocalFocus();
		return;
	}

	FHitResult Hit;
	FVector Start, End;

	AActor* Candidate = TraceForInteractable(Hit, Start, End);
	FString Reason;

	if (Candidate && !CanReachTarget(Candidate, Reason))
	{
		Candidate = nullptr;
	}

	bool bInventoryFull = false;

	if (Candidate)
	{
		const bool bIsInventoryStorable = IMTInteractableInterface::Execute_IsInventoryStorable(Candidate);

		bInventoryFull =
			bIsInventoryStorable && IMTInteractableInterface::Execute_IsInventoryFullFor(Candidate, GetOwner());

		const bool bCanInteract = IMTInteractableInterface::Execute_CanInteract(Candidate, GetOwner());

		if (!bCanInteract && !bInventoryFull)
		{
			Candidate = nullptr;
		}
	}

	SetFocusedActor(Candidate, bInventoryFull);

	if (IsDebugEnabled())
	{
		DrawTraceDebug(Hit, Start, End, Candidate, Reason, MTInteraction::FocusInterval);
	}
}

void UMTInteractionComponent::SetFocusedActor(AActor* NewFocus, const bool bInventoryFull)
{
	const bool bSameFocus = FocusedActor.Get() == NewFocus && bFocusedInventoryFull == bInventoryFull;
	const bool bFocusStateReady = NewFocus ? !OutlinedPrimitives.IsEmpty()
										 : (OutlinedPrimitives.IsEmpty() && FocusedPrompts.IsEmpty());
	if (bSameFocus && bFocusStateReady)
	{
		return;
	}

	HideFocusedPrompts();

	for (const FMTOutlinePrimitiveState& State : OutlinedPrimitives)
	{
		if (UPrimitiveComponent* Primitive = State.Primitive.Get())
		{
			Primitive->SetRenderCustomDepth(State.bRenderCustomDepth);
			Primitive->SetCustomDepthStencilValue(State.StencilValue);
			Primitive->SetCustomDepthStencilWriteMask(State.StencilWriteMask);
		}
	}
	OutlinedPrimitives.Reset();
	FocusedActor = NewFocus;
	bFocusedInventoryFull = bInventoryFull;
	if (AMTCharacter* Character = Cast<AMTCharacter>(GetOwner()))
	{
		Character->SetInteractionOutlineInventoryFull(bInventoryFull);
	}
	if (!IsValid(NewFocus))
	{
		return;
	}
	ShowFocusedPrompts(NewFocus);

	TInlineComponentArray<UPrimitiveComponent*> Primitives(NewFocus);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (!Primitive->IsVisible() || Primitive->IsA<UWidgetComponent>())
		{
			continue;
		}
		FMTOutlinePrimitiveState& State = OutlinedPrimitives.AddDefaulted_GetRef();
		State.Primitive = Primitive;
		State.bRenderCustomDepth = Primitive->bRenderCustomDepth;
		State.StencilValue = Primitive->CustomDepthStencilValue;
		State.StencilWriteMask = Primitive->CustomDepthStencilWriteMask;
		Primitive->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_255);
		Primitive->SetCustomDepthStencilValue(bInventoryFull ? MTInteraction::InventoryFullStencil
															 : MTInteraction::OutlineStencil);
		Primitive->SetRenderCustomDepth(true);
		Primitive->MarkRenderStateDirty();
	}
}

void UMTInteractionComponent::HideFocusedPrompts()
{
	for (const TWeakObjectPtr<UWidgetComponent>& Entry : FocusedPrompts)
	{
		if (UWidgetComponent* Prompt = Entry.Get())
		{
			Prompt->SetVisibility(false);
			Prompt->SetHiddenInGame(true);
		}
	}
	FocusedPrompts.Reset();
}

void UMTInteractionComponent::InitializePrompts(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}
	TInlineComponentArray<UWidgetComponent*> Prompts(Actor);
	for (UWidgetComponent* Prompt : Prompts)
	{
		if (!Prompt->ComponentHasTag(MTInteraction::PromptTag))
		{
			continue;
		}
		Prompt->SetVisibility(false);
		Prompt->SetHiddenInGame(true);
	}
}

void UMTInteractionComponent::ShowFocusedPrompts(AActor* Target)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!IsValid(Target) || !Pawn || !Pawn->IsLocallyControlled() || !Controller || !Controller->GetLocalPlayer())
	{
		return;
	}

	TInlineComponentArray<UWidgetComponent*> Prompts(Target);
	for (UWidgetComponent* Prompt : Prompts)
	{
		if (!Prompt->ComponentHasTag(MTInteraction::PromptTag))
		{
			continue;
		}

		Prompt->SetOwnerPlayer(Controller->GetLocalPlayer());
		Prompt->InitWidget();
		Prompt->SetHiddenInGame(false);
		Prompt->SetVisibility(true);
		FocusedPrompts.Add(Prompt);
	}
}

void UMTInteractionComponent::TryInteract()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	FHitResult Hit;
	FVector Start, End;
	AActor* Target = TraceForInteractable(Hit, Start, End);
	FString Reason;
	const bool bCanRequest = Target && CanReachTarget(Target, Reason, true) &&
							 IMTInteractableInterface::Execute_CanInteract(Target, GetOwner());
	if (IsDebugEnabled())
	{
		DrawTraceDebug(Hit, Start, End, bCanRequest ? Target : nullptr, Reason, 2.0f);
	}
	if (bCanRequest)
	{
		ServerInteract(Target);
	}
	UpdateLocalFocus();
}

AActor* UMTInteractionComponent::TraceForInteractable(FHitResult& Hit, FVector& Start, FVector& End) const
{
	Start = End = FVector::ZeroVector;
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const APlayerController* Controller = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!Controller || !GetWorld())
	{
		return nullptr;
	}

	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(Start, ViewRotation);
	End = Start + ViewRotation.Vector() * FMath::Max(0.0f, InteractionDistance);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MTInteractionTrace), false, Character);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params) && IsValid(Hit.GetActor()) &&
		Hit.GetActor()->Implements<UMTInteractableInterface>())
	{
		return Hit.GetActor();
	}
	return nullptr;
}

bool UMTInteractionComponent::CanReachTarget(AActor* TargetActor, FString& Reason, const bool bDrawValidation) const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !GetWorld() || !IsValid(TargetActor) || TargetActor == Character ||
		TargetActor->IsActorBeingDestroyed() || TargetActor->GetWorld() != GetWorld() ||
		!TargetActor->Implements<UMTInteractableInterface>())
	{
		Reason = TEXT("invalid target");
		return false;
	}

	const FVector Location = IMTInteractableInterface::Execute_GetInteractionLocation(TargetActor);
	const FVector Start = Character->GetPawnViewLocation();
	if (Location.ContainsNaN() || FVector::DistSquared(Start, Location) > FMath::Square(InteractionDistance))
	{
		Reason = TEXT("out of character reach");
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MTReachTrace), false, Character);
	const FVector End = Location + (Location - Start).GetSafeNormal() * 2.0f;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);
	const bool bVisible = bHit && Hit.GetActor() == TargetActor;
	if (!bVisible)
	{
		Reason = TEXT("blocked from pawn / missing trace collision");
	}

	if (bDrawValidation && IsDebugEnabled())
	{
		DrawDebugLine(GetWorld(), Start, End, bVisible ? FColor::Cyan : FColor::Magenta, false, 2.0f, 0, 1.5f);
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.0f, 8, FColor::Cyan, false, 2.0f);
		}
	}
	return bVisible;
}

void UMTInteractionComponent::ServerInteract_Implementation(AActor* TargetActor)
{
	FString Reason;
	if (!ValidateServerInteraction(TargetActor, Reason))
	{
		UE_LOG(LogMTInteraction, Verbose, TEXT("Server rejected %s -> %s: %s"), *GetNameSafe(GetOwner()),
			   *GetNameSafe(TargetActor), *Reason);
		return;
	}
	UE_LOG(LogMTInteraction, Log, TEXT("Server accepted %s -> %s"), *GetNameSafe(GetOwner()),
		   *GetNameSafe(TargetActor));
	IMTInteractableInterface::Execute_Interact(TargetActor, GetOwner());
}

bool UMTInteractionComponent::ValidateServerInteraction(AActor* TargetActor, FString& Reason) const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const APlayerController* Controller = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!Character || !Character->HasAuthority() || !Controller || Controller->GetPawn() != Character ||
		Character->GetOwner() != Controller)
	{
		Reason = TEXT("invalid owning player");
		return false;
	}
	if (!CanReachTarget(TargetActor, Reason, true))
	{
		return false;
	}
	if (!IMTInteractableInterface::Execute_CanInteract(TargetActor, GetOwner()))
	{
		Reason = TEXT("target unavailable");
		return false;
	}
	return true;
}

bool UMTInteractionComponent::IsDebugEnabled() const
{
	const AMTCharacter* Character = Cast<AMTCharacter>(GetOwner());
	return Character && Character->bIsDebug;
}

void UMTInteractionComponent::DrawTraceDebug(const FHitResult& Hit, const FVector& Start, const FVector& End,
											 AActor* Candidate, const FString& Reason, const float Lifetime) const
{
	const FColor Color = Candidate ? FColor::Green : (Hit.bBlockingHit ? FColor::Yellow : FColor::Red);
	const FVector Impact = Hit.bBlockingHit ? Hit.ImpactPoint : End;
	DrawDebugLine(GetWorld(), Start, Impact, Color, false, Lifetime, 0, 1.5f);
	DrawDebugPoint(GetWorld(), Impact, 12.0f, Color, false, Lifetime);
	if (Hit.bBlockingHit)
	{
		DrawDebugDirectionalArrow(GetWorld(), Impact, Impact + Hit.ImpactNormal * 25.0f, 6.0f, Color, false, Lifetime);
	}
	const FString Label =
		FString::Printf(TEXT("Interact hit: %s | %s"), *GetNameSafe(Hit.GetActor()),
						Candidate ? TEXT("READY") : (Reason.IsEmpty() ? TEXT("no usable target") : *Reason));
	DrawDebugString(GetWorld(), Impact + FVector(0, 0, 12), Label, nullptr, Color, Lifetime, false);
}
