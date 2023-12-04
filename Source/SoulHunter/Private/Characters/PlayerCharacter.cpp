// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/PlayerCharacter.h"

#include "Components\InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Items\Item.h"
#include "Items\Weapons\Weapon.h"
#include "Items/Soul.h"
#include "Items/Treasure.h"
#include "Animation/AnimMontage.h"
#include "Components\BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AttributeComponent.h"
#include "HUD/PlayerHUD.h"
#include "HUD/PlayerOverlay.h"
#include "LockOnTargetComponent.h"
#include "TargetHandlers/WeightedTargetHandler.h"
#include "Components/ActorComponent.h"
#include "Kismet/KismetMathLibrary.h"

#pragma region Main

APlayerCharacter::APlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 600.f, 0.f);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 300;

	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;

	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	ViewCamera->SetupAttachment(SpringArm);

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	LockOnTarget = CreateDefaultSubobject<ULockOnTargetComponent>(TEXT("LockOnTarget"));

	auto* MyTargetHandler = CreateDefaultSubobject<UWeightedTargetHandler>(TEXT("TargetHandler"));
	LockOnTarget->SetDefaultTargetHandler(MyTargetHandler);
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}

		InitializePlayerOverlay(PlayerController);
	}

	Tags.Add(FName("EngageableTarget"));

	if (Attributes)
		SprintingTimerDelegate.BindUFunction(this, FName("DepleteStaminaFromSprinting"), Attributes->GetSprintCost());

	LockOnTarget->OnTargetLocked.AddDynamic(this, &APlayerCharacter::OnTargetLocked);
	LockOnTarget->OnTargetUnlocked.AddDynamic(this, &APlayerCharacter::OnTargetUnlocked);
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Attributes && PlayerOverlay)
	{
		Attributes->RegenStamina(DeltaTime);
		PlayerOverlay->SetStaminaBarPercent(Attributes->GetStaminaPercent());
	}
}

float APlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (PlayerOverlay && Attributes)
		PlayerOverlay->SetHealthBarPercent(Attributes->GetHealthPercent());

	return DamageAmount;
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MovementAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Jump);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &APlayerCharacter::InteractKeyPressed);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Attack);
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Dodge);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::StartSprinting);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::EndSprinting);
		EnhancedInputComponent->BindAction(LockOnTargetAction, ETriggerEvent::Triggered, this, &APlayerCharacter::ToggleLockOnTarget);
		EnhancedInputComponent->BindAction(LockOnRightAction, ETriggerEvent::Triggered, this, &APlayerCharacter::LockOnRight);
		EnhancedInputComponent->BindAction(LockOnLeftAction, ETriggerEvent::Triggered, this, &APlayerCharacter::LockOnLeft);
	}
}

void APlayerCharacter::Jump()
{
	if (IsUnoccupied())
		Super::Jump();
}

void APlayerCharacter::GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter)
{
	Super::GetHit_Implementation(ImpactPoint, Hitter);

	ActionState = EActionState::EAS_HitReact;
}

void APlayerCharacter::SetOverlappingItem(AItem* Item)
{
	OverlappingItem = Item;
}

void APlayerCharacter::AddSouls(ASoul* Soul)
{
	if (Attributes)
	{
		Attributes->AddSouls(Soul->GetSouls());
		PlayerOverlay->SetSoulsCountText(Attributes->GetSouls());
	}
}

void APlayerCharacter::AddGold(ATreasure* Treasure)
{
	if (Attributes)
	{
		Attributes->AddGold(Treasure->GetGold());
		PlayerOverlay->SetGoldCountText(Attributes->GetGold());
	}
}

void APlayerCharacter::Death(const FVector& ImpactPoint)
{
	ActionState = EActionState::EAS_Dead;
	Tags.Add(FName("Dead"));

	StartRagdoll(ImpactPoint, 1500.f);
	DropWeapon();
}

void APlayerCharacter::DropWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		EquippedWeapon->EnablePhysics();
	}
}

void APlayerCharacter::BackToUnoccupiedState()
{
	ActionState = EActionState::EAS_Unoccupied;

	if (LockOnTarget->IsTargetLocked())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
	}
}

void APlayerCharacter::InitializePlayerOverlay(APlayerController* PlayerController)
{
	APlayerHUD* PlayerHUD = Cast<APlayerHUD>(PlayerController->GetHUD());
	if (PlayerHUD)
	{
		PlayerOverlay = PlayerHUD->GetPlayerOverlay();

		if (PlayerOverlay)
		{
			PlayerOverlay->SetHealthBarPercent(Attributes->GetHealthPercent());
			PlayerOverlay->SetStaminaBarPercent(1.f);
			PlayerOverlay->SetGoldCountText(0);
			PlayerOverlay->SetSoulsCountText(0);
		}
	}
}

#pragma endregion

#pragma region Locomotion

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (ActionState != EActionState::EAS_Unoccupied)
		return;

	const FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator ControllerRotation = GetControlRotation();
	const FRotator YawRotation(0.f, ControllerRotation.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MovementVector.Y);
	AddMovementInput(Right, MovementVector.X);
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();

	if (Controller && LookValue != FVector2D::ZeroVector)
	{
		AddControllerYawInput(LookValue.X);
		AddControllerPitchInput(LookValue.Y);
	}
}

#pragma endregion

#pragma region Interaction

void APlayerCharacter::InteractKeyPressed()
{
	UE_LOG(LogTemp, Warning, TEXT("Interact Key Pressed"));

	if (AWeapon* OverlappingWeapon = Cast<AWeapon>(OverlappingItem))
	{
		if (EquippedWeapon)
			EquippedWeapon->Destroy();

		EquipWeapon(OverlappingWeapon);
	}
	else
	{
		if (CanDisarm())
		{
			PlayArmDisarmMontage(FName("Disarm"));

			ActionState = EActionState::EAS_Occupied;
			CharacterState = ECharacterState::ECS_Unequipped;
		}
		else if (CanArm())
		{
			PlayArmDisarmMontage(FName("Arm"));
			
			ActionState = EActionState::EAS_Occupied;
			CharacterState = ECharacterState::ECS_EquippedOneHandedWeapon;
		}
	}
}

void APlayerCharacter::EquipWeapon(AWeapon* OverlappingWeapon)
{
	OverlappingWeapon->Equip(GetMesh(), WeaponSocket, this, this);
	CharacterState = ECharacterState::ECS_EquippedOneHandedWeapon;

	OverlappingItem = nullptr;

	EquippedWeapon = OverlappingWeapon;
}

#pragma endregion

#pragma region Combat

void APlayerCharacter::Attack()
{
	if (CanAttack())
	{
		PlayMontageRandomSection(AttackMontage, LastSelectedAttackMontageSection);
		ActionState = EActionState::EAS_Attacking;
	}
}

bool APlayerCharacter::CanAttack()
{
	return CharacterState != ECharacterState::ECS_Unequipped && 
		   ActionState == EActionState::EAS_Unoccupied;
}

void APlayerCharacter::Arm()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->AttackMeshToSocket(GetMesh(), FName("RightHandSocket"));
	}
}

void APlayerCharacter::Disarm()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->AttackMeshToSocket(GetMesh(), FName("SpineSocket"));
	}
}

bool APlayerCharacter::CanArm()
{
	return ActionState == EActionState::EAS_Unoccupied &&
		CharacterState == ECharacterState::ECS_Unequipped &&
		EquippedWeapon;
}

bool APlayerCharacter::CanDisarm()
{
	return ActionState == EActionState::EAS_Unoccupied &&
		CharacterState != ECharacterState::ECS_Unequipped;
}

void APlayerCharacter::PlayArmDisarmMontage(const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ArmDisarmMontage)
	{
		AnimInstance->Montage_Play(ArmDisarmMontage);
		AnimInstance->Montage_JumpToSection(SectionName, ArmDisarmMontage);
	}
}

void APlayerCharacter::Dodge()
{
	float DodgeCost = 0;

	if (Attributes)
		DodgeCost = Attributes->GetDodgeCost();

	if (ActionState != EActionState::EAS_Unoccupied || !HasEnoughStamina(DodgeCost)) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && DodgeMontage)
	{
		if (LockOnTarget->IsTargetLocked())
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
			GetCharacterMovement()->bUseControllerDesiredRotation = false;
		}


		FVector CurrentAcceleration = GetCharacterMovement()->GetCurrentAcceleration();

		if (!CurrentAcceleration.IsNearlyZero())
		{
			FVector AccelerationNormal = CurrentAcceleration.GetSafeNormal();

			FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(AccelerationNormal);

			SetActorRotation(NewRotation);
		}

		AnimInstance->Montage_Play(DodgeMontage);
		ActionState = EActionState::EAS_Occupied;

		if (Attributes && PlayerOverlay)
		{
			Attributes->UseStamina(DodgeCost);
			PlayerOverlay->SetStaminaBarPercent(Attributes->GetStaminaPercent());
		}
	}
}

void APlayerCharacter::StartSprinting()
{
	float SprintCost = 0;

	if (Attributes)
		SprintCost = Attributes->GetSprintCost();

	if (ActionState != EActionState::EAS_Unoccupied || !HasEnoughStamina(SprintCost)) return;

	GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;

	GetWorldTimerManager().SetTimer(SprintingTimer, SprintingTimerDelegate, .05f, true);
}

void APlayerCharacter::EndSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	GetWorldTimerManager().ClearTimer(SprintingTimer);
}

void APlayerCharacter::DepleteStaminaFromSprinting(float StaminaToDeplete)
{
	if (ActionState != EActionState::EAS_Unoccupied || !HasEnoughStamina(StaminaToDeplete))
		EndSprinting();

	if (GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero(3.f))
		EndSprinting();

	if (Attributes && PlayerOverlay)
	{
		Attributes->UseStamina(StaminaToDeplete);
		PlayerOverlay->SetStaminaBarPercent(Attributes->GetStaminaPercent());
	}
}

void APlayerCharacter::ToggleLockOnTarget()
{
	LockOnTarget->EnableTargeting();
}

void APlayerCharacter::LockOnRight()
{
	LockOnTarget->SwitchTargetManual(FVector2D(1, 0));
}

void APlayerCharacter::LockOnLeft()
{
	LockOnTarget->SwitchTargetManual(FVector2D(-1, 0));
}

void APlayerCharacter::OnTargetLocked(UTargetComponent* Target, FName Socket)
{
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	SpringArm->SetRelativeRotation(FRotator(-25, 0, 0));
	SpringArm->bInheritPitch = false;
}

void APlayerCharacter::OnTargetUnlocked(UTargetComponent* Target, FName Socket)
{
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	SpringArm->bInheritPitch = true;
}

bool APlayerCharacter::HasEnoughStamina(float StaminaToUse)
{
	return Attributes &&
		   Attributes->GetStamina() > StaminaToUse;
}

#pragma endregion
