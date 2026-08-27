#include "Player/MTCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Interaction/MTInteractionComponent.h"
#include "Inventory/MTInventoryComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MTLog.h"
#include "Player/MTPlayerController.h"

AMTCharacter::AMTCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	BaseEyeHeight = 64.0f;

	GetCapsuleComponent()->InitCapsuleSize(34.0f, 96.0f);
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(TEXT("NoCollision"));
	FirstPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(FirstPersonMesh, TEXT("head"));
	FollowCamera->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FollowCamera->bUsePawnControlRotation = true;
	FollowCamera->bEnableFirstPersonFieldOfView = true;
	FollowCamera->bEnableFirstPersonScale = true;
	FollowCamera->FirstPersonFieldOfView = 70.0f;
	FollowCamera->FirstPersonScale = 0.6f;

	InteractionComponent = CreateDefaultSubobject<UMTInteractionComponent>(TEXT("InteractionComponent"));
	InventoryComponent = CreateDefaultSubobject<UMTInventoryComponent>(TEXT("InventoryComponent"));
	InteractionOutlineMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/MultiplayerTest/Materials/M_PP_InteractionOutline.M_PP_InteractionOutline")));
}

void AMTCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeLocalFeatures();
}

FVector AMTCharacter::GetPawnViewLocation() const
{
	return FollowCamera->GetComponentLocation();
}

void AMTCharacter::Restart()
{
	Super::Restart();
	InitializeLocalFeatures();
}

void AMTCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	InitializeLocalFeatures();
}

void AMTCharacter::UnPossessed()
{
	RemoveLocalFeatures();
	Super::UnPossessed();
}

void AMTCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	if (IsLocallyControlled())
	{
		InitializeLocalFeatures();
	}
	else
	{
		RemoveLocalFeatures();
	}
}

void AMTCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveLocalFeatures();
	Super::EndPlay(EndPlayReason);
}

void AMTCharacter::InitializeLocalFeatures()
{
	if (!HasActorBegunPlay() || !IsLocallyControlled())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (InputSubsystem && DefaultMappingContext && AppliedInputSubsystem.Get() != InputSubsystem)
	{
		InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
		AppliedInputSubsystem = InputSubsystem;
	}

	if (!OutlineMaterialInstance && !InteractionOutlineMaterial.IsNull())
	{
		if (UMaterialInterface* Material = InteractionOutlineMaterial.LoadSynchronous())
		{
			OutlineMaterialInstance = UMaterialInstanceDynamic::Create(Material, this);
			OutlineMaterialInstance->SetVectorParameterValue(TEXT("OutlineColor"), InteractionOutlineColor);
			OutlineMaterialInstance->SetScalarParameterValue(TEXT("Thickness"), InteractionOutlineThickness);
			OutlineMaterialInstance->SetScalarParameterValue(TEXT("SelectedStencil"), 252.0f);
			FollowCamera->AddOrUpdateBlendable(OutlineMaterialInstance, 1.0f);
		}
		else
		{
			UE_LOG(LogMTInteraction, Warning, TEXT("Interaction outline material is missing: %s"),
				   *InteractionOutlineMaterial.ToString());
		}
	}
	InteractionComponent->StartLocalFocus();
}

void AMTCharacter::RemoveLocalFeatures()
{
	InteractionComponent->StopLocalFocus();
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = AppliedInputSubsystem.Get())
	{
		if (DefaultMappingContext)
		{
			InputSubsystem->RemoveMappingContext(DefaultMappingContext);
		}
	}
	AppliedInputSubsystem.Reset();
	if (OutlineMaterialInstance)
	{
		FollowCamera->RemoveBlendable(OutlineMaterialInstance);
		OutlineMaterialInstance = nullptr;
	}
}

void AMTCharacter::SetInteractionOutlineInventoryFull(const bool bInventoryFull)
{
	if (!OutlineMaterialInstance)
	{
		return;
	}

	OutlineMaterialInstance->SetVectorParameterValue(
		TEXT("OutlineColor"), bInventoryFull ? InventoryFullOutlineColor : InteractionOutlineColor);
	OutlineMaterialInstance->SetScalarParameterValue(TEXT("SelectedStencil"), bInventoryFull ? 253.0f : 252.0f);
}

void AMTCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogMTInteraction, Error, TEXT("%s requires EnhancedInputComponent; check DefaultInput.ini"), *GetName());
		return;
	}
	InitializeLocalFeatures();

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	}
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	}
	if (MouseLookAction && MouseLookAction != LookAction)
	{
		EnhancedInput->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	}
	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::Interact);
	}
	if (InventoryAction)
	{
		EnhancedInput->BindAction(InventoryAction, ETriggerEvent::Started, this, &ThisClass::ToggleInventory);
	}
}

void AMTCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Movement = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Movement.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Movement.X);
}

void AMTCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void AMTCharacter::Interact()
{
	if (const AMTPlayerController* PlayerController = Cast<AMTPlayerController>(Controller);
		PlayerController && PlayerController->IsInventoryOpen())
	{
		return;
	}
	InteractionComponent->TryInteract();
}

void AMTCharacter::ToggleInventory()
{
	if (AMTPlayerController* PlayerController = Cast<AMTPlayerController>(Controller))
	{
		PlayerController->ToggleInventory();
	}
}
