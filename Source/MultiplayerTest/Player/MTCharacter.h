#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MTCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UMTInteractionComponent;
class UMTInventoryComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UMaterialInterface;
class UMaterialInstanceDynamic;
struct FInputActionValue;

UCLASS()
class MULTIPLAYERTEST_API AMTCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMTCharacter();
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Restart() override;
	virtual void PawnClientRestart() override;
	virtual void UnPossessed() override;
	virtual FVector GetPawnViewLocation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Debug", meta = (DisplayName = "isDebug"))
	bool bIsDebug = false;

	UFUNCTION(BlueprintPure, Category = "Components")
	UMTInventoryComponent* GetInventoryComponent() const
	{
		return InventoryComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Components")
	UMTInteractionComponent* GetInteractionComponent() const
	{
		return InteractionComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Components")
	USkeletalMeshComponent* GetFirstPersonMesh() const
	{
		return FirstPersonMesh;
	}

	void SetInteractionOutlineInventoryFull(bool bInventoryFull);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_Controller() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Outline")
	TSoftObjectPtr<UMaterialInterface> InteractionOutlineMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Outline")
	FLinearColor InteractionOutlineColor = FLinearColor(1.0f, 0.85f, 0.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Outline")
	FLinearColor InventoryFullOutlineColor = FLinearColor::Red;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Outline",
			  meta = (ClampMin = "1.0", ClampMax = "5.0"))
	float InteractionOutlineThickness = 2.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMTInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMTInventoryComponent> InventoryComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InventoryAction;

private:
	void InitializeLocalFeatures();
	void RemoveLocalFeatures();
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> AppliedInputSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OutlineMaterialInstance;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Interact();
	void ToggleInventory();
};
